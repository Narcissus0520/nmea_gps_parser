#include "report_writer.h"

#include <QFile>
#include <QTextStream>

bool ReportWriter::write_html(const QString &path, const AnalysisSummary &summary, QString *error)
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
           "table{border-collapse:collapse;width:720px}td,th{border:1px solid #13f7ff;padding:8px}"
           "h1{color:#ff2bd6}.value{color:#ffd166}</style></head><body>";
    out << "<h1>GNSS Cyberpunk Host Analysis Report</h1>";
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
    out << "timestamp,fix,lat,lon,alt,speed_kmh,course,satellites,hdop,pdop,vdop\n";
    for (const GnssEpoch &epoch : epochs) {
        out << epoch.timestamp.toString(Qt::ISODate) << ","
            << (epoch.has_fix ? 1 : 0) << ","
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
