#include "waveform_panel.h"
#include "egottol_theme.h"

#include <QLabel>
#include <QPaintEvent>
#include <QPainter>
#include <QVBoxLayout>

namespace deepiri {

WaveformPanel::WaveformPanel(QWidget *parent) : QWidget(parent) {
  setMinimumHeight(120);
  auto *layout = new QVBoxLayout(this);
  auto *hint = new QLabel(tr("Waveform / DC plot — Stage 5"), this);
  hint->setStyleSheet(QStringLiteral("color:%1;font-family:monospace;")
                          .arg(ui::colorText().name()));
  layout->addWidget(hint);
}

void WaveformPanel::showDcResults(const QStringList &nodes,
                                  const QVector<double> &volts) {
  dcNodes_ = nodes;
  dcVolts_ = volts;
  update();
  // TODO Stage 5: replace paintEvent stub with proper bar chart (Qt Charts)
}

void WaveformPanel::clear() {
  dcNodes_.clear();
  dcVolts_.clear();
  update();
}

void WaveformPanel::paintEvent(QPaintEvent *event) {
  QWidget::paintEvent(event);
  if (dcVolts_.isEmpty())
    return;

  QPainter p(this);
  p.fillRect(rect(), ui::colorDockBg());
  const int n = dcVolts_.size();
  const double barW = width() / double(n + 1);
  p.setPen(ui::colorWire());
  for (int i = 0; i < n; ++i) {
    const double h = qBound(0.0, dcVolts_[i] * 10.0, double(height() - 20));
    p.fillRect(int((i + 1) * barW - barW * 0.3), int(height() - h - 10),
               int(barW * 0.6), int(h), ui::colorWire());
  }
}

} // namespace deepiri
