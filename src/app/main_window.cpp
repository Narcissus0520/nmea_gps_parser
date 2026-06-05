#include "main_window.h"

#include "../report/report_writer.h"

#include <QFile>
#include <QFileDialog>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QSizePolicy>
#include <QTextStream>
#include <QVBoxLayout>

namespace {
constexpr qsizetype max_raw_nmea_bytes = 64 * 1024 * 1024;
constexpr int max_stored_epochs = 200000;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("GNSS Cyberpunk Host");
    resize(1384, 840);
    setMinimumSize(1180, 740);

    QWidget *central = new QWidget(this);
    QVBoxLayout *root_layout = new QVBoxLayout(central);
    root_layout->setContentsMargins(12, 12, 12, 12);
    root_layout->setSpacing(12);
    root_layout->addWidget(create_header_panel());
    root_layout->addWidget(create_serial_panel());
    root_layout->addWidget(create_dashboard_panel(), 1);
    root_layout->addWidget(create_bottom_panel());
    setCentralWidget(central);

    refresh_ports();

    connect(&serial_, &SerialManager::line_received, this, &MainWindow::process_nmea_line);
    connect(&serial_, &SerialManager::raw_received, this, [this](const QByteArray &data) {
        append_raw_nmea(data);
    });
    connect(&serial_, &SerialManager::status_changed, status_label_, &QLabel::setText);
    connect(&serial_, &SerialManager::error_message, this, [this](const QString &message) {
        append_log("[serial error] " + message);
    });
    connect(&replay_, &ReplayController::line_replayed, this, &MainWindow::process_nmea_line);
    connect(&replay_, &ReplayController::progress_changed, this, [this](int current, int total) {
        replay_progress_->setMaximum(total);
        replay_progress_->setValue(current);
    });
    connect(&replay_, &ReplayController::status_changed, this, &MainWindow::append_log);
    connect(&simulation_, &SimulationController::line_generated, this, [this](const QString &line) {
        append_raw_nmea(line.toUtf8());
        append_raw_nmea("\r\n");
        process_nmea_line(line);
    });
    connect(&simulation_, &SimulationController::status_changed, this, &MainWindow::append_log);
    connect(&parser_, &NmeaParser::epoch_updated, this, &MainWindow::update_epoch);
    connect(&parser_, &NmeaParser::parse_error, this, &MainWindow::append_log);
}

QWidget *MainWindow::create_header_panel(void)
{
    QWidget *panel = new QWidget(this);
    panel->setObjectName("topHeader");
    QHBoxLayout *layout = new QHBoxLayout(panel);
    layout->setContentsMargins(18, 8, 18, 8);
    layout->setSpacing(18);

    QLabel *brand = new QLabel("◇ NMEA GPS PARSER", panel);
    brand->setObjectName("brandLabel");
    QLabel *subtitle = new QLabel("基于 NMEA-0183 协议的 GNSS 报文解析工具", panel);
    subtitle->setObjectName("subtitleLabel");
    QLabel *version = new QLabel("v 1.1.2", panel);
    version->setObjectName("versionLabel");
    QLabel *state = new QLabel("● 未连接", panel);
    state->setObjectName("connectionLabel");
    state->setMinimumWidth(150);
    status_label_ = state;

    layout->addWidget(brand);
    layout->addWidget(subtitle);
    layout->addWidget(version);
    layout->addStretch();
    layout->addWidget(state);
    panel->setFixedHeight(48);
    return panel;
}

void MainWindow::refresh_ports(void)
{
    port_combo_->clear();
    port_combo_->addItems(serial_.available_ports());
}

void MainWindow::toggle_serial(void)
{
    if (serial_.is_open()) {
        serial_.close_port();
        connect_button_->setText("Connect");
        return;
    }

    const QString port = port_combo_->currentText();
    const int baud = baud_combo_->currentText().toInt();
    if (port.isEmpty()) {
        QMessageBox::warning(this, "Serial", "No COM port selected.");
        return;
    }

    if (serial_.open_port(port, baud)) {
        connect_button_->setText("Disconnect");
    }
}

void MainWindow::send_command(void)
{
    QString error;
    if (!serial_.send_command(command_edit_->toPlainText(),
                              selected_command_mode(),
                              selected_line_ending(),
                              &error)) {
        QMessageBox::warning(this, "Send command", error);
        return;
    }
    append_log("[tx] command sent");
}

void MainWindow::load_replay_file(void)
{
    const QString path = QFileDialog::getOpenFileName(this, "Open NMEA", QString(), "NMEA (*.nmea *.txt *.log);;All Files (*)");
    QString error;
    if (path.isEmpty()) {
        return;
    }
    if (!replay_.load_file(path, &error)) {
        QMessageBox::warning(this, "Replay", error);
    }
}

void MainWindow::export_nmea(void)
{
    const QString path = QFileDialog::getSaveFileName(this, "Export NMEA", "gnss_raw.nmea", "NMEA (*.nmea);;Text (*.txt)");
    if (path.isEmpty()) {
        return;
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, "Export NMEA", file.errorString());
        return;
    }
    file.write(raw_nmea_);
}

void MainWindow::load_truth_csv(void)
{
    const QString path = QFileDialog::getOpenFileName(this, "Open truth CSV", QString(), "CSV (*.csv);;All Files (*)");
    QString error;
    if (path.isEmpty()) {
        return;
    }
    if (!analysis_.load_truth_csv(path, &error)) {
        QMessageBox::warning(this, "Truth CSV", error.isEmpty() ? "No valid truth points loaded." : error);
        return;
    }
    analysis_text_->setPlainText(analysis_.summary_text());
}

void MainWindow::export_report(void)
{
    const QString html_path = QFileDialog::getSaveFileName(this, "Export HTML report", "gnss_report.html", "HTML (*.html)");
    if (html_path.isEmpty()) {
        return;
    }

    QString error;
    const AnalysisSummary summary = analysis_.summarize();
    if (!ReportWriter::write_html(html_path, summary, &error)) {
        QMessageBox::warning(this, "Report", error);
        return;
    }

    const QString csv_path = html_path.left(html_path.lastIndexOf('.')) + "_epochs.csv";
    if (!ReportWriter::write_csv(csv_path, analysis_.epochs(), &error)) {
        QMessageBox::warning(this, "Report CSV", error);
        return;
    }
    QMessageBox::information(this, "Report", "HTML report and CSV details exported.");
}

void MainWindow::toggle_simulation(void)
{
    if (simulation_.is_running()) {
        simulation_.stop();
        simulation_button_->setText("Start Sim");
        status_label_->setText("Simulation stopped");
        return;
    }

    if (serial_.is_open()) {
        serial_.close_port();
        connect_button_->setText("Connect");
    }

    simulation_.start();
    simulation_button_->setText("Stop Sim");
    status_label_->setText("Simulation running");
}

void MainWindow::choose_map_tile_root(void)
{
    const QString path = QFileDialog::getExistingDirectory(this,
                                                           "Select offline map tile root",
                                                           map_widget_->tile_root());
    if (path.isEmpty()) {
        return;
    }

    map_widget_->set_tile_root(path);
    update_map_status();
}

void MainWindow::show_map_tile_help(void)
{
    QMessageBox::information(
        this,
        "地图瓦片 / Map Tiles",
        "离线地图使用 PNG XYZ 瓦片目录：\n"
        "{root}/{z}/{x}/{y}.png\n\n"
        "加载新瓦片：\n"
        "1. 准备一个瓦片根目录，例如 D:/maps/shenzhen_tiles。\n"
        "2. 确认目录下存在 16/52364/34012.png 这类 z/x/y.png 文件。\n"
        "3. 点击 Tiles...，选择这个根目录。\n"
        "4. 用 Zoom + / Zoom - 或鼠标滚轮切换缩放级别。\n\n"
        "下载/准备瓦片：\n"
        "当前工具不内置批量抓取公网瓦片功能，避免违反地图服务条款。请使用授权地图源、公司内部地图服务或离线地图工具导出 PNG XYZ 瓦片后再加载。\n\n"
        "如果当前位置对应缩放级别没有瓦片，地图会显示深色网格，但轨迹仍会正常绘制。");
}

void MainWindow::zoom_map_in(void)
{
    map_widget_->zoom_in();
    update_map_status();
}

void MainWindow::zoom_map_out(void)
{
    map_widget_->zoom_out();
    update_map_status();
}

void MainWindow::process_nmea_line(const QString &line)
{
    GnssEpoch epoch;
    append_log(line);
    raw_total_count_++;
    if (parser_.parse_line(line, &epoch)) {
        raw_valid_count_++;
    } else {
        raw_error_count_++;
    }
    update_raw_stats();
}

void MainWindow::update_epoch(const GnssEpoch &epoch)
{
    latest_epoch_ = epoch;
    has_latest_epoch_ = true;
    stored_epochs_.append(epoch);
    while (stored_epochs_.size() > max_stored_epochs) {
        stored_epochs_.removeFirst();
    }

    fix_dashboard_->set_value(epoch.has_fix ? "FIX" : "NO FIX");
    fix_dashboard_->set_subtitle(QString("Q%1 T%2").arg(epoch.fix_quality).arg(epoch.fix_type));
    sat_dashboard_->set_value(QString::number(epoch.satellites_used));
    sat_dashboard_->set_subtitle("satellites used");
    speed_dashboard_->set_value(QString::number(epoch.speed_kmh, 'f', 1));
    speed_dashboard_->set_subtitle("km/h");
    alt_dashboard_->set_value(QString::number(epoch.altitude, 'f', 1));
    alt_dashboard_->set_subtitle("m");
    position_dashboard_->set_value(QString("%1\n%2")
                                       .arg(epoch.latitude, 0, 'f', 6)
                                       .arg(epoch.longitude, 0, 'f', 6));
    position_dashboard_->set_subtitle("lat / lon");
    dop_dashboard_->set_value(QString("H%1 P%2")
                                  .arg(epoch.hdop, 0, 'f', 1)
                                  .arg(epoch.pdop, 0, 'f', 1));
    dop_dashboard_->set_subtitle("DOP");
    refresh_satellite_views(epoch);
    if (epoch.has_fix) {
        map_widget_->add_position(epoch.latitude, epoch.longitude);
        update_map_status();
    }
    analysis_.add_epoch(epoch_with_filtered_satellites(epoch));
    analysis_text_->setPlainText(analysis_.summary_text());
}

QWidget *MainWindow::create_serial_panel(void)
{
    QGroupBox *box = new QGroupBox("串口配置 / SERIAL CONFIG");
    QGridLayout *layout = new QGridLayout(box);
    layout->setContentsMargins(16, 20, 16, 12);
    layout->setHorizontalSpacing(14);
    layout->setVerticalSpacing(6);
    port_combo_ = new QComboBox(box);
    baud_combo_ = new QComboBox(box);
    baud_combo_->addItems({"9600", "38400", "57600", "115200", "230400", "460800", "921600"});
    baud_combo_->setCurrentText("115200");
    QPushButton *refresh_button = new QPushButton("Refresh", box);
    connect_button_ = new QPushButton("Connect", box);
    QComboBox *data_bits_combo = new QComboBox(box);
    QComboBox *stop_bits_combo = new QComboBox(box);
    QComboBox *parity_combo = new QComboBox(box);
    QLabel *inline_status = new QLabel("● 未连接", box);
    data_bits_combo->addItems({"8", "7"});
    stop_bits_combo->addItems({"1", "2"});
    parity_combo->addItems({"None", "Even", "Odd"});

    layout->addWidget(new QLabel("端口", box), 0, 0);
    layout->addWidget(new QLabel("波特率", box), 0, 1);
    layout->addWidget(new QLabel("数据位", box), 0, 2);
    layout->addWidget(new QLabel("停止位", box), 0, 3);
    layout->addWidget(new QLabel("校验", box), 0, 4);
    layout->addWidget(port_combo_, 1, 0);
    layout->addWidget(baud_combo_, 1, 1);
    layout->addWidget(data_bits_combo, 1, 2);
    layout->addWidget(stop_bits_combo, 1, 3);
    layout->addWidget(parity_combo, 1, 4);
    layout->addWidget(inline_status, 1, 5);
    layout->addWidget(connect_button_, 1, 6);
    layout->addWidget(refresh_button, 1, 7);
    layout->setColumnStretch(5, 1);
    box->setFixedHeight(116);

    connect(refresh_button, &QPushButton::clicked, this, &MainWindow::refresh_ports);
    connect(connect_button_, &QPushButton::clicked, this, &MainWindow::toggle_serial);
    connect(&serial_, &SerialManager::status_changed, inline_status, &QLabel::setText);
    return box;
}

QWidget *MainWindow::create_command_panel(void)
{
    QGroupBox *box = new QGroupBox("COM 命令 / COMMAND");
    QVBoxLayout *layout = new QVBoxLayout(box);
    layout->setContentsMargins(10, 18, 10, 10);
    QHBoxLayout *options = new QHBoxLayout();

    command_mode_combo_ = new QComboBox(box);
    command_mode_combo_->addItems({"Text", "HEX"});
    line_ending_combo_ = new QComboBox(box);
    line_ending_combo_->addItems({"CRLF", "CR", "LF", "None"});
    command_edit_ = new QPlainTextEdit(box);
    command_edit_->setPlaceholderText("Text: PMTK command / HEX: AA 55 01 0D 0A or AA55010D0A");
    command_edit_->setMaximumHeight(82);
    QPushButton *send_button = new QPushButton("发送指令", box);

    options->addWidget(command_mode_combo_);
    options->addWidget(line_ending_combo_);
    layout->addLayout(options);
    layout->addWidget(command_edit_);
    layout->addWidget(send_button);
    connect(send_button, &QPushButton::clicked, this, &MainWindow::send_command);
    return box;
}

QWidget *MainWindow::create_replay_panel(void)
{
    QGroupBox *box = new QGroupBox("数据回放 / REPLAY");
    QVBoxLayout *layout = new QVBoxLayout(box);
    layout->setContentsMargins(10, 18, 10, 10);
    QHBoxLayout *buttons = new QHBoxLayout();

    QPushButton *open_button = new QPushButton("载入文件回放", box);
    QPushButton *start_button = new QPushButton("开始", box);
    QPushButton *pause_button = new QPushButton("暂停", box);
    QPushButton *stop_button = new QPushButton("停止", box);
    QPushButton *export_button = new QPushButton("导出 NMEA", box);
    speed_combo_ = new QComboBox(box);
    speed_combo_->addItems({"1x", "2x", "5x", "10x"});
    replay_progress_ = new QProgressBar(box);

    buttons->addWidget(open_button);
    buttons->addWidget(start_button);
    buttons->addWidget(pause_button);
    buttons->addWidget(stop_button);
    layout->addLayout(buttons);
    layout->addWidget(speed_combo_);
    layout->addWidget(replay_progress_);
    layout->addWidget(export_button);

    connect(open_button, &QPushButton::clicked, this, &MainWindow::load_replay_file);
    connect(start_button, &QPushButton::clicked, &replay_, &ReplayController::start);
    connect(pause_button, &QPushButton::clicked, &replay_, &ReplayController::pause);
    connect(stop_button, &QPushButton::clicked, &replay_, &ReplayController::stop);
    connect(export_button, &QPushButton::clicked, this, &MainWindow::export_nmea);
    connect(speed_combo_, &QComboBox::currentTextChanged, this, [this](const QString &text) {
        replay_.set_speed(text.left(text.size() - 1).toDouble());
    });
    return box;
}

QWidget *MainWindow::create_dashboard_panel(void)
{
    QWidget *panel = new QWidget();
    QGridLayout *layout = new QGridLayout(panel);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    QGroupBox *dashboard_box = new QGroupBox("仪表盘 / DASHBOARD", panel);
    QGridLayout *dashboard_layout = new QGridLayout(dashboard_box);
    dashboard_layout->setContentsMargins(10, 18, 10, 10);
    dashboard_layout->setHorizontalSpacing(8);
    dashboard_layout->setVerticalSpacing(6);
    fix_dashboard_ = new DashboardWidget("定位质量", dashboard_box);
    sat_dashboard_ = new DashboardWidget("搜星", dashboard_box);
    speed_dashboard_ = new DashboardWidget("速度", dashboard_box);
    alt_dashboard_ = new DashboardWidget("海拔", dashboard_box);
    dop_dashboard_ = new DashboardWidget("DOP", panel);
    dashboard_layout->addWidget(speed_dashboard_, 0, 0);
    dashboard_layout->addWidget(dop_dashboard_, 0, 1);
    dashboard_layout->addWidget(alt_dashboard_, 0, 2);
    dashboard_layout->addWidget(fix_dashboard_, 1, 0);
    dashboard_layout->addWidget(sat_dashboard_, 1, 1);
    dashboard_box->setMaximumHeight(170);

    QGroupBox *position_box = new QGroupBox("位置信息 / POSITION", panel);
    QVBoxLayout *position_layout = new QVBoxLayout(position_box);
    position_layout->setContentsMargins(10, 18, 10, 10);
    position_layout->setSpacing(8);
    position_dashboard_ = new DashboardWidget("LAT / LON", position_box);
    QHBoxLayout *map_tools = new QHBoxLayout();
    map_tools->setSpacing(8);
    QPushButton *tiles_button = new QPushButton("Tiles...", position_box);
    QPushButton *map_help_button = new QPushButton("Help", position_box);
    QPushButton *zoom_out_button = new QPushButton("Zoom -", position_box);
    QPushButton *zoom_in_button = new QPushButton("Zoom +", position_box);
    map_status_label_ = new QLabel("Map not initialized", position_box);
    map_status_label_->setMinimumWidth(170);
    map_status_label_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    map_tools->addWidget(tiles_button);
    map_tools->addWidget(map_help_button);
    map_tools->addWidget(zoom_out_button);
    map_tools->addWidget(zoom_in_button);
    map_tools->addWidget(map_status_label_, 1);
    map_widget_ = new OfflineMapWidget(position_box);
    map_widget_->setMinimumHeight(240);
    position_layout->addWidget(position_dashboard_);
    position_layout->addLayout(map_tools);
    position_layout->addWidget(map_widget_, 1);
    connect(tiles_button, &QPushButton::clicked, this, &MainWindow::choose_map_tile_root);
    connect(map_help_button, &QPushButton::clicked, this, &MainWindow::show_map_tile_help);
    connect(zoom_out_button, &QPushButton::clicked, this, &MainWindow::zoom_map_out);
    connect(zoom_in_button, &QPushButton::clicked, this, &MainWindow::zoom_map_in);
    update_map_status();

    QGroupBox *sky_box = new QGroupBox("卫星天空图 / SKY PLOT", panel);
    QVBoxLayout *sky_layout = new QVBoxLayout(sky_box);
    sky_layout->setContentsMargins(14, 20, 14, 14);
    sky_layout->setSpacing(8);
    QHBoxLayout *sat_filter_layout = new QHBoxLayout();
    sat_filter_layout->setSpacing(6);
    sat_filter_layout->addWidget(new QLabel("Filter", sky_box));
    const QStringList constellations = {"GPS", "BDS", "GLO", "GAL", "QZSS", "GNSS"};
    for (const QString &constellation : constellations) {
        QCheckBox *check_box = new QCheckBox(constellation, sky_box);
        check_box->setChecked(true);
        constellation_filter_checks_.append(check_box);
        sat_filter_layout->addWidget(check_box);
        connect(check_box, &QCheckBox::toggled, this, &MainWindow::update_satellite_filter);
    }
    sat_filter_layout->addStretch();
    sky_widget_ = new SkyPlotWidget(sky_box);
    cn0_widget_ = new Cn0BarWidget(sky_box);
    sky_layout->addLayout(sat_filter_layout);
    sky_layout->addWidget(sky_widget_, 4);
    sky_layout->addWidget(cn0_widget_, 1);

    QGroupBox *raw_box = new QGroupBox("原始报文 / RAW NMEA", panel);
    QVBoxLayout *raw_layout = new QVBoxLayout(raw_box);
    raw_layout->setContentsMargins(12, 22, 12, 12);
    QHBoxLayout *raw_tools = new QHBoxLayout();
    raw_stats_label_ = new QLabel("Total: 0  Valid: 0  Errors: 0", raw_box);
    raw_tools->addWidget(raw_stats_label_);
    raw_tools->addStretch();
    QComboBox *filter_combo = new QComboBox(raw_box);
    filter_combo->addItems({"ALL", "GGA", "RMC", "GSA", "GSV", "VTG"});
    QPushButton *clear_raw_button = new QPushButton("清空", raw_box);
    raw_tools->addWidget(filter_combo);
    raw_tools->addWidget(clear_raw_button);
    raw_log_ = new QPlainTextEdit(raw_box);
    raw_log_->setReadOnly(true);
    raw_log_->setMaximumBlockCount(1200);
    raw_layout->addLayout(raw_tools);
    raw_layout->addWidget(raw_log_, 1);
    connect(clear_raw_button, &QPushButton::clicked, raw_log_, &QPlainTextEdit::clear);

    layout->addWidget(dashboard_box, 0, 0);
    layout->addWidget(position_box, 1, 0);
    layout->addWidget(sky_box, 0, 1, 2, 1);
    layout->addWidget(raw_box, 0, 2, 2, 1);
    layout->setColumnStretch(0, 34);
    layout->setColumnStretch(1, 30);
    layout->setColumnStretch(2, 36);
    layout->setRowStretch(0, 26);
    layout->setRowStretch(1, 74);
    return panel;
}

QWidget *MainWindow::create_analysis_panel(void)
{
    QGroupBox *box = new QGroupBox("分析报告 / ANALYSIS");
    QVBoxLayout *layout = new QVBoxLayout(box);
    layout->setContentsMargins(10, 18, 10, 10);
    QPushButton *truth_button = new QPushButton("载入标准轨迹", box);
    QPushButton *report_button = new QPushButton("导出报告", box);
    QPushButton *clear_button = new QPushButton("清空数据", box);
    simulation_button_ = new QPushButton("Start Sim", box);
    analysis_text_ = new QPlainTextEdit(box);
    analysis_text_->setReadOnly(true);
    analysis_text_->setMaximumHeight(130);

    layout->addWidget(simulation_button_);
    layout->addWidget(truth_button);
    layout->addWidget(report_button);
    layout->addWidget(clear_button);
    layout->addWidget(analysis_text_);

    connect(simulation_button_, &QPushButton::clicked, this, &MainWindow::toggle_simulation);
    connect(truth_button, &QPushButton::clicked, this, &MainWindow::load_truth_csv);
    connect(report_button, &QPushButton::clicked, this, &MainWindow::export_report);
    connect(clear_button, &QPushButton::clicked, this, [this]() {
        simulation_.stop();
        if (simulation_button_ != nullptr) {
            simulation_button_->setText("Start Sim");
        }
        parser_.reset();
        analysis_.clear_epochs();
        stored_epochs_.clear();
        has_latest_epoch_ = false;
        map_widget_->clear_track();
        update_map_status();
        raw_nmea_.clear();
        raw_buffer_truncated_ = false;
        raw_total_count_ = 0;
        raw_valid_count_ = 0;
        raw_error_count_ = 0;
        update_raw_stats();
        raw_log_->clear();
        analysis_text_->clear();
    });
    return box;
}

QWidget *MainWindow::create_bottom_panel(void)
{
    QWidget *panel = new QWidget(this);
    panel->setObjectName("bottomTools");
    QHBoxLayout *layout = new QHBoxLayout(panel);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    QWidget *command = create_command_panel();
    QWidget *replay = create_replay_panel();
    QWidget *analysis = create_analysis_panel();
    command->setMinimumWidth(330);
    replay->setMinimumWidth(430);
    analysis->setMinimumWidth(320);
    layout->addWidget(command, 3);
    layout->addWidget(replay, 4);
    layout->addWidget(analysis, 3);
    panel->setFixedHeight(174);
    return panel;
}

LineEnding MainWindow::selected_line_ending(void) const
{
    const QString text = line_ending_combo_->currentText();
    if (text == "CR") {
        return LineEnding::Cr;
    }
    if (text == "LF") {
        return LineEnding::Lf;
    }
    if (text == "None") {
        return LineEnding::None;
    }
    return LineEnding::Crlf;
}

CommandMode MainWindow::selected_command_mode(void) const
{
    return command_mode_combo_->currentText() == "HEX" ? CommandMode::Hex : CommandMode::Text;
}

void MainWindow::append_log(const QString &text)
{
    if (raw_log_ != nullptr) {
        raw_log_->appendPlainText(text);
    }
}

void MainWindow::append_raw_nmea(const QByteArray &data)
{
    if (data.isEmpty()) {
        return;
    }

    raw_nmea_.append(data);
    if (raw_nmea_.size() <= max_raw_nmea_bytes) {
        return;
    }

    raw_nmea_.remove(0, raw_nmea_.size() - max_raw_nmea_bytes);
    if (!raw_buffer_truncated_) {
        raw_buffer_truncated_ = true;
        append_log("[security] Raw NMEA buffer capped at 64 MiB; older bytes were discarded.");
    }
}

void MainWindow::update_raw_stats(void)
{
    if (raw_stats_label_ == nullptr) {
        return;
    }

    raw_stats_label_->setText(QString("Total: %1  Valid: %2  Errors: %3")
                                  .arg(raw_total_count_)
                                  .arg(raw_valid_count_)
                                  .arg(raw_error_count_));
}

void MainWindow::update_map_status(void)
{
    if (map_status_label_ == nullptr || map_widget_ == nullptr) {
        return;
    }

    const QString tile_name = QDir(map_widget_->tile_root()).dirName();
    map_status_label_->setText(QString("z%1 | pts:%2 | tiles:%3")
                                   .arg(map_widget_->zoom())
                                   .arg(map_widget_->track_count())
                                   .arg(tile_name.isEmpty() ? "default" : tile_name));
}

void MainWindow::update_satellite_filter(void)
{
    if (has_latest_epoch_) {
        refresh_satellite_views(latest_epoch_);
    }
    rebuild_analysis();
}

QList<SatelliteInfo> MainWindow::filtered_satellites(const QList<SatelliteInfo> &satellites) const
{
    QSet<QString> enabled_constellations;
    for (const QCheckBox *check_box : constellation_filter_checks_) {
        if (check_box != nullptr && check_box->isChecked()) {
            enabled_constellations.insert(check_box->text());
        }
    }

    if (enabled_constellations.isEmpty()) {
        return {};
    }

    QList<SatelliteInfo> filtered;
    for (const SatelliteInfo &satellite : satellites) {
        const QString constellation = satellite.constellation.isEmpty() ? "GNSS" : satellite.constellation;
        if (enabled_constellations.contains(constellation)) {
            filtered.append(satellite);
        }
    }
    return filtered;
}

GnssEpoch MainWindow::epoch_with_filtered_satellites(const GnssEpoch &epoch) const
{
    GnssEpoch filtered_epoch = epoch;
    filtered_epoch.satellites = filtered_satellites(epoch.satellites);
    filtered_epoch.satellites_used = filtered_epoch.satellites.size();
    return filtered_epoch;
}

void MainWindow::refresh_satellite_views(const GnssEpoch &epoch)
{
    const QList<SatelliteInfo> satellites = filtered_satellites(epoch.satellites);
    cn0_widget_->set_satellites(satellites);
    sky_widget_->set_satellites(satellites);
}

void MainWindow::rebuild_analysis(void)
{
    analysis_.clear_epochs();
    for (const GnssEpoch &epoch : stored_epochs_) {
        analysis_.add_epoch(epoch_with_filtered_satellites(epoch));
    }

    if (analysis_text_ != nullptr) {
        analysis_text_->setPlainText(analysis_.summary_text());
    }
}
