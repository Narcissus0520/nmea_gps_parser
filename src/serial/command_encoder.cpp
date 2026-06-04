#include "command_encoder.h"

QByteArray CommandEncoder::encode_text(const QString &text, LineEnding ending)
{
    QByteArray bytes = text.toUtf8();

    switch (ending) {
    case LineEnding::Cr:
        bytes.append('\r');
        break;
    case LineEnding::Lf:
        bytes.append('\n');
        break;
    case LineEnding::Crlf:
        bytes.append("\r\n");
        break;
    case LineEnding::None:
        break;
    }

    return bytes;
}

bool CommandEncoder::encode_hex(const QString &text, QByteArray *bytes, QString *error)
{
    QString compact;

    if (bytes == nullptr) {
        return false;
    }

    bytes->clear();
    for (const QChar ch : text) {
        if (ch.isSpace()) {
            continue;
        }
        const ushort code = ch.toLower().unicode();
        if (!ch.isDigit() && (code < 'a' || code > 'f')) {
            if (error != nullptr) {
                *error = "HEX input contains an invalid character.";
            }
            return false;
        }
        compact.append(ch);
    }

    if (compact.isEmpty()) {
        if (error != nullptr) {
            *error = "HEX input is empty.";
        }
        return false;
    }

    if (compact.size() % 2 != 0) {
        if (error != nullptr) {
            *error = "HEX input must contain an even number of digits.";
        }
        return false;
    }

    for (int i = 0; i < compact.size(); i += 2) {
        bool ok = false;
        const char value = static_cast<char>(compact.mid(i, 2).toInt(&ok, 16));
        if (!ok) {
            if (error != nullptr) {
                *error = "HEX byte conversion failed.";
            }
            return false;
        }
        bytes->append(value);
    }

    return true;
}
