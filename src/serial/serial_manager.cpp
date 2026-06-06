#include "serial_manager.h"

#include <QSerialPortInfo>

namespace {
constexpr int max_serial_buffer_bytes = 8192;
constexpr int max_nmea_line_bytes = 4096;

QString serial_config_text(int baud_rate,
                           QSerialPort::DataBits data_bits,
                           QSerialPort::Parity parity,
                           QSerialPort::StopBits stop_bits)
{
    const QString parity_text = parity == QSerialPort::EvenParity ? "E" : (parity == QSerialPort::OddParity ? "O" : "N");
    const QString stop_bits_text = stop_bits == QSerialPort::TwoStop ? "2" : "1";
    return QString("%1 %2%3%4").arg(baud_rate).arg(static_cast<int>(data_bits)).arg(parity_text).arg(stop_bits_text);
}
}

SerialManager::SerialManager(QObject *parent)
    : QObject(parent)
{
    connect(&serial_, &QSerialPort::readyRead, this, &SerialManager::read_ready_data);
    connect(&serial_, &QSerialPort::errorOccurred, this, [this](QSerialPort::SerialPortError error) {
        if (error != QSerialPort::NoError) {
            emit error_message(serial_.errorString());
        }
    });
}

QStringList SerialManager::available_ports(void) const
{
    QStringList ports;
    const QList<QSerialPortInfo> infos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : infos) {
        ports.append(info.portName());
    }
    return ports;
}

bool SerialManager::open_port(const QString &port_name,
                              int baud_rate,
                              QSerialPort::DataBits data_bits,
                              QSerialPort::Parity parity,
                              QSerialPort::StopBits stop_bits)
{
    if (serial_.isOpen()) {
        serial_.close();
    }

    serial_.setPortName(port_name);
    if (!serial_.setBaudRate(baud_rate)
        || !serial_.setDataBits(data_bits)
        || !serial_.setParity(parity)
        || !serial_.setStopBits(stop_bits)
        || !serial_.setFlowControl(QSerialPort::NoFlowControl)) {
        emit error_message("Invalid serial configuration.");
        return false;
    }

    if (!serial_.open(QIODevice::ReadWrite)) {
        emit error_message(serial_.errorString());
        return false;
    }

    buffer_.clear();
    emit status_changed(QString("Connected: %1 %2").arg(port_name, serial_config_text(baud_rate, data_bits, parity, stop_bits)));
    return true;
}

void SerialManager::close_port(void)
{
    if (serial_.isOpen()) {
        serial_.close();
    }
    emit status_changed("Disconnected");
}

bool SerialManager::is_open(void) const
{
    return serial_.isOpen();
}

bool SerialManager::send_command(const QString &command,
                                 CommandMode mode,
                                 LineEnding ending,
                                 QString *error)
{
    QByteArray bytes;

    if (!serial_.isOpen()) {
        if (error != nullptr) {
            *error = "Serial port is not open.";
        }
        return false;
    }

    if (mode == CommandMode::Text) {
        bytes = CommandEncoder::encode_text(command, ending);
    } else if (!CommandEncoder::encode_hex(command, &bytes, error)) {
        return false;
    }

    if (serial_.write(bytes) != bytes.size()) {
        if (error != nullptr) {
            *error = "Failed to write all command bytes.";
        }
        return false;
    }

    return serial_.flush();
}

void SerialManager::read_ready_data(void)
{
    const QByteArray data = serial_.readAll();
    emit raw_received(data);

    buffer_.append(data);
    if (buffer_.size() > max_serial_buffer_bytes) {
        buffer_.clear();
        emit error_message("Serial receive buffer exceeded safety limit; buffered bytes were discarded.");
        return;
    }

    while (true) {
        int newline_index = buffer_.indexOf('\n');
        if (newline_index < 0) {
            break;
        }

        QByteArray line = buffer_.left(newline_index + 1);
        buffer_.remove(0, newline_index + 1);
        if (line.size() > max_nmea_line_bytes) {
            emit error_message("NMEA line exceeded safety limit and was ignored.");
            continue;
        }
        emit line_received(QString::fromUtf8(line).trimmed());
    }
}
