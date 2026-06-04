#ifndef SKY_PLOT_WIDGET_H
#define SKY_PLOT_WIDGET_H

#include "../nmea/gnss_types.h"

#include <QWidget>

class SkyPlotWidget : public QWidget {
    Q_OBJECT

public:
    explicit SkyPlotWidget(QWidget *parent = nullptr);

    void set_satellites(const QList<SatelliteInfo> &satellites);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QList<SatelliteInfo> satellites_;
};

#endif
