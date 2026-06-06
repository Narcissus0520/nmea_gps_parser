#include "report_writer.h"

#include <QFile>
#include <QMap>
#include <QTextStream>
#include <QtGlobal>
#include <algorithm>

namespace {
struct SecondStats {
    QString second;
    int tracked_total = 0;
    int ephemeris_estimate = 0;
    int used_total = 0;
    double hdop = 0.0;
    double pdop = 0.0;
    double vdop = 0.0;
    double speed_kmh = 0.0;
    double course_deg = 0.0;
    QMap<QString, int> tracked_by_constellation;
    QMap<QString, int> ephemeris_by_constellation;
    QMap<QString, int> used_by_constellation;
    QMap<QString, QList<int>> cn0_by_constellation;
};

QString html_escape(const QString &text)
{
    QString escaped = text;
    escaped.replace('&', "&amp;");
    escaped.replace('<', "&lt;");
    escaped.replace('>', "&gt;");
    escaped.replace('"', "&quot;");
    return escaped;
}

QString constellation_key(const SatelliteInfo &satellite)
{
    return satellite.constellation.isEmpty() ? "GNSS" : satellite.constellation;
}

QString top_cn0_text(QList<int> values)
{
    std::sort(values.begin(), values.end(), std::greater<int>());
    while (values.size() > 5) {
        values.removeLast();
    }

    QStringList parts;
    for (int value : values) {
        parts.append(QString::number(value));
    }
    return parts.isEmpty() ? "-" : parts.join("/");
}

QString svg_polyline(const QList<double> &values, const QString &title, const QString &color)
{
    if (values.isEmpty()) {
        return QString("<p>%1: no data</p>").arg(html_escape(title));
    }

    double max_value = 1.0;
    for (double value : values) {
        max_value = qMax(max_value, value);
    }

    const int width = 760;
    const int height = 180;
    const int left = 42;
    const int top = 16;
    const int chart_width = width - left - 16;
    const int chart_height = height - top - 32;
    QStringList points;
    for (int i = 0; i < values.size(); ++i) {
        const double x = left + (values.size() == 1 ? 0.0 : chart_width * i / static_cast<double>(values.size() - 1));
        const double y = top + chart_height - chart_height * values.at(i) / max_value;
        points.append(QString("%1,%2").arg(x, 0, 'f', 1).arg(y, 0, 'f', 1));
    }

    return QString("<h3>%1</h3><svg width=\"%2\" height=\"%3\" viewBox=\"0 0 %2 %3\">"
                   "<rect x=\"0\" y=\"0\" width=\"%2\" height=\"%3\" fill=\"#061522\"/>"
                   "<line x1=\"%4\" y1=\"%5\" x2=\"%6\" y2=\"%5\" stroke=\"#1d3b52\"/>"
                   "<line x1=\"%4\" y1=\"%7\" x2=\"%4\" y2=\"%5\" stroke=\"#1d3b52\"/>"
                   "<polyline fill=\"none\" stroke=\"%8\" stroke-width=\"2\" points=\"%9\"/>"
                   "<text x=\"8\" y=\"14\" fill=\"#8ad7ff\" font-size=\"11\">max %10</text></svg>")
        .arg(html_escape(title))
        .arg(width)
        .arg(height)
        .arg(left)
        .arg(top + chart_height)
        .arg(left + chart_width)
        .arg(top)
        .arg(color)
        .arg(points.join(' '))
        .arg(max_value, 0, 'f', 2);
}

QList<SecondStats> build_second_stats(const QList<GnssEpoch> &epochs)
{
    QMap<QString, SecondStats> by_second;
    int fallback_index = 0;
    for (const GnssEpoch &epoch : epochs) {
        const QString second = epoch.timestamp.isValid()
                                   ? epoch.timestamp.toUTC().toString("yyyy-MM-dd HH:mm:ss")
                                   : QString("epoch_%1").arg(++fallback_index);
        SecondStats &stats = by_second[second];
        stats.second = second;
        stats.tracked_total = epoch.satellites.size();
        stats.used_total = epoch.satellites_used;
        stats.hdop = epoch.hdop;
        stats.pdop = epoch.pdop;
        stats.vdop = epoch.vdop;
        stats.speed_kmh = epoch.speed_kmh;
        stats.course_deg = epoch.course_deg;
        stats.ephemeris_estimate = 0;
        stats.tracked_by_constellation.clear();
        stats.ephemeris_by_constellation.clear();
        stats.used_by_constellation.clear();
        stats.cn0_by_constellation.clear();

        for (const SatelliteInfo &satellite : epoch.satellites) {
            const QString constellation = constellation_key(satellite);
            stats.tracked_by_constellation[constellation]++;
            if (satellite.cn0 > 0) {
                stats.ephemeris_estimate++;
                stats.ephemeris_by_constellation[constellation]++;
                stats.cn0_by_constellation[constellation].append(satellite.cn0);
            }
            if (satellite.used) {
                stats.used_by_constellation[constellation]++;
            }
        }
    }
    return by_second.values();
}
}

bool ReportWriter::write_html(const QString &path, const AnalysisSummary &summary, const QList<GnssEpoch> &epochs, QString *error)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (error != nullptr) {
            *error = file.errorString();
        }
        return false;
    }

    QTextStream out(&file);
    out << "<!doctype html><html><head><meta charset=\"utf-8\">";
    out << "<title>GNSS Analysis Report</title>";
    out << "<style>body{background:#090b12;color:#d8f7ff;font-family:Segoe UI,sans-serif;}"
           "table{border-collapse:collapse;width:100%;max-width:1180px;margin:12px 0}td,th{border:1px solid #13f7ff;padding:6px;font-size:12px}"
           "h1{color:#ff2bd6}.value{color:#ffd166}.note{color:#8ad7ff}</style></head><body>";
    out << "<h1>GNSS Cyberpunk Host Analysis Report</h1>";
    out << "<p class=\"note\">Ephemeris count is estimated from tracked satellites with CN0 &gt; 0 when standard NMEA ephemeris state is unavailable.</p>";
    out << "<table><tr><th>Metric</th><th>Value</th></tr>";
    out << "<tr><td>Epoch count</td><td class=\"value\">" << summary.epoch_count << "</td></tr>";
    out << "<tr><td>Fixed count</td><td class=\"value\">" << summary.fixed_count << "</td></tr>";
    out << "<tr><td>Fix rate</td><td class=\"value\">" << QString::number(summary.fix_rate, 'f', 1) << "%</td></tr>";
    out << "<tr><td>Average satellites</td><td class=\"value\">" << QString::number(summary.average_satellites, 'f', 1) << "</td></tr>";
    out << "<tr><td>Average HDOP</td><td class=\"value\">" << QString::number(summary.average_hdop, 'f', 2) << "</td></tr>";
    out << "<tr><td>Average PDOP</td><td class=\"value\">" << QString::number(summary.average_pdop, 'f', 2) << "</td></tr>";
    out << "<tr><td>Average CN0</td><td class=\"value\">" << QString::number(summary.average_cn0, 'f', 1) << "</td></tr>";
    out << "<tr><td>Horizontal RMS</td><td class=\"value\">" << QString::number(summary.horizontal_rms, 'f', 2) << " m</td></tr>";
    out << "<tr><td>Horizontal MAX</td><td class=\"value\">" << QString::number(summary.horizontal_max, 'f', 2) << " m</td></tr>";
    out << "<tr><td>CEP50</td><td class=\"value\">" << QString::number(summary.horizontal_cep50, 'f', 2) << " m</td></tr>";
    out << "<tr><td>CEP95</td><td class=\"value\">" << QString::number(summary.horizontal_cep95, 'f', 2) << " m</td></tr>";
    out << "<tr><td>Altitude RMS</td><td class=\"value\">" << QString::number(summary.altitude_rms, 'f', 2) << " m</td></tr>";
    out << "<tr><td>Altitude MAX</td><td class=\"value\">" << QString::number(summary.altitude_max, 'f', 2) << " m</td></tr>";
    out << "</table>";

    const QList<SecondStats> second_stats = build_second_stats(epochs);
    QList<double> tracked_values;
    QList<double> hdop_values;
    QList<double> speed_values;
    QList<double> course_values;
    for (const SecondStats &stats : second_stats) {
        tracked_values.append(stats.tracked_total);
        hdop_values.append(stats.hdop);
        speed_values.append(stats.speed_kmh);
        course_values.append(stats.course_deg);
    }
    out << svg_polyline(tracked_values, "Tracked Satellites Per Second", "#13f7ff");
    out << svg_polyline(hdop_values, "HDOP Per Second", "#ffae00");
    out << svg_polyline(speed_values, "Speed km/h Per Second", "#00ff91");
    out << svg_polyline(course_values, "Course deg Per Second", "#ff2bd6");

    out << "<h2>Per-second Satellite/DOP Trend</h2>";
    out << "<table><tr><th>Second</th><th>Total</th><th>By mode</th><th>Ephemeris est.</th><th>Used</th><th>HDOP</th><th>PDOP</th><th>VDOP</th><th>Speed</th><th>Course</th></tr>";
    for (const SecondStats &stats : second_stats) {
        QStringList constellation_parts;
        const QStringList keys = stats.tracked_by_constellation.keys();
        for (const QString &key : keys) {
            constellation_parts.append(QString("%1:%2").arg(key).arg(stats.tracked_by_constellation.value(key)));
        }
        out << "<tr><td>" << html_escape(stats.second) << "</td><td>" << stats.tracked_total
            << "</td><td>" << html_escape(constellation_parts.join(" "))
            << "</td><td>" << stats.ephemeris_estimate
            << "</td><td>" << stats.used_total
            << "</td><td>" << QString::number(stats.hdop, 'f', 2)
            << "</td><td>" << QString::number(stats.pdop, 'f', 2)
            << "</td><td>" << QString::number(stats.vdop, 'f', 2)
            << "</td><td>" << QString::number(stats.speed_kmh, 'f', 2)
            << "</td><td>" << QString::number(stats.course_deg, 'f', 2) << "</td></tr>";
    }
    out << "</table>";

    out << "<h2>Per-mode TOP5 CN0</h2>";
    out << "<table><tr><th>Second</th><th>Mode</th><th>TOP5 CN0</th><th>Ephemeris est.</th><th>Used satellites</th></tr>";
    for (const SecondStats &stats : second_stats) {
        QStringList keys = stats.cn0_by_constellation.keys();
        keys.sort();
        for (const QString &key : keys) {
            out << "<tr><td>" << html_escape(stats.second)
                << "</td><td>" << html_escape(key)
                << "</td><td>" << html_escape(top_cn0_text(stats.cn0_by_constellation.value(key)))
                << "</td><td>" << stats.ephemeris_by_constellation.value(key)
                << "</td><td>" << stats.used_by_constellation.value(key) << "</td></tr>";
        }
    }
    out << "</table></body></html>";
    return true;
}

bool ReportWriter::write_csv(const QString &path, const QList<GnssEpoch> &epochs, QString *error)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (error != nullptr) {
            *error = file.errorString();
        }
        return false;
    }

    QTextStream out(&file);
    out << "timestamp,fix,fix_category,lat,lon,alt,speed_kmh,course,satellites,hdop,pdop,vdop\n";
    for (const GnssEpoch &epoch : epochs) {
        out << epoch.timestamp.toString(Qt::ISODate) << ","
            << (epoch.has_fix ? 1 : 0) << ","
            << epoch.fix_category << ","
            << QString::number(epoch.latitude, 'f', 8) << ","
            << QString::number(epoch.longitude, 'f', 8) << ","
            << QString::number(epoch.altitude, 'f', 3) << ","
            << QString::number(epoch.speed_kmh, 'f', 3) << ","
            << QString::number(epoch.course_deg, 'f', 3) << ","
            << epoch.satellites_used << ","
            << QString::number(epoch.hdop, 'f', 2) << ","
            << QString::number(epoch.pdop, 'f', 2) << ","
            << QString::number(epoch.vdop, 'f', 2) << "\n";
    }
    return true;
}
