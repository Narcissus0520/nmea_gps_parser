#include "kml_nmea_converter.h"

#include "../nmea/nmea_parser.h"

#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTextStream>
#include <QTimeZone>
#include <QXmlStreamReader>
#include <QtMath>

namespace {
constexpr qint64 max_convert_file_bytes = 64 * 1024 * 1024;
constexpr int max_convert_points = 300000;

struct KmlPoint {
    double longitude = 0.0;
    double latitude = 0.0;
    double altitude = 0.0;
};
}

bool KmlNmeaConverter::nmea_to_kml(const QString &nmea_path, const QString &kml_path, QString *error)
{
    const QFileInfo info(nmea_path);
    if (info.size() > max_convert_file_bytes) {
        if (error != nullptr) {
            *error = "NMEA file is too large for conversion.";
        }
        return false;
    }

    QFile input(nmea_path);
    if (!input.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (error != nullptr) {
            *error = input.errorString();
        }
        return false;
    }

    QList<KmlPoint> points;
    NmeaParser parser;
    QTextStream in(&input);
    while (!in.atEnd()) {
        GnssEpoch epoch;
        if (parser.parse_line(in.readLine(), &epoch) && epoch.has_fix) {
            KmlPoint point;
            point.longitude = epoch.longitude;
            point.latitude = epoch.latitude;
            point.altitude = epoch.altitude;
            points.append(point);
            if (points.size() > max_convert_points) {
                if (error != nullptr) {
                    *error = "NMEA conversion point limit exceeded.";
                }
                return false;
            }
        }
    }

    if (points.isEmpty()) {
        if (error != nullptr) {
            *error = "No valid fixed positions found in NMEA file.";
        }
        return false;
    }

    QFile output(kml_path);
    if (!output.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (error != nullptr) {
            *error = output.errorString();
        }
        return false;
    }

    QTextStream out(&output);
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    out << "<kml xmlns=\"http://www.opengis.net/kml/2.2\"><Document>\n";
    out << "<name>GNSS Track</name><Placemark><name>NMEA Track</name>\n";
    out << "<LineString><tessellate>1</tessellate><coordinates>\n";
    for (const KmlPoint &point : points) {
        out << QString::number(point.longitude, 'f', 8) << ","
            << QString::number(point.latitude, 'f', 8) << ","
            << QString::number(point.altitude, 'f', 3) << "\n";
    }
    out << "</coordinates></LineString></Placemark></Document></kml>\n";
    return true;
}

bool KmlNmeaConverter::kml_to_nmea(const QString &kml_path, const QString &nmea_path, QString *error)
{
    const QFileInfo info(kml_path);
    if (info.size() > max_convert_file_bytes) {
        if (error != nullptr) {
            *error = "KML file is too large for conversion.";
        }
        return false;
    }

    QFile input(kml_path);
    if (!input.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (error != nullptr) {
            *error = input.errorString();
        }
        return false;
    }

    QList<KmlPoint> points;
    QXmlStreamReader reader(&input);
    while (!reader.atEnd()) {
        reader.readNext();
        if (!reader.isStartElement() || reader.name().toString() != "coordinates") {
            continue;
        }

        const QStringList entries = reader.readElementText().split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        for (const QString &entry : entries) {
            const QStringList fields = entry.split(',');
            if (fields.size() < 2) {
                continue;
            }
            bool longitude_ok = false;
            bool latitude_ok = false;
            bool altitude_ok = true;
            KmlPoint point;
            point.longitude = fields.at(0).toDouble(&longitude_ok);
            point.latitude = fields.at(1).toDouble(&latitude_ok);
            point.altitude = fields.size() > 2 ? fields.at(2).toDouble(&altitude_ok) : 0.0;
            if (longitude_ok && latitude_ok && altitude_ok) {
                points.append(point);
                if (points.size() > max_convert_points) {
                    if (error != nullptr) {
                        *error = "KML conversion point limit exceeded.";
                    }
                    return false;
                }
            }
        }
    }

    if (reader.hasError()) {
        if (error != nullptr) {
            *error = reader.errorString();
        }
        return false;
    }
    if (points.isEmpty()) {
        if (error != nullptr) {
            *error = "No KML coordinates found.";
        }
        return false;
    }

    QFile output(nmea_path);
    if (!output.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (error != nullptr) {
            *error = output.errorString();
        }
        return false;
    }

    QTextStream out(&output);
    QDateTime timestamp = QDateTime::currentDateTimeUtc();
    for (const KmlPoint &point : points) {
        const QString time_text = timestamp.time().toString("HHmmss");
        const QString date_text = timestamp.date().toString("ddMMyy");
        const QString gga = QString("GNGGA,%1,%2,%3,%4,%5,1,08,1.0,%6,M,0.0,M,,")
                                .arg(time_text,
                                     format_latitude(point.latitude),
                                     point.latitude < 0.0 ? "S" : "N",
                                     format_longitude(point.longitude),
                                     point.longitude < 0.0 ? "W" : "E")
                                .arg(point.altitude, 0, 'f', 1);
        const QString rmc = QString("GNRMC,%1,A,%2,%3,%4,%5,0.0,0.0,%6,,,A")
                                .arg(time_text,
                                     format_latitude(point.latitude),
                                     point.latitude < 0.0 ? "S" : "N",
                                     format_longitude(point.longitude),
                                     point.longitude < 0.0 ? "W" : "E",
                                     date_text);
        out << with_checksum(gga) << "\r\n" << with_checksum(rmc) << "\r\n";
        timestamp = timestamp.addSecs(1);
    }
    return true;
}

QString KmlNmeaConverter::with_checksum(const QString &payload)
{
    int checksum = 0;
    for (const QChar ch : payload) {
        checksum ^= ch.toLatin1();
    }
    return QString("$%1*%2").arg(payload).arg(checksum, 2, 16, QLatin1Char('0')).toUpper();
}

QString KmlNmeaConverter::format_latitude(double latitude)
{
    const int degrees = static_cast<int>(qAbs(latitude));
    const double minutes = (qAbs(latitude) - degrees) * 60.0;
    return QString("%1%2").arg(degrees, 2, 10, QLatin1Char('0')).arg(minutes, 7, 'f', 4, QLatin1Char('0'));
}

QString KmlNmeaConverter::format_longitude(double longitude)
{
    const int degrees = static_cast<int>(qAbs(longitude));
    const double minutes = (qAbs(longitude) - degrees) * 60.0;
    return QString("%1%2").arg(degrees, 3, 10, QLatin1Char('0')).arg(minutes, 7, 'f', 4, QLatin1Char('0'));
}
