#pragma once

#include <QWidget>

/**
 * waveform_panel.h — Bottom-left plot area (Python: PyQtGraph PlotWidget).
 *
 * STAGE 1: Placeholder label. STAGE 5: DC bar chart. STAGE 6: multi-trace time plot + FFT.
 */
namespace deepiri {

class WaveformPanel : public QWidget {
    Q_OBJECT

public:
    explicit WaveformPanel(QWidget* parent = nullptr);

    /** Stage 5: pass node names + voltages for bar display. */
    void showDcResults(const QStringList& nodes, const QVector<double>& volts);

    void clear();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QStringList dcNodes_;
    QVector<double> dcVolts_;
};

} // namespace deepiri
