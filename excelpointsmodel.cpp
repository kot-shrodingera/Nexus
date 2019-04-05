#include "excelpointsmodel.h"

#include <cmath>

#include <xlsxdocument.h>

#include <QCheckBox>
#include <QDebug>
#include <QGridLayout>
#include <QPushButton>
#include <QScrollArea>

#include "globalsettings.h"
#include <QJsonArray>

#include "threadrunner.h"

ExcelPointsModel::ExcelPointsModel(QObject *parent)
  : QAbstractTableModel(parent)
{

}

int ExcelPointsModel::rowCount([[maybe_unused]] const QModelIndex &parent) const
{
  return points_list.size();
}

int ExcelPointsModel::columnCount(
    [[maybe_unused]] const QModelIndex &parent) const
{
  return using_headers_list.size();
}

QVariant ExcelPointsModel::headerData(int section,
                                      Qt::Orientation orientation,
                                      int role) const
{
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
    if (section < using_headers_list.size()) {
      return using_headers_list[section];
    }
  }
  return QAbstractTableModel::headerData(section, orientation, role);
}

QVariant ExcelPointsModel::data(const QModelIndex &index, int role) const
{
  if (role == Qt::DisplayRole) {
    auto row = index.row();
    auto col = index.column();
//    if (row >= points_list.size() || col >= points_list[row].size()) {
//      qDebug() << row << col;
//      qFatal("Range error");
//    }
    return points_list[row][col];
  }
  return QVariant();
}

void ExcelPointsModel::loadExcel(const QString &path)
{
  qDebug() << "loadExcel";
  emit updateStatus("Загрузка Excel-файла. Подождите...");
//  QXlsx::Document* doc = nullptr;
//  ThreadRunner::ThreadRunner([this,  &path, doc]() mutable {
//  qDebug() << "0";
//  QThread::sleep(1);
//  qDebug() << "1";
//  QThread::sleep(1);
//  qDebug() << "2";
//  QThread::sleep(1);
//  qDebug() << "3";
//  QThread::sleep(1);
//  qDebug() << "4";
//  QThread::sleep(1);
//  qDebug() << "5";
  auto doc = new QXlsx::Document(path);
  qDebug() << "loadedExcel";
  if (doc->selectSheet("Общая_база_данных")
      || doc->selectSheet("Общая база данных")) {
    beginResetModel();
    auto worksheet = doc->currentWorksheet();
    QMap<int, QMap<int, QPair<int, int>>> merged_cells_headers;
    for (const auto& cell_range : worksheet->mergedCells()) {
      for (int row = cell_range.firstRow();
           row <= cell_range.lastRow(); ++row) {
        for (int col = cell_range.firstColumn();
             col <= cell_range.lastColumn(); ++col) {
          if (row != cell_range.firstRow() || col != cell_range.firstColumn()) {
            merged_cells_headers[row][col]
                = {cell_range.firstRow(), cell_range.firstColumn()};
          }
        }
      }
    }
    /*QList<QString> headers_list*/;
    emit updateStatus("Считывание заголовков. Подождите...");
    for (int i = 1; i <= doc->dimension().lastColumn(); ++i) {
      QXlsx::Cell* header_cell;
      if (merged_cells_headers.contains(2)
          && merged_cells_headers[2].contains(i)) {
        header_cell = worksheet->cellAt(
              merged_cells_headers[2][i].first,
              merged_cells_headers[2][i].second);
      } else {
        header_cell = worksheet->cellAt(2, i);
      }
      if (header_cell) {
        auto header_value = header_cell->value();
//        if (!header_value.isNull())
          full_headers_list.append(header_value.toString());
      }
    }
    emit updateStatus("Считывание заголовков. Подождите... Завершено");
//    HeadersChooser(full_headers_list, using_headers_list).exec();
    emit requestHeadersChooser();
    emit updateStatus("Считывание данных. Подождите...");
    int row_count = worksheet->dimension().lastRow();
    for (int row = 5; row <= row_count; ++row) {
      auto kks_cell = worksheet->cellAt(row, 5);
      if (kks_cell
          && !kks_cell->value().isNull()
          && !kks_cell->format().fontStrikeOut()) {
        points_list.append(QVector<QString>());
        auto& current_point = points_list.last();
        for (int col = 1; col <= full_headers_list.size(); ++col) {
          if (using_headers_list.contains(full_headers_list[col - 1])) {
            auto cell = worksheet->cellAt(row, col);
            QString value;
            if (cell) {
              auto cell_value = cell->value();
              if (!cell_value.isNull()) {
                value = cell->value().toString();
              }
            }
            current_point.append(value);
          }
        }
      }
      updateProgress(std::lround(100.0 * row /row_count));
    }
    emit updateStatus("Считывание данных. Подождите... Завершено");
    endResetModel();
  } else {
    qFatal("Нужный лист не существует");
  }
//  });
  qDebug() << "ExcelLoaded";
}

void ExcelPointsModel::openHeadersChooser()
{
  HeadersChooser(full_headers_list, using_headers_list).exec();
  emit closedHeadersChooser();
}

void ExcelPointsModel::clear()
{
  beginResetModel();
  using_headers_list.clear();
  full_headers_list.clear();
  points_list.clear();
  endResetModel();
}

ExcelPointsModel::HeadersChooser::HeadersChooser
(const QList<QString> &full_headers_list, QList<QString>& using_headers_list)
  : QDialog()
{
  setWindowTitle("Выбор столбцов");
  setMinimumSize(100, 100);
  auto scrollArea = new QScrollArea(this);
  scrollArea->setWidgetResizable(true);
  scrollArea->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
  scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  auto scrollWidget = new QWidget(this);
  scrollArea->setWidget(scrollWidget);
  auto scrollLayout = new QGridLayout(scrollWidget);
//  QList<QCheckBox*> checkBox_list;
  QStringList default_headers;
  for (auto header : global_settings["ExcelDBHeaders"].toArray()) {
    default_headers.append(header.toString());
  }
  for(int i = 0; i < full_headers_list.size(); ++i) {
    auto checkBox = new QCheckBox(full_headers_list[i], this);
    if (default_headers.contains(full_headers_list[i]))
      checkBox->setChecked(true);
    checkBox_list.append(checkBox);
    scrollLayout->addWidget(checkBox, scrollLayout->rowCount(), 0);
  }
  auto layout = new QGridLayout(this);
  layout->addWidget(scrollArea, 0, 0);
  auto applyButton = new QPushButton("Применить", this);
  layout->addWidget(applyButton, layout->rowCount(), 0);
  connect(applyButton, &QPushButton::clicked,
          this, [this, &full_headers_list, &using_headers_list] {
    for (int i = 0; i < checkBox_list.size(); ++i) {
      if (checkBox_list[i]->isChecked()) {
        using_headers_list.append(full_headers_list[i]);
      }
    }
    close();
  });
}
