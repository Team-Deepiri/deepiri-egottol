#include "component_palette.h"
#include "egottol_theme.h"

#include "../io/symbol_library.h"

#include <QListWidget>
#include <QListWidgetItem>
#include <QVBoxLayout>

namespace deepiri {

ComponentPalette::ComponentPalette(QWidget *parent) : QWidget(parent) {
  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);

  list_ = new QListWidget(this);
  list_->setStyleSheet(
      QStringLiteral("QListWidget{background:%1;color:%2;font-family:monospace;"
                     "font-size:12px;}"
                     "QListWidget::item:hover{background:#2a2a42;}"
                     "QListWidget::item:selected{background:#44475a;}")
          .arg(ui::colorDockBg().name(), ui::colorText().name()));
  layout->addWidget(list_);

  connect(list_, &QListWidget::itemClicked, this,
          [this](QListWidgetItem *) { onItemClicked(); });

  populateFromLibrary();
}

void ComponentPalette::populateFromLibrary() {
  list_->clear();
  SymbolLibrary lib;
  for (const std::string &name : lib.listSymbols()) {
    const QString key = QString::fromStdString(name);
    auto *item = new QListWidgetItem(QStringLiteral("  ") + key);
    item->setData(Qt::UserRole, key);
    item->setForeground(ui::categoryColor(QStringLiteral("passive")));
    list_->addItem(item);
  }
  // TODO Stage 2: map SymbolLibrary entries to Python registry keys (RES vs R)
  // and categories
}

void ComponentPalette::onItemClicked() {
  QListWidgetItem *item = list_->currentItem();
  if (!item)
    return;
  emit componentRequested(item->data(Qt::UserRole).toString());
}

} // namespace deepiri
