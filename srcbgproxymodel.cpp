#include "srcbgproxymodel.h"
#include <QDebug>
#include <QGridLayout>
#include <QLineEdit>
#include <QPushButton>

SrcBGProxyModel::SrcBGProxyModel(
    QObject *parent) : QSortFilterProxyModel(parent) {
  dataModel = new DataModel(parent);
  setSourceModel(dataModel);
  optionsButton = new QPushButton("Допустимые макросы");
  optionsDialog = new OptionsDialog(this);
  optionsDialog->listWidget->addItem("Macro 316");
  updateOptionsButtonText();

  connect(optionsButton, &QPushButton::clicked,
          optionsDialog, &SrcBGProxyModel::OptionsDialog::exec);
}

bool SrcBGProxyModel::filterAcceptsRow(
    int source_row,
    [[maybe_unused]] const QModelIndex &source_parent) const {
  QStringList macros_filtered_out;
  for (int i = 0; i < optionsDialog->listWidget->count(); ++i) {
    macros_filtered_out.append(optionsDialog
                               ->listWidget->item(i)->data(Qt::DisplayRole)
                                                      .toString());
  }
  for (const auto& substring
       : sourceModel()->data(sourceModel()->index(source_row, 2),
                             Qt::UserRole).toStringList()) {
    if (!macros_filtered_out.contains(substring))
      return true;
  }
  return false;

}

SrcBGProxyModel::DataModel::DataModel(QObject* parent)
    : QAbstractTableModel(parent) {}

int SrcBGProxyModel::DataModel::rowCount(
        [[maybe_unused]] const QModelIndex& parent) const {
  return bg_errors.size();
}

int SrcBGProxyModel::DataModel::columnCount(
        [[maybe_unused]] const QModelIndex& parent) const {
  return 3;
}

QVariant SrcBGProxyModel::DataModel::data(
        const QModelIndex& index, int role) const {
  if (role == Qt::DisplayRole) {
    auto row = index.row();
    auto col = index.column();
    switch (col) {
    case 0:
      return bg_errors[row].file_name;
    case 1:
      return bg_errors[row].line;
    case 2:
      return bg_errors[row].errors.join(", ");
    }
  } else if (role == Qt::UserRole && index.column() == 2) {
    return bg_errors[index.row()].errors;
  }
  return QVariant();
}

QVariant SrcBGProxyModel::DataModel::headerData(int section,
                                              Qt::Orientation orientation,
                                              int role) const {
  if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
    switch (section) {
    case 0:
      return "Имя файла";
    case 1:
      return "Номер строки";
    case 2:
      return "Ошибки";
    }
  }
  return QAbstractTableModel::headerData(section, orientation, role);
}

void SrcBGProxyModel::DataModel::setBGErrors(const QList<Data> &new_bg_errors) {
  beginResetModel();
  bg_errors = new_bg_errors;
  endResetModel();
}

SrcBGProxyModel::OptionsDialog::OptionsDialog(SrcBGProxyModel *srcBGProxyModel/*,
                                              QWidget *parent,
                                              Qt::WindowFlags f*/)
  : QDialog(/*parent, f*/) {
  setWindowTitle("Допустимые макросы");
  resize(400, 200);
  auto layout = new QGridLayout(this);

  listWidget = new QListWidget(this);
  layout->addWidget(listWidget, layout->rowCount(), 0, 1, -1);

  auto lineEdit = new QLineEdit(this);
  layout->addWidget(lineEdit, layout->rowCount(), 0, 1, 1);

  auto addButton = new QPushButton("Добавить", this);
  layout->addWidget(addButton, layout->rowCount() - 1, 1, 1, 1);

  auto deleteButton = new QPushButton("Удалить", this);
  layout->addWidget(deleteButton, layout->rowCount(), 0, 1, -1);

  connect(addButton, &QPushButton::clicked,
          this, [this, srcBGProxyModel, lineEdit]() {
    bool ok;
    auto macro_number = QString::number(lineEdit->text().toInt(&ok));
    if (ok && listWidget->findItems("Macro " + macro_number,
                                    Qt::MatchFlag::MatchExactly).isEmpty()) {
      listWidget->addItem("Macro " + macro_number);
      srcBGProxyModel->updateOptionsButtonText();
      srcBGProxyModel->invalidate();
    }
  });

  connect(deleteButton, &QPushButton::clicked, this, [this, srcBGProxyModel]() {
    for (auto&& item : listWidget->selectedItems()) {
      delete listWidget->takeItem(listWidget->row(item));
      srcBGProxyModel->updateOptionsButtonText();
      srcBGProxyModel->invalidate();
    }
  });
}

void SrcBGProxyModel::updateOptionsButtonText() {
  QStringList macros_list;
  for (const auto& item
       : optionsDialog->listWidget->findItems("",
                                              Qt::MatchFlag::MatchContains)) {
    auto text = item->text();
    if (text.startsWith("Macro ")) {
      macros_list += text.mid(QString("Macro ").length());
    }
  }
  QString button_text = "Допустимые макросы";
  if (!macros_list.isEmpty()) {
    button_text += " (" + macros_list.join(", ") + ")";
  }
  if (button_text.length() > 40) {
    button_text.truncate(button_text.lastIndexOf(',', 40));
    button_text += "...)";
  }
  optionsButton->setText(button_text);
}

void SrcBGProxyModel::DataModel::clear() {
  bg_errors.clear();
}
