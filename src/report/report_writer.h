#ifndef REPORT_WRITER_H
#define REPORT_WRITER_H

#include "../analysis/analysis_engine.h"

#include <QString>

class ReportWriter {
public:
    static bool write_html(const QString &path, const AnalysisSummary &summary, const QList<GnssEpoch> &epochs, QString *error);
    static bool write_csv(const QString &path, const QList<GnssEpoch> &epochs, QString *error);
};

#endif
