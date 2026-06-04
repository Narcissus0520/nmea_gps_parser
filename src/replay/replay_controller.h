#ifndef REPLAY_CONTROLLER_H
#define REPLAY_CONTROLLER_H

#include <QObject>
#include <QStringList>
#include <QTimer>

class ReplayController : public QObject {
    Q_OBJECT

public:
    explicit ReplayController(QObject *parent = nullptr);

    bool load_file(const QString &path, QString *error);
    void start(void);
    void pause(void);
    void stop(void);
    void set_speed(double speed);
    int progress(void) const;
    int total_count(void) const;

signals:
    void line_replayed(const QString &line);
    void progress_changed(int current, int total);
    void status_changed(const QString &status);

private slots:
    void replay_next_line(void);

private:
    void update_timer_interval(void);

    QStringList lines_;
    QTimer timer_;
    int index_ = 0;
    double speed_ = 1.0;
};

#endif
