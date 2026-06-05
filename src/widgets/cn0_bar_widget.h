#ifndef CN0_BAR_WIDGET_H
#define CN0_BAR_WIDGET_H

#include "../nmea/gnss_types.h"

#include <QColor>
#include <QString>
#include <QStringList>
#include <QWidget>

class QPainter;
class QRect;

class Cn0BarWidget : public QWidget {
    Q_OBJECT

public:
    explicit Cn0BarWidget(QWidget *parent = nullptr);

    void set_satellites(const QList<SatelliteInfo> &satellites);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QStringList constellation_order(void) const;
    QList<SatelliteInfo> top_satellites_for_constellation(const QString &constellation) const;
    QColor constellation_color(const QString &constellation) const;
    QString constellation_label_prefix(const QString &constellation) const;
    void draw_constellation_group(QPainter *painter,
                                  const QRect &group_rect,
                                  const QString &constellation,
                                  const QList<SatelliteInfo> &top) const;

    QList<SatelliteInfo> satellites_;
};

#endif
