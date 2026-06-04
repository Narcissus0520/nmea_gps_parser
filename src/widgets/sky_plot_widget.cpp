#include "sky_plot_widget.h"

#include <QPainter>
#include <QPaintEvent>
#include <QtMath>

SkyPlotWidget::SkyPlotWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(360, 360);
}

void SkyPlotWidget::set_satellites(const QList<SatelliteInfo> &satellites)
{
    satellites_ = satellites;
    update();
}

void SkyPlotWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), QColor("#061522"));

    const QRectF plot_rect = rect().adjusted(18, 24, -18, -34);
    const QPointF center(plot_rect.center());
    const double radius = qMin(plot_rect.width(), plot_rect.height()) / 2.0 - 8.0;

    painter.setPen(QPen(QColor("#00c8ff"), 1));
    painter.drawText(10, 18, "Satellite Distribution");

    painter.setPen(QPen(QColor(0, 122, 99, 180), 1));
    painter.drawEllipse(center, radius, radius);
    painter.setPen(QPen(QColor(0, 122, 99, 130), 1, Qt::DashLine));
    painter.drawEllipse(center, radius * 0.75, radius * 0.75);
    painter.drawEllipse(center, radius * 0.50, radius * 0.50);
    painter.drawEllipse(center, radius * 0.25, radius * 0.25);

    painter.setPen(QPen(QColor(0, 90, 112, 160), 1));
    for (int degree = 0; degree < 360; degree += 45) {
        const double angle = qDegreesToRadians(static_cast<double>(degree));
        const QPointF end(center.x() + qCos(angle) * radius, center.y() + qSin(angle) * radius);
        painter.drawLine(center, end);
    }

    painter.setFont(QFont("Consolas", 10, QFont::Bold));
    painter.setPen(QColor("#66899b"));
    painter.drawText(QRectF(center.x() - 20, center.y() - radius - 24, 40, 18), Qt::AlignCenter, "N");
    painter.drawText(QRectF(center.x() + radius + 6, center.y() - 9, 24, 18), Qt::AlignCenter, "E");
    painter.drawText(QRectF(center.x() - 20, center.y() + radius + 6, 40, 18), Qt::AlignCenter, "S");
    painter.drawText(QRectF(center.x() - radius - 30, center.y() - 9, 24, 18), Qt::AlignCenter, "W");
    painter.setPen(QColor("#ff00d4"));
    painter.drawText(QRectF(center.x() - 20, center.y() - radius - 24, 40, 18), Qt::AlignCenter, "N");

    painter.setFont(QFont("Consolas", 8));
    painter.setPen(QColor("#66899b"));
    painter.drawText(QPointF(center.x() + 8, center.y() - radius * 0.75), "75°");
    painter.drawText(QPointF(center.x() + 8, center.y() - radius * 0.50), "50°");
    painter.drawText(QPointF(center.x() + 8, center.y() - radius * 0.25), "25°");

    for (const SatelliteInfo &sat : satellites_) {
        const double azimuth = qDegreesToRadians(static_cast<double>(sat.azimuth) - 90.0);
        const double sat_radius = radius * (90.0 - qBound(0, sat.elevation, 90)) / 90.0;
        const QPointF point(center.x() + qCos(azimuth) * sat_radius,
                            center.y() + qSin(azimuth) * sat_radius);
        QColor color("#00ff91");
        if (sat.constellation == "BDS") {
            color = QColor("#00c8ff");
        } else if (sat.constellation == "GLO") {
            color = QColor("#ffff00");
        } else if (sat.constellation == "GAL") {
            color = QColor("#ff00d4");
        }
        if (sat.cn0 > 0 && sat.cn0 < 30) {
            color = QColor("#ffae00");
        }

        const double size = qBound(8.0, 7.0 + sat.cn0 / 5.0, 16.0);
        painter.setBrush(color);
        painter.setPen(QPen(QColor("#f0ffff"), 1));
        painter.drawEllipse(point, size, size);
        painter.setPen(QColor("#001b17"));
        painter.setFont(QFont("Consolas", 7, QFont::Bold));
        painter.drawText(QRectF(point.x() - size, point.y() - 6, size * 2.0, 12),
                         Qt::AlignCenter,
                         QString("%1%2").arg(sat.constellation.left(1)).arg(sat.prn));
    }

    painter.setPen(QColor("#00c8ff"));
    painter.setFont(QFont("Consolas", 9, QFont::Bold));
    painter.drawText(rect().adjusted(10, 0, -10, -8),
                     Qt::AlignLeft | Qt::AlignBottom,
                     QString("可见卫星 / VISIBLE: %1").arg(satellites_.size()));
}
