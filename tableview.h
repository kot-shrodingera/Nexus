#pragma once

#include <QTableView>
#include <QKeyEvent>

class TableView : public QTableView {
  Q_OBJECT
public:
  TableView(QAbstractItemModel* model, QWidget* parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent* event) override;
};
