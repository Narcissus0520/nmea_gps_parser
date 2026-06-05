#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include "../analysis/analysis_engine.h"
#include "../map/offline_map_widget.h"
#include "../nmea/nmea_parser.h"
#include "../replay/replay_controller.h"
#include "../serial/serial_manager.h"
#include "../sim/simulation_controller.h"
#include "../widgets/cn0_bar_widget.h"
#include "../widgets/dashboard_widget.h"
#include "../widgets/sky_plot_widget.h"

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QList>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QSet>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void refresh_ports(void);
    void toggle_serial(void);
    void send_command(void);
    void load_replay_file(void);
    void export_nmea(void);
    void load_truth_csv(void);
    void export_report(void);
    void toggle_simulation(void);
    void choose_map_tile_root(void);
    void show_map_tile_help(void);
    void zoom_map_in(void);
    void zoom_map_out(void);
    void process_nmea_line(const QString &line);
    void update_epoch(const GnssEpoch &epoch);
    void update_satellite_filter(void);

private:
    QWidget *create_header_panel(void);
    QWidget *create_serial_panel(void);
    QWidget *create_command_panel(void);
    QWidget *create_replay_panel(void);
    QWidget *create_dashboard_panel(void);
    QWidget *create_analysis_panel(void);
    QWidget *create_bottom_panel(void);
    LineEnding selected_line_ending(void) const;
    CommandMode selected_command_mode(void) const;
    void append_log(const QString &text);
    void append_raw_nmea(const QByteArray &data);
    void update_raw_stats(void);
    void update_map_status(void);
    QList<SatelliteInfo> filtered_satellites(const QList<SatelliteInfo> &satellites) const;
    GnssEpoch epoch_with_filtered_satellites(const GnssEpoch &epoch) const;
    void refresh_satellite_views(const GnssEpoch &epoch);
    void rebuild_analysis(void);

    SerialManager serial_;
    ReplayController replay_;
    SimulationController simulation_;
    NmeaParser parser_;
    AnalysisEngine analysis_;
    QByteArray raw_nmea_;
    QList<GnssEpoch> stored_epochs_;
    GnssEpoch latest_epoch_;
    bool has_latest_epoch_ = false;
    bool raw_buffer_truncated_ = false;
    int raw_total_count_ = 0;
    int raw_valid_count_ = 0;
    int raw_error_count_ = 0;

    QComboBox *port_combo_ = nullptr;
    QComboBox *baud_combo_ = nullptr;
    QPushButton *connect_button_ = nullptr;
    QPushButton *simulation_button_ = nullptr;
    QLabel *status_label_ = nullptr;
    QPlainTextEdit *raw_log_ = nullptr;
    QLabel *raw_stats_label_ = nullptr;
    QList<QCheckBox *> constellation_filter_checks_;
    QPlainTextEdit *command_edit_ = nullptr;
    QComboBox *command_mode_combo_ = nullptr;
    QComboBox *line_ending_combo_ = nullptr;
    QComboBox *speed_combo_ = nullptr;
    QProgressBar *replay_progress_ = nullptr;
    QPlainTextEdit *analysis_text_ = nullptr;
    QLabel *map_status_label_ = nullptr;

    DashboardWidget *fix_dashboard_ = nullptr;
    DashboardWidget *sat_dashboard_ = nullptr;
    DashboardWidget *speed_dashboard_ = nullptr;
    DashboardWidget *alt_dashboard_ = nullptr;
    DashboardWidget *position_dashboard_ = nullptr;
    DashboardWidget *dop_dashboard_ = nullptr;
    Cn0BarWidget *cn0_widget_ = nullptr;
    SkyPlotWidget *sky_widget_ = nullptr;
    OfflineMapWidget *map_widget_ = nullptr;
};

#endif
