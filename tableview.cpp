#include "tableview.h"

#include <QHeaderView>

#include <QApplication>
#include <QClipboard>

TableView::TableView(QAbstractItemModel* model, QWidget* parent)
    : QTableView(parent) {
  setModel(model);
  horizontalHeader()->setResizeContentsPrecision(-1);
//  horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
//  connect(this->model(), &QAbstractTableModel::modelReset,
//          this, [this]() {
//   horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
//  });
}

void TableView::keyPressEvent(QKeyEvent* event) {
  if(!selectedIndexes().isEmpty()){
    if(event->matches(QKeySequence::Copy)) {
      QStringList contents;
      for (const auto& range : selectionModel()->selection()) {
        for (auto i = range.top(); i <= range.bottom(); ++i) {
          QStringList row_contents;
          for (auto j = range.left(); j <= range.right(); ++j) {
            row_contents << model()->index(i, j).data().toString();
          }
          contents.append(row_contents.join('\t'));
        }
      }
      QApplication::clipboard()->setText(contents.join('\n'));
    } else {
      QTableView::keyPressEvent(event);
    }
  }
}
