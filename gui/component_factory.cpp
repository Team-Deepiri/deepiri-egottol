#include "component_factory.h"
#include "component_item.h"
#include "port_layout.h"
#include "scene.h"
#include "schematic_document.h"

#include "../io/symbol_library.h"

#include <QUuid>
#include <optional>

namespace deepiri {

QString ComponentFactory::makeInstanceId(const QString &registryKey) {
  return registryKey + QStringLiteral("_") +
         QUuid::createUuid().toString(QUuid::WithoutBraces).left(6);
}

ComponentItem *ComponentFactory::placeComponent(SchematicScene *scene,
                                                SchematicDocument *document,
                                                const QString &registryKey,
                                                const QPointF &scenePos) {
  if (!scene || !document)
    return nullptr;

  SymbolLibrary lib;
  const QString symbolKey = PortLayout::symbolKeyForRegistry(registryKey);

  // SymbolLibrary keys are often short names ("R"); registry keys match Python
  // ("RES").
  std::optional<SymbolDefinition> symOpt =
      lib.getSymbol(registryKey.toStdString());
  if (!symOpt)
    symOpt = lib.getSymbol(symbolKey.toStdString());

  QString displayName = registryKey;
  if (symOpt) {
    displayName = QString::fromStdString(symOpt->description);
  }

  const QString instanceId = makeInstanceId(registryKey);

  ComponentItem *item = new ComponentItem(ComponentType::CUSTOM, instanceId);
  item->set_registry_key(registryKey);
  item->set_symbol_key(symbolKey);
  item->setPos(scenePos);
  item->setToolTip(displayName);

  for (const PortDef &p : PortLayout::portsForSymbol(symbolKey)) {
    Pin pin;
    pin.name = p.name;
    pin.position = QPointF(p.x, p.y);
    pin.is_input = true;
    pin.bit_width = 1;
    item->add_pin(pin);
  }

  scene->add_component(item);

  SchematicComponent docComp;
  docComp.id = instanceId;
  docComp.registryKey = registryKey;
  docComp.displayName = displayName;
  docComp.category = QStringLiteral("passive");
  docComp.symbolKey = symbolKey;
  docComp.position = scenePos;
  document->addComponent(std::move(docComp));

  return item;
}

} // namespace deepiri
