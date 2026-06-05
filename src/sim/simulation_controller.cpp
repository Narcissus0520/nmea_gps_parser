#include "simulation_controller.h"

#include <QDateTime>
#include <QtMath>
#include <cmath>

SimulationController::SimulationController(QObject *parent)
    : QObject(parent)
{
    timer_.setInterval(1000);
    connect(&timer_, &QTimer::timeout, this, &SimulationController::generate_epoch);
}

void SimulationController::start(void)
{
    if (timer_.isActive()) {
        return;
    }

    timer_.start();
    emit status_changed("Simulation started");
    generate_epoch();
}

void SimulationController::stop(void)
{
    timer_.stop();
    emit status_changed("Simulation stopped");
}

bool SimulationController::is_running(void) const
{
    return timer_.isActive();
}

void SimulationController::generate_epoch(void)
{
    const QDateTime now = QDateTime::currentDateTimeUtc();
    const QTime time = now.time();
    const QDate date = now.date();
    const double angle = tick_ * 0.13;
    const double latitude = 22.543096 + qSin(angle) * 0.0018 + tick_ * 0.000015;
    const double longitude = 114.057865 + qCos(angle * 0.8) * 0.0018 + tick_ * 0.000018;
    const double altitude = 31.8 + qSin(angle * 0.7) * 2.5;
    const double speed_kmh = 36.0 + qSin(angle) * 8.0;
    const double speed_knots = speed_kmh / 1.852;
    const double course = std::fmod(84.0 + tick_ * 2.7, 360.0);
    const QString time_text = time.toString("hhmmss") + ".00";
    const QString date_text = date.toString("ddMMyy");
    const QString lat_text = format_latitude(latitude);
    const QString lon_text = format_longitude(longitude);

    const QString gga = QString("GNGGA,%1,%2,N,%3,E,1,26,0.8,%4,M,0.0,M,,")
                            .arg(time_text)
                            .arg(lat_text)
                            .arg(lon_text)
                            .arg(altitude, 0, 'f', 1);
    const QString rmc = QString("GNRMC,%1,A,%2,N,%3,E,%4,%5,%6,,,A")
                            .arg(time_text)
                            .arg(lat_text)
                            .arg(lon_text)
                            .arg(speed_knots, 0, 'f', 1)
                            .arg(course, 0, 'f', 1)
                            .arg(date_text);
    const QString gsa = "GPGSA,A,3,12,05,24,03,29,31,17,19,22,,,,1.3,0.8,1.1";
    const QString gps_gsv1 = QString("GPGSV,2,1,08,12,72,148,%1,05,48,250,%2,24,29,076,%3,03,18,310,%4")
                                 .arg(dynamic_cn0(52, 0))
                                 .arg(dynamic_cn0(44, 2))
                                 .arg(dynamic_cn0(31, 4))
                                 .arg(dynamic_cn0(37, 6));
    const QString gps_gsv2 = QString("GPGSV,2,2,08,29,64,022,%1,31,38,188,%2,17,26,332,%3,19,12,118,%4")
                                 .arg(dynamic_cn0(49, 8))
                                 .arg(dynamic_cn0(42, 10))
                                 .arg(dynamic_cn0(35, 12))
                                 .arg(dynamic_cn0(27, 14));
    const QString bds_gsv1 = QString("BDGSV,2,1,08,07,66,038,%1,21,22,186,%2,16,41,120,%3,30,12,260,%4")
                                 .arg(dynamic_cn0(47, 1))
                                 .arg(dynamic_cn0(34, 3))
                                 .arg(dynamic_cn0(30, 5))
                                 .arg(dynamic_cn0(28, 7));
    const QString bds_gsv2 = QString("BDGSV,2,2,08,03,58,296,%1,09,34,074,%2,25,19,212,%3,37,44,006,%4")
                                 .arg(dynamic_cn0(46, 9))
                                 .arg(dynamic_cn0(40, 11))
                                 .arg(dynamic_cn0(33, 13))
                                 .arg(dynamic_cn0(29, 15));
    const QString glo_gsv1 = QString("GLGSV,2,1,08,03,70,041,%1,08,54,156,%2,14,32,242,%3,22,18,318,%4")
                                 .arg(dynamic_cn0(45, 4))
                                 .arg(dynamic_cn0(41, 6))
                                 .arg(dynamic_cn0(36, 8))
                                 .arg(dynamic_cn0(24, 10));
    const QString glo_gsv2 = QString("GLGSV,2,2,08,01,62,088,%1,11,28,204,%2,18,35,284,%3,24,16,132,%4")
                                 .arg(dynamic_cn0(43, 12))
                                 .arg(dynamic_cn0(38, 14))
                                 .arg(dynamic_cn0(32, 16))
                                 .arg(dynamic_cn0(26, 18));
    const QString gal_gsv1 = QString("GAGSV,2,1,08,11,68,330,%1,19,47,102,%2,25,31,214,%3,30,20,286,%4")
                                 .arg(dynamic_cn0(50, 5))
                                 .arg(dynamic_cn0(43, 7))
                                 .arg(dynamic_cn0(39, 9))
                                 .arg(dynamic_cn0(31, 11));
    const QString gal_gsv2 = QString("GAGSV,2,2,08,02,59,018,%1,07,36,176,%2,15,24,256,%3,27,14,066,%4")
                                 .arg(dynamic_cn0(48, 13))
                                 .arg(dynamic_cn0(41, 15))
                                 .arg(dynamic_cn0(34, 17))
                                 .arg(dynamic_cn0(28, 19));
    const QString vtg = QString("GNVTG,%1,T,,M,%2,N,%3,K,A")
                            .arg(course, 0, 'f', 1)
                            .arg(speed_knots, 0, 'f', 1)
                            .arg(speed_kmh, 0, 'f', 1);

    emit line_generated(with_checksum(gga));
    emit line_generated(with_checksum(rmc));
    emit line_generated(with_checksum(gsa));
    emit line_generated(with_checksum(gps_gsv1));
    emit line_generated(with_checksum(gps_gsv2));
    emit line_generated(with_checksum(bds_gsv1));
    emit line_generated(with_checksum(bds_gsv2));
    emit line_generated(with_checksum(glo_gsv1));
    emit line_generated(with_checksum(glo_gsv2));
    emit line_generated(with_checksum(gal_gsv1));
    emit line_generated(with_checksum(gal_gsv2));
    emit line_generated(with_checksum(vtg));
    tick_++;
}

QString SimulationController::with_checksum(const QString &payload) const
{
    int checksum = 0;
    for (const QChar ch : payload) {
        checksum ^= ch.toLatin1();
    }
    return QString("$%1*%2")
        .arg(payload)
        .arg(checksum, 2, 16, QLatin1Char('0'))
        .toUpper();
}

QString SimulationController::format_latitude(double latitude) const
{
    const int degrees = static_cast<int>(qAbs(latitude));
    const double minutes = (qAbs(latitude) - degrees) * 60.0;
    return QString("%1%2")
        .arg(degrees, 2, 10, QLatin1Char('0'))
        .arg(minutes, 7, 'f', 4, QLatin1Char('0'));
}

QString SimulationController::format_longitude(double longitude) const
{
    const int degrees = static_cast<int>(qAbs(longitude));
    const double minutes = (qAbs(longitude) - degrees) * 60.0;
    return QString("%1%2")
        .arg(degrees, 3, 10, QLatin1Char('0'))
        .arg(minutes, 7, 'f', 4, QLatin1Char('0'));
}

int SimulationController::dynamic_cn0(int base, int offset) const
{
    return qBound(18, base + static_cast<int>(qRound(qSin((tick_ + offset) * 0.45) * 5.0)), 58);
}
