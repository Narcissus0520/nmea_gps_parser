#ifndef DASHBOARD_WIDGET_H
#define DASHBOARD_WIDGET_H

#include <QWidget>

class DashboardWidget : public QWidget {
    Q_OBJECT

public:
    explicit DashboardWidget(const QString &title, QWidget *parent = nullptr);

    void set_value(const QString &value);
    void set_subtitle(const QString &subtitle);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QString title_;
    QString value_ = "--";
    QString subtitle_;
};

#endif
