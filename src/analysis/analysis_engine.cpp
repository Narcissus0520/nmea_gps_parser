#include "analysis_engine.h"

#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QtMath>
#include <algorithm>
#include <cmath>

namespace {
constexpr qint64 max_truth_file_bytes = 16 * 1024 * 1024;
constexpr int max_truth_line_chars = 4096;
constexpr int max_truth_points = 200000;
constexpr int max_analysis_epochs = 200000;
}

AnalysisEngine::AnalysisEngine(QObject *parent)
    : QObject(parent)
{
}

bool AnalysisEngine::load_truth_csv(const QString &path, QString *error)
{
    const QFileInfo info(path);
    if (info.size() > max_truth_file_bytes) {
        if (error != nullptr) {
            *error = "Truth CSV is too large. Maximum allowed size is 16 MiB.";
        }
        return false;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (error != nullptr) {
            *error = file.errorString();
        }
        return false;
    }

    truth_points_.clear();
    QTextStream stream(&file);
    bool first_line = true;
    while (!stream.atEnd()) {
        const QString line = stream.readLine().trimmed();
        if (line.size() > max_truth_line_chars) {
            if (error != nullptr) {
                *error = "Truth CSV contains an overlong line.";
            }
            truth_points_.clear();
            return false;
        }
        if (line.isEmpty()) {
            continue;
        }
        if (first_line && line.toLower().startsWith("timestamp")) {
            first_line = false;
            continue;
        }
        first_line = false;

        const QStringList fields = line.split(',');
        if (fields.size() < 4) {
            continue;
        }

        TruthPoint point;
        bool latitude_ok = false;
        bool longitude_ok = false;
        bool altitude_ok = false;
        point.timestamp = QDateTime::fromString(fields.at(0).trimmed(), Qt::ISODate);
        point.latitude = fields.at(1).trimmed().toDouble(&latitude_ok);
        point.longitude = fields.at(2).trimmed().toDouble(&longitude_ok);
        point.altitude = fields.at(3).trimmed().toDouble(&altitude_ok);
        const bool coordinates_valid = latitude_ok
                                       && longitude_ok
                                       && altitude_ok
                                       && std::isfinite(point.latitude)
                                       && std::isfinite(point.longitude)
                                       && std::isfinite(point.altitude)
                                       && point.latitude >= -90.0
                                       && point.latitude <= 90.0
                                       && point.longitude >= -180.0
                                       && point.longitude <= 180.0;
        if (point.timestamp.isValid() && coordinates_valid) {
            truth_points_.append(point);
            if (truth_points_.size() > max_truth_points) {
                if (error != nullptr) {
                    *error = "Truth CSV contains too many points.";
                }
                truth_points_.clear();
                return false;
            }
        }
    }

    return !truth_points_.isEmpty();
}

void AnalysisEngine::clear_epochs(void)
{
    epochs_.clear();
}

void AnalysisEngine::add_epoch(const GnssEpoch &epoch)
{
    if (epoch.timestamp.isValid() || epoch.has_fix) {
        epochs_.append(epoch);
        while (epochs_.size() > max_analysis_epochs) {
            epochs_.removeFirst();
        }
    }
}

AnalysisSummary AnalysisEngine::summarize(void) const
{
    AnalysisSummary summary;
    double satellite_sum = 0.0;
    double hdop_sum = 0.0;
    double pdop_sum = 0.0;
    double cn0_sum = 0.0;
    int cn0_count = 0;
    QList<double> horizontal_errors;
    QList<double> altitude_errors;

    summary.epoch_count = epochs_.size();
    if (summary.epoch_count == 0) {
        return summary;
    }

    for (const GnssEpoch &epoch : epochs_) {
        if (epoch.has_fix) {
            summary.fixed_count++;
        }
        satellite_sum += epoch.satellites_used;
        hdop_sum += epoch.hdop;
        pdop_sum += epoch.pdop;

        for (const SatelliteInfo &satellite : epoch.satellites) {
            if (satellite.cn0 > 0) {
                cn0_sum += satellite.cn0;
                cn0_count++;
            }
        }

        const TruthPoint *truth = nearest_truth(epoch.timestamp);
        if (truth != nullptr && epoch.has_fix) {
            const double horizontal = horizontal_distance_m(epoch.latitude,
                                                            epoch.longitude,
                                                            truth->latitude,
                                                            truth->longitude);
            const double altitude = qAbs(epoch.altitude - truth->altitude);
            horizontal_errors.append(horizontal);
            altitude_errors.append(altitude);
            summary.horizontal_max = qMax(summary.horizontal_max, horizontal);
            summary.altitude_max = qMax(summary.altitude_max, altitude);
        }
    }

    summary.fix_rate = 100.0 * summary.fixed_count / summary.epoch_count;
    summary.average_satellites = satellite_sum / summary.epoch_count;
    summary.average_hdop = hdop_sum / summary.epoch_count;
    summary.average_pdop = pdop_sum / summary.epoch_count;
    summary.average_cn0 = cn0_count == 0 ? 0.0 : cn0_sum / cn0_count;

    double horizontal_square_sum = 0.0;
    for (double value : horizontal_errors) {
        horizontal_square_sum += value * value;
    }
    if (!horizontal_errors.isEmpty()) {
        summary.horizontal_rms = qSqrt(horizontal_square_sum / horizontal_errors.size());
        summary.horizontal_cep50 = percentile(horizontal_errors, 0.50);
        summary.horizontal_cep95 = percentile(horizontal_errors, 0.95);
    }

    double altitude_square_sum = 0.0;
    for (double value : altitude_errors) {
        altitude_square_sum += value * value;
    }
    if (!altitude_errors.isEmpty()) {
        summary.altitude_rms = qSqrt(altitude_square_sum / altitude_errors.size());
    }

    return summary;
}

QString AnalysisEngine::summary_text(void) const
{
    const AnalysisSummary summary = summarize();
    return QString("Epochs: %1\nFix rate: %2%\nAvg satellites: %3\nAvg HDOP: %4\n"
                   "Avg CN0: %5 dB-Hz\nHorizontal RMS: %6 m\nCEP95: %7 m")
        .arg(summary.epoch_count)
        .arg(summary.fix_rate, 0, 'f', 1)
        .arg(summary.average_satellites, 0, 'f', 1)
        .arg(summary.average_hdop, 0, 'f', 2)
        .arg(summary.average_cn0, 0, 'f', 1)
        .arg(summary.horizontal_rms, 0, 'f', 2)
        .arg(summary.horizontal_cep95, 0, 'f', 2);
}

const QList<GnssEpoch> &AnalysisEngine::epochs(void) const
{
    return epochs_;
}

const TruthPoint *AnalysisEngine::nearest_truth(const QDateTime &timestamp) const
{
    if (!timestamp.isValid() || truth_points_.isEmpty()) {
        return nullptr;
    }

    const TruthPoint *best = nullptr;
    qint64 best_delta = 3000;
    for (const TruthPoint &point : truth_points_) {
        const qint64 delta = qAbs(point.timestamp.msecsTo(timestamp));
        if (delta < best_delta) {
            best_delta = delta;
            best = &point;
        }
    }
    return best;
}

double AnalysisEngine::horizontal_distance_m(double lat1, double lon1, double lat2, double lon2) const
{
    const double earth_radius = 6378137.0;
    const double dlat = qDegreesToRadians(lat2 - lat1);
    const double dlon = qDegreesToRadians(lon2 - lon1);
    const double a = qSin(dlat / 2.0) * qSin(dlat / 2.0)
                     + qCos(qDegreesToRadians(lat1)) * qCos(qDegreesToRadians(lat2))
                           * qSin(dlon / 2.0) * qSin(dlon / 2.0);
    return 2.0 * earth_radius * qAtan2(qSqrt(a), qSqrt(1.0 - a));
}

double AnalysisEngine::percentile(QList<double> values, double ratio) const
{
    if (values.isEmpty()) {
        return 0.0;
    }

    std::sort(values.begin(), values.end());
    const int index = qBound(0, static_cast<int>(qRound((values.size() - 1) * ratio)), values.size() - 1);
    return values.at(index);
}
