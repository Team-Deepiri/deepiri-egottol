#pragma once

#include <QColor>
#include <QMap>
#include <QString>

/**
 * egottol_theme.h — Visual constants matching egottol/ui/main.py (Python GUI).
 *
 * Use these everywhere instead of hard-coded colors so Stage 1 layout matches
 * Python. When adjusting the look, change values here only.
 */
namespace deepiri {
namespace ui {

constexpr int kGridSize = 20;

inline QColor colorBackground() { return QColor("#1a1b2e"); }
inline QColor colorBackgroundDeep() { return QColor("#12121e"); }
inline QColor colorGridDot() { return QColor("#2e3057"); }
inline QColor colorWire() { return QColor("#50fa7b"); }
inline QColor colorComponent() { return QColor("#8be9fd"); }
inline QColor colorPort() { return QColor("#ffb86c"); }
inline QColor colorSelected() { return QColor("#ff79c6"); }
inline QColor colorText() { return QColor("#f8f8f2"); }
inline QColor colorLabel() { return QColor("#bd93f9"); }
inline QColor colorResult() { return QColor("#f1fa8c"); }
inline QColor colorToolbarBg() { return QColor("#2a2a42"); }
inline QColor colorDockBg() { return QColor("#12121e"); }
inline QColor colorHighlight() { return QColor("#bd93f9"); }

/** Palette category colors — keys match SymbolLibrary / registry categories. */
inline QColor categoryColor(const QString &category) {
  static const QMap<QString, QColor> map = {
      {"passive", QColor("#8be9fd")},
      {"active", QColor("#50fa7b")},
      {"analog", QColor("#50fa7b")},
      {"semiconductor", QColor("#ff79c6")},
      {"source", QColor("#f1fa8c")},
      {"power", QColor("#ff5555")},
      {"logic", QColor("#bd93f9")},
      {"ic_block", QColor("#ffb86c")},
      {"rf", QColor("#ff79c6")},
      {"experimental", QColor("#6272a4")},
      {"quantum", QColor("#00ffff")},
      {"sensor", QColor("#ffaa00")},
      {"electromechanical", QColor("#ff69b4")},
      {"connector", QColor("#a0a0a0")},
  };
  return map.value(category, colorText());
}

} // namespace ui
} // namespace deepiri
