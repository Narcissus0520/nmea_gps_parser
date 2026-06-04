#ifndef GNSS_TYPES_H
#define GNSS_TYPES_H

#include <QDateTime>
#include <QList>
#include <QString>

struct SatelliteInfo {
    QString constellation;
    int prn = 0;
    int elevation = 0;
    int azimuth = 0;
    int cn0 = 0;
};

struct GnssEpoch {
    QDateTime timestamp;
    bool has_fix = false;
    int fix_quality = 0;
    int fix_type = 0;
    double latitude = 0.0;
    double longitude = 0.0;
    double altitude = 0.0;
    double speed_kmh = 0.0;
    double course_deg = 0.0;
    int satellites_used = 0;
    double hdop = 0.0;
    double pdop = 0.0;
    double vdop = 0.0;
    QList<SatelliteInfo> satellites;
    QString last_sentence;
};

struct TruthPoint {
    QDateTime timestamp;
    double latitude = 0.0;
    double longitude = 0.0;
    double altitude = 0.0;
};

#endif
