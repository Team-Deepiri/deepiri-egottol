#pragma once

#include <QPointF>
#include <QString>

class QGraphicsItem;

namespace deepiri {

class SchematicDocument;
class SchematicScene;
class ComponentItem;

/**
 * component_factory.h — Create canvas items + document entries from registry
 * keys.
 *
 * Python equivalent: SchematicScene._drop_component() in main.py.
 *
 * Flow when user clicks canvas in place mode:
 *   1. MainWindow receives registry key from palette/toolbar
 *   2. SchematicScene calls ComponentFactory::placeComponent(...)
 *   3. Factory creates ComponentItem, adds to scene, appends SchematicComponent
 * to document
 */
class ComponentFactory {
public:
  /**
   * Place one component at scene position (already grid-snapped).
   * @param registryKey  e.g. "RES", "VSRC" — from SymbolLibrary / palette
   * @return new ComponentItem owned by scene, or nullptr on unknown key
   */
  static ComponentItem *placeComponent(SchematicScene *scene,
                                       SchematicDocument *document,
                                       const QString &registryKey,
                                       const QPointF &scenePos);

  /** Generate instance id like Python: "{KEY}_{uuid6}". */
  static QString makeInstanceId(const QString &registryKey);
};

} // namespace deepiri
