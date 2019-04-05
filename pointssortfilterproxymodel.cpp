#include "pointssortfilterproxymodel.h"

#include <QDebug>

#include "pointstablemodel.h"

PointsSortFilterProxyModel::PointsSortFilterProxyModel(QObject* parent)
    : QSortFilterProxyModel(parent) {
  sort(0);
}

PointsSortFilterProxyModel::PointsSortFilterProxyModel(
        PointsSortFilterProxyModel* model) : QSortFilterProxyModel() {
  setSourceModel(model->sourceModel());
  sort_columns = model->sort_columns;
  setFilterRegExp(model->filterRegExp());
  sort(0);
}

QVariant PointsSortFilterProxyModel::data(const QModelIndex& index,
                                          int role) const {
  if (role == Qt::BackgroundColorRole) {
    auto model = static_cast<PointsTableModel*>(sourceModel());
    auto row = mapToSource(index).row();
    auto column = mapToSource(index).column();
    auto color = model->getCellColor(row, column, mode_);
    if (color.isValid()) {
      return color;
    } else {
      return QSortFilterProxyModel::data(index, role);
    }
  } else {
    return QSortFilterProxyModel::data(index, role);
  }

}

bool PointsSortFilterProxyModel::filterAcceptsRow(
        int source_row,
        [[maybe_unused]] const QModelIndex& source_parent) const {
  bool kks_filter = true;
  if (!filterRegExp().isEmpty()) {
    auto model = sourceModel();
    auto kks = model->data(model->index(source_row, 0)).toString();
    kks_filter = filterRegExp().exactMatch(kks);
  }
  if (kks_filter) {
    if (mode_ != 0) {
      auto model = static_cast<PointsTableModel*>(sourceModel());
      return model->filtering.hasError(
                  model->data(model->index(source_row, 0)).toString(),
                  PointsTableModel::filters[mode_].mode);
    } else {
      return true;
    }
  }
  return false;
}

bool PointsSortFilterProxyModel::filterAcceptsColumn(
        int source_column,
        [[maybe_unused]] const QModelIndex& source_parent) const {
  if (mode_ == 0) {
    return true;
  } else if (mode_ < PointsTableModel::filters.size()) {
    return PointsTableModel::filters[mode_].shown_parameters.contains(
                PointInfo::point_parameters[source_column]);
  } else {
    return false;
  }
}

QVariant PointsSortFilterProxyModel::headerData(int section,
                                                Qt::Orientation orientation,
                                                int role) const {
  if(role == Qt::DisplayRole) {
    if (orientation == Qt::Vertical) {
      return section + 1;
    } else {
      return QSortFilterProxyModel::headerData(section, orientation, role);
    }
  }
  return QVariant::Invalid;
}

int PointsSortFilterProxyModel::getFilterMode() const {
  return mode_;
}

void PointsSortFilterProxyModel::setFilterMode(const int& mode) {
  auto filter_wildcard = filterRegExp();
  setFilterWildcard("");
  mode_ = mode;
  if (mode == 5) {
    multiSort({
      parameterToColumnIndex(PointInfo::Parameter::DROP),
      parameterToColumnIndex(PointInfo::Parameter::IO_LOCATION),
      parameterToColumnIndex(PointInfo::Parameter::IO_CHANNEL)
    });
  } else if (!sort_columns.empty()) {
    sort_columns.clear();
    sort(0);
  }
  invalidateFilter();
  emit filterChanged();
  setFilterWildcard(filter_wildcard.pattern());
}

void PointsSortFilterProxyModel::multiSort(const QList<int>& columns) {
  sort_columns = columns;
  sort(sort_columns[0]);
}

bool PointsSortFilterProxyModel::lessThan(
        const QModelIndex& source_left,const QModelIndex& source_right) const {
  if (sort_columns.isEmpty()) {
    return QSortFilterProxyModel::lessThan(source_left, source_right);
  } else {
    for (const auto& column : sort_columns) {

      auto index_left = sourceModel()->index(source_left.row(), column);
      auto index_right = sourceModel()->index(source_right.row(), column);

      if (sourceModel()->data(index_left).toString()
              < sourceModel()->data(index_right).toString()) {
        return true;
      } else if (sourceModel()->data(index_left).toString()
                 > sourceModel()->data(index_right).toString()) {
        return false;
      }
    }
    return false;
  }
}

int PointsSortFilterProxyModel::parameterToColumnIndex(
        PointInfo::Parameter parameter) {
  return PointInfo::point_parameters.indexOf(parameter);
}
