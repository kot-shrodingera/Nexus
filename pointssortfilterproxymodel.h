#pragma once

#include <QSortFilterProxyModel>

#include <QDebug>

#include "point.h"

class PointsSortFilterProxyModel : public QSortFilterProxyModel {
  Q_OBJECT
public:
  PointsSortFilterProxyModel(QObject* parent = nullptr);
  PointsSortFilterProxyModel(PointsSortFilterProxyModel* model);
  QVariant data(const QModelIndex& index,
                int role = Qt::DisplayRole) const override;
  QVariant headerData(int section,
                      Qt::Orientation orientation,
                      int role) const override;
  int getFilterMode() const;
  void setFilterMode(const int& mode);
  void multiSort(const QList<int>& columns);

  void invalidate() {
    qDebug() << "Invalidating";
    QSortFilterProxyModel::invalidate();
    qDebug() << "Invalidated";
  }

protected:
  bool filterAcceptsRow(int source_row,
                        const QModelIndex &source_parent) const override;
  bool filterAcceptsColumn(int source_column,
                           const QModelIndex &source_parent) const override;
  bool lessThan(const QModelIndex& source_left,
                const QModelIndex& source_right) const override;

signals:
  void filterChanged();

private:
  int mode_ = 0;
  QList<int> sort_columns;

  int parameterToColumnIndex(PointInfo::Parameter parameter);
};

