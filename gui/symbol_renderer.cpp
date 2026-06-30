#include "symbol_renderer.h"
#include "egottol_theme.h"

#include <QFont>
#include <QPainter>

namespace deepiri {

bool SymbolRenderer::draw(QPainter *painter, const QString &symbolKey,
                          bool selected) {
  Q_UNUSED(painter);
  Q_UNUSED(symbolKey);
  Q_UNUSED(selected);
  // TODO Stage 2: switch(symbolKey) and execute Python SYMBOLS command lists
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
