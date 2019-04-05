#pragma once

#include <QVector>
#include <QMap>
#include <QObject>

#include <QJsonObject>

#include <QMetaEnum>

namespace PointInfo {
  Q_NAMESPACE

  enum class Type {
    AnalogPoint,
    DigitalPoint,
    AlgorithmPoint,
    PackedPoint,
    ModulePoint,
    DropPoint
  };
  Q_ENUM_NS(Type)
  QString toString(const Type& type);
  Type typeFromString(const QString& type);
  extern const QVector<Type> point_types;

  enum class Parameter {
    KKS,
    DROP,
    TYPE,
    SCANGROUP_FREQUENCY,
    BROADCAST_FREQUENCY,
    OPERATING_RANGE_LOW,
    OPERATING_RANGE_HIGH,
    LOW_ENGINEERING_LIMIT,
    HIGH_ENGINEERING_LIMIT,
    MINIMUM_SCALE,
    MAXIMUM_SCALE,
    LOW_ALARM_PRIORITY_1,
    LOW_ALARM_PRIORITY_2,
    LOW_ALARM_PRIORITY_3,
    LOW_ALARM_PRIORITY_4,
    LOW_ALARM_PRIORITY_USER,
    HIGH_ALARM_PRIORITY_1,
    HIGH_ALARM_PRIORITY_2,
    HIGH_ALARM_PRIORITY_3,
    HIGH_ALARM_PRIORITY_4,
    HIGH_ALARM_PRIORITY_USER,
    LOW_ALARM_LIMIT_1_TYPE,
    LOW_ALARM_LIMIT_1_VALUE,
    LOW_ALARM_LIMIT_2_TYPE,
    LOW_ALARM_LIMIT_2_VALUE,
    LOW_ALARM_LIMIT_3_TYPE,
    LOW_ALARM_LIMIT_3_VALUE,
    LOW_ALARM_LIMIT_4_TYPE,
    LOW_ALARM_LIMIT_4_VALUE,
    HIGH_ALARM_LIMIT_1_TYPE,
    HIGH_ALARM_LIMIT_1_VALUE,
    HIGH_ALARM_LIMIT_2_TYPE,
    HIGH_ALARM_LIMIT_2_VALUE,
    HIGH_ALARM_LIMIT_3_TYPE,
    HIGH_ALARM_LIMIT_3_VALUE,
    HIGH_ALARM_LIMIT_4_TYPE,
    HIGH_ALARM_LIMIT_4_VALUE,
    IO_LOCATION,
    IO_CHANNEL,
    IO_TASK_INDEX,
    CHARACTERISTICS,
    ANC_1,
    ANC_2,
    ANC_3,
    ANC_4,
    ANC_5,
    ANC_6,
    ANC_7,
    SOE_POINT,
    SOE_ENABLED,
    APPEAR_IN_FILES
  };
  Q_ENUM_NS(Parameter)
  QString toString(const Parameter& parameter);
  Parameter parameterFromString(const QString& parameter);
  extern const QVector<Parameter> point_parameters;

  inline uint qHash(const Parameter& parameter, uint seed = 0) {
      return ::qHash(static_cast<uint>(parameter), seed);
  };
};

class Point {
public:

  Point(QHash<PointInfo::Parameter, QString> parameters);

//    const static QVector<QString> dbid_parameters_names;
//    const static QVector<QString> additional_parameters_names;
//    const static QVector<QString> parameters_names;

  const bool& isInDBID() const;
  const bool& isInSRC() const;
  const bool& isInXML() const;
  const bool& isInOPHXML() const;

  const QString operator[](const PointInfo::Parameter& parameter) const;
  QString& operator[](const PointInfo::Parameter& parameter);

  void addParameters(QHash<PointInfo::Parameter, QString> parameters);
  void addToAppearInFiles(const QString& appear_in_file);

//  bool hasParameter(const QString& parameter) const;

//  QMap<QString, QString>::const_key_value_iterator begin() const;
//  QMap<QString, QString>::const_key_value_iterator end() const;

//  const static QString empty_string;

private:

  QHash<PointInfo::Parameter, QString> parameters_;

  QStringList appear_in_files_;
  bool is_in_dbid = false;
  bool is_in_src = false;
  bool is_in_xml = false;
  bool is_in_ophxml = false;
};
