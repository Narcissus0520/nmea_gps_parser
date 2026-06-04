#include "offline_map_widget.h"

#include <QDir>
#include <QPainter>
#include <QPainterPath>
#include <QtMath>

OfflineMapWidget::OfflineMapWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(420, 320);
}

void OfflineMapWidget::set_tile_root(const QString &path)
{
    tile_root_ = path;
    update();
}

void OfflineMapWidget::add_position(double latitude, double longitude)
{
    if (qAbs(latitude) < 0.000001 && qAbs(longitude) < 0.000001) {
        return;
    }
    track_.append(QPointF(longitude, latitude));
    update();
}

void OfflineMapWidget::clear_track(void)
{
    track_.clear();
    update();
}

void OfflineMapWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), QColor("#071018"));

    if (track_.isEmpty()) {
        painter.setPen(QColor("#264a68"));
        for (int x = 0; x < width(); x += 32) {
            painter.drawLine(x, 0, x, height());
        }
        for (int y = 0; y < height(); y += 32) {
            painter.drawLine(0, y, width(), y);
        }
        painter.setPen(QColor("#13f7ff"));
        painter.drawText(rect(), Qt::AlignCenter, "Offline map / waiting for position");
        return;
    }

    const QPointF current_geo = track_.last();
    const QPointF center_world = geo_to_world(current_geo.y(), current_geo.x(), zoom_);
    const QPointF viewport_center(width() / 2.0, height() / 2.0);
    const int tile_size = 256;

    for (int dx = -2; dx <= 2; ++dx) {
        for (int dy = -2; dy <= 2; ++dy) {
            const int tile_x = static_cast<int>(qFloor(center_world.x() / tile_size)) + dx;
            const int tile_y = static_cast<int>(qFloor(center_world.y() / tile_size)) + dy;
            const QString path = QString("%1/%2/%3/%4.png").arg(tile_root_).arg(zoom_).arg(tile_x).arg(tile_y);
            const QPointF tile_world(tile_x * tile_size, tile_y * tile_size);
            const QPointF screen = viewport_center + (tile_world - center_world);
            QPixmap pixmap(path);
            if (!pixmap.isNull()) {
                painter.drawPixmap(screen.toPoint(), pixmap);
            } else {
                painter.fillRect(QRectF(screen, QSizeF(tile_size, tile_size)), QColor("#0d1322"));
                painter.setPen(QColor("#1d3b52"));
                painter.drawRect(QRectF(screen, QSizeF(tile_size, tile_size)));
            }
        }
    }

    QPainterPath path;
    bool first = true;
    for (const QPointF &geo : track_) {
        const QPointF world = geo_to_world(geo.y(), geo.x(), zoom_);
        const QPointF screen = viewport_center + (world - center_world);
        if (first) {
            path.moveTo(screen);
            first = false;
        } else {
            path.lineTo(screen);
        }
    }

    painter.setPen(QPen(QColor("#ff2bd6"), 3));
    painter.drawPath(path);
    painter.setBrush(QColor("#13f7ff"));
    painter.setPen(QColor("#ffffff"));
    painter.drawEllipse(viewport_center, 7, 7);
}

QPointF OfflineMapWidget::geo_to_world(double latitude, double longitude, int zoom) const
{
    static const double pi = 3.14159265358979323846;
    const double lat_rad = qDegreesToRadians(latitude);
    const double scale = 256.0 * qPow(2.0, zoom);
    const double x = (longitude + 180.0) / 360.0 * scale;
    const double y = (0.5 - qLn((1.0 + qSin(lat_rad)) / (1.0 - qSin(lat_rad))) / (4.0 * pi)) * scale;
    return QPointF(x, y);
}
