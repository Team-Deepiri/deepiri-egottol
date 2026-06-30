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
#include <QGraphicsView>
#include <QKeyEvent>
#include <QPainter>

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
    addItem(component);
    emit component_added(component);
  }
}

void SchematicScene::add_wire(WireItem *wire) {
  if (wire && !d->wires_.contains(wire)) {
    d->wires_.append(wire);
    addItem(wire);
    emit wire_added(wire);
  }
}

void SchematicScene::remove_component(ComponentItem *component) {
  if (component && d->components_.contains(component)) {
    d->components_.removeAll(component);
    removeItem(component);
    emit component_removed(component);
  }
}

void SchematicScene::remove_wire(WireItem *wire) {
  if (wire && d->wires_.contains(wire)) {
    d->wires_.removeAll(wire);
    removeItem(wire);
    emit wire_removed(wire);
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
  if (d->wire_tool_)
    d->wire_tool_->cancel();
}

void SchematicScene::clear_canvas() {
  clear();
  d->components_.clear();
  d->wires_.clear();
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
    QList<QGraphicsItem *> selected_items = selectedItems();
    for (QGraphicsItem *item : selected_items) {
      removeItem(item);
    }
    emit selection_changed(selected_items);
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