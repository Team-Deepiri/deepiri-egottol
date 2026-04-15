#include "wire_tool.h"
#include "wire_item.h"
#include "scene.h"
#include "component_item.h"

#include <QGraphicsSceneMouseEvent>
#include <QDebug>

namespace deepiri {

class WireTool::WireToolImpl {
public:
    SchematicScene* scene_ = nullptr;
    WireItem* current_wire_ = nullptr;
    WireItem* preview_wire_ = nullptr;
    QPointF start_point_;
    QPointF current_point_;
    bool active_ = false;
    bool drawing_ = false;
    QColor wire_color_;
};

WireTool::WireTool(QObject* parent)
    : QObject(parent)
    , d(new WireToolImpl)
{
    d->wire_color_ = Qt::green;
}

WireTool::~WireTool() {
    delete d;
}

void WireTool::activate() {
    d->active_ = true;
    emit activated();
}

void WireTool::deactivate() {
    cancel();
    d->active_ = false;
    emit deactivated();
}

bool WireTool::is_active() const {
    return d->active_;
}

void WireTool::set_scene(SchematicScene* scene) {
    d->scene_ = scene;
}

SchematicScene* WireTool::scene() const {
    return d->scene_;
}

void WireTool::handle_click(QGraphicsSceneMouseEvent* event) {
    if (!d->active_ || !d->scene_) return;
    
    QPointF pos = event->scenePos();
    QPointF conn_point = find_connection_point(pos);
    
    if (!d->drawing_) {
        d->start_point_ = conn_point;
        d->drawing_ = true;
        
        d->current_wire_ = new WireItem(d->start_point_, conn_point);
        Q_UNUSED(d->wire_color_);
    } else {
        if (d->current_wire_) {
            QList<QPointF> pts = d->current_wire_->points();
            if (pts.size() >= 2) {
                pts[pts.size() - 1] = conn_point;
                d->current_wire_->set_points(pts);
            }
            
            d->scene_->add_wire(d->current_wire_);
            emit wire_completed(d->current_wire_);
            d->current_wire_ = nullptr;
        }
        d->drawing_ = false;
    }
}

void WireTool::handle_move(QGraphicsSceneMouseEvent* event) {
    if (!d->active_ || !d->scene_) return;
    
    QPointF pos = d->scene_->snap_position(event->scenePos());
    QPointF conn_point = find_connection_point(pos);
    d->current_point_ = conn_point;
    
    if (d->current_wire_) {
        QList<QPointF> pts = d->current_wire_->points();
        if (pts.size() >= 2) {
            pts[pts.size() - 1] = conn_point;
            d->current_wire_->set_points(pts);
        }
    }
}

void WireTool::handle_release(QGraphicsSceneMouseEvent* event) {
    Q_UNUSED(event);
}

void WireTool::set_wire_color(const QColor& color) {
    d->wire_color_ = color;
}

QColor WireTool::wire_color() const {
    return d->wire_color_;
}

void WireTool::cancel() {
    if (d->current_wire_) {
        d->scene_->removeItem(d->current_wire_);
        delete d->current_wire_;
        d->current_wire_ = nullptr;
    }
    d->drawing_ = false;
}

QPointF WireTool::find_connection_point(const QPointF& pos) const {
    if (!d->scene_) return pos;
    
    ComponentItem* comp = d->scene_->component_at(pos);
    if (comp) {
        QString pin_name = comp->pin_at(pos);
        if (!pin_name.isEmpty()) {
            return comp->pin_position(pin_name);
        }
    }
    
    return d->scene_->grid_position(pos);
}

ComponentItem* WireTool::find_component_at(const QPointF& pos) const {
    if (!d->scene_) return nullptr;
    return d->scene_->component_at(pos);
}

void WireTool::update_preview_wire() {
}

}