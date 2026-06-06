QT += core gui widgets serialport

greaterThan(QT_MAJOR_VERSION, 5): QT += widgets

CONFIG += c++17 warn_on
CONFIG -= app_bundle

TARGET = GnssCyberpunkHost
TEMPLATE = app

QMAKE_CXXFLAGS += -Wall -Wextra
QMAKE_CXXFLAGS += -Wno-sfinae-incomplete

SOURCES += \
    src/app/main.cpp \
    src/app/main_window.cpp \
    src/convert/kml_nmea_converter.cpp \
    src/serial/command_encoder.cpp \
    src/serial/command_test_runner.cpp \
    src/serial/serial_manager.cpp \
    src/nmea/nmea_parser.cpp \
    src/replay/replay_controller.cpp \
    src/sim/simulation_controller.cpp \
    src/analysis/analysis_engine.cpp \
    src/map/offline_map_widget.cpp \
    src/report/report_writer.cpp \
    src/widgets/cn0_bar_widget.cpp \
    src/widgets/dashboard_widget.cpp \
    src/widgets/sky_plot_widget.cpp

HEADERS += \
    src/app/main_window.h \
    src/convert/kml_nmea_converter.h \
    src/serial/command_encoder.h \
    src/serial/command_test_runner.h \
    src/serial/serial_manager.h \
    src/nmea/gnss_types.h \
    src/nmea/nmea_parser.h \
    src/replay/replay_controller.h \
    src/sim/simulation_controller.h \
    src/analysis/analysis_engine.h \
    src/map/offline_map_widget.h \
    src/report/report_writer.h \
    src/widgets/cn0_bar_widget.h \
    src/widgets/dashboard_widget.h \
    src/widgets/sky_plot_widget.h

RESOURCES += resources/resources.qrc
