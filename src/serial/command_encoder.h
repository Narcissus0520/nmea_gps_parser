#ifndef COMMAND_ENCODER_H
#define COMMAND_ENCODER_H

#include <QByteArray>
#include <QString>

enum class CommandMode {
    Text,
    Hex
};

enum class LineEnding {
    None,
    Cr,
    Lf,
    Crlf
};

class CommandEncoder {
public:
    static QByteArray encode_text(const QString &text, LineEnding ending);
    static bool encode_hex(const QString &text, QByteArray *bytes, QString *error);
};

#endif
