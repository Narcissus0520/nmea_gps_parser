#include "command_test_runner.h"

#include <QFile>
#include <QFileInfo>
#include <QTextStream>

namespace {
constexpr qint64 max_command_test_file_bytes = 2 * 1024 * 1024;
constexpr int max_command_test_cases = 1000;
constexpr int max_response_buffer_chars = 65536;
constexpr int command_test_step_delay_ms = 80;
}

CommandTestRunner::CommandTestRunner(QObject *parent)
    : QObject(parent)
{
    timeout_timer_.setSingleShot(true);
    connect(&timeout_timer_, &QTimer::timeout, this, &CommandTestRunner::handle_timeout);
}

bool CommandTestRunner::load_file(const QString &path, QString *error)
{
    const QFileInfo info(path);
    if (!info.exists() || !info.isFile()) {
        if (error != nullptr) {
            *error = "Command test file does not exist.";
        }
        return false;
    }
    if (info.size() > max_command_test_file_bytes) {
        if (error != nullptr) {
            *error = "Command test file exceeds the 2 MiB safety limit.";
        }
        return false;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (error != nullptr) {
            *error = file.errorString();
        }
        return false;
    }

    QList<CommandTestCase> loaded_cases;
    QTextStream stream(&file);
    int line_number = 0;
    bool header_checked = false;
    while (!stream.atEnd()) {
        const QString raw_line = stream.readLine();
        line_number++;
        const QString line = raw_line.trimmed();
        if (line.isEmpty() || line.startsWith('#')) {
            continue;
        }

        bool csv_ok = false;
        const QStringList fields = parse_csv_line(raw_line, &csv_ok);
        if (!csv_ok || fields.size() != 6) {
            if (error != nullptr) {
                *error = QString("Invalid CSV at line %1. Expected 6 fields.").arg(line_number);
            }
            return false;
        }

        if (!header_checked) {
            header_checked = true;
            if (fields.at(0).trimmed().compare("name", Qt::CaseInsensitive) == 0
                && fields.at(1).trimmed().compare("mode", Qt::CaseInsensitive) == 0) {
                continue;
            }
        }

        CommandTestCase test_case;
        test_case.name = fields.at(0).trimmed();
        if (test_case.name.isEmpty()) {
            test_case.name = QString("case_%1").arg(loaded_cases.size() + 1);
        }
        if (!parse_mode(fields.at(1), &test_case.mode)) {
            if (error != nullptr) {
                *error = QString("Invalid mode at line %1. Use text or hex.").arg(line_number);
            }
            return false;
        }
        if (!parse_ending(fields.at(2), &test_case.ending)) {
            if (error != nullptr) {
                *error = QString("Invalid ending at line %1. Use crlf, cr, lf, or none.").arg(line_number);
            }
            return false;
        }
        test_case.command = fields.at(3);
        test_case.expect = fields.at(4);

        bool timeout_ok = false;
        const QString timeout_text = fields.at(5).trimmed();
        if (timeout_text.isEmpty()) {
            test_case.timeout_ms = 1000;
        } else {
            test_case.timeout_ms = timeout_text.toInt(&timeout_ok);
            if (!timeout_ok) {
                if (error != nullptr) {
                    *error = QString("Invalid timeout at line %1.").arg(line_number);
                }
                return false;
            }
        }
        test_case.timeout_ms = qBound(50, test_case.timeout_ms, 60000);

        loaded_cases.append(test_case);
        if (loaded_cases.size() > max_command_test_cases) {
            if (error != nullptr) {
                *error = "Command test file exceeds the 1000 case safety limit.";
            }
            return false;
        }
    }

    if (loaded_cases.isEmpty()) {
        if (error != nullptr) {
            *error = "No command test cases were loaded.";
        }
        return false;
    }

    cases_ = loaded_cases;
    emit status_changed(QString("Loaded %1 command test cases.").arg(cases_.size()));
    return true;
}

bool CommandTestRunner::has_cases(void) const
{
    return !cases_.isEmpty();
}

bool CommandTestRunner::is_running(void) const
{
    return running_;
}

int CommandTestRunner::case_count(void) const
{
    return cases_.size();
}

void CommandTestRunner::start(SerialManager *serial)
{
    if (running_) {
        return;
    }
    if (serial == nullptr || !serial->is_open()) {
        emit status_changed("Open a serial port before running command tests.");
        return;
    }
    if (cases_.isEmpty()) {
        emit status_changed("Load command test CSV before running.");
        return;
    }

    serial_ = serial;
    current_index_ = 0;
    passed_count_ = 0;
    failed_count_ = 0;
    response_buffer_.clear();
    running_ = true;
    awaiting_response_ = false;
    emit status_changed(QString("Command test started: %1 cases.").arg(cases_.size()));
    start_next_case();
}

void CommandTestRunner::stop(void)
{
    if (!running_) {
        return;
    }
    timeout_timer_.stop();
    running_ = false;
    awaiting_response_ = false;
    emit status_changed("Command test stopped.");
    emit finished(passed_count_, failed_count_);
}

void CommandTestRunner::append_response(const QByteArray &data)
{
    if (!running_ || !awaiting_response_ || current_index_ >= cases_.size() || data.isEmpty()) {
        return;
    }

    const CommandTestCase &test_case = cases_.at(current_index_);
    if (test_case.expect.isEmpty()) {
        return;
    }

    response_buffer_.append(QString::fromUtf8(data));
    if (response_buffer_.size() > max_response_buffer_chars) {
        response_buffer_.remove(0, response_buffer_.size() - max_response_buffer_chars);
    }

    if (response_buffer_.contains(test_case.expect)) {
        timeout_timer_.stop();
        awaiting_response_ = false;
        complete_current_case(true, QString("matched \"%1\"").arg(test_case.expect));
    }
}

void CommandTestRunner::handle_timeout(void)
{
    if (!running_ || current_index_ >= cases_.size()) {
        return;
    }

    const CommandTestCase &test_case = cases_.at(current_index_);
    QString sample = response_buffer_.left(120);
    sample.replace('\r', ' ');
    sample.replace('\n', ' ');
    awaiting_response_ = false;
    complete_current_case(false,
                          QString("timeout waiting for \"%1\"; response: %2")
                              .arg(test_case.expect, sample));
}

void CommandTestRunner::start_next_case(void)
{
    if (!running_) {
        return;
    }
    if (current_index_ >= cases_.size()) {
        running_ = false;
        awaiting_response_ = false;
        emit status_changed(QString("Command test finished. PASS:%1 FAIL:%2").arg(passed_count_).arg(failed_count_));
        emit finished(passed_count_, failed_count_);
        return;
    }

    const CommandTestCase &test_case = cases_.at(current_index_);
    response_buffer_.clear();

    QString error;
    if (serial_ == nullptr || !serial_->send_command(test_case.command, test_case.mode, test_case.ending, &error)) {
        complete_current_case(false, error.isEmpty() ? "send failed" : error);
        return;
    }

    emit test_result(QString("[%1/%2] TX %3 %4 %5")
                         .arg(current_index_ + 1)
                         .arg(cases_.size())
                         .arg(test_case.name,
                              mode_text(test_case.mode),
                              test_case.mode == CommandMode::Text ? ending_text(test_case.ending) : "raw"));

    if (test_case.expect.isEmpty()) {
        complete_current_case(true, "sent; no response expectation");
        return;
    }

    awaiting_response_ = true;
    timeout_timer_.start(test_case.timeout_ms);
}

QStringList CommandTestRunner::parse_csv_line(const QString &line, bool *ok)
{
    QStringList fields;
    QString field;
    bool in_quotes = false;

    for (int i = 0; i < line.size(); ++i) {
        const QChar ch = line.at(i);
        if (ch == '"') {
            if (in_quotes && i + 1 < line.size() && line.at(i + 1) == '"') {
                field.append('"');
                ++i;
            } else {
                in_quotes = !in_quotes;
            }
            continue;
        }

        if (ch == ',' && !in_quotes) {
            fields.append(field);
            field.clear();
            continue;
        }

        field.append(ch);
    }

    fields.append(field);
    if (ok != nullptr) {
        *ok = !in_quotes;
    }
    return fields;
}

bool CommandTestRunner::parse_mode(const QString &text, CommandMode *mode)
{
    if (mode == nullptr) {
        return false;
    }

    const QString value = text.trimmed().toLower();
    if (value == "text" || value == "ascii") {
        *mode = CommandMode::Text;
        return true;
    }
    if (value == "hex") {
        *mode = CommandMode::Hex;
        return true;
    }
    return false;
}

bool CommandTestRunner::parse_ending(const QString &text, LineEnding *ending)
{
    if (ending == nullptr) {
        return false;
    }

    const QString value = text.trimmed().toLower();
    if (value.isEmpty() || value == "crlf") {
        *ending = LineEnding::Crlf;
        return true;
    }
    if (value == "cr") {
        *ending = LineEnding::Cr;
        return true;
    }
    if (value == "lf") {
        *ending = LineEnding::Lf;
        return true;
    }
    if (value == "none" || value == "raw") {
        *ending = LineEnding::None;
        return true;
    }
    return false;
}

QString CommandTestRunner::mode_text(CommandMode mode)
{
    return mode == CommandMode::Hex ? "HEX" : "TEXT";
}

QString CommandTestRunner::ending_text(LineEnding ending)
{
    switch (ending) {
    case LineEnding::Cr:
        return "CR";
    case LineEnding::Lf:
        return "LF";
    case LineEnding::None:
        return "NONE";
    case LineEnding::Crlf:
        return "CRLF";
    }
    return "CRLF";
}

void CommandTestRunner::complete_current_case(bool passed, const QString &detail)
{
    if (!running_ || current_index_ >= cases_.size()) {
        return;
    }

    const CommandTestCase &test_case = cases_.at(current_index_);
    awaiting_response_ = false;
    if (passed) {
        passed_count_++;
    } else {
        failed_count_++;
    }

    emit test_result(QString("%1 %2 - %3")
                         .arg(passed ? "PASS" : "FAIL",
                              test_case.name,
                              detail));
    current_index_++;
    QTimer::singleShot(command_test_step_delay_ms, this, &CommandTestRunner::start_next_case);
}
