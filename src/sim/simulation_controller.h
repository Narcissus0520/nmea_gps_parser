#ifndef SIMULATION_CONTROLLER_H
#define SIMULATION_CONTROLLER_H

#include <QObject>
#include <QTimer>

class SimulationController : public QObject {
    Q_OBJECT

public:
    explicit SimulationController(QObject *parent = nullptr);

    void start(void);
    void stop(void);
    bool is_running(void) const;

signals:
    void line_generated(const QString &line);
    void status_changed(const QString &status);

private slots:
    void generate_epoch(void);

private:
    QString with_checksum(const QString &payload) const;
    QString format_latitude(double latitude) const;
    QString format_longitude(double longitude) const;
    int dynamic_cn0(int base, int offset) const;

    QTimer timer_;
    int tick_ = 0;
};

#endif
