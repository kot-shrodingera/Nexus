#pragma once

#include <QAbstractTableModel>

#include <QJsonObject>
#include <QColor>

#include "point.h"
#include "loader.h"

class PointsTableModel : public QAbstractTableModel {
  Q_OBJECT

public:
  PointsTableModel(QObject* parent = nullptr);

  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  int columnCount(const QModelIndex& parent = QModelIndex()) const override;
  QVariant data(const QModelIndex& index,
                int role = Qt::DisplayRole) const override;
  QVariant headerData(int section,
                      Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const override;

  QVector<QHash<PointInfo::Parameter, QString>> loadDbidRootItem(
          Loader::DbidTreeItem* dbidRootItem);
  void loadPoints(
          const QVector<QHash<PointInfo::Parameter, QString>>& container);

  void clear();

  struct Drop_info {
    QMap<QString, QString> module_soe_input_info;
    QMap<QString, QString> task_period_info;
  };

  QMap<QString, Drop_info> drop_info;

  enum class FilterMode {
    ALL,
    NOT_IN_SRC_XML,
    NOT_IN_DBID,
    SCALE_ERRORS,
    LIMITS_ERRORS,
    SINGLE_MODULE_MULTITASK_ERRORS,
    CHARACTERISTICS_ERRORS,
    ANCILLARY_ERRORS,
    SCANGROUP_BROADCAST_FREQUENCY_MISSMATCH,
    BROADCAST_FREQUENCY_TASK_UPDATETIME_MISSMATCH,
    LIMITS_PRIORITY_ERRORS,
    SOE_INPUT_ERRORS
  };

  struct FilterInfo {
    FilterMode mode;
    QString description;
    QString details;
    QList<PointInfo::Parameter> shown_parameters;
    std::function<bool (bool dbid, bool src, bool xml, bool ophxml)> enabled;
  };

  static const QList<FilterInfo> filters;

  class Filtering {
  public:
    enum class InfoType {
      WARNING, ERROR
    };

    void addErrorInfo(const QString& kks, FilterMode mode, const QString& info);
    void addErrorColor(const QString& kks,
                       FilterMode mode,
                       PointInfo::Parameter parameter,
                       InfoType filter_type = InfoType::ERROR);
    bool hasError(const QString& kks, FilterMode mode) const;
    QStringList getErrorInfo(const QString& kks, FilterMode mode) const;
    QColor getErrorColor(const QString& kks,
                         FilterMode mode,
                         PointInfo::Parameter parameter) const;
    void clear(FilterMode filter_mode);
    void clear();
  private:
    QHash<QString, QHash<FilterMode, QStringList>> error_info;
    QHash<QString, QHash<FilterMode, QHash<PointInfo::Parameter, InfoType>>>
    color_info;
  } filtering;

  QColor getCellColor(int row, int column, int filter_mode) const;

  void updateFiltering(QList<FilterMode> filter_modes = {});

  struct CharacteristicsFilter {
    QString mask = "?-------";
    bool compare_equal = false;
  } characteristicsFilter;

  struct AncillaryFilter {
    QMap<PointInfo::Parameter, QPair<bool, PointInfo::Parameter>> order = {
      {PointInfo::Parameter::DROP, {true, PointInfo::Parameter::ANC_5}},
      {PointInfo::Parameter::IO_LOCATION, {true, PointInfo::Parameter::ANC_6}},
      {PointInfo::Parameter::IO_CHANNEL, {true, PointInfo::Parameter::ANC_7}}
    };
  } ancillaryFilter;

  struct AlarmsFilter {
    struct structure {
      PointInfo::Parameter parameter;
      bool enabled;
      int value;
    };

    QMap<PointInfo::Parameter, QPair<bool, int>> priority = {
      {PointInfo::Parameter::LOW_ALARM_PRIORITY_1, {true, 2}},
      {PointInfo::Parameter::LOW_ALARM_PRIORITY_2, {true, 2}},
      {PointInfo::Parameter::LOW_ALARM_PRIORITY_3, {true, 1}},
      {PointInfo::Parameter::LOW_ALARM_PRIORITY_4, {true, 1}},
      {PointInfo::Parameter::LOW_ALARM_PRIORITY_USER, {true, 8}},
      {PointInfo::Parameter::HIGH_ALARM_PRIORITY_1, {true, 2}},
      {PointInfo::Parameter::HIGH_ALARM_PRIORITY_2, {true, 2}},
      {PointInfo::Parameter::HIGH_ALARM_PRIORITY_3, {true, 1}},
      {PointInfo::Parameter::HIGH_ALARM_PRIORITY_4, {true, 1}},
      {PointInfo::Parameter::HIGH_ALARM_PRIORITY_USER, {true, 8}}
    };
  } alarmsFilter;

signals:
  void updateStatus(const QString& status, int timeout = 0);
  void updateProgress(int percent);
  void filteringUpdated();

private:
  QVector<Point*> points;
  QHash<QString, int> point_index_by_name;
  QMap<QString, QMap<QString, QStringList>> tasks_in_drop_and_location;



};

inline uint qHash(const PointsTableModel::FilterMode& mode, uint seed = 0) {
    return qHash(static_cast<uint>(mode), seed);
};
