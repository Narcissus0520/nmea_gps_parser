#ifndef OFFLINE_MAP_WIDGET_H
#define OFFLINE_MAP_WIDGET_H

#include <QList>
#include <QPointF>
#include <QWidget>

class OfflineMapWidget : public QWidget {
    Q_OBJECT

public:
    explicit OfflineMapWidget(QWidget *parent = nullptr);

    void set_tile_root(const QString &path);
    void add_position(double latitude, double longitude);
    void clear_track(void);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QPointF geo_to_world(double latitude, double longitude, int zoom) const;

    QString tile_root_ = "tiles";
    QList<QPointF> track_;
    int zoom_ = 16;
};

#endif
