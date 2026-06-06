#ifndef SERIAL_MANAGER_H
#define SERIAL_MANAGER_H

#include "command_encoder.h"

#include <QObject>
#include <QSerialPort>
#include <QStringList>

class SerialManager : public QObject {
    Q_OBJECT

public:
    explicit SerialManager(QObject *parent = nullptr);

    QStringList available_ports(void) const;
    bool open_port(const QString &port_name,
                   int baud_rate = 115200,
                   QSerialPort::DataBits data_bits = QSerialPort::Data8,
                   QSerialPort::Parity parity = QSerialPort::NoParity,
                   QSerialPort::StopBits stop_bits = QSerialPort::OneStop);
    void close_port(void);
    bool is_open(void) const;
    bool send_command(const QString &command,
                      CommandMode mode,
                      LineEnding ending,
                      QString *error);

signals:
    void line_received(const QString &line);
    void raw_received(const QByteArray &data);
    void status_changed(const QString &status);
    void error_message(const QString &message);

private slots:
    void read_ready_data(void);

private:
    QSerialPort serial_;
    QByteArray buffer_;
};

#endif
