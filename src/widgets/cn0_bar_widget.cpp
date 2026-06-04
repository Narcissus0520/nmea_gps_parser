#include "cn0_bar_widget.h"

#include <QPainter>
#include <algorithm>

Cn0BarWidget::Cn0BarWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(280, 180);
}

void Cn0BarWidget::set_satellites(const QList<SatelliteInfo> &satellites)
{
    satellites_ = satellites;
    update();
}

void Cn0BarWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), QColor("#061522"));

    const QRect chart = rect().adjusted(18, 24, -18, -28);
    painter.setPen(QColor("#00c8ff"));
    painter.drawText(rect().adjusted(10, 6, -10, -6), Qt::AlignTop | Qt::AlignLeft, "TOP7 CN0");
    painter.setPen(QColor("#007a63"));
    painter.drawRect(chart);
    painter.setPen(QPen(QColor(0, 122, 99, 90), 1, Qt::DashLine));
    for (int i = 1; i < 4; ++i) {
        const int y = chart.bottom() - chart.height() * i / 4;
        painter.drawLine(chart.left(), y, chart.right(), y);
    }

    const QList<SatelliteInfo> top = top_satellites();
    if (top.isEmpty()) {
        painter.setPen(QColor("#66899b"));
        painter.drawText(chart, Qt::AlignCenter, "No satellite CN0");
        return;
    }

    const int gap = 8;
    const int bar_width = qMax(8, (chart.width() - gap * (top.size() + 1)) / top.size());
    for (int i = 0; i < top.size(); ++i) {
        const SatelliteInfo &sat = top.at(i);
        const double ratio = qBound(0.0, sat.cn0 / 60.0, 1.0);
        const int bar_height = static_cast<int>(chart.height() * ratio);
        const int x = chart.left() + gap + i * (bar_width + gap);
        const int y = chart.bottom() - bar_height;
        QRect bar(x, y, bar_width, bar_height);

        QLinearGradient gradient(bar.topLeft(), bar.bottomLeft());
        gradient.setColorAt(0.0, QColor("#00ff91"));
        gradient.setColorAt(0.55, QColor("#00c8ff"));
        gradient.setColorAt(1.0, QColor("#00695d"));
        painter.fillRect(bar, gradient);
        painter.setPen(QColor("#ffff00"));
        painter.drawText(QRect(x - 4, chart.bottom() + 2, bar_width + 8, 20),
                         Qt::AlignCenter,
                         QString("%1%2").arg(sat.constellation.left(1)).arg(sat.prn));
        painter.setPen(QColor("#f0ffff"));
        painter.drawText(QRect(x - 4, y - 18, bar_width + 8, 16),
                         Qt::AlignCenter,
                         QString::number(sat.cn0));
    }
}

QList<SatelliteInfo> Cn0BarWidget::top_satellites(void) const
{
    QList<SatelliteInfo> top = satellites_;
    std::sort(top.begin(), top.end(), [](const SatelliteInfo &left, const SatelliteInfo &right) {
        return left.cn0 > right.cn0;
    });
    while (top.size() > 7) {
        top.removeLast();
    }
    return top;
}
