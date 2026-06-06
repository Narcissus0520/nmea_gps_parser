#ifndef COMMAND_TEST_RUNNER_H
#define COMMAND_TEST_RUNNER_H

#include "command_encoder.h"
#include "serial_manager.h"

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>

struct CommandTestCase {
    QString name;
    CommandMode mode = CommandMode::Text;
    LineEnding ending = LineEnding::Crlf;
    QString command;
    QString expect;
    int timeout_ms = 1000;
};

class CommandTestRunner : public QObject {
    Q_OBJECT

public:
    explicit CommandTestRunner(QObject *parent = nullptr);

    bool load_file(const QString &path, QString *error);
    bool has_cases(void) const;
    bool is_running(void) const;
    int case_count(void) const;

    void start(SerialManager *serial);
    void stop(void);
    void append_response(const QByteArray &data);

signals:
    void status_changed(const QString &status);
    void test_result(const QString &line);
    void finished(int passed, int failed);

private slots:
    void handle_timeout(void);
    void start_next_case(void);

private:
    static QStringList parse_csv_line(const QString &line, bool *ok);
    static bool parse_mode(const QString &text, CommandMode *mode);
    static bool parse_ending(const QString &text, LineEnding *ending);
    static QString mode_text(CommandMode mode);
    static QString ending_text(LineEnding ending);

    void complete_current_case(bool passed, const QString &detail);

    QList<CommandTestCase> cases_;
    SerialManager *serial_ = nullptr;
    QTimer timeout_timer_;
    QString response_buffer_;
    int current_index_ = 0;
    int passed_count_ = 0;
    int failed_count_ = 0;
    bool running_ = false;
    bool awaiting_response_ = false;
};

#endif
