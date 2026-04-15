#include "schematic_view.h"
#include "scene.h"

#include <QWheelEvent>
#include <QKeyEvent>
#include <QMouseEvent>

namespace deepiri {

class SchematicView::SchematicViewImpl {
public:
    qreal zoom_factor_ = 1.0;
    bool grid_visible_ = true;
    QPointF pan_start_;
    bool panning_ = false;
};

SchematicView::SchematicView(QWidget* parent)
    : QGraphicsView(parent)
    , d(new SchematicViewImpl)
{
    initialize();
}

SchematicView::SchematicView(SchematicScene* scene, QWidget* parent)
    : QGraphicsView(scene, parent)
    , d(new SchematicViewImpl)
{
    initialize();
}

SchematicView::~SchematicView() {
    delete d;
}

void SchematicView::set_schematic_scene(SchematicScene* scene) {
    setScene(scene);
}

SchematicScene* SchematicView::schematic_scene() const {
    return dynamic_cast<SchematicScene*>(scene());
}

void SchematicView::zoom_in() {
    d->zoom_factor_ *= 1.2;
    update_zoom();
    emit zoom_changed(d->zoom_factor_);
}

void SchematicView::zoom_out() {
    d->zoom_factor_ /= 1.2;
    update_zoom();
    emit zoom_changed(d->zoom_factor_);
}

void SchematicView::zoom_fit() {
    fitInView(scene()->sceneRect(), Qt::KeepAspectRatio);
    d->zoom_factor_ = transform().m11();
    emit zoom_changed(d->zoom_factor_);
}

void SchematicView::zoom_reset() {
    d->zoom_factor_ = 1.0;
    resetTransform();
    emit zoom_changed(d->zoom_factor_);
}

qreal SchematicView::zoom_factor() const {
    return d->zoom_factor_;
}

void SchematicView::set_zoom_factor(qreal factor) {
    d->zoom_factor_ = factor;
    update_zoom();
}

void SchematicView::pan_to(const QPointF& center) {
    centerOn(center);
    emit pan_changed(center);
}

QPointF SchematicView::center() const {
    return mapToScene(QPoint(width() / 2, height() / 2));
}

void SchematicView::set_grid_visible(bool visible) {
    d->grid_visible_ = visible;
    if (scene()) scene()->update();
}

bool SchematicView::is_grid_visible() const {
    return d->grid_visible_;
}

void SchematicView::handle_zoom_in() {
    zoom_in();
}

void SchematicView::handle_zoom_out() {
    zoom_out();
}

void SchematicView::wheelEvent(QWheelEvent* event) {
    if (event->modifiers() & Qt::ControlModifier) {
        if (event->angleDelta().y() > 0) {
            zoom_in();
        } else {
            zoom_out();
        }
        event->accept();
    } else {
        QGraphicsView::wheelEvent(event);
    }
}

void SchematicView::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Plus || event->key() == Qt::Key_Equal) {
        zoom_in();
    } else if (event->key() == Qt::Key_Minus) {
        zoom_out();
    } else if (event->key() == Qt::Key_0) {
        zoom_reset();
    } else if (event->key() == Qt::Key_1) {
        zoom_fit();
    } else {
        QGraphicsView::keyPressEvent(event);
    }
}

void SchematicView::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::MiddleButton || 
        (event->button() == Qt::LeftButton && event->modifiers() & Qt::ShiftModifier)) {
        d->panning_ = true;
        d->pan_start_ = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
    } else {
        QGraphicsView::mousePressEvent(event);
    }
}

void SchematicView::mouseMoveEvent(QMouseEvent* event) {
    if (d->panning_) {
        QPointF delta = event->pos() - d->pan_start_;
        QPointF scene_delta = mapToScene(delta.toPoint()) - mapToScene(QPoint(0, 0));
        QPointF current_center = center();
        pan_to(current_center - scene_delta);
        d->pan_start_ = event->pos();
        event->accept();
    } else {
        QGraphicsView::mouseMoveEvent(event);
    }
}

void SchematicView::mouseReleaseEvent(QMouseEvent* event) {
    if (d->panning_ && (event->button() == Qt::MiddleButton || event->button() == Qt::LeftButton)) {
        d->panning_ = false;
        unsetCursor();
        event->accept();
    } else {
        QGraphicsView::mouseReleaseEvent(event);
    }
}

void SchematicView::focusInEvent(QFocusEvent* event) {
    setFocus();
    QGraphicsView::focusInEvent(event);
}

void SchematicView::focusOutEvent(QFocusEvent* event) {
    QGraphicsView::focusOutEvent(event);
}

void SchematicView::initialize() {
    setRenderHint(QPainter::Antialiasing);
    setRenderHint(QPainter::SmoothPixmapTransform);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorUnderMouse);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setBackgroundBrush(QColor(45, 45, 48));
    setFrameShape(QFrame::NoFrame);
}

void SchematicView::update_zoom() {
    resetTransform();
    scale(d->zoom_factor_, d->zoom_factor_);
}

}