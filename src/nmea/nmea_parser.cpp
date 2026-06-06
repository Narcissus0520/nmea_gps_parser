#include "nmea_parser.h"

#include <QTimeZone>
#include <QtMath>

NmeaParser::NmeaParser(QObject *parent)
    : QObject(parent)
{
}

bool NmeaParser::parse_line(const QString &line, GnssEpoch *epoch)
{
    const QString trimmed = line.trimmed();

    if (trimmed.isEmpty()) {
        return false;
    }

    if (!trimmed.startsWith("$")) {
        emit parse_error("NMEA line does not start with '$'.");
        return false;
    }

    if (!verify_checksum(trimmed)) {
        emit parse_error("NMEA checksum failed: " + trimmed);
        return false;
    }

    const QStringList fields = split_payload(trimmed);
    if (fields.isEmpty()) {
        return false;
    }

    const QString type = talker_type(fields.at(0));
    state_.last_sentence = trimmed;

    if (type == "GGA") {
        parse_gga(fields);
    } else if (type == "RMC") {
        parse_rmc(fields);
    } else if (type == "GSA") {
        parse_gsa(fields);
    } else if (type == "GSV") {
        parse_gsv(fields);
    } else if (type == "VTG") {
        parse_vtg(fields);
    } else {
        return false;
    }

    if (epoch != nullptr) {
        *epoch = state_;
    }
    emit epoch_updated(state_);
    return true;
}

const GnssEpoch &NmeaParser::state(void) const
{
    return state_;
}

void NmeaParser::reset(void)
{
    state_ = GnssEpoch();
    last_date_ = QDate();
    used_satellite_keys_.clear();
}

bool NmeaParser::verify_checksum(const QString &line) const
{
    const int star_index = line.indexOf('*');
    int checksum = 0;
    bool ok = false;

    if (star_index < 0) {
        return true;
    }

    for (int i = 1; i < star_index; ++i) {
        checksum ^= line.at(i).toLatin1();
    }

    const int expected = line.mid(star_index + 1, 2).toInt(&ok, 16);
    return ok && checksum == expected;
}

QStringList NmeaParser::split_payload(const QString &line) const
{
    const int star_index = line.indexOf('*');
    const QString payload = line.mid(1, star_index < 0 ? -1 : star_index - 1);
    return payload.split(',');
}

QString NmeaParser::talker_type(const QString &field) const
{
    if (field.size() < 3) {
        return QString();
    }
    return field.right(3);
}

QString NmeaParser::constellation_from_talker(const QString &talker) const
{
    if (talker.startsWith("GP")) {
        return "GPS";
    }
    if (talker.startsWith("BD") || talker.startsWith("GB")) {
        return "BDS";
    }
    if (talker.startsWith("GL")) {
        return "GLO";
    }
    if (talker.startsWith("GA")) {
        return "GAL";
    }
    if (talker.startsWith("GQ")) {
        return "QZSS";
    }
    return "GNSS";
}

QString NmeaParser::satellite_key(const QString &constellation, int prn) const
{
    return QString("%1:%2").arg(constellation.isEmpty() ? "GNSS" : constellation).arg(prn);
}

QString NmeaParser::fix_category_text(void) const
{
    if (!state_.has_fix) {
        return "NONE";
    }
    if (state_.positioning_mode == "E") {
        return "VDR";
    }
    if (state_.fix_quality == 4) {
        return "RTK FIX";
    }
    if (state_.fix_quality == 5) {
        return "RTK FLOAT";
    }
    if (state_.fix_type == 3) {
        return "3D";
    }
    if (state_.fix_type == 2) {
        return "2D";
    }
    if (state_.fix_quality == 2 || state_.positioning_mode == "D") {
        return "DGPS";
    }
    return "FIX";
}

void NmeaParser::refresh_fix_category(void)
{
    state_.fix_category = fix_category_text();
}

void NmeaParser::mark_used_satellites(const QString &constellation, const QStringList &fields)
{
    const QString prefix = QString("%1:").arg(constellation.isEmpty() ? "GNSS" : constellation);
    for (int i = used_satellite_keys_.size() - 1; i >= 0; --i) {
        if (used_satellite_keys_.at(i).startsWith(prefix)) {
            used_satellite_keys_.removeAt(i);
        }
    }
    for (int i = 3; i <= 14 && i < fields.size(); ++i) {
        const int prn = fields.value(i).toInt();
        if (prn > 0) {
            used_satellite_keys_.append(satellite_key(constellation, prn));
        }
    }

    for (SatelliteInfo &satellite : state_.satellites) {
        satellite.used = used_satellite_keys_.contains(satellite_key(satellite.constellation, satellite.prn));
    }
}

double NmeaParser::parse_lat_lon(const QString &value, const QString &hemisphere) const
{
    bool ok = false;
    const double raw = value.toDouble(&ok);
    if (!ok || value.size() < 4) {
        return 0.0;
    }

    const int degrees = static_cast<int>(raw / 100.0);
    const double minutes = raw - degrees * 100.0;
    double result = degrees + minutes / 60.0;
    if (hemisphere == "S" || hemisphere == "W") {
        result = -result;
    }
    return result;
}

QDate NmeaParser::parse_date(const QString &value) const
{
    if (value.size() != 6) {
        return QDate();
    }

    const int day = value.mid(0, 2).toInt();
    const int month = value.mid(2, 2).toInt();
    const int year = 2000 + value.mid(4, 2).toInt();
    return QDate(year, month, day);
}

QTime NmeaParser::parse_time(const QString &value) const
{
    if (value.size() < 6) {
        return QTime();
    }

    const int hour = value.mid(0, 2).toInt();
    const int minute = value.mid(2, 2).toInt();
    const double second_value = value.mid(4).toDouble();
    const int second = static_cast<int>(second_value);
    const int msec = static_cast<int>(qRound((second_value - second) * 1000.0));
    return QTime(hour, minute, second, msec);
}

void NmeaParser::update_datetime(const QString &time_value, const QString &date_value)
{
    const QTime time = parse_time(time_value);
    const QDate date = date_value.isEmpty() ? last_date_ : parse_date(date_value);

    if (date.isValid()) {
        last_date_ = date;
    }

    if (time.isValid() && last_date_.isValid()) {
        state_.timestamp = QDateTime(last_date_, time, QTimeZone::UTC);
    }
}

void NmeaParser::parse_gga(const QStringList &fields)
{
    if (fields.size() < 10) {
        return;
    }

    update_datetime(fields.value(1), QString());
    state_.latitude = parse_lat_lon(fields.value(2), fields.value(3));
    state_.longitude = parse_lat_lon(fields.value(4), fields.value(5));
    state_.fix_quality = fields.value(6).toInt();
    state_.has_fix = state_.fix_quality > 0;
    state_.satellites_used = fields.value(7).toInt();
    state_.hdop = fields.value(8).toDouble();
    state_.altitude = fields.value(9).toDouble();
    refresh_fix_category();
}

void NmeaParser::parse_rmc(const QStringList &fields)
{
    if (fields.size() < 10) {
        return;
    }

    update_datetime(fields.value(1), fields.value(9));
    state_.has_fix = fields.value(2) == "A";
    state_.latitude = parse_lat_lon(fields.value(3), fields.value(4));
    state_.longitude = parse_lat_lon(fields.value(5), fields.value(6));
    state_.speed_kmh = fields.value(7).toDouble() * 1.852;
    state_.course_deg = fields.value(8).toDouble();
    state_.positioning_mode = fields.value(12);
    refresh_fix_category();
}

void NmeaParser::parse_gsa(const QStringList &fields)
{
    if (fields.size() < 17) {
        return;
    }

    state_.fix_type = fields.value(2).toInt();
    state_.pdop = fields.value(fields.size() - 3).toDouble();
    state_.hdop = fields.value(fields.size() - 2).toDouble();
    state_.vdop = fields.value(fields.size() - 1).toDouble();
    mark_used_satellites(constellation_from_talker(fields.value(0).left(2)), fields);
    refresh_fix_category();
}

void NmeaParser::parse_gsv(const QStringList &fields)
{
    if (fields.size() < 4) {
        return;
    }

    const QString constellation = constellation_from_talker(fields.value(0).left(2));
    for (int i = 4; i + 3 < fields.size(); i += 4) {
        SatelliteInfo satellite;
        satellite.constellation = constellation;
        satellite.prn = fields.value(i).toInt();
        satellite.elevation = fields.value(i + 1).toInt();
        satellite.azimuth = fields.value(i + 2).toInt();
        satellite.cn0 = fields.value(i + 3).toInt();
        satellite.used = used_satellite_keys_.contains(satellite_key(satellite.constellation, satellite.prn));

        bool replaced = false;
        for (SatelliteInfo &existing : state_.satellites) {
            if (existing.constellation == satellite.constellation && existing.prn == satellite.prn) {
                existing = satellite;
                replaced = true;
                break;
            }
        }
        if (!replaced && satellite.prn > 0) {
            state_.satellites.append(satellite);
        }
    }
}

void NmeaParser::parse_vtg(const QStringList &fields)
{
    if (fields.size() < 9) {
        return;
    }

    state_.course_deg = fields.value(1).toDouble();
    state_.speed_kmh = fields.value(7).toDouble();
}
