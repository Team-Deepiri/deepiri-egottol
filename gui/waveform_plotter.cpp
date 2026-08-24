#include "waveform_plotter.h"

#include "../io/waveform_writer.h"

#include <QPainter>
#include <QPaintEvent>
#include <QTimer>
#include <QDateTime>
#include <algorithm>
#include <limits>

namespace deepiri {

namespace {
const QColor kPalette[] = {
    QColor(0x50, 0xfa, 0x7b),  // green
    QColor(0xbd, 0x93, 0xf9),  // purple
    QColor(0xff, 0xb8, 0x6c),  // orange
    QColor(0x8b, 0xe9, 0xfd),  // cyan
    QColor(0xff, 0x79, 0xc6),  // pink
};
}

WaveformPlotter::WaveformPlotter(QWidget* parent) : QWidget(parent) {
    setMinimumHeight(160);

    sweepTimer_ = new QTimer(this);
    sweepTimer_->setInterval(16);
    connect(sweepTimer_, &QTimer::timeout, this, [this]() {
        qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - sweepStartMs_;
        sweepFraction_ = sweepDurationMs_ > 0
            ? std::min(1.0, static_cast<double>(elapsed) / sweepDurationMs_)
            : 1.0;
        if (sweepFraction_ >= 1.0) {
            sweepTimer_->stop();
        }
        update();
    });
}

WaveformPlotter::~WaveformPlotter() = default;

void WaveformPlotter::clear() {
    traces_.clear();
    sweepFraction_ = 1.0;
    update();
}

void WaveformPlotter::setTrace(const QString& name, const std::vector<double>& timePoints,
                                const std::vector<double>& values, const QColor& color) {
    Trace t;
    t.name = name;
    t.timePoints = timePoints;
    t.values = values;
    t.color = color.isValid() ? color : kPalette[traces_.size() % (sizeof(kPalette) / sizeof(kPalette[0]))];
    traces_.push_back(std::move(t));
    update();
}

bool WaveformPlotter::exportCSV(const QString& filename) const {
    if (traces_.empty()) return false;
    std::vector<WaveformData> waveforms;
    waveforms.reserve(traces_.size());
    for (const auto& t : traces_) {
        WaveformData wd;
        wd.name = t.name.toStdString();
        wd.unit = "V";
        wd.time_points = t.timePoints;
        wd.values = t.values;
        waveforms.push_back(std::move(wd));
    }
    WaveformWriter writer;
    writer.setFormat(WaveformFormat::CSV);
    return writer.writeCSV(filename.toStdString(), waveforms);
}

void WaveformPlotter::animateSweep(int durationMs) {
    sweepDurationMs_ = durationMs;
    sweepFraction_ = 0.0;
    sweepStartMs_ = QDateTime::currentMSecsSinceEpoch();
    sweepTimer_->start();
    update();
}

void WaveformPlotter::drawGrid(QPainter& painter) {
    painter.fillRect(rect(), QColor(0x1a, 0x1b, 0x2e));
    painter.setPen(QPen(QColor(0x3a, 0x3b, 0x5e), 1));
    const int cols = 10;
    const int rows = 6;
    for (int c = 0; c <= cols; ++c) {
        int x = width() * c / cols;
        painter.drawLine(x, 0, x, height());
    }
    for (int r = 0; r <= rows; ++r) {
        int y = height() * r / rows;
        painter.drawLine(0, y, width(), y);
    }
}

void WaveformPlotter::drawTraces(QPainter& painter) {
    if (traces_.empty()) {
        painter.setPen(QColor(0x6b, 0x6f, 0x8e));
        painter.drawText(rect(), Qt::AlignCenter, "No simulation data — run a simulation to see traces");
        return;
    }

    double tMin = std::numeric_limits<double>::max();
    double tMax = std::numeric_limits<double>::lowest();
    double vMin = std::numeric_limits<double>::max();
    double vMax = std::numeric_limits<double>::lowest();
    for (const auto& trace : traces_) {
        for (double t : trace.timePoints) { tMin = std::min(tMin, t); tMax = std::max(tMax, t); }
        for (double v : trace.values) { vMin = std::min(vMin, v); vMax = std::max(vMax, v); }
    }
    if (tMax <= tMin) tMax = tMin + 1.0;
    if (vMax <= vMin) { vMin -= 1.0; vMax += 1.0; }
    double vPad = (vMax - vMin) * 0.1;
    vMin -= vPad;
    vMax += vPad;

    const int margin = 8;
    const double w = width() - 2.0 * margin;
    const double h = height() - 2.0 * margin;

    for (const auto& trace : traces_) {
        size_t n = std::min(trace.timePoints.size(), trace.values.size());
        size_t visible = static_cast<size_t>(n * sweepFraction_);
        if (visible < 2) continue;

        painter.setPen(QPen(trace.color, 2));

        QPointF prev;
        for (size_t i = 0; i < visible; ++i) {
            double nx = (trace.timePoints[i] - tMin) / (tMax - tMin);
            double ny = (trace.values[i] - vMin) / (vMax - vMin);
            QPointF p(margin + nx * w, margin + h - ny * h);
            if (i > 0) painter.drawLine(prev, p);
            prev = p;
        }
    }

    int legendY = margin;
    for (const auto& trace : traces_) {
        painter.setPen(trace.color);
        painter.drawText(margin, legendY + 12, trace.name);
        legendY += 14;
    }
}

void WaveformPlotter::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    drawGrid(painter);
    drawTraces(painter);
}

}
