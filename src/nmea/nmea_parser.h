#ifndef NMEA_PARSER_H
#define NMEA_PARSER_H

#include "gnss_types.h"

#include <QObject>
#include <QString>
#include <QStringList>

class NmeaParser : public QObject {
    Q_OBJECT

public:
    explicit NmeaParser(QObject *parent = nullptr);

    bool parse_line(const QString &line, GnssEpoch *epoch);
    const GnssEpoch &state(void) const;
    void reset(void);

signals:
    void epoch_updated(const GnssEpoch &epoch);
    void parse_error(const QString &message);

private:
    bool verify_checksum(const QString &line) const;
    QStringList split_payload(const QString &line) const;
    QString talker_type(const QString &field) const;
    QString constellation_from_talker(const QString &talker) const;
    double parse_lat_lon(const QString &value, const QString &hemisphere) const;
    QDate parse_date(const QString &value) const;
    QTime parse_time(const QString &value) const;
    void update_datetime(const QString &time_value, const QString &date_value);
    void parse_gga(const QStringList &fields);
    void parse_rmc(const QStringList &fields);
    void parse_gsa(const QStringList &fields);
    void parse_gsv(const QStringList &fields);
    void parse_vtg(const QStringList &fields);

    GnssEpoch state_;
    QDate last_date_;
};

#endif
