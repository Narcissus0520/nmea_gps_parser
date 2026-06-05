#ifndef SKY_PLOT_WIDGET_H
#define SKY_PLOT_WIDGET_H

#include "../nmea/gnss_types.h"

#include <QList>
#include <QRectF>
#include <QWidget>

class QMouseEvent;

class SkyPlotWidget : public QWidget {
    Q_OBJECT

public:
    explicit SkyPlotWidget(QWidget *parent = nullptr);

    void set_satellites(const QList<SatelliteInfo> &satellites);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    struct RenderedSatellite {
        SatelliteInfo satellite;
        QRectF hit_rect;
    };

    QList<SatelliteInfo> satellites_;
    QList<RenderedSatellite> rendered_satellites_;
};

#endif
