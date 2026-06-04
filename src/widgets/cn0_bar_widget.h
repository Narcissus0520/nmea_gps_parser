#ifndef CN0_BAR_WIDGET_H
#define CN0_BAR_WIDGET_H

#include "../nmea/gnss_types.h"

#include <QWidget>

class Cn0BarWidget : public QWidget {
    Q_OBJECT

public:
    explicit Cn0BarWidget(QWidget *parent = nullptr);

    void set_satellites(const QList<SatelliteInfo> &satellites);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QList<SatelliteInfo> top_satellites(void) const;

    QList<SatelliteInfo> satellites_;
};

#endif
