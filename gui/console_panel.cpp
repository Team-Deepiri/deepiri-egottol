#include "console_panel.h"
#include "egottol_theme.h"

#include <QTextEdit>
#include <QVBoxLayout>

namespace deepiri {

ConsolePanel::ConsolePanel(QWidget *parent) : QWidget(parent) {
  text_ = new QTextEdit(this);
  text_->setReadOnly(true);
  text_->setStyleSheet(
      QStringLiteral(
          "background:%1;color:%2;font-family:monospace;font-size:11px;")
          .arg(ui::colorDockBg().name(), ui::colorWire().name()));
  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(text_);
}

void ConsolePanel::appendLine(const QString &line) { text_->append(line); }

void ConsolePanel::clear() { text_->clear(); }

} // namespace deepiri
