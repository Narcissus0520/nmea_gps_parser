#ifndef KML_NMEA_CONVERTER_H
#define KML_NMEA_CONVERTER_H

#include <QString>

class KmlNmeaConverter {
public:
    static bool nmea_to_kml(const QString &nmea_path, const QString &kml_path, QString *error);
    static bool kml_to_nmea(const QString &kml_path, const QString &nmea_path, QString *error);

private:
    static QString with_checksum(const QString &payload);
    static QString format_latitude(double latitude);
    static QString format_longitude(double longitude);
};

#endif
