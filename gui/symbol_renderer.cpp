#include "symbol_renderer.h"
#include "egottol_theme.h"

#include <QFont>
#include <QPainter>
#include <QPainterPath>

namespace deepiri {

bool SymbolRenderer::draw(QPainter *painter, const QString &symbolKey,
                          bool selected) {
  if (!painter)
    return false;

  const QColor stroke = selected ? ui::colorSelected() : ui::colorComponent();
  painter->setPen(QPen(stroke, 1.8, Qt::SolidLine, Qt::RoundCap,
                       Qt::RoundJoin));
  painter->setBrush(Qt::NoBrush);

  if (symbolKey == "R") {
    painter->drawLine(0, -25, 0, -18);
    QPainterPath path(QPointF(0, -18));
    path.lineTo(-8, -13);
    path.lineTo(8, -7);
    path.lineTo(-8, -1);
    path.lineTo(8, 5);
    path.lineTo(-8, 11);
    path.lineTo(0, 18);
    painter->drawPath(path);
    painter->drawLine(0, 18, 0, 25);
    return true;
  }
  if (symbolKey == "C") {
    painter->drawLine(0, -25, 0, -5);
    painter->drawLine(-14, -5, 14, -5);
    painter->drawLine(-14, 5, 14, 5);
    painter->drawLine(0, 5, 0, 25);
    return true;
  }
  if (symbolKey == "L") {
    painter->drawLine(0, -25, 0, -18);
    for (int y = -18; y < 18; y += 9)
      painter->drawArc(QRectF(-7, y, 14, 9), 90 * 16, 180 * 16);
    painter->drawLine(0, 18, 0, 25);
    return true;
  }
  if (symbolKey == "DIODE") {
    painter->drawLine(0, -25, 0, -12);
    QPainterPath triangle(QPointF(-12, -12));
    triangle.lineTo(12, -12);
    triangle.lineTo(0, 9);
    triangle.closeSubpath();
    painter->drawPath(triangle);
    painter->drawLine(-12, 10, 12, 10);
    painter->drawLine(0, 10, 0, 25);
    return true;
  }
  if (symbolKey == "VSRC" || symbolKey == "ISRC") {
    painter->drawLine(0, -25, 0, -17);
    painter->drawEllipse(QPointF(0, 0), 17, 17);
    painter->drawLine(0, 17, 0, 25);
    if (symbolKey == "VSRC") {
      painter->drawLine(-5, -7, 5, -7);
      painter->drawLine(0, -12, 0, -2);
      painter->drawLine(-5, 8, 5, 8);
    } else {
      painter->drawLine(0, 9, 0, -8);
      painter->drawLine(0, -8, -5, -2);
      painter->drawLine(0, -8, 5, -2);
    }
    return true;
  }
  if (symbolKey == "GND") {
    painter->drawLine(0, -25, 0, 5);
    painter->drawLine(-15, 5, 15, 5);
    painter->drawLine(-10, 11, 10, 11);
    painter->drawLine(-5, 17, 5, 17);
    return true;
  }
  if (symbolKey == "VCC") {
    painter->drawLine(0, 25, 0, -8);
    painter->drawLine(0, -8, -10, 3);
    painter->drawLine(0, -8, 10, 3);
    return true;
  }
  if (symbolKey == "Q_NPN" || symbolKey == "Q_PNP") {
    painter->drawLine(-30, 0, -8, 0);
    painter->drawLine(-8, -16, -8, 16);
    painter->drawLine(-8, -9, 16, -28);
    painter->drawLine(-8, 9, 16, 28);
    const bool npn = symbolKey == "Q_NPN";
    const QPointF tip(10, npn ? 23 : 14);
    painter->drawLine(tip, tip + QPointF(-7, npn ? -1 : 7));
    painter->drawLine(tip, tip + QPointF(-1, npn ? -7 : 7));
    return true;
  }
  if (symbolKey.startsWith("GATE_")) {
    const bool invert = symbolKey == "GATE_NOT";
    if (invert) {
      painter->drawLine(-22, 0, -16, 0);
      QPainterPath triangle(QPointF(-16, -16));
      triangle.lineTo(-16, 16);
      triangle.lineTo(18, 0);
      triangle.closeSubpath();
      painter->drawPath(triangle);
      painter->drawEllipse(QPointF(23, 0), 5, 5);
      painter->drawLine(28, 0, 30, 0);
      return true;
    }
    painter->drawLine(-32, -8, -20, -8);
    painter->drawLine(-32, 8, -20, 8);
    painter->drawLine(24, 0, 30, 0);
    QPainterPath body(QPointF(-20, -20));
    body.lineTo(-20, 20);
    if (symbolKey == "GATE_AND") {
      body.lineTo(0, 20);
      body.arcTo(QRectF(-2, -20, 52, 40), -90, 180);
      body.lineTo(-20, -20);
    } else {
      body.cubicTo(-8, -12, -8, 12, -20, 20);
      body.cubicTo(4, 20, 18, 12, 24, 0);
      body.cubicTo(18, -12, 4, -20, -20, -20);
      if (symbolKey == "GATE_XOR") {
        QPainterPath extra(QPointF(-25, -20));
        extra.cubicTo(-13, -10, -13, 10, -25, 20);
        painter->drawPath(extra);
      }
    }
    painter->drawPath(body);
    return true;
  }
  if (symbolKey == "DFF") {
    painter->drawRect(QRectF(-22, -24, 44, 48));
    painter->drawLine(-28, -16, -22, -16);
    painter->drawLine(-28, 8, -22, 8);
    painter->drawLine(22, -16, 28, -16);
    painter->drawLine(22, 8, 28, 8);
    painter->drawText(QPointF(-15, -10), QStringLiteral("D"));
    painter->drawText(QPointF(7, -10), QStringLiteral("Q"));
    QPainterPath clock(QPointF(-22, 3));
    clock.lineTo(-15, 8);
    clock.lineTo(-22, 13);
    painter->drawPath(clock);
    return true;
  }
  if (symbolKey == "OPAMP") {
    QPainterPath body(QPointF(-24, -24));
    body.lineTo(-24, 24);
    body.lineTo(26, 0);
    body.closeSubpath();
    painter->drawPath(body);
    painter->drawLine(-30, -12, -24, -12);
    painter->drawLine(-30, 12, -24, 12);
    painter->drawLine(26, 0, 32, 0);
    painter->drawText(QPointF(-20, -7), QStringLiteral("+"));
    painter->drawText(QPointF(-20, 17), QStringLiteral("−"));
    return true;
  }
  return false;
}

void SymbolRenderer::drawPlaceholder(QPainter *painter,
                                     const QString &symbolKey,
                                     const QString &instanceId, bool selected) {
  const QColor stroke = selected ? ui::colorSelected() : ui::colorComponent();
  painter->setPen(QPen(stroke, 1.8));
  painter->setBrush(QBrush(ui::colorBackground()));
  painter->drawRect(-22, -28, 44, 56);

  painter->setPen(ui::colorLabel());
  QFont f = painter->font();
  f.setFamily(QStringLiteral("Menlo")); // TODO: match Python Monospace
  f.setPointSize(7);
  painter->setFont(f);
  painter->drawText(-18, -8, symbolKey);
  painter->drawText(-18, 10, instanceId.left(10));
}

} // namespace deepiri
