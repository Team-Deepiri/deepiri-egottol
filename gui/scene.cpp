#include "scene.h"
#include "component_factory.h"
#include "component_item.h"
#include "egottol_theme.h"
#include "schematic_document.h"
#include "selection_tool.h"
#include "wire_item.h"
#include "wire_tool.h"

#include <QDebug>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QKeyEvent>
#include <QPainter>
#include <QUuid>

namespace deepiri {

class SchematicScene::SchematicSceneImpl {
public:
  QList<ComponentItem *> components_;
  QList<WireItem *> wires_;
  double grid_size_ = ui::kGridSize;
  bool snap_to_grid_ = true;
  SelectionTool *selection_tool_ = nullptr;
  WireTool *wire_tool_ = nullptr;
  SchematicDocument *document_ = nullptr;
  QString place_mode_key_;
  WireItem *pending_wire_ = nullptr;
  QString pending_component_id_;
  QString pending_port_;
  QList<QGraphicsTextItem *> result_labels_;
};

SchematicScene::SchematicScene(QObject *parent)
    : QGraphicsScene(parent), d(new SchematicSceneImpl) {
  setSceneRect(-3000, -3000, 6000, 6000);
  setBackgroundBrush(ui::colorBackground());
}

SchematicScene::~SchematicScene() { delete d; }

void SchematicScene::add_component(ComponentItem *component) {
  if (component && !d->components_.contains(component)) {
    d->components_.append(component);
    if (component->scene() != this)
      addItem(component);
    emit component_added(component);
    mark_modified();
  }
}

void SchematicScene::add_wire(WireItem *wire) {
  if (wire && !d->wires_.contains(wire)) {
    d->wires_.append(wire);
    if (wire->scene() != this)
      addItem(wire);
    emit wire_added(wire);
    mark_modified();
  }
}

void SchematicScene::remove_component(ComponentItem *component) {
  if (component && d->components_.contains(component)) {
    const QString id = component->label();
    const QList<WireItem *> attached = d->wires_;
    for (WireItem *wire : attached) {
      if (wire->from_component_id() == id || wire->to_component_id() == id)
        remove_wire(wire);
    }
    d->components_.removeAll(component);
    removeItem(component);
    if (d->document_)
      d->document_->removeComponent(id);
    emit component_removed(component);
    delete component;
    mark_modified();
  }
}

void SchematicScene::remove_wire(WireItem *wire) {
  if (wire && d->wires_.contains(wire)) {
    d->wires_.removeAll(wire);
    removeItem(wire);
    if (d->document_) {
      QString documentWireId;
      for (const auto &documentWire : d->document_->wires()) {
        if (documentWire.fromComponentId == wire->from_component_id() &&
            documentWire.fromPort == wire->from_port() &&
            documentWire.toComponentId == wire->to_component_id() &&
            documentWire.toPort == wire->to_port()) {
          documentWireId = documentWire.id;
          break;
        }
      }
      if (!documentWireId.isEmpty())
        d->document_->removeWire(documentWireId);
    }
    emit wire_removed(wire);
    delete wire;
    mark_modified();
  }
}

QList<ComponentItem *> SchematicScene::components() const {
  return d->components_;
}

QList<WireItem *> SchematicScene::wires() const { return d->wires_; }

ComponentItem *SchematicScene::component_at(const QPointF &pos) const {
  for (QGraphicsItem *item : items(pos)) {
    ComponentItem *comp = dynamic_cast<ComponentItem *>(item);
    if (comp && comp->isVisible()) {
      return comp;
    }
  }
  return nullptr;
}

ComponentItem *SchematicScene::component_by_id(const QString &id) const {
  for (ComponentItem *component : d->components_) {
    if (component->label() == id)
      return component;
  }
  return nullptr;
}

QList<WireItem *> SchematicScene::wires_at(const QPointF &pos) const {
  QList<WireItem *> result;
  for (QGraphicsItem *item : items(pos)) {
    WireItem *wire = dynamic_cast<WireItem *>(item);
    if (wire && wire->isVisible()) {
      result.append(wire);
    }
  }
  return result;
}

void SchematicScene::set_grid_size(double size) {
  d->grid_size_ = size;
  update();
}

double SchematicScene::grid_size() const { return d->grid_size_; }

void SchematicScene::set_snap_to_grid(bool snap) { d->snap_to_grid_ = snap; }

bool SchematicScene::snap_to_grid() const { return d->snap_to_grid_; }

void SchematicScene::set_selection_tool(SelectionTool *tool) {
  d->selection_tool_ = tool;
}

void SchematicScene::set_wire_tool(WireTool *tool) { d->wire_tool_ = tool; }

QPointF SchematicScene::snap_position(const QPointF &pos) const {
  if (d->snap_to_grid_) {
    return grid_position(pos);
  }
  return pos;
}

QPointF SchematicScene::grid_position(const QPointF &pos) const {
  qreal x = qRound(pos.x() / d->grid_size_) * d->grid_size_;
  qreal y = qRound(pos.y() / d->grid_size_) * d->grid_size_;
  return QPointF(x, y);
}

void SchematicScene::set_document(SchematicDocument *document) {
  d->document_ = document;
}

SchematicDocument *SchematicScene::document() const { return d->document_; }

void SchematicScene::set_place_mode(const QString &registryKey) {
  d->place_mode_key_ = registryKey;
  emit placeModeChanged(QStringLiteral("PLACE: ") + registryKey);
  for (QGraphicsView *v : views()) {
    v->setCursor(Qt::CrossCursor);
  }
}

void SchematicScene::clear_place_mode() {
  d->place_mode_key_.clear();
  emit placeModeChanged(QStringLiteral("SELECT"));
  for (QGraphicsView *v : views()) {
    v->unsetCursor();
  }
}

QString SchematicScene::place_mode() const { return d->place_mode_key_; }

void SchematicScene::handle_escape() {
  clear_place_mode();
  cancel_wire();
  if (d->wire_tool_)
    d->wire_tool_->cancel();
}

bool SchematicScene::start_wire(const QString &componentId,
                                const QString &portName,
                                const QPointF &scenePosition) {
  if (d->pending_wire_)
    return finish_wire(componentId, portName, scenePosition);
  if (!component_by_id(componentId) || portName.isEmpty())
    return false;
  d->pending_component_id_ = componentId;
  d->pending_port_ = portName;
  d->pending_wire_ = new WireItem(scenePosition, scenePosition);
  d->pending_wire_->setZValue(-1);
  addItem(d->pending_wire_);
  return true;
}

bool SchematicScene::finish_wire(const QString &componentId,
                                 const QString &portName,
                                 const QPointF &scenePosition) {
  if (!d->pending_wire_ || !d->document_)
    return false;
  if (componentId == d->pending_component_id_ && portName == d->pending_port_) {
    cancel_wire();
    return false;
  }

  d->pending_wire_->set_end_point(scenePosition);
  d->pending_wire_->set_endpoints(d->pending_component_id_, d->pending_port_,
                                  componentId, portName);
  SchematicWire wire;
  wire.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
  wire.fromComponentId = d->pending_component_id_;
  wire.fromPort = d->pending_port_;
  wire.toComponentId = componentId;
  wire.toPort = portName;
  const size_t oldSize = d->document_->wires().size();
  d->document_->addWire(wire);
  if (d->document_->wires().size() == oldSize) {
    cancel_wire();
    return false;
  }

  WireItem *completed = d->pending_wire_;
  d->pending_wire_ = nullptr;
  d->pending_component_id_.clear();
  d->pending_port_.clear();
  add_wire(completed);
  return true;
}

void SchematicScene::update_wire_preview(const QPointF &scenePosition) {
  if (d->pending_wire_)
    d->pending_wire_->set_end_point(snap_position(scenePosition));
}

void SchematicScene::cancel_wire() {
  if (d->pending_wire_) {
    removeItem(d->pending_wire_);
    delete d->pending_wire_;
    d->pending_wire_ = nullptr;
  }
  d->pending_component_id_.clear();
  d->pending_port_.clear();
}

bool SchematicScene::wire_in_progress() const {
  return d->pending_wire_ != nullptr;
}

void SchematicScene::refresh_wires_for_component(const QString &componentId) {
  for (WireItem *wire : d->wires_) {
    if (wire->from_component_id() == componentId) {
      if (ComponentItem *component = component_by_id(componentId))
        wire->set_start_point(component->pin_position(wire->from_port()));
    }
    if (wire->to_component_id() == componentId) {
      if (ComponentItem *component = component_by_id(componentId))
        wire->set_end_point(component->pin_position(wire->to_port()));
    }
  }
}

void SchematicScene::delete_selection() {
  const QList<QGraphicsItem *> selected = selectedItems();
  QList<ComponentItem *> componentsToDelete;
  QList<WireItem *> wiresToDelete;
  for (QGraphicsItem *item : selected) {
    if (auto *component = dynamic_cast<ComponentItem *>(item))
      componentsToDelete.append(component);
    else if (auto *wire = dynamic_cast<WireItem *>(item))
      wiresToDelete.append(wire);
  }
  for (ComponentItem *component : componentsToDelete)
    remove_component(component);
  for (WireItem *wire : wiresToDelete) {
    if (d->wires_.contains(wire))
      remove_wire(wire);
  }
  emit selection_changed({});
}

void SchematicScene::annotate_dc_results(
    const QMap<QString, double> &portVoltages) {
  for (QGraphicsTextItem *label : d->result_labels_) {
    removeItem(label);
    delete label;
  }
  d->result_labels_.clear();
  for (auto it = portVoltages.cbegin(); it != portVoltages.cend(); ++it) {
    const int separator = it.key().lastIndexOf(':');
    if (separator < 0)
      continue;
    ComponentItem *component = component_by_id(it.key().left(separator));
    if (!component)
      continue;
    auto *label = addText(QStringLiteral("%1 V").arg(it.value(), 0, 'g', 4));
    label->setDefaultTextColor(ui::colorLabel());
    label->setScale(0.75);
    label->setPos(component->pin_position(it.key().mid(separator + 1)) +
                  QPointF(6, -12));
    d->result_labels_.append(label);
  }
}

void SchematicScene::mark_modified() {
  annotate_dc_results({});
  emit schematicChanged();
}

void SchematicScene::clear_canvas() {
  cancel_wire();
  clear();
  d->components_.clear();
  d->wires_.clear();
  d->result_labels_.clear();
  d->place_mode_key_.clear();
  emit placeModeChanged(QStringLiteral("SELECT"));
}

void SchematicScene::drawBackground(QPainter *painter, const QRectF &rect) {
  QGraphicsScene::drawBackground(painter, rect);
  painter->setPen(QPen(ui::colorGridDot(), 1));
  const int grid = int(d->grid_size_);
  int x = int(rect.left()) / grid * grid;
  while (x <= int(rect.right()) + grid) {
    int y = int(rect.top()) / grid * grid;
    while (y <= int(rect.bottom()) + grid) {
      painter->drawPoint(x, y);
      y += grid;
    }
    x += grid;
  }
}

void SchematicScene::mousePressEvent(QGraphicsSceneMouseEvent *event) {
  // Place mode: drop component before tool handlers (Python
  // SchematicScene.mousePressEvent)
  if (!d->place_mode_key_.isEmpty() && event->button() == Qt::LeftButton) {
    const QPointF pos = grid_position(event->scenePos());
    if (d->document_) {
      ComponentFactory::placeComponent(this, d->document_, d->place_mode_key_,
                                       pos);
    }
    if (!(event->modifiers() & Qt::ShiftModifier)) {
      clear_place_mode();
    }
    return;
  }

  if (d->wire_tool_ && d->wire_tool_->is_active()) {
    d->wire_tool_->handle_click(event);
    return;
  }
  QGraphicsScene::mousePressEvent(event);
}

void SchematicScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
  if (d->pending_wire_) {
    update_wire_preview(event->scenePos());
    event->accept();
    return;
  }
  if (d->wire_tool_ && d->wire_tool_->is_active()) {
    d->wire_tool_->handle_move(event);
    return;
  }
  QGraphicsScene::mouseMoveEvent(event);
}

void SchematicScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
  if (d->wire_tool_ && d->wire_tool_->is_active()) {
    d->wire_tool_->handle_release(event);
    return;
  }
  QGraphicsScene::mouseReleaseEvent(event);
}

void SchematicScene::contextMenuEvent(QGraphicsSceneContextMenuEvent *event) {
  QGraphicsScene::contextMenuEvent(event);
}

void SchematicScene::keyPressEvent(QKeyEvent *event) {
  if (event->key() == Qt::Key_Escape) {
    handle_escape();
  }
  if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
    delete_selection();
    event->accept();
    return;
  }
  QGraphicsScene::keyPressEvent(event);
}

void SchematicScene::draw_grid(QPainter *painter) {
  QPen grid_pen(QColor(60, 60, 64));
  grid_pen.setWidth(1);
  painter->setPen(grid_pen);

  qreal left = sceneRect().left();
  qreal right = sceneRect().right();
  qreal top = sceneRect().top();
  qreal bottom = sceneRect().bottom();

  for (qreal x = left; x <= right; x += d->grid_size_) {
    painter->drawLine(QPointF(x, top), QPointF(x, bottom));
  }
  for (qreal y = top; y <= bottom; y += d->grid_size_) {
    painter->drawLine(QPointF(left, y), QPointF(right, y));
  }
}

} // namespace deepiri