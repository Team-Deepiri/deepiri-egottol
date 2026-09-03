#pragma once

#include <QString>
#include <QWidget>

class QListWidget;

/**
 * component_palette.h — Left dock: component list (Python: EgottolApp._palette
 * QListWidget).
 *
 * Populated from SymbolLibrary::listSymbols(). Emits registryKey when user
 * clicks an entry. MainWindow connects componentRequested →
 * SchematicScene::set_place_mode(key).
 */
namespace deepiri {

class ComponentPalette : public QWidget {
  Q_OBJECT

public:
  explicit ComponentPalette(QWidget *parent = nullptr);

  /** Fill list from io/symbol_library. Call once at startup or after library
   * reload. */
  void populateFromLibrary();

signals:
  /** Registry key to place, e.g. "RES", "VSRC" — same strings as Python
   * COMPONENT_LIBRARY. */
  void componentRequested(const QString &registryKey);

private:
  void onItemClicked();

  QListWidget *list_ = nullptr;
};

} // namespace deepiri
