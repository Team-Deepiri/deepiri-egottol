#pragma once

#include <QWidget>
#include <QColor>
#include <QString>
#include <vector>

QT_BEGIN_NAMESPACE
class QTimer;
class QPaintEvent;
QT_END_NAMESPACE

namespace deepiri {

class WaveformPlotter : public QWidget {
    Q_OBJECT

public:
    explicit WaveformPlotter(QWidget* parent = nullptr);
    ~WaveformPlotter() override;

    struct Trace {
        QString name;
        QColor color;
        std::vector<double> timePoints;
        std::vector<double> values;
    };

    void clear();
    void setTrace(const QString& name, const std::vector<double>& timePoints,
                  const std::vector<double>& values, const QColor& color = QColor());

    const std::vector<Trace>& traces() const { return traces_; }
    bool exportCSV(const QString& filename) const;

    // Animates the sweep from 0% to 100% of the traces over durationMs,
    // like an oscilloscope trigger sweep, instead of drawing the full
    // buffer instantly.
    void animateSweep(int durationMs = 900);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void drawGrid(class QPainter& painter);
    void drawTraces(class QPainter& painter);

    std::vector<Trace> traces_;
    double sweepFraction_ = 1.0;
    QTimer* sweepTimer_ = nullptr;
    qint64 sweepStartMs_ = 0;
    int sweepDurationMs_ = 900;
};

}
