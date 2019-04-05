#include "pointstablemodel.h"

#include <cmath>

#include <QJsonObject>
#include <QRegularExpression>

#include <QDebug>

using P = PointInfo::Parameter;

PointsTableModel::PointsTableModel(QObject* parent)
    : QAbstractTableModel(parent) {}

const QList<PointsTableModel::FilterInfo> PointsTableModel::filters = {
  {FilterMode::ALL,
   "Все точки",
   "Отобразить все точки",
   {},
   []([[maybe_unused]] bool dbid,
      [[maybe_unused]] bool src,
      [[maybe_unused]] bool xml,
      [[maybe_unused]] bool ophxml) -> bool {
     return true;
   }},
  {FilterMode::NOT_IN_SRC_XML,
   "Отсутствующие в SRC, XML",
   "Точки, которые присутствуют в DBID, но отсутствуют в SRC и XML-файлах\n"
   "(OPHXML не учитывается)",
   {P::KKS},
   [](bool dbid,
      bool src,
      bool xml,
      [[maybe_unused]] bool ophxml) -> bool {
     return (dbid && src) || (dbid && xml);
   }},
  {FilterMode::NOT_IN_DBID,
   "Отсутствующие в DBID",
   "Точки, которые отсутствуют в DBID, но присутствуют в SRC и XML-файлах\n"
   "(OPHXML не учитывается)",
   {P::KKS, P::APPEAR_IN_FILES},
   [](bool dbid,
      bool src,
      bool xml,
      [[maybe_unused]] bool ophxml) -> bool {
     return (dbid && src) || (dbid && xml);
   }},
  {FilterMode::SCALE_ERRORS,
   "Ошибки шкал",
   "Точки, у которых не идентичны\n"
   "OPERATING_RANGE_(LOW|HIGH)\n"
   "(LOW|HIGH)_ENGINEERING_LIMIT\n"
   "(MINIMUM|MAXIMUM)_SCALE",
   {P::KKS, P::OPERATING_RANGE_LOW, P::OPERATING_RANGE_HIGH,
    P::LOW_ENGINEERING_LIMIT, P::HIGH_ENGINEERING_LIMIT,
    P::MINIMUM_SCALE, P::MAXIMUM_SCALE},
   [](bool dbid,
      [[maybe_unused]] bool src,
      [[maybe_unused]] bool xml,
      [[maybe_unused]] bool ophxml) -> bool {
     return dbid;
   }},
  {FilterMode::LIMITS_ERRORS,
   "Ошибки уставок",
   "Точки, у которых есть уставки\n"
   "(LOW|HIGH)_ALARM_LIMIT_TYPE_(1|2|3|4) = \"V\"\n"
   "значение которых\n"
   "(LOW|HIGH)_ALARM_LIMIT_VALUE_(1|2|3|4)\n"
   "выходят за границы шкалы\n"
   "[OPERATING_RANGE_LOW, OPERATING_RANGE_HIGH]",
   {P::KKS, P::OPERATING_RANGE_LOW, P::OPERATING_RANGE_HIGH,
    P::LOW_ALARM_LIMIT_1_VALUE, P::LOW_ALARM_LIMIT_2_VALUE,
    P::LOW_ALARM_LIMIT_3_VALUE, P::LOW_ALARM_LIMIT_4_VALUE,
    P::HIGH_ALARM_LIMIT_1_VALUE, P::HIGH_ALARM_LIMIT_2_VALUE,
    P::HIGH_ALARM_LIMIT_3_VALUE, P::HIGH_ALARM_LIMIT_4_VALUE},
   [](bool dbid,
      [[maybe_unused]] bool src,
      [[maybe_unused]] bool xml,
      [[maybe_unused]] bool ophxml) -> bool {
     return dbid;
   }},
  {FilterMode::SINGLE_MODULE_MULTITASK_ERRORS,
   "Точки одного модуля в разных тасках",
   "Все точки тех модулей, в которых присутствуют точки из хотя бы 2 разных тасков",
   {P::KKS,P::DROP, P::IO_LOCATION, P::IO_CHANNEL, P::IO_TASK_INDEX},
   [](bool dbid,
      [[maybe_unused]] bool src,
      [[maybe_unused]] bool xml,
      [[maybe_unused]] bool ophxml) -> bool {
     return dbid;
   }},
  {FilterMode::CHARACTERISTICS_ERRORS,
   "Характеристика",
   "Точки, у которых характеристика не подходит по маске\n"
   "Точки с пустой характеристикой не учитываются (не отображаются)",
   {P::KKS, P::CHARACTERISTICS},
   [](bool dbid,
      [[maybe_unused]] bool src,
      [[maybe_unused]] bool xml,
      [[maybe_unused]] bool ophxml) -> bool {
     return dbid;
   }},
  {FilterMode::ANCILLARY_ERRORS,
   "Корректность Ancillary",
   "Точки, у которых значения полей\n"
   "DROP - IO_LOCATION - IO_CHANNEL\n"
   "не соответствуют указанным полям ANC\n"
   "По умолчанию\n"
   "ANC_5 - ANC_6 - ANC_7\n"
   "Модульные точки (MODULE_POINT) не учитываются",
   {P::KKS, P::DROP, P::IO_LOCATION, P::IO_CHANNEL,
    P::ANC_1, P::ANC_2, P::ANC_3, P::ANC_4, P::ANC_5, P::ANC_6, P::ANC_7},
   [](bool dbid,
      [[maybe_unused]] bool src,
      [[maybe_unused]] bool xml,
      [[maybe_unused]] bool ophxml) -> bool {
     return dbid;
   }},
  {FilterMode::SCANGROUP_BROADCAST_FREQUENCY_MISSMATCH,
   "Несоответствие частоты передачи и архивирования",
   "Точки, у которых частота передачи, указанная в DBID\n"
   "BROADCAST_FREQUENCY\n"
   "не соответствует частоте архивирования, указанной в OPHXML\n"
   "SCANGROUP_FREQUENCY\n"
   "BROADCAST_FREQUENCY == \"A\" игнорируется",
   {P::KKS, P::SCANGROUP_FREQUENCY, P::BROADCAST_FREQUENCY},
   [](bool dbid,
      [[maybe_unused]] bool src,
      [[maybe_unused]] bool xml,
      bool ophxml) -> bool {
     return dbid && ophxml;
   }},
  {FilterMode::BROADCAST_FREQUENCY_TASK_UPDATETIME_MISSMATCH,
   "Несоответствие частоты передачи и обновления значения",
   "Точки, у которых частота передачи, указанная в DBID\n"
   "BROADCAST_FREQUENCY\nне соответствует частоте обовления, определяемой, OPHXML\n"
   "IO_TASK_INDEX\n"
   "!Пока что таск 1 считается быстрым, таск 2 - медленным и всё",
   {P::KKS, P::BROADCAST_FREQUENCY, P::IO_TASK_INDEX},
   [](bool dbid,
      [[maybe_unused]] bool src,
      [[maybe_unused]] bool xml,
      [[maybe_unused]] bool ophxml) -> bool {
     return dbid;
   }},
  {FilterMode::LIMITS_PRIORITY_ERRORS,
   "Приоритеты уставок",
   "Точки, у которых приоритеты алармов не равны указанным",
   {P::KKS, P::LOW_ALARM_PRIORITY_1, P::LOW_ALARM_PRIORITY_2,
    P::LOW_ALARM_PRIORITY_3, P::LOW_ALARM_PRIORITY_4,
    P::LOW_ALARM_PRIORITY_USER,
    P::HIGH_ALARM_PRIORITY_1, P::HIGH_ALARM_PRIORITY_2,
    P::HIGH_ALARM_PRIORITY_3, P::HIGH_ALARM_PRIORITY_4,
    P::HIGH_ALARM_PRIORITY_USER},
   [](bool dbid,
      [[maybe_unused]] bool src,
      [[maybe_unused]] bool xml,
      [[maybe_unused]] bool ophxml) -> bool {
     return dbid;
   }},
  {FilterMode::SOE_INPUT_ERRORS,
   "Ошибки SOE Input",
   "Точки, у которых параметры\n"
   "SOE_POINT и SOE_ENABLED\n"
   "не соответствуют значению, определённому в описании модуля\n"
   "EVENT_TAGGING_ENABLED",
   {P::KKS, P::DROP, P::IO_LOCATION, P::IO_CHANNEL, P::IO_TASK_INDEX,
    P::SOE_POINT, P::SOE_ENABLED},
   [](bool dbid,
      [[maybe_unused]] bool src,
      [[maybe_unused]] bool xml,
      [[maybe_unused]] bool ophxml) -> bool {
     return dbid;
   }},
};

int PointsTableModel::rowCount(
        [[maybe_unused]] const QModelIndex& parent) const {
  return points.size();
}

int PointsTableModel::columnCount(
        [[maybe_unused]] const QModelIndex& parent) const {
  return PointInfo::point_parameters.size();
}

QVariant PointsTableModel::data(const QModelIndex& index, int role) const {
  if (role == Qt::DisplayRole
      || role == Qt::ForegroundRole) {
    auto point = (*points[index.row()]);
    auto parameter = headerData(index.column(),
                                Qt::Horizontal,
                                Qt::DisplayRole).toString();
    auto value = point[PointInfo::parameterFromString(parameter)];
//    if (value.isNull()) {
//      if (role == Qt::DisplayRole) {
//        return "Null";
//      } else {
//        return QBrush(QColor(Qt::GlobalColor::lightGray));
//      }
//    }
    if (role == Qt::DisplayRole) {
      return value;
    } else if (role == Qt::ForegroundRole) {
      if (value.isNull()) {
        return QBrush(QColor(Qt::GlobalColor::lightGray));
      }
    }
  }
  return QVariant();
}

QVariant PointsTableModel::headerData(int section,
                                      Qt::Orientation orientation,
                                      int role) const {
  if(role == Qt::DisplayRole) {
    if(orientation == Qt::Horizontal) {
      if (section < columnCount()) {
        return PointInfo::toString(PointInfo::point_parameters[section]);
      }
    } else {
      return QAbstractTableModel::headerData(section, orientation, role);
    }
  }

  return QVariant();
}

QVector<QHash<PointInfo::Parameter, QString>> PointsTableModel::loadDbidRootItem(
        Loader::DbidTreeItem* dbidRootItem) {
  emit updateStatus("Генерация данных о точках и модулях. Подождите...");
  std::function<Loader::DbidTreeItem*
          (Loader::DbidTreeItem*, const QPair<QString, QString>&)> findItem =
      [&findItem](Loader::DbidTreeItem* root_item,
          const QPair<QString, QString>& keys) -> Loader::DbidTreeItem* {
    if (root_item == nullptr)
      return nullptr;
    if ((keys.first.isEmpty() && keys.second.isEmpty())
       || (root_item->parameter.startsWith(keys.first)
           && root_item->value.startsWith(keys.second)))
      return root_item;
    for (auto child : root_item->children) {
      auto result = findItem(child, keys);
      if (result != nullptr)
        return result;
    }
    return nullptr;
  };
  std::function<QList<Loader::DbidTreeItem*>
          (Loader::DbidTreeItem*, const QPair<QString, QString>&)> findItems =
      [](Loader::DbidTreeItem* root_item,
          const QPair<QString, QString>& keys) -> QList<Loader::DbidTreeItem*> {
    if (root_item == nullptr || root_item->children.isEmpty())
      return {};
    QList<Loader::DbidTreeItem*> result;
    for (auto child : root_item->children) {
      if (child->parameter.startsWith(keys.first)
              && child->value.startsWith(keys.second))
        result.append(child);
    }
    return result;
  };
  auto unit_item = findItem(dbidRootItem, {"UNIT", "Unit"});

  QVector<QHash<PointInfo::Parameter, QString>> container;

  if (unit_item != nullptr) {
    auto drop_items = findItems(unit_item, {"", "Drop"});
    for (const auto& drop_item : drop_items) {
      auto io_device_item = findItem(drop_item,
                                     {"I/O Device 0 IOIC", "IoDevice"});
      auto io_interface_items = findItems(io_device_item,
                                          {"I/O Interface ", "IoDevice"});
      for (auto io_interface_item : io_interface_items) {
        auto io_interface_number =
                io_interface_item->parameter.mid(
                    QString("I/O Interface ").length(), 1);
        if (io_interface_number == "1" || io_interface_number == "2") {
          auto branch_items = findItems(io_interface_item,
                                        {"Branch ", "Branch"});
          for (auto branch_item : branch_items) {
            auto slot_items = findItems(branch_item,
                                        {"Slot ", "RSlot"});
            for (auto slot_item : slot_items) {
              auto module_items = findItems(slot_item,
                                            {"", "RModule"});
              for (auto module_item : module_items) {
                auto drop_number =
                        drop_item->parameter.mid(QString("DROP").length(), 2);
                auto branch_number =
                        branch_item->parameter.mid(QString("Branch ").length(),
                                                   1);
                auto slot_number =
                        slot_item->parameter.mid(QString("Slot ").length(), 1);
                auto module_point_name_item = findItem(module_item,
                                                       {"POINT_NAME", ""});
                if (module_point_name_item != nullptr) {
                  if (module_point_name_item->value
                          != QString("MP_") + drop_number + "_"
                             + io_interface_number + "_" + branch_number
                             + "_" + slot_number) {
                    qFatal(qPrintable(module_point_name_item->value
                                      + " does not refer to real location"));
                  }
                } else {
                  qFatal(qPrintable(module_item->parameter
                                    + " does not have POINT_NAME"));
                }
                auto module_event_tagging_enable_item =
                        findItem(module_item, {"EVENT_TAGGING_ENABLE", ""});
                if (module_event_tagging_enable_item != nullptr) {
                  drop_info[drop_item->parameter]
                          .module_soe_input_info[io_interface_number
                                                 + "." + branch_number
                                                 + "." + slot_number] =
                          module_event_tagging_enable_item->value;
                }
              }
            }
          }
        }
      }
      auto controller_item = findItem(drop_item,
                                      {"Controller", "ConfigController"});
      auto control_task_items = findItems(controller_item,
                                          {"Control Task ", "ConfigDPUCtrlTask"});
      for (auto control_task_item : control_task_items) {
        auto control_task_number =
                control_task_item->parameter.mid(QString("Control Task ").length(),
                                                 1);
        auto periodtime_item = findItem(control_task_item,
                                        {"periodtime", ""});
        if (periodtime_item != nullptr)
          drop_info[drop_item->parameter].task_period_info[control_task_number] =
                  periodtime_item->value;
      }
      for (auto point_item : drop_item->children) {
        if (PointInfo::point_types.contains(
                    PointInfo::typeFromString(point_item->value))) {
          QHash<PointInfo::Parameter, QString> parameters;
          parameters[PointInfo::Parameter::KKS] = point_item->parameter;
          parameters[PointInfo::Parameter::TYPE] = point_item->value;
          parameters[PointInfo::Parameter::APPEAR_IN_FILES] = "DBID.imp";
          parameters[PointInfo::Parameter::DROP] = drop_item->parameter;
          for (auto parameter_item : point_item->children) {
            auto parameter =
                    PointInfo::parameterFromString(parameter_item->parameter);
            if (PointInfo::point_parameters.contains(parameter)) {
              parameters[parameter] = parameter_item->value;
            }
          }
          container.append(parameters);
        }
      }
    }
  }
//  loadPoints(container);
  emit updateStatus("Генерация данных о точках и модулях. Подождите... Завершено");
  return container;
}

QColor PointsTableModel::getCellColor(int row,
                                      int column,
                                      int filter_mode) const {
  auto kks = (*points[row])[P::KKS];
  return filtering.getErrorColor(kks, filters[filter_mode].mode,
                                 PointInfo::point_parameters[column]);
}

void PointsTableModel::loadPoints(
        const QVector<QHash<PointInfo::Parameter, QString>>& container) {
  emit updateStatus("Загрузка данных о всех точках в модель. Подождите...");
  if (!container.isEmpty()) {
    beginResetModel();
    int i = 0;
    for (const auto& parameters : container) {
      const auto& kks = parameters[P::KKS];
      if (!kks.isEmpty()) {
        if (!point_index_by_name.contains(kks)) {
          point_index_by_name.insert(kks, points.size());
          if (parameters[P::TYPE]
                  != PointInfo::toString(PointInfo::Type::ModulePoint)) {
            const auto& drop = parameters[P::DROP];
            const auto& io_location = parameters[P::IO_LOCATION];
            const auto& task = parameters[P::IO_TASK_INDEX];
            if (!drop.isEmpty() && !io_location.isEmpty()
                && !tasks_in_drop_and_location[drop][io_location].contains(task)) {
              tasks_in_drop_and_location[drop][io_location].append(task);
            }
          }
          points.append(new Point(parameters));
        } else {
          points[point_index_by_name[kks]]->addParameters(parameters);
        }
      }
      emit updateProgress(std::lround(100.0 * ++i / container.size()));
    }
    emit updateStatus("Загрузка данных о всех точках в модель. Подождите... Завершено");
    updateFiltering();
    endResetModel();
  }
}

void PointsTableModel::clear() {
  beginResetModel();
  filtering.clear();
  drop_info.clear();
  tasks_in_drop_and_location.clear();
  point_index_by_name.clear();
  points.clear();
  endResetModel();
}

void PointsTableModel::Filtering::addErrorInfo(const QString& kks,
                                               FilterMode mode,
                                               const QString& info) {
  error_info[kks][mode].append(info);
}

void PointsTableModel::Filtering::addErrorColor(
        const QString& kks,
        FilterMode mode,
        PointInfo::Parameter parameter,
        PointsTableModel::Filtering::InfoType filter_type) {
  auto& filter_type_ = color_info[kks][mode][parameter];
  if (filter_type != filter_type_ && filter_type_ != InfoType::ERROR) {
    filter_type_ = filter_type;
  }
}

bool PointsTableModel::Filtering::hasError(const QString& kks,
                                           FilterMode mode) const {
  return error_info[kks].contains(mode);
}

QColor PointsTableModel::Filtering::getErrorColor(
        const QString& kks,
        FilterMode mode,
        PointInfo::Parameter parameter) const {
  if (color_info.contains(kks)
          && color_info[kks].contains(mode)
          && color_info[kks][mode].contains(parameter)) {
    if (color_info[kks][mode][parameter] == InfoType::ERROR) {
      return QColor(Qt::GlobalColor::red);
    } else if (color_info[kks][mode][parameter] == InfoType::WARNING) {
      return QColor(Qt::GlobalColor::gray);
    }
  }
  return QColor::Invalid;
}

QStringList PointsTableModel::Filtering::getErrorInfo(const QString& kks,
                                                      FilterMode mode) const {
  return error_info[kks][mode];
}

void PointsTableModel::Filtering::clear(PointsTableModel::FilterMode filter_mode) {
  for (auto& info : error_info) {
    info.remove(filter_mode);
  }
  for (auto& info : color_info) {
    info.remove(filter_mode);
  }
}

void PointsTableModel::Filtering::clear() {
  error_info.clear();
  color_info.clear();
}

void PointsTableModel::updateFiltering(
        QList<PointsTableModel::FilterMode> filter_modes) {
  if (filter_modes.empty()) {
    filtering.clear();
  } else {
    for (const auto& filter_mode : filter_modes) {
      filtering.clear(filter_mode);
    }
  }
  int count = 0;
  emit updateStatus("Фильтрация (проверка ошибок). Подождите...");
  for (auto pointer_to_point : points) {
    const auto& point = *pointer_to_point;
    auto kks = point[P::KKS];
    using FilterType = Filtering::InfoType;
    for (const auto& filter_info : filters) {
      auto filter_mode = filter_info.mode;
      auto filter_description = filter_info.description;
      if (filter_mode == FilterMode::NOT_IN_SRC_XML
              && (filter_modes.contains(filter_mode)
                  || filter_modes.isEmpty())) {
        if (!point.isInSRC() && !point.isInXML()) {
          filtering.addErrorInfo(
                      kks,
                     filter_mode,
                     "Точка присутствует в DBID, но отсутствует в других файлах");
        }
      } else if (filter_mode == FilterMode::NOT_IN_DBID
                 && (filter_modes.contains(filter_mode)
                     || filter_modes.isEmpty())) {
        if (!point.isInDBID()) {
          filtering.addErrorInfo(
                      kks,
                     filter_mode,
                     "Точка отсутствует в DBID, но присутствует в других файлах");
        }
      } else if (filter_mode == FilterMode::SCALE_ERRORS
                 && (filter_modes.contains(filter_mode)
                     || filter_modes.isEmpty())) {
        if ((point[P::OPERATING_RANGE_LOW] != point[P::LOW_ENGINEERING_LIMIT])
            || (point[P::OPERATING_RANGE_LOW] != point[P::MINIMUM_SCALE])
            || (point[P::OPERATING_RANGE_HIGH] != point[P::HIGH_ENGINEERING_LIMIT])
            || (point[P::OPERATING_RANGE_HIGH] != point[P::MAXIMUM_SCALE])) {
          bool empty = false;
          for (const auto& pair : QList<QPair<P, P>>({
              {P::OPERATING_RANGE_LOW, P::LOW_ENGINEERING_LIMIT},
              {P::OPERATING_RANGE_HIGH, P::HIGH_ENGINEERING_LIMIT},
              {P::OPERATING_RANGE_LOW, P::MINIMUM_SCALE},
              {P::OPERATING_RANGE_HIGH, P::MAXIMUM_SCALE},
              {P::LOW_ENGINEERING_LIMIT, P::MINIMUM_SCALE},
              {P::HIGH_ENGINEERING_LIMIT, P::MAXIMUM_SCALE}
            })) {
            auto first = point[pair.first];
            auto second = point[pair.second];
            if (first.isEmpty() || second.isEmpty())
              empty = true;
            if (!first.isEmpty() && !second.isEmpty() && (first != second)) {
              filtering.addErrorInfo(
                          kks,
                          filter_mode,
                          "Несоответствие значений "
                          + PointInfo::toString(pair.first)
                          + " и " + PointInfo::toString(pair.second));
              filtering.addErrorInfo(kks, filter_mode, first + " != " + second);
              filtering.addErrorColor(kks, filter_mode, pair.first);
              filtering.addErrorColor(kks, filter_mode, pair.second);
            }
          }
          if (empty) {
            filtering.addErrorInfo(kks,
                                   filter_mode,
                                   "Некоторые шкалы отсутствуют:");
            QList<P> error_parameters;
            for (auto limit : {
                 P::OPERATING_RANGE_LOW, P::OPERATING_RANGE_HIGH,
                 P::LOW_ENGINEERING_LIMIT, P::HIGH_ENGINEERING_LIMIT,
                 P::MINIMUM_SCALE, P::MAXIMUM_SCALE}) {
              if (point[limit].isEmpty()) {
                filtering.addErrorColor(kks,
                                        filter_mode,
                                        limit,
                                        FilterType::WARNING);
                error_parameters.append(limit);
              }
            }
            QStringList error_parameter_string;
            for (auto p : error_parameters) {
              error_parameter_string.append(PointInfo::toString(p));
              filtering.addErrorColor(kks, filter_mode, p, FilterType::WARNING);
            }
            filtering.addErrorInfo(kks,
                                   filter_mode,
                                   error_parameter_string.join(", "));
          }
        }
      } else if (filter_mode == FilterMode::LIMITS_ERRORS
                 && (filter_modes.contains(filter_mode)
                     || filter_modes.isEmpty())) {
        QList<P> missing_operating_ranges;
        for (auto [p_low_type, p_low_value, p_high_type, p_high_value] :
             QList<std::tuple<P, P, P, P>>{
        {P::LOW_ALARM_LIMIT_1_TYPE, P::LOW_ALARM_LIMIT_1_VALUE,
             P::HIGH_ALARM_LIMIT_1_TYPE, P::HIGH_ALARM_LIMIT_1_VALUE},
        {P::LOW_ALARM_LIMIT_2_TYPE, P::LOW_ALARM_LIMIT_2_VALUE,
             P::HIGH_ALARM_LIMIT_2_TYPE, P::HIGH_ALARM_LIMIT_2_VALUE},
        {P::LOW_ALARM_LIMIT_3_TYPE, P::LOW_ALARM_LIMIT_3_VALUE,
             P::HIGH_ALARM_LIMIT_3_TYPE, P::HIGH_ALARM_LIMIT_3_VALUE},
        {P::LOW_ALARM_LIMIT_4_TYPE, P::LOW_ALARM_LIMIT_4_VALUE,
             P::HIGH_ALARM_LIMIT_4_TYPE, P::HIGH_ALARM_LIMIT_4_VALUE}
      }) {
          auto low_type = point[p_low_type];
          auto low_value = point[p_low_value];
          auto high_type = point[p_high_type];
          auto high_value = point[p_high_value];
          for (auto [p_type, type, p_value, value] :
               QList<std::tuple<P, QString, P, QString>>{
          {p_low_type, low_type, p_low_value, low_value},
          {p_high_type, high_type, p_high_value, high_value}
        }) {
            if (type == "V" && value.isEmpty()) {
              filtering.addErrorInfo(kks,
                                     filter_mode,
                                     PointInfo::toString(p_type)
                                     + " = \"V\", но значение "
                                     + PointInfo::toString(p_value)
                                     + " отсутствует");
              filtering.addErrorColor(kks,
                                      filter_mode,
                                      p_type,
                                      FilterType::WARNING);
              filtering.addErrorColor(kks,
                                      filter_mode,
                                      p_value,
                                      FilterType::WARNING);
            } else if (type.isEmpty() && !value.isEmpty()) {
              filtering.addErrorInfo(kks,
                                     filter_mode,
                                     PointInfo::toString(p_type)
                                     + " отсутствует, но значение "
                                     + PointInfo::toString(p_value)
                                     + " задано");
              filtering.addErrorColor(kks,
                                      filter_mode,
                                      p_type,
                                      FilterType::WARNING);
              filtering.addErrorColor(kks,
                                      filter_mode,
                                      p_value,
                                      FilterType::WARNING);
            }
            if (!value.isEmpty()) {
              auto f_value = value.toFloat();
              if (point[P::OPERATING_RANGE_LOW].isEmpty()) {
                missing_operating_ranges.append(P::OPERATING_RANGE_LOW);
              } else if (f_value < point[P::OPERATING_RANGE_LOW].toFloat()) {
                filtering.addErrorInfo(
                            kks,
                           filter_mode,
                           PointInfo::toString(p_value)
                           + " ниже чем "
                           + PointInfo::toString(P::OPERATING_RANGE_LOW));
                filtering.addErrorInfo(kks,
                                       filter_mode,
                                       value + " < "
                                       + point[P::OPERATING_RANGE_LOW]);
                filtering.addErrorColor(kks, filter_mode, p_value);
              }
              if (point[P::OPERATING_RANGE_HIGH].isEmpty()) {
                missing_operating_ranges.append(P::OPERATING_RANGE_HIGH);
              } else if (f_value > point[P::OPERATING_RANGE_HIGH].toFloat()) {
                filtering.addErrorInfo(
                            kks,
                           filter_mode,
                           PointInfo::toString(p_value)
                           + " выше чем "
                           + PointInfo::toString(P::OPERATING_RANGE_HIGH));
                filtering.addErrorInfo(kks,
                                       filter_mode,
                                       value + " > "
                                       + point[P::OPERATING_RANGE_HIGH]);
                filtering.addErrorColor(kks, filter_mode, p_value);
              }
              if (value == low_value
                      && !high_value.isEmpty()
                      && f_value >= high_value.toFloat()) {
                filtering.addErrorInfo(kks,
                                       filter_mode,
                                       PointInfo::toString(p_low_value)
                                       + " выше или равно "
                                       + PointInfo::toString(p_high_value));
                filtering.addErrorInfo(kks,
                                       filter_mode,
                                       value + " >= " + high_value);
                filtering.addErrorColor(kks, filter_mode, p_value);
                filtering.addErrorColor(kks, filter_mode, p_high_value);
              }
            }
          }
        }
        for (auto range : missing_operating_ranges) {
          filtering.addErrorInfo(
                      kks,
                      filter_mode,
                      PointInfo::toString(range)
                      + " отсутствует, хотя установлена один или несколько уставок");
          filtering.addErrorColor(kks, filter_mode, range);
        }
      } else if (filter_mode == FilterMode::SINGLE_MODULE_MULTITASK_ERRORS
                 && (filter_modes.contains(filter_mode)
                     || filter_modes.isEmpty())) {
        if (point[P::TYPE]
                != PointInfo::toString(PointInfo::Type::ModulePoint)) {
          const auto& drop = point[P::DROP];
          const auto& io_location = point[P::IO_LOCATION];
          if (tasks_in_drop_and_location.contains(drop)
                  && tasks_in_drop_and_location[drop].contains(io_location)
              && tasks_in_drop_and_location[drop][io_location].size() > 1) {
            filtering.addErrorInfo(
                        kks,
                        filter_mode,
                        "Точка находится в модуле ("
                        + drop + " "
                        + io_location
                        + "), в котором находятся точки в разных тасках");
          }
        }
      } else if (filter_mode == FilterMode::CHARACTERISTICS_ERRORS
                 && (filter_modes.contains(filter_mode)
                     || filter_modes.isEmpty())) {
        if (!point[P::CHARACTERISTICS].isEmpty()) {
          QRegExp rx(characteristicsFilter.mask);
          rx.setPatternSyntax(QRegExp::Wildcard);
          if (!rx.isEmpty()
                  && (rx.exactMatch(point[P::CHARACTERISTICS])
                      == characteristicsFilter.compare_equal)) {
            filtering.addErrorInfo(
                        kks,
                        filter_mode,
                        "Характеристика ("
                        + point[P::CHARACTERISTICS]
                        + ") не соответствует маске ("
                        + characteristicsFilter.mask + ")");
          }
        }
      } else if (filter_mode == FilterMode::ANCILLARY_ERRORS
                 && (filter_modes.contains(filter_mode)
                     || filter_modes.isEmpty())) {
        if (point[P::TYPE] != PointInfo::toString(PointInfo::Type::ModulePoint)) {
          auto temp_kks = kks;
          if (temp_kks.endsWith("XQ02")) {
            if (temp_kks.at(temp_kks.length() - 5) == "_") {
              temp_kks.remove(temp_kks.length() - 5, 1);
            }
            temp_kks.replace(temp_kks.length() - 1, 1, "1");
          }
          if (!point_index_by_name.contains(temp_kks)) {
            temp_kks = kks;
          }
          const auto& temp_point = *points[point_index_by_name[temp_kks]];
          const auto& io_location = temp_point[P::IO_LOCATION];
          const auto& io_channel = temp_point[P::IO_CHANNEL];

          const auto& drop = point[P::DROP];
          const auto& anc_drop = point[ancillaryFilter.order[P::DROP].second];
          const auto& anc_io_location =
                  point[ancillaryFilter.order[P::IO_LOCATION].second];
          const auto& anc_io_channel =
                  point[ancillaryFilter.order[P::IO_CHANNEL].second];

          bool drop_check = ancillaryFilter.order[P::DROP].first
              && (!std::all_of(anc_drop.begin(),
                               anc_drop.end(),
                               [](const QChar& c) {return c.isDigit();})
                || (drop
                    != "DROP" + anc_drop
                       + "/DROP" + QString::number(anc_drop.toInt() + 50)));
          bool io_location_check =
                  ancillaryFilter.order[P::IO_LOCATION].first
                  && io_location != anc_io_location;
          bool io_channel_check =
                  ancillaryFilter.order[P::IO_CHANNEL].first
                  && io_channel != anc_io_channel;

          if (!anc_drop.isEmpty()
              || !io_channel.isEmpty()
              || !anc_io_channel.isEmpty()
              || !io_channel.isEmpty()
              || !anc_io_channel.isEmpty()) {
            if (temp_kks != kks
                && (drop_check
                    || io_location_check
                    || io_channel_check)) {
              filtering.addErrorInfo(kks,
                                     filter_mode,
                                     "Проверяется соответствующая XQ01 точка - "
                                     + temp_kks);
            }
            for (auto tuple : QList<std::tuple<bool, P, QString, P, QString>>{
            {drop_check, P::DROP, drop,
                 ancillaryFilter.order[P::DROP].second, anc_drop},
            {io_location_check, P::IO_LOCATION, io_location,
                 ancillaryFilter.order[P::IO_LOCATION].second, anc_io_location},
            {io_channel_check, P::IO_CHANNEL, io_channel,
                 ancillaryFilter.order[P::IO_CHANNEL].second, anc_io_channel}
            }) {
              auto check = std::get<0>(tuple);
              auto parameter = std::get<1>(tuple);
              const auto& parameter_value = std::get<2>(tuple);
              auto anc_parameter = std::get<3>(tuple);
              const auto& anc_parameter_value = std::get<4>(tuple);
              if (check) {
                filtering.addErrorInfo(kks,
                                       filter_mode,
                                       PointInfo::toString(parameter)
                                       + " (" + parameter_value
                                       + ") не соответствует "
                                       + PointInfo::toString(anc_parameter)
                                       + " (" + anc_parameter_value + ")");
                filtering.addErrorColor(kks,
                                        filter_mode,
                                        parameter,
                                        point[parameter].isEmpty()
                                        ? FilterType::WARNING : FilterType::ERROR);
                filtering.addErrorColor(kks,
                                        filter_mode,
                                        anc_parameter,
                                        point[anc_parameter].isEmpty()
                                        ? FilterType::WARNING : FilterType::ERROR);
              }
            }
          }
        }
      } else if (filter_mode ==
                 FilterMode::SCANGROUP_BROADCAST_FREQUENCY_MISSMATCH
                 && (filter_modes.contains(filter_mode)
                     || filter_modes.isEmpty())) {
        if (!point[P::SCANGROUP_FREQUENCY].isEmpty()
                && point[P::BROADCAST_FREQUENCY] != "A") {
          if (point[P::BROADCAST_FREQUENCY] == "S"
                  && point[P::SCANGROUP_FREQUENCY] == "0.1") {
            filtering.addErrorInfo(
                        kks,
                        filter_mode,
                        "Быстрая скангруппа не соответствует медленной частоте передачи");
            filtering.addErrorColor(kks,
                                    filter_mode,
                                    P::BROADCAST_FREQUENCY);
            filtering.addErrorColor(kks,
                                    filter_mode,
                                    P::SCANGROUP_FREQUENCY);
          } else if (point[P::BROADCAST_FREQUENCY] == "F"
                     && point[P::SCANGROUP_FREQUENCY] == "1") {
            filtering.addErrorInfo(
                        kks,
                        filter_mode,
                        "Медленная скангруппа не соответствует быстрой частоте передачи");
            filtering.addErrorColor(kks,
                                    filter_mode,
                                    P::BROADCAST_FREQUENCY,
                                    FilterType::WARNING);
            filtering.addErrorColor(kks,
                                    filter_mode,
                                    P::SCANGROUP_FREQUENCY,
                                    FilterType::WARNING);
          } else if (point[P::SCANGROUP_FREQUENCY] != "0.1"
                     && point[P::SCANGROUP_FREQUENCY] != "1") {
            filtering.addErrorInfo(kks, filter_mode, "Нестандартная скангруппа");
            filtering.addErrorColor(kks, filter_mode, P::SCANGROUP_FREQUENCY);
          }
        }
      } else if (filter_mode
                 == FilterMode::BROADCAST_FREQUENCY_TASK_UPDATETIME_MISSMATCH
                 && (filter_modes.contains(filter_mode)
                     || filter_modes.isEmpty())) {
        if (point[P::TYPE] == PointInfo::toString(PointInfo::Type::AnalogPoint)
                && !point[P::IO_TASK_INDEX].isEmpty()
                && !point[P::BROADCAST_FREQUENCY].isEmpty()) {
          if (point[P::BROADCAST_FREQUENCY] == "S"
                  && drop_info[point[P::DROP]]
                    .task_period_info[point[P::IO_TASK_INDEX]].toInt() <= 100) {
            filtering.addErrorInfo(
                        kks,
                        filter_mode,
                        "Низкая частота передачи не соответствует быстрому таску"
                        "\nIO_TASK_INDEX: " + point[P::IO_TASK_INDEX]
                        + " (periodtime: "
                        + drop_info[point[P::DROP]]
                          .task_period_info[point[P::IO_TASK_INDEX]] + ")");
            filtering.addErrorColor(kks,
                                    filter_mode,
                                    P::IO_TASK_INDEX,
                                    FilterType::WARNING);
            filtering.addErrorColor(kks,
                                    filter_mode,
                                    P::BROADCAST_FREQUENCY,
                                    FilterType::WARNING);
          } else if (point[P::BROADCAST_FREQUENCY] == "F"
                     && drop_info[point[P::DROP]]
                        .task_period_info[point[P::IO_TASK_INDEX]]
                        .toInt() > 100) {
            filtering.addErrorInfo(
                        kks,
                        filter_mode,
                        "Высокая частота передачи не соответствует медленному таску"
                        "\nIO_TASK_INDEX: " + point[P::IO_TASK_INDEX]
                        + " (periodtime: "
                        + drop_info[point[P::DROP]]
                          .task_period_info[point[P::IO_TASK_INDEX]] + ")");
            filtering.addErrorColor(kks, filter_mode, P::IO_TASK_INDEX);
            filtering.addErrorColor(kks, filter_mode, P::BROADCAST_FREQUENCY);
          }
        }
      } else if (filter_mode == FilterMode::LIMITS_PRIORITY_ERRORS
                 && (filter_modes.contains(filter_mode)
                     || filter_modes.isEmpty())) {
        if (point[P::TYPE]
                == PointInfo::toString(PointInfo::Type::AnalogPoint)) {
          for (auto it = alarmsFilter.priority.keyValueBegin();
               it != alarmsFilter.priority.keyValueEnd(); ++it) {
            auto parameter = (*it).first;
            auto enabled = (*it).second.first;
            auto value = (*it).second.second;
            if (enabled && point[parameter].toInt() != value) {
              filtering.addErrorInfo(kks,
                                     filter_mode,
                                     PointInfo::toString(parameter)
                                     + " != " + QString::number(value));
              filtering.addErrorColor(kks,
                                      filter_mode,
                                      parameter,
                                      FilterType::WARNING);
            }
          }
        }
      } else if (filter_mode == FilterMode::SOE_INPUT_ERRORS
                 && (filter_modes.contains(filter_mode)
                     || filter_modes.isEmpty())) {
        if (point[P::TYPE] == PointInfo::toString(PointInfo::Type::DigitalPoint)
            && !point[P::IO_LOCATION].isEmpty()
            && !point[P::IO_CHANNEL].isEmpty()) {
          auto soe_point = point[P::SOE_POINT];
          auto soe_enabled = point[P::SOE_ENABLED];
          QString soe_input, module_soe_input_info_hex, module_soe_input_info_binary;
          if (drop_info.value(point[P::DROP])
                  .module_soe_input_info.contains(point[P::IO_LOCATION])) {
            module_soe_input_info_hex =
                    drop_info[point[P::DROP]]
                    .module_soe_input_info[point[P::IO_LOCATION]];
            module_soe_input_info_binary =
                    QString::number(module_soe_input_info_hex
                                    .toUInt(nullptr, 16), 2);
            soe_input =
                    module_soe_input_info_binary
                    .rightJustified(16, '0')[16 - point[P::IO_CHANNEL].toInt()];
          }
          QString info;
          if (soe_input.isEmpty()) {
            info = "SOE Input (EVENT_TAGGING_ENABLE) не указан";
          } else {
            info = "SOE Input (EVENT_TAGGING_ENABLE): "
                    + module_soe_input_info_hex + "\n("
                    + module_soe_input_info_binary.rightJustified(16, '0') + ")"
                    + " (bit " + point[P::IO_CHANNEL] + ": " + soe_input + ")";
          }
          info += "\nSOE_POINT: \"" + soe_point +"\""
              + "\nSOE_ENABLED: \"" + soe_enabled + "\"";
          if (soe_point != soe_enabled) {
            filtering.addErrorInfo(kks, filter_mode, info);
            filtering.addErrorColor(kks, filter_mode, P::SOE_POINT);
            filtering.addErrorColor(kks, filter_mode, P::SOE_ENABLED);
          } else if (soe_input.isEmpty()) {
            filtering.addErrorInfo(kks, filter_mode, info);
            filtering.addErrorColor(kks,
                                    filter_mode,
                                    P::SOE_POINT,
                                    soe_point == "0"
                                    ? FilterType::WARNING : FilterType::ERROR);
            filtering.addErrorColor(kks, filter_mode,
                                    P::SOE_ENABLED,
                                    soe_enabled == "0"
                                    ? FilterType::WARNING : FilterType::ERROR);
          } else if (soe_input != soe_point) {
            filtering.addErrorInfo(kks, filter_mode, info);
            filtering.addErrorColor(kks,
                                    filter_mode,
                                    P::SOE_POINT,
                                    soe_input == "1"
                                    ? FilterType::WARNING : FilterType::ERROR);
            filtering.addErrorColor(kks,
                                    filter_mode,
                                    P::SOE_ENABLED,
                                    soe_input == "1"
                                    ? FilterType::WARNING : FilterType::ERROR);
          }
          if (soe_point == "1"
              && soe_enabled == "1"
              && drop_info[point[P::DROP]]
                 .module_soe_input_info[point[P::IO_TASK_INDEX]].toInt() > 100) {
            filtering.addErrorInfo(
                        kks,
                        filter_mode,
                        "SOE-точка в медленном таске (>100мс)\n"
                        "IO_TASK_INDEX: " + point[P::IO_TASK_INDEX]
                        + " (periodtime: "
                        + drop_info[point[P::DROP]]
                          .module_soe_input_info[point[P::IO_TASK_INDEX]]
                        + ")");
            filtering.addErrorColor(kks, filter_mode, P::IO_TASK_INDEX);
            filtering.addErrorColor(kks, filter_mode, P::SOE_POINT);
            filtering.addErrorColor(kks, filter_mode, P::SOE_ENABLED);
          }
        }
      }
    }
    emit updateProgress(
                static_cast<int>(
                    std::floor(
                        100.0 * ++count / points.size())));
  }
  emit updateStatus("Фильтрация (проверка ошибок). Подождите... Завершено");
  emit filteringUpdated();
  emit updateStatus("filteringUpdated()");
}
