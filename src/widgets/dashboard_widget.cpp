#include "dashboard_widget.h"

#include <QPainter>
#include <QPaintEvent>
#include <QSizePolicy>

DashboardWidget::DashboardWidget(const QString &title, QWidget *parent)
    : QWidget(parent),
      title_(title)
{
    setMinimumSize(82, 56);
    setMaximumHeight(76);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
}

QSize DashboardWidget::sizeHint(void) const
{
    return QSize(92, 64);
}

QSize DashboardWidget::minimumSizeHint(void) const
{
    return QSize(82, 56);
}

void DashboardWidget::set_value(const QString &value)
{
    value_ = value;
    update();
}

void DashboardWidget::set_subtitle(const QString &subtitle)
{
    subtitle_ = subtitle;
    update();
}

void DashboardWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const QRectF rect = this->rect().adjusted(3, 3, -3, -3);
    painter.fillRect(rect, QColor("#071b2a"));
    painter.setPen(QPen(QColor("#007a63"), 1));
    painter.drawRect(rect);

    painter.setPen(QPen(QColor(0, 200, 255, 30), 1));
    for (int y = static_cast<int>(rect.top()) + 6; y < rect.bottom(); y += 5) {
        painter.drawLine(QPointF(rect.left(), y), QPointF(rect.right(), y));
    }

    painter.setPen(QColor("#66899b"));
    painter.setFont(QFont("Consolas", 7, QFont::Bold));
    painter.drawText(rect.adjusted(7, 5, -7, -5), Qt::AlignTop | Qt::AlignLeft, title_);

    painter.setPen(QColor("#00ff91"));
    painter.setFont(QFont("Consolas", value_.contains('\n') ? 10 : 16, QFont::Bold));
    painter.drawText(rect, Qt::AlignCenter, value_);

    painter.setPen(QColor("#00c8ff"));
    painter.setFont(QFont("Consolas", 7));
    painter.drawText(rect.adjusted(7, 0, -7, -5), Qt::AlignBottom | Qt::AlignRight, subtitle_);
}
