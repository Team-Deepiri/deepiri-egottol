#include "component_item.h"
#include "egottol_theme.h"
#include "property_editor.h"
#include "scene.h"
#include "schematic_document.h"
#include "symbol_renderer.h"

#include <QDebug>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QPainter>
#include <QStyleOptionGraphicsItem>

namespace deepiri {

class ComponentItem::ComponentItemImpl {
public:
  ComponentType type_;
  QString label_;
  QString registry_key_;
  QString symbol_key_;
  QList<Pin> pins_;
  QColor selection_color_;
  QSizeF size_;
};

ComponentItem::ComponentItem(ComponentType type, const QString &label,
                             QGraphicsItem *parent)
    : QGraphicsItem(parent), d(new ComponentItemImpl) {
  d->type_ = type;
  d->label_ = label;
  d->size_ = QSizeF(60, 40);
  setFlag(QGraphicsItem::ItemIsSelectable);
  setFlag(QGraphicsItem::ItemIsMovable);
  setFlag(QGraphicsItem::ItemSendsGeometryChanges);
  setAcceptHoverEvents(true);
  update_pins();
}

ComponentItem::~ComponentItem() { delete d; }

ComponentType ComponentItem::component_type() const { return d->type_; }

QString ComponentItem::label() const { return d->label_; }

void ComponentItem::set_label(const QString &label) {
  d->label_ = label;
  update();
}

QString ComponentItem::registry_key() const { return d->registry_key_; }
void ComponentItem::set_registry_key(const QString &key) {
  d->registry_key_ = key;
}

QString ComponentItem::symbol_key() const { return d->symbol_key_; }
void ComponentItem::set_symbol_key(const QString &key) { d->symbol_key_ = key; }

QList<Pin> ComponentItem::pins() const { return d->pins_; }

void ComponentItem::add_pin(const Pin &pin) {
  d->pins_.append(pin);
  update();
}

QPointF ComponentItem::pin_position(const QString &name) const {
  for (const Pin &pin : d->pins_) {
    if (pin.name == name) {
      return mapToScene(pin.position);
    }
  }
  return QPointF();
}

QString ComponentItem::pin_at(const QPointF &pos) const {
  QPointF local_pos = mapFromScene(pos);
  for (const Pin &pin : d->pins_) {
    if (QLineF(local_pos, pin.position).length() < 10) {
      return pin.name;
    }
  }
  return QString();
}

void ComponentItem::set_selection_color(const QColor &color) {
  d->selection_color_ = color;
  update();
}

QColor ComponentItem::selection_color() const { return d->selection_color_; }

QRectF ComponentItem::boundingRect() const {
  return QRectF(-46, -34, 92, 82);
}

void ComponentItem::paint(QPainter *painter,
                          const QStyleOptionGraphicsItem *option,
                          QWidget *widget) {
  Q_UNUSED(widget);

  painter->setRenderHint(QPainter::RenderHint::Antialiasing);
  const bool selected = option->state & QStyle::State_Selected;
  const QString sym =
      d->symbol_key_.isEmpty() ? QStringLiteral("DEFAULT") : d->symbol_key_;

  if (!SymbolRenderer::draw(painter, sym, selected)) {
    SymbolRenderer::drawPlaceholder(painter, sym, d->label_, selected);
  }

  // Port dots — match Python COLORS["port"]
  painter->setPen(QPen(ui::colorPort(), 1));
  painter->setBrush(QBrush(ui::colorPort()));
  for (const Pin &pin : d->pins_) {
    painter->drawEllipse(pin.position, 4, 4);
  }

  painter->setPen(ui::colorLabel());
  QFont labelFont = painter->font();
  labelFont.setPointSize(7);
  painter->setFont(labelFont);
  painter->drawText(QRectF(-44, 31, 88, 14), Qt::AlignHCenter,
                    d->label_);
}

int ComponentItem::type() const { return Type + 1; }

QVariant ComponentItem::itemChange(GraphicsItemChange change,
                                   const QVariant &value) {
  if (change == ItemPositionHasChanged) {
    if (auto *schematic = dynamic_cast<SchematicScene *>(scene())) {
      if (schematic->document())
        schematic->document()->setComponentPosition(d->label_,
                                                    value.toPointF());
      schematic->refresh_wires_for_component(d->label_);
      schematic->mark_modified();
    }
  }
  return QGraphicsItem::itemChange(change, value);
}

void ComponentItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    const QString port = pin_at(event->scenePos());
    if (!port.isEmpty()) {
      if (auto *schematic = dynamic_cast<SchematicScene *>(scene())) {
        schematic->start_wire(d->label_, port, pin_position(port));
        event->accept();
        return;
      }
    }
  }
  QGraphicsItem::mousePressEvent(event);
}

void ComponentItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
  QGraphicsItem::mouseReleaseEvent(event);
}

void ComponentItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) {
  if (auto *schematic = dynamic_cast<SchematicScene *>(scene())) {
    if (SchematicDocument *document = schematic->document()) {
      if (const SchematicComponent *component =
              document->findComponent(d->label_)) {
        QWidget *parent =
            schematic->views().isEmpty() ? nullptr : schematic->views().first();
        PropertyEditor editor(d->label_, component->parameters, parent);
        if (editor.exec() == QDialog::Accepted) {
          document->setComponentParameters(d->label_, editor.parameters());
          schematic->mark_modified();
        }
        event->accept();
        return;
      }
    }
  }
  QGraphicsItem::mouseDoubleClickEvent(event);
}

void ComponentItem::update_pins() {
  d->pins_.clear();

  switch (d->type_) {
  case ComponentType::AND_GATE:
  case ComponentType::OR_GATE:
  case ComponentType::NAND_GATE:
  case ComponentType::NOR_GATE:
  case ComponentType::XOR_GATE:
  case ComponentType::XNOR_GATE:
    add_pin({"A", QPointF(-30, -10), true, 1});
    add_pin({"B", QPointF(-30, 10), true, 1});
    add_pin({"Y", QPointF(30, 0), false, 1});
    break;
  case ComponentType::NOT_GATE:
    add_pin({"A", QPointF(-30, 0), true, 1});
    add_pin({"Y", QPointF(30, 0), false, 1});
    break;
  case ComponentType::D_FLIPFLOP:
  case ComponentType::JK_FLIPFLOP:
  case ComponentType::T_FLIPFLOP:
    add_pin({"D", QPointF(-30, -10), true, 1});
    add_pin({"CLK", QPointF(-30, 10), true, 1});
    add_pin({"Q", QPointF(30, -10), false, 1});
    add_pin({"QN", QPointF(30, 10), false, 1});
    break;
  case ComponentType::VCC:
    add_pin({"VCC", QPointF(0, 20), false, 1});
    break;
  case ComponentType::GROUND:
    add_pin({"GND", QPointF(0, 20), false, 1});
    break;
  default:
    break;
  }
}

} // namespace deepiri