#ifndef ANALYSIS_ENGINE_H
#define ANALYSIS_ENGINE_H

#include "../nmea/gnss_types.h"

#include <QObject>
#include <QString>

struct AnalysisSummary {
    int epoch_count = 0;
    int fixed_count = 0;
    double fix_rate = 0.0;
    double average_satellites = 0.0;
    double average_hdop = 0.0;
    double average_pdop = 0.0;
    double average_cn0 = 0.0;
    double horizontal_rms = 0.0;
    double horizontal_max = 0.0;
    double horizontal_cep50 = 0.0;
    double horizontal_cep95 = 0.0;
    double altitude_rms = 0.0;
    double altitude_max = 0.0;
};

class AnalysisEngine : public QObject {
    Q_OBJECT

public:
    explicit AnalysisEngine(QObject *parent = nullptr);

    bool load_truth_csv(const QString &path, QString *error);
    void clear_epochs(void);
    void add_epoch(const GnssEpoch &epoch);
    AnalysisSummary summarize(void) const;
    QString summary_text(void) const;
    const QList<GnssEpoch> &epochs(void) const;

private:
    const TruthPoint *nearest_truth(const QDateTime &timestamp) const;
    double horizontal_distance_m(double lat1, double lon1, double lat2, double lon2) const;
    double percentile(QList<double> values, double ratio) const;

    QList<GnssEpoch> epochs_;
    QList<TruthPoint> truth_points_;
};

#endif
