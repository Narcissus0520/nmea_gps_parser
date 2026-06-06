#include "replay_controller.h"

#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QtGlobal>

namespace {
constexpr qint64 max_replay_file_bytes = 64 * 1024 * 1024;
constexpr int max_replay_line_chars = 4096;
}

ReplayController::ReplayController(QObject *parent)
    : QObject(parent)
{
    connect(&timer_, &QTimer::timeout, this, &ReplayController::replay_next_line);
    update_timer_interval();
}

bool ReplayController::load_file(const QString &path, QString *error)
{
    const QFileInfo info(path);
    if (info.size() > max_replay_file_bytes) {
        if (error != nullptr) {
            *error = "Replay file is too large. Maximum allowed size is 64 MiB.";
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

    lines_.clear();
    QTextStream stream(&file);
    while (!stream.atEnd()) {
        const QString line = stream.readLine().trimmed();
        if (line.size() > max_replay_line_chars) {
            if (error != nullptr) {
                *error = "Replay file contains an overlong line.";
            }
            lines_.clear();
            return false;
        }
        if (!line.isEmpty()) {
            lines_.append(line);
        }
    }

    index_ = 0;
    emit progress_changed(index_, lines_.size());
    emit status_changed(QString("Loaded %1 NMEA lines").arg(lines_.size()));
    return true;
}

void ReplayController::start(void)
{
    if (lines_.isEmpty()) {
        emit status_changed("No replay file loaded");
        return;
    }
    timer_.start();
    emit status_changed("Replay started");
}

void ReplayController::pause(void)
{
    timer_.stop();
    emit status_changed("Replay paused");
}

void ReplayController::stop(void)
{
    timer_.stop();
    index_ = 0;
    emit progress_changed(index_, lines_.size());
    emit status_changed("Replay stopped");
}

void ReplayController::seek(int index)
{
    if (lines_.isEmpty()) {
        emit status_changed("No replay file loaded");
        return;
    }

    const bool was_running = timer_.isActive();
    timer_.stop();
    index_ = qBound(0, index, lines_.size());

    if (index_ >= lines_.size()) {
        emit progress_changed(index_, lines_.size());
        emit status_changed("Replay seeked to end");
        if (was_running) {
            timer_.start();
        }
        return;
    }

    emit line_replayed(lines_.at(index_));
    index_++;
    emit progress_changed(index_, lines_.size());
    emit status_changed(QString("Replay seeked to %1/%2").arg(index_).arg(lines_.size()));

    if (was_running) {
        timer_.start();
    }
}

void ReplayController::set_speed(double speed)
{
    speed_ = speed <= 0.0 ? 1.0 : speed;
    update_timer_interval();
}

int ReplayController::progress(void) const
{
    return index_;
}

int ReplayController::total_count(void) const
{
    return lines_.size();
}

void ReplayController::replay_next_line(void)
{
    if (index_ >= lines_.size()) {
        timer_.stop();
        emit status_changed("Replay finished");
        return;
    }

    emit line_replayed(lines_.at(index_));
    index_++;
    emit progress_changed(index_, lines_.size());
}

void ReplayController::update_timer_interval(void)
{
    const int base_interval_ms = 1000;
    timer_.setInterval(static_cast<int>(base_interval_ms / speed_));
}
