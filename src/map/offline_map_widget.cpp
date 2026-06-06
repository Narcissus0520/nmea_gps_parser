#include "offline_map_widget.h"

#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QPainter>
#include <QPainterPath>
#include <QWheelEvent>
#include <QtMath>

namespace {
constexpr double default_preview_latitude = 22.543096;
constexpr double default_preview_longitude = 114.057865;
}

OfflineMapWidget::OfflineMapWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(420, 320);
    tile_root_ = choose_default_tile_root();
    setFocusPolicy(Qt::WheelFocus);
}

void OfflineMapWidget::set_tile_root(const QString &path)
{
    tile_root_ = QDir::cleanPath(path);
    update();
}

QString OfflineMapWidget::tile_root(void) const
{
    return tile_root_;
}

void OfflineMapWidget::zoom_in(void)
{
    zoom_ = qMin(20, zoom_ + 1);
    update();
}

void OfflineMapWidget::zoom_out(void)
{
    zoom_ = qMax(1, zoom_ - 1);
    update();
}

int OfflineMapWidget::zoom(void) const
{
    return zoom_;
}

int OfflineMapWidget::track_count(void) const
{
    return track_.size();
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

    const bool has_track = !track_.isEmpty();
    const QPointF current_geo = has_track
                                    ? track_.last()
                                    : QPointF(default_preview_longitude, default_preview_latitude);
    const QPointF center_world = geo_to_world(current_geo.y(), current_geo.x(), zoom_);
    const QPointF viewport_center(width() / 2.0, height() / 2.0);
    const int tile_size = 256;
    const int center_tile_x = static_cast<int>(qFloor(center_world.x() / tile_size));
    const int center_tile_y = static_cast<int>(qFloor(center_world.y() / tile_size));
    int loaded_tiles = 0;

    for (int dx = -2; dx <= 2; ++dx) {
        for (int dy = -2; dy <= 2; ++dy) {
            const int tile_x = center_tile_x + dx;
            const int tile_y = center_tile_y + dy;
            const QString path = tile_path(zoom_, tile_x, tile_y);
            const QPointF tile_world(tile_x * tile_size, tile_y * tile_size);
            const QPointF screen = viewport_center + (tile_world - center_world);
            QPixmap pixmap(path);
            if (!pixmap.isNull()) {
                painter.drawPixmap(screen.toPoint(), pixmap);
                loaded_tiles++;
            } else {
                painter.fillRect(QRectF(screen, QSizeF(tile_size, tile_size)), QColor("#0d1322"));
                painter.setPen(QColor("#1d3b52"));
                painter.drawRect(QRectF(screen, QSizeF(tile_size, tile_size)));
            }
        }
    }

    if (loaded_tiles == 0) {
        painter.setPen(QPen(QColor(0, 122, 99, 100), 1));
        for (int x = 0; x < width(); x += 32) {
            painter.drawLine(x, 0, x, height());
        }
        for (int y = 0; y < height(); y += 32) {
            painter.drawLine(0, y, width(), y);
        }
        painter.setPen(QColor("#66899b"));
        painter.drawText(rect().adjusted(12, 30, -12, -12),
                         Qt::AlignTop | Qt::AlignLeft,
                         QString("No tile image found. Drawing trajectory on fallback grid.\n"
                                 "Tile root: %1\n"
                                 "Expected center tile: %2")
                             .arg(QDir::toNativeSeparators(tile_root_),
                                  QDir::toNativeSeparators(tile_path(zoom_, center_tile_x, center_tile_y))));
    }

    if (has_track) {
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
    } else {
        painter.setPen(QColor("#00c8ff"));
        painter.setFont(QFont("Consolas", 10, QFont::Bold));
        painter.drawText(rect().adjusted(10, 10, -10, -10),
                         Qt::AlignTop | Qt::AlignHCenter,
                         "Waiting for GNSS position");
    }

    painter.setPen(QColor("#00c8ff"));
    painter.setFont(QFont("Consolas", 9));
    painter.drawText(rect().adjusted(10, 0, -10, -8),
                     Qt::AlignLeft | Qt::AlignBottom,
                     QString("z%1 | points:%2 | tiles:%3")
                         .arg(zoom_)
                         .arg(track_.size())
                         .arg(QDir::toNativeSeparators(tile_root_)));
    painter.setPen(QColor("#b8f7ef"));
    painter.drawText(rect().adjusted(10, 0, -10, -8),
                     Qt::AlignRight | Qt::AlignBottom,
                     "Tiles (C) OpenStreetMap contributors");
}

void OfflineMapWidget::wheelEvent(QWheelEvent *event)
{
    if (event->angleDelta().y() > 0) {
        zoom_in();
    } else if (event->angleDelta().y() < 0) {
        zoom_out();
    }
    event->accept();
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

QString OfflineMapWidget::choose_default_tile_root(void) const
{
    const QStringList candidates = default_tile_root_candidates();
    for (const QString &candidate : candidates) {
        if (tile_root_has_tiles(candidate)) {
            return candidate;
        }
    }

    return candidates.isEmpty() ? QString() : candidates.first();
}

QStringList OfflineMapWidget::default_tile_root_candidates(void) const
{
    QStringList candidates;
    const QString app_dir = QCoreApplication::applicationDirPath();
    const QString work_dir = QDir::currentPath();

    candidates.append(QDir(app_dir).filePath("tiles"));
    candidates.append(QDir(work_dir).filePath("tiles"));
    candidates.append(QDir(work_dir).filePath("release/tiles"));
    candidates.append(QDir(app_dir).filePath("../release/tiles"));
    candidates.append(QDir(app_dir).filePath("../tiles"));
    candidates.append(QDir(work_dir).filePath("dist/GnssCyberpunkHost/tiles"));

    QStringList cleaned;
    for (const QString &candidate : candidates) {
        const QString clean = QDir::cleanPath(candidate);
        if (!cleaned.contains(clean)) {
            cleaned.append(clean);
        }
    }
    return cleaned;
}

QString OfflineMapWidget::tile_path(int zoom, int tile_x, int tile_y) const
{
    return QDir(tile_root_).filePath(QString("%1/%2/%3.png").arg(zoom).arg(tile_x).arg(tile_y));
}

bool OfflineMapWidget::tile_root_has_tiles(const QString &path) const
{
    const QDir root(path);
    if (!root.exists()) {
        return false;
    }

    QDirIterator iterator(root.absolutePath(), QStringList() << "*.png", QDir::Files, QDirIterator::Subdirectories);
    return iterator.hasNext();
}

void OfflineMapWidget::draw_empty_grid(QPainter *painter, const QString &message) const
{
    painter->setPen(QColor("#264a68"));
    for (int x = 0; x < width(); x += 32) {
        painter->drawLine(x, 0, x, height());
    }
    for (int y = 0; y < height(); y += 32) {
        painter->drawLine(0, y, width(), y);
    }

    painter->setPen(QColor("#00c8ff"));
    painter->setFont(QFont("Consolas", 10, QFont::Bold));
    painter->drawText(rect().adjusted(10, 10, -10, -10), Qt::AlignCenter, message);
    painter->setPen(QColor("#66899b"));
    painter->setFont(QFont("Consolas", 8));
    painter->drawText(rect().adjusted(10, 0, -10, -8),
                      Qt::AlignLeft | Qt::AlignBottom,
                      QString("z%1 | tiles:%2").arg(zoom_).arg(QDir::toNativeSeparators(tile_root_)));
}
