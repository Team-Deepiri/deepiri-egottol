#pragma once

#include <QString>

class QPainter;

/**
 * symbol_renderer.h — Draw schematic symbols (Python: SYMBOLS dict in main.py).
 *
 * STAGE 1: drawPlaceholder() draws a labeled box so placement/wiring can be tested.
 * STAGE 2: implement draw() with command list port from Python:
 *   ("line", x1,y1,x2,y2), ("rect",...), ("circle",...), ("arc",...),
 *   ("path", points, close), ("text", x,y, str), ("bubble", cx,cy,r)
 *
 * Called from ComponentItem::paint() — keep all symbol geometry here, not in component_item.
 */
namespace deepiri {

class SymbolRenderer {
public:
    /**
     * Full symbol draw. Returns true if symbolKey is implemented.
     * TODO Stage 2: return true for R, C, L, VSRC, gates, etc.
     */
    static bool draw(QPainter* painter, const QString& symbolKey, bool selected);

    /** Stage 1 fallback — rectangle + symbol key text until Stage 2 symbols land. */
    static void drawPlaceholder(QPainter* painter, const QString& symbolKey,
                                const QString& instanceId, bool selected);
};

} // namespace deepiri
