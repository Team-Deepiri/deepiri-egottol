#include "wire_item.h"

#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QPainterPath>

namespace deepiri {

class WireItem::WireItemImpl {
public:
    QList<QPointF> points_;
    bool highlighted_ = false;
    bool error_ = false;
    QPainterPath path_;
};

WireItem::WireItem(QGraphicsItem* parent)
    : QGraphicsObject(parent)
    , d(new WireItemImpl)
{
    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setAcceptHoverEvents(true);
}

WireItem::WireItem(const QPointF& start, const QPointF& end, QGraphicsItem* parent)
    : QGraphicsObject(parent)
    , d(new WireItemImpl)
{
    d->points_.append(start);
    d->points_.append(end);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setAcceptHoverEvents(true);
    update_path();
}

WireItem::~WireItem() {
    delete d;
}

void WireItem::set_points(const QList<QPointF>& points) {
    d->points_ = points;
    update_path();
    emit points_changed(points);
}

QList<QPointF> WireItem::points() const {
    return d->points_;
}

void WireItem::add_point(const QPointF& point) {
    d->points_.append(point);
    update_path();
    emit points_changed(d->points_);
}

QPointF WireItem::start_point() const {
    if (d->points_.isEmpty()) return QPointF();
    return d->points_.first();
}

QPointF WireItem::end_point() const {
    if (d->points_.isEmpty()) return QPointF();
    return d->points_.last();
}

void WireItem::set_start_point(const QPointF& point) {
    if (d->points_.isEmpty()) {
        d->points_.append(point);
    } else {
        d->points_[0] = point;
    }
    update_path();
}

void WireItem::set_end_point(const QPointF& point) {
    if (d->points_.isEmpty()) {
        d->points_.append(point);
    } else if (d->points_.size() == 1) {
        d->points_.append(point);
    } else {
        d->points_[d->points_.size() - 1] = point;
    }
    update_path();
}

void WireItem::set_highlighted(bool highlighted) {
    d->highlighted_ = highlighted;
    update();
}

bool WireItem::is_highlighted() const {
    return d->highlighted_;
}

void WireItem::set_error(bool error) {
    d->error_ = error;
    update();
}

bool WireItem::has_error() const {
    return d->error_;
}

QRectF WireItem::boundingRect() const {
    return d->path_.boundingRect();
}

void WireItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    Q_UNUSED(option);
    Q_UNUSED(widget);
    
    QPen pen;
    pen.setWidth(2);
    
    if (option->state & QStyle::State_Selected) {
        pen.setColor(QColor(0, 120, 215));
    } else if (d->error_) {
        pen.setColor(Qt::red);
    } else if (d->highlighted_) {
        pen.setColor(Qt::yellow);
    } else {
        pen.setColor(Qt::green);
    }
    
    painter->setPen(pen);
    painter->drawPath(d->path_);
}

int WireItem::type() const {
    return Type + 2;
}

QVariant WireItem::itemChange(GraphicsItemChange change, const QVariant& value) {
    if (change == ItemPositionHasChanged) {
        update_path();
    }
    return QGraphicsItem::itemChange(change, value);
}

void WireItem::update_path() {
    d->path_ = QPainterPath();
    
    if (d->points_.isEmpty()) return;
    
    d->path_.moveTo(d->points_.first());
    for (int i = 1; i < d->points_.size(); ++i) {
        d->path_.lineTo(d->points_[i]);
    }
}

}