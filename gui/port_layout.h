#pragma once

#include <QString>
#include <QPointF>
#include <vector>

/**
 * port_layout.h — Pin positions per symbol key (Python: PORT_OFFSETS in main.py).
 *
 * STAGE 1: Stub returns empty or a default two-pin vertical layout.
 * STAGE 2: Port the full PORT_OFFSETS table from Python — one entry per symbol key.
 *
 * Coordinates are in ComponentItem local space (origin at component center).
 * Each port: (x, y, name).
 */
namespace deepiri {

struct PortDef {
    qreal x = 0;
    qreal y = 0;
    QString name;
};

class PortLayout {
public:
    /**
     * Return port list for symbolKey (e.g. "R", "GATE_AND", "VSRC").
     * TODO Stage 2: implement full table; until then basic fallbacks for toolbar parts.
     */
    static std::vector<PortDef> portsForSymbol(const QString& symbolKey);

    /** Map registry key → symbol key (Python SYMBOL_KEY dict). */
    static QString symbolKeyForRegistry(const QString& registryKey);
};

} // namespace deepiri
