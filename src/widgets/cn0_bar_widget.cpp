#include "cn0_bar_widget.h"

#include <QLinearGradient>
#include <QPainter>
#include <algorithm>

Cn0BarWidget::Cn0BarWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(300, 140);
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

    painter.setPen(QColor("#00c8ff"));
    painter.setFont(QFont("Consolas", 9, QFont::Bold));
    painter.drawText(rect().adjusted(10, 6, -10, -6),
                     Qt::AlignTop | Qt::AlignLeft,
                     "CN0 TOP7 BY MODE");

    const QStringList constellations = constellation_order();
    const QRect area = rect().adjusted(10, 30, -10, -10);
    if (constellations.isEmpty()) {
        painter.setPen(QColor("#007a63"));
        painter.drawRect(area);
        painter.setPen(QColor("#66899b"));
        painter.drawText(area, Qt::AlignCenter, "No satellite CN0");
        return;
    }

    int columns = constellations.size() == 1 ? 1 : 2;
    if (height() < 190 && constellations.size() <= 4) {
        columns = constellations.size();
    }
    const int rows = (constellations.size() + columns - 1) / columns;
    const int gap = 8;
    const int group_width = (area.width() - gap * (columns - 1)) / columns;
    const int group_height = (area.height() - gap * (rows - 1)) / rows;

    for (int i = 0; i < constellations.size(); ++i) {
        const int row = i / columns;
        const int column = i % columns;
        const QRect group_rect(area.left() + column * (group_width + gap),
                               area.top() + row * (group_height + gap),
                               group_width,
                               group_height);
        const QString constellation = constellations.at(i);
        draw_constellation_group(&painter,
                                 group_rect,
                                 constellation,
                                 top_satellites_for_constellation(constellation));
    }
}

QStringList Cn0BarWidget::constellation_order(void) const
{
    QStringList present;
    for (const SatelliteInfo &satellite : satellites_) {
        if (satellite.cn0 <= 0) {
            continue;
        }

        const QString constellation = satellite.constellation.isEmpty() ? "GNSS" : satellite.constellation;
        if (!present.contains(constellation)) {
            present.append(constellation);
        }
    }

    const QStringList preferred = {"GPS", "BDS", "GLO", "GAL", "QZSS", "GNSS"};
    QStringList ordered;
    for (const QString &constellation : preferred) {
        if (present.removeAll(constellation) > 0) {
            ordered.append(constellation);
        }
    }
    present.sort();
    ordered.append(present);
    return ordered;
}

QList<SatelliteInfo> Cn0BarWidget::top_satellites_for_constellation(const QString &constellation) const
{
    QList<SatelliteInfo> top;
    for (const SatelliteInfo &satellite : satellites_) {
        const QString satellite_constellation = satellite.constellation.isEmpty() ? "GNSS" : satellite.constellation;
        if (satellite_constellation == constellation && satellite.cn0 > 0) {
            top.append(satellite);
        }
    }

    std::sort(top.begin(), top.end(), [](const SatelliteInfo &left, const SatelliteInfo &right) {
        if (left.cn0 == right.cn0) {
            return left.prn < right.prn;
        }
        return left.cn0 > right.cn0;
    });
    while (top.size() > 7) {
        top.removeLast();
    }
    return top;
}

QColor Cn0BarWidget::constellation_color(const QString &constellation) const
{
    if (constellation == "BDS") {
        return QColor("#00c8ff");
    }
    if (constellation == "GLO") {
        return QColor("#ffff00");
    }
    if (constellation == "GAL") {
        return QColor("#ff00d4");
    }
    if (constellation == "QZSS") {
        return QColor("#ffae00");
    }
    if (constellation == "GNSS") {
        return QColor("#b8ff4d");
    }
    return QColor("#00ff91");
}

QString Cn0BarWidget::constellation_label_prefix(const QString &constellation) const
{
    if (constellation == "GPS") {
        return "G";
    }
    if (constellation == "BDS") {
        return "B";
    }
    if (constellation == "GLO") {
        return "R";
    }
    if (constellation == "GAL") {
        return "E";
    }
    if (constellation == "QZSS") {
        return "Q";
    }
    return constellation.left(1);
}

void Cn0BarWidget::draw_constellation_group(QPainter *painter,
                                            const QRect &group_rect,
                                            const QString &constellation,
                                            const QList<SatelliteInfo> &top) const
{
    const QRect panel = group_rect.adjusted(0, 0, -1, -1);
    const QColor color = constellation_color(constellation);
    painter->setPen(QPen(color.darker(125), 1));
    painter->setBrush(QColor(0, 200, 255, 8));
    painter->drawRect(panel);

    painter->setBrush(Qt::NoBrush);
    painter->setPen(color);
    const bool compact = panel.height() < 95;
    painter->setFont(QFont("Consolas", compact ? 7 : 8, QFont::Bold));
    painter->drawText(panel.adjusted(8, 4, -8, -4),
                      Qt::AlignTop | Qt::AlignLeft,
                      QString("%1 TOP7").arg(constellation));

    const QRect chart = panel.adjusted(6, compact ? 18 : 26, -6, compact ? -13 : -18);
    if (chart.width() <= 18 || chart.height() <= 10) {
        return;
    }

    painter->setPen(QPen(QColor(0, 122, 99, 90), 1, Qt::DashLine));
    for (int i = 1; i < 4; ++i) {
        const int y = chart.bottom() - chart.height() * i / 4;
        painter->drawLine(chart.left(), y, chart.right(), y);
    }

    if (top.isEmpty()) {
        painter->setPen(QColor("#66899b"));
        painter->drawText(chart, Qt::AlignCenter, "No CN0");
        return;
    }

    const int gap = qMax(3, chart.width() / 70);
    const int bar_width = qMax(5, (chart.width() - gap * (top.size() + 1)) / top.size());
    for (int i = 0; i < top.size(); ++i) {
        const SatelliteInfo &sat = top.at(i);
        const double ratio = qBound(0.0, sat.cn0 / 60.0, 1.0);
        const int bar_height = qMax(1, static_cast<int>(chart.height() * ratio));
        const int x = chart.left() + gap + i * (bar_width + gap);
        const int y = chart.bottom() - bar_height;
        const QRect bar(x, y, bar_width, bar_height);

        QLinearGradient gradient(bar.topLeft(), bar.bottomLeft());
        gradient.setColorAt(0.0, color.lighter(135));
        gradient.setColorAt(0.55, color);
        gradient.setColorAt(1.0, QColor("#003b45"));
        painter->fillRect(bar, gradient);
        if (!compact || chart.height() >= 30) {
            painter->setPen(QColor("#f0ffff"));
            painter->setFont(QFont("Consolas", 7, QFont::Bold));
            painter->drawText(QRect(x - 4, y - 14, bar_width + 8, 13),
                              Qt::AlignCenter,
                              QString::number(sat.cn0));
        }
        painter->setPen(color.lighter(120));
        painter->setFont(QFont("Consolas", compact ? 6 : 7, QFont::Bold));
        painter->drawText(QRect(x - 6, chart.bottom() + 1, bar_width + 12, 16),
                          Qt::AlignCenter,
                          QString("%1%2").arg(constellation_label_prefix(constellation)).arg(sat.prn));
    }
}
