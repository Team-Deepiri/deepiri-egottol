#include "waveform_panel.h"
#include "egottol_theme.h"

#include <QLabel>
#include <QPaintEvent>
#include <QPainter>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>

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
  if (n == 0)
    return;
  double maximum = 0.0;
  for (double voltage : dcVolts_)
    maximum = std::max(maximum, std::abs(voltage));
  if (maximum < 1e-12)
    maximum = 1.0;

  const int top = 24;
  const int bottom = height() - 26;
  const int zeroY = (top + bottom) / 2;
  const double scale = (bottom - top) * 0.45 / maximum;
  const double barW = width() / double(n + 1);
  p.setPen(QPen(ui::colorGridDot(), 1));
  p.drawLine(0, zeroY, width(), zeroY);
  for (int i = 0; i < n; ++i) {
    const double signedHeight = dcVolts_[i] * scale;
    const int x = int((i + 1) * barW - barW * 0.3);
    const int y = signedHeight >= 0 ? int(zeroY - signedHeight) : zeroY;
    p.fillRect(x, y, std::max(2, int(barW * 0.6)),
               std::max(1, int(std::abs(signedHeight))), ui::colorWire());
    p.setPen(ui::colorText());
    p.drawText(QRectF((i + 0.5) * barW, bottom + 4, barW, 18),
               Qt::AlignCenter, dcNodes_.value(i));
    p.drawText(QRectF((i + 0.5) * barW, top - 18, barW, 18),
               Qt::AlignCenter,
               QStringLiteral("%1 V").arg(dcVolts_[i], 0, 'g', 4));
  }
}

} // namespace deepiri
