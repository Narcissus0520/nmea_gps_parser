#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include "../analysis/analysis_engine.h"
#include "../map/offline_map_widget.h"
#include "../nmea/nmea_parser.h"
#include "../replay/replay_controller.h"
#include "../serial/command_test_runner.h"
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
#include <QPushButton>
#include <QSet>
#include <QSerialPort>
#include <QSlider>
#include <QSpinBox>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void refresh_ports(void);
    void toggle_serial(void);
    void send_command(void);
    void load_command_test_file(void);
    void toggle_command_test_run(void);
    void show_quick_start_help(void);
    void load_replay_file(void);
    void export_nmea(void);
    void load_analysis_nmea(void);
    void load_truth_csv(void);
    void export_report(void);
    void convert_nmea_to_kml(void);
    void convert_kml_to_nmea(void);
    void toggle_simulation(void);
    void choose_map_tile_root(void);
    void show_map_tile_help(void);
    void zoom_map_in(void);
    void zoom_map_out(void);
    void process_nmea_line(const QString &line);
    void update_epoch(const GnssEpoch &epoch);
    void update_satellite_filter(void);
    void handle_replay_slider_released(void);

private:
    QWidget *create_header_panel(void);
    QWidget *create_serial_panel(void);
    QWidget *create_command_panel(void);
    QWidget *create_replay_panel(void);
    QWidget *create_dashboard_panel(void);
    QWidget *create_analysis_panel(void);
    QWidget *create_bottom_panel(void);
    QSerialPort::DataBits selected_data_bits(void) const;
    QSerialPort::Parity selected_parity(void) const;
    QSerialPort::StopBits selected_stop_bits(void) const;
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
    void rebuild_replay_state_to(int index);
    void clear_runtime_data(bool clear_raw_log);
    void apply_epoch_to_ui(const GnssEpoch &epoch);
    QString satellite_filter_summary(void) const;

    SerialManager serial_;
    ReplayController replay_;
    CommandTestRunner command_test_runner_;
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
    QComboBox *data_bits_combo_ = nullptr;
    QComboBox *stop_bits_combo_ = nullptr;
    QComboBox *parity_combo_ = nullptr;
    QPushButton *connect_button_ = nullptr;
    QPushButton *simulation_button_ = nullptr;
    QLabel *status_label_ = nullptr;
    QPlainTextEdit *raw_log_ = nullptr;
    QLabel *raw_stats_label_ = nullptr;
    QList<QCheckBox *> constellation_filter_checks_;
    QPlainTextEdit *command_edit_ = nullptr;
    QPlainTextEdit *command_test_results_ = nullptr;
    QComboBox *command_mode_combo_ = nullptr;
    QComboBox *line_ending_combo_ = nullptr;
    QPushButton *command_test_run_button_ = nullptr;
    QComboBox *speed_combo_ = nullptr;
    QLabel *replay_progress_label_ = nullptr;
    QSlider *replay_progress_ = nullptr;
    QSpinBox *cn0_min_spin_ = nullptr;
    QSpinBox *cn0_max_spin_ = nullptr;
    QSpinBox *azimuth_min_spin_ = nullptr;
    QSpinBox *azimuth_max_spin_ = nullptr;
    QSpinBox *elevation_min_spin_ = nullptr;
    QSpinBox *elevation_max_spin_ = nullptr;
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
