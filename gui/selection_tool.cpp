#include "selection_tool.h"
#include "scene.h"
#include "component_item.h"
#include "wire_item.h"

#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>
#include <QDebug>

namespace deepiri {

class SelectionTool::SelectionToolImpl {
public:
    SchematicScene* scene_ = nullptr;
    bool active_ = false;
    bool selecting_ = false;
    QPointF selection_start_;
    QPointF selection_end_;
};

SelectionTool::SelectionTool(QObject* parent)
    : QObject(parent)
    , d(new SelectionToolImpl)
{
}

SelectionTool::~SelectionTool() {
    delete d;
}

void SelectionTool::activate() {
    d->active_ = true;
    emit activated();
}

void SelectionTool::deactivate() {
    d->active_ = false;
    emit deactivated();
}

bool SelectionTool::is_active() const {
    return d->active_;
}

void SelectionTool::set_scene(SchematicScene* scene) {
    d->scene_ = scene;
}

SchematicScene* SelectionTool::scene() const {
    return d->scene_;
}

void SelectionTool::handle_click(QGraphicsSceneMouseEvent* event) {
    if (!d->active_ || !d->scene_) return;
    
    QPointF pos = event->scenePos();
    
    if (event->modifiers() & Qt::ControlModifier) {
        return;
    }
    
    ComponentItem* comp = find_component_underMouse(pos);
    WireItem* wire = find_wire_underMouse(pos);
    
    if (comp) {
        if (!comp->isSelected()) {
            d->scene_->clearSelection();
            comp->setSelected(true);
        }
    } else if (wire) {
        if (!wire->isSelected()) {
            d->scene_->clearSelection();
            wire->setSelected(true);
        }
    } else {
        d->scene_->clearSelection();
        d->selecting_ = true;
        d->selection_start_ = pos;
        d->selection_end_ = pos;
    }
    
    emit selection_changed(selected_items());
}

void SelectionTool::handle_move(QGraphicsSceneMouseEvent* event) {
    if (!d->active_ || !d->scene_) return;
    
    if (d->selecting_) {
        d->selection_end_ = event->scenePos();
        update_selection_rect();
    }
}

void SelectionTool::handle_release(QGraphicsSceneMouseEvent* event) {
    if (!d->active_ || !d->scene_) return;
    
    if (d->selecting_) {
        d->selecting_ = false;
        
        QRectF rect = QRectF(d->selection_start_, d->selection_end_).normalized();
        QList<QGraphicsItem*> items = d->scene_->items(rect);
        
        d->scene_->clearSelection();
        for (QGraphicsItem* item : items) {
            item->setSelected(true);
        }
        
        emit selection_changed(selected_items());
    }
}

void SelectionTool::handle_key_press(QKeyEvent* event) {
    if (!d->active_) return;
    
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        delete_selected();
    } else if (event->key() == Qt::Key_A && event->modifiers() & Qt::ControlModifier) {
        select_all();
    } else if (event->key() == Qt::Key_Escape) {
        cancel();
    }
}

void SelectionTool::select_all() {
    if (!d->scene_) return;
    
    for (QGraphicsItem* item : d->scene_->items()) {
        item->setSelected(true);
    }
    emit selection_changed(selected_items());
}

void SelectionTool::clear_selection() {
    if (!d->scene_) return;
    d->scene_->clearSelection();
    emit selection_changed(selected_items());
}

void SelectionTool::delete_selected() {
    if (!d->scene_) return;
    
    QList<QGraphicsItem*> items = selected_items();
    for (QGraphicsItem* item : items) {
        d->scene_->removeItem(item);
    }
    emit selection_changed(selected_items());
}

void SelectionTool::copy_selected() {
}

void SelectionTool::paste_selected() {
}

QList<QGraphicsItem*> SelectionTool::selected_items() const {
    if (!d->scene_) return QList<QGraphicsItem*>();
    return d->scene_->selectedItems();
}

void SelectionTool::cancel() {
    clear_selection();
    d->selecting_ = false;
}

ComponentItem* SelectionTool::find_component_underMouse(const QPointF& pos) const {
    if (!d->scene_) return nullptr;
    return d->scene_->component_at(pos);
}

WireItem* SelectionTool::find_wire_underMouse(const QPointF& pos) const {
    if (!d->scene_) return nullptr;
    QList<WireItem*> wires = d->scene_->wires_at(pos);
    if (!wires.isEmpty()) {
        return wires.first();
    }
    return nullptr;
}

void SelectionTool::update_selection_rect() {
}

}