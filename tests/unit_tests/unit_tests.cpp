#include "../../src/nmea/nmea_parser.h"
#include "../../src/serial/command_encoder.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QList>
#include <QString>
#include <QTextStream>
#include <QtGlobal>
#include <algorithm>

namespace {
QString with_checksum(const QString &payload)
{
    int checksum = 0;
    for (const QChar ch : payload) {
        checksum ^= ch.toLatin1();
    }
    return QString("$%1*%2").arg(payload).arg(checksum, 2, 16, QLatin1Char('0')).toUpper();
}

void expect(bool condition, const QString &message, QList<QString> *failures)
{
    if (!condition) {
        failures->append(message);
    }
}

void test_encode_text_line_endings(QList<QString> *failures)
{
    expect(CommandEncoder::encode_text("PMTK", LineEnding::Crlf) == QByteArray("PMTK\r\n"), "CRLF text encoding failed", failures);
    expect(CommandEncoder::encode_text("PMTK", LineEnding::Cr) == QByteArray("PMTK\r"), "CR text encoding failed", failures);
    expect(CommandEncoder::encode_text("PMTK", LineEnding::Lf) == QByteArray("PMTK\n"), "LF text encoding failed", failures);
    expect(CommandEncoder::encode_text("PMTK", LineEnding::None) == QByteArray("PMTK"), "No-ending text encoding failed", failures);
}

void test_encode_hex_accepts_spaced_and_compact_input(QList<QString> *failures)
{
    QByteArray bytes;
    QString error;

    expect(CommandEncoder::encode_hex("AA 55 01 0D 0A", &bytes, &error), "Spaced HEX input was rejected", failures);
    expect(bytes == QByteArray::fromHex("AA55010D0A"), "Spaced HEX bytes mismatch", failures);

    expect(CommandEncoder::encode_hex("aa55010d0a", &bytes, &error), "Compact lowercase HEX input was rejected", failures);
    expect(bytes == QByteArray::fromHex("AA55010D0A"), "Compact HEX bytes mismatch", failures);
}

void test_encode_hex_rejects_invalid_input(QList<QString> *failures)
{
    QByteArray bytes;
    QString error;

    expect(!CommandEncoder::encode_hex("AA 5Z", &bytes, &error), "Invalid HEX character was accepted", failures);
    expect(!error.isEmpty(), "Invalid HEX character did not report an error", failures);

    error.clear();
    expect(!CommandEncoder::encode_hex("AA5", &bytes, &error), "Odd HEX length was accepted", failures);
    expect(!error.isEmpty(), "Odd HEX length did not report an error", failures);
}

void test_parse_valid_position_and_motion_sentences(QList<QString> *failures)
{
    NmeaParser parser;
    GnssEpoch epoch;

    expect(parser.parse_line(with_checksum("GNRMC,123519,A,2232.5458,N,11403.4719,E,10.0,84.4,050626,,,A"), &epoch),
           "Valid RMC sentence was rejected",
           failures);
    expect(epoch.timestamp.isValid(), "RMC timestamp was not parsed", failures);
    expect(epoch.has_fix, "RMC fix status was not parsed", failures);
    expect(qAbs(epoch.latitude - 22.542430) < 0.000001, "RMC latitude mismatch", failures);
    expect(qAbs(epoch.longitude - 114.057865) < 0.000001, "RMC longitude mismatch", failures);
    expect(qAbs(epoch.speed_kmh - 18.52) < 0.001, "RMC speed mismatch", failures);
    expect(qAbs(epoch.course_deg - 84.4) < 0.001, "RMC course mismatch", failures);

    expect(parser.parse_line(with_checksum("GNGGA,123520,2232.5458,N,11403.4719,E,1,12,0.8,31.5,M,0.0,M,,"), &epoch),
           "Valid GGA sentence was rejected",
           failures);
    expect(epoch.has_fix, "GGA fix status was not parsed", failures);
    expect(epoch.fix_quality == 1, "GGA fix quality mismatch", failures);
    expect(epoch.satellites_used == 12, "GGA satellite count mismatch", failures);
    expect(qAbs(epoch.hdop - 0.8) < 0.001, "GGA HDOP mismatch", failures);
    expect(qAbs(epoch.altitude - 31.5) < 0.001, "GGA altitude mismatch", failures);

    expect(parser.parse_line(with_checksum("GPGSA,A,3,12,05,24,03,29,31,17,19,22,,,,1.3,0.8,1.1"), &epoch),
           "Valid GSA sentence was rejected",
           failures);
    expect(epoch.fix_type == 3, "GSA fix type mismatch", failures);
    expect(qAbs(epoch.pdop - 1.3) < 0.001, "GSA PDOP mismatch", failures);
    expect(qAbs(epoch.hdop - 0.8) < 0.001, "GSA HDOP mismatch", failures);
    expect(qAbs(epoch.vdop - 1.1) < 0.001, "GSA VDOP mismatch", failures);

    expect(parser.parse_line(with_checksum("GNVTG,85.5,T,,M,10.0,N,18.5,K,A"), &epoch),
           "Valid VTG sentence was rejected",
           failures);
    expect(qAbs(epoch.course_deg - 85.5) < 0.001, "VTG course mismatch", failures);
    expect(qAbs(epoch.speed_kmh - 18.5) < 0.001, "VTG speed mismatch", failures);
}

void test_parse_multi_constellation_gsv(QList<QString> *failures)
{
    NmeaParser parser;
    GnssEpoch epoch;

    expect(parser.parse_line(with_checksum("GPGSV,1,1,04,12,72,148,52,05,48,250,44,24,29,076,31,03,18,310,37"), &epoch),
           "GPS GSV sentence was rejected",
           failures);
    expect(parser.parse_line(with_checksum("BDGSV,1,1,04,07,66,038,47,21,22,186,34,16,41,120,30,30,12,260,28"), &epoch),
           "BDS GSV sentence was rejected",
           failures);
    expect(parser.parse_line(with_checksum("GLGSV,1,1,01,03,70,041,45"), &epoch),
           "GLO GSV sentence was rejected",
           failures);

    expect(epoch.satellites.size() == 9, "GSV satellite count mismatch", failures);
    expect(std::any_of(epoch.satellites.cbegin(), epoch.satellites.cend(), [](const SatelliteInfo &satellite) {
               return satellite.constellation == "GPS" && satellite.prn == 12 && satellite.cn0 == 52;
           }),
           "GPS satellite fields mismatch",
           failures);
    expect(std::any_of(epoch.satellites.cbegin(), epoch.satellites.cend(), [](const SatelliteInfo &satellite) {
               return satellite.constellation == "BDS" && satellite.prn == 7 && satellite.azimuth == 38;
           }),
           "BDS satellite fields mismatch",
           failures);
    expect(std::any_of(epoch.satellites.cbegin(), epoch.satellites.cend(), [](const SatelliteInfo &satellite) {
               return satellite.constellation == "GLO" && satellite.prn == 3 && satellite.elevation == 70;
           }),
           "GLO satellite fields mismatch",
           failures);
}

void test_parse_fix_category(QList<QString> *failures)
{
    NmeaParser parser;
    GnssEpoch epoch;

    expect(parser.parse_line(with_checksum("GNGGA,123520,2232.5458,N,11403.4719,E,4,12,0.8,31.5,M,0.0,M,,"), &epoch),
           "RTK GGA sentence was rejected",
           failures);
    expect(epoch.fix_category == "RTK FIX", "RTK fix category mismatch", failures);

    expect(parser.parse_line(with_checksum("GNRMC,123521,A,2232.5458,N,11403.4719,E,0.0,0.0,050626,,,E"), &epoch),
           "VDR RMC sentence was rejected",
           failures);
    expect(epoch.fix_category == "VDR", "VDR fix category mismatch", failures);

    expect(parser.parse_line(with_checksum("GPGSA,A,3,12,05,24,03,29,31,17,19,22,,,,1.3,0.8,1.1"), &epoch),
           "3D GSA sentence was rejected",
           failures);
    expect(epoch.fix_type == 3, "3D GSA fix type mismatch", failures);
}

void test_reject_checksum_failure(QList<QString> *failures)
{
    NmeaParser parser;
    GnssEpoch epoch;
    expect(!parser.parse_line("$GNGGA,123520,2232.5458,N,11403.4719,E,1,12,0.8,31.5,M,0.0,M,,*00", &epoch),
           "Checksum failure was accepted",
           failures);
}
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    Q_UNUSED(app);

    QList<QString> failures;
    QTextStream out(stdout);
    QTextStream err(stderr);

    test_encode_text_line_endings(&failures);
    test_encode_hex_accepts_spaced_and_compact_input(&failures);
    test_encode_hex_rejects_invalid_input(&failures);
    test_parse_valid_position_and_motion_sentences(&failures);
    test_parse_multi_constellation_gsv(&failures);
    test_parse_fix_category(&failures);
    test_reject_checksum_failure(&failures);

    if (failures.isEmpty()) {
        out << "unit_tests: all tests passed\n";
        return 0;
    }

    err << "unit_tests: " << failures.size() << " failure(s)\n";
    for (const QString &failure : failures) {
        err << "  - " << failure << "\n";
    }
    return 1;
}
