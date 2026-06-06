#ifndef OFFLINE_MAP_WIDGET_H
#define OFFLINE_MAP_WIDGET_H

#include <QList>
#include <QPointF>
#include <QString>
#include <QStringList>
#include <QWidget>

class OfflineMapWidget : public QWidget {
    Q_OBJECT

public:
    explicit OfflineMapWidget(QWidget *parent = nullptr);

    void set_tile_root(const QString &path);
    QString tile_root(void) const;
    void zoom_in(void);
    void zoom_out(void);
    int zoom(void) const;
    int track_count(void) const;
    void add_position(double latitude, double longitude);
    void clear_track(void);

protected:
    void paintEvent(QPaintEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    QPointF geo_to_world(double latitude, double longitude, int zoom) const;
    void draw_empty_grid(QPainter *painter, const QString &message) const;
    QString choose_default_tile_root(void) const;
    QStringList default_tile_root_candidates(void) const;
    QString tile_path(int zoom, int tile_x, int tile_y) const;
    bool tile_root_has_tiles(const QString &path) const;

    QString tile_root_;
    QList<QPointF> track_;
    int zoom_ = 16;
};

#endif
