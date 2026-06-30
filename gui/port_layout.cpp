#include "port_layout.h"

#include <QMap>

namespace deepiri {

QString PortLayout::symbolKeyForRegistry(const QString& registryKey) {
    // TODO Stage 2: port full SYMBOL_KEY map from egottol/ui/main.py
    static const QMap<QString, QString> map = {
        {"RES", "R"}, {"CAP", "C"}, {"IND", "L"},
        {"DIODE", "DIODE"}, {"VSRC", "VSRC"}, {"ISRC", "ISRC"},
        {"GND", "GND"}, {"VCC", "VCC"},
        {"NPN", "Q_NPN"}, {"PNP", "Q_PNP"},
        {"AND", "GATE_AND"}, {"OR", "GATE_OR"}, {"NOT", "GATE_NOT"},
        {"XOR", "GATE_XOR"}, {"DFF", "DFF"}, {"LM741", "OPAMP"},
    };
    if (map.contains(registryKey)) return map.value(registryKey);
    return QStringLiteral("DEFAULT");
}

std::vector<PortDef> PortLayout::portsForSymbol(const QString& symbolKey) {
    // TODO Stage 2: replace with full PORT_OFFSETS from Python
    if (symbolKey == "R" || symbolKey == "C" || symbolKey == "L") {
        return {{0, -25, "1"}, {0, 25, "2"}};
    }
    if (symbolKey == "VSRC" || symbolKey == "ISRC") {
        return {{0, -25, "+"}, {0, 25, QString::fromUtf8("−")}};
    }
    if (symbolKey == "GND") {
        return {{0, -25, "G"}};
    }
    if (symbolKey == "VCC") {
        return {{0, 25, "V"}};
    }
    if (symbolKey == "GATE_AND" || symbolKey == "GATE_OR" ||
        symbolKey == "GATE_XOR" || symbolKey == "GATE_NAND") {
        return {{-32, -8, "A"}, {-32, 8, "B"}, {30, 0, "Q"}};
    }
    if (symbolKey == "GATE_NOT") {
        return {{-22, 0, "A"}, {30, 0, "Q"}};
    }
    if (symbolKey == "DFF") {
        return {{-28, -16, "D"}, {-28, 8, "CLK"}, {28, -16, "Q"}, {28, 8, "QB"}};
    }
    if (symbolKey == "OPAMP") {
        return {{-20, -10, "+"}, {-20, 10, QString::fromUtf8("−")}, {30, 0, "OUT"}};
    }
    // DEFAULT — generic two-pin horizontal (Python PORT_OFFSETS["DEFAULT"])
    return {{-18, 0, "1"}, {18, 0, "2"}};
}

} // namespace deepiri
