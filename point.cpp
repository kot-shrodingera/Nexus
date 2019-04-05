#include "point.h"

#include <QVariant>
#include <QMetaEnum>
#include <QString>

QString PointInfo::toString(const PointInfo::Type& type) {
  return QVariant::fromValue(type).toString();
}

PointInfo::Type PointInfo::typeFromString(const QString& type) {
  auto e = QMetaEnum::fromType<PointInfo::Type>();
  return static_cast<PointInfo::Type>(e.keyToValue(qUtf8Printable(type)));
}

const QVector<PointInfo::Type> PointInfo::point_types = []() {
  auto e = QMetaEnum::fromType<PointInfo::Type>();
  QVector<PointInfo::Type> result;
  for (int i = 0; i < e.keyCount(); ++i) {
    result.append(static_cast<PointInfo::Type>(e.value(i)));
  }
  return result;
}();

QString PointInfo::toString(const PointInfo::Parameter& parameter) {
  return QVariant::fromValue(parameter).toString();
}

PointInfo::Parameter PointInfo::parameterFromString(const QString& parameter) {
  auto e = QMetaEnum::fromType<PointInfo::Parameter>();
  return static_cast<PointInfo::Parameter>(e.keyToValue(qUtf8Printable(parameter)));
}

const QVector<PointInfo::Parameter> PointInfo::point_parameters = []() {
  auto e = QMetaEnum::fromType<PointInfo::Parameter>();
  QVector<PointInfo::Parameter> result;
  for (int i = 0; i < e.keyCount(); ++i) {
    result.append(static_cast<PointInfo::Parameter>(e.value(i)));
  }
  return result;
}();

Point::Point(QHash<PointInfo::Parameter, QString> parameters)
    : parameters_(parameters) {
  const auto& appear_in_file = parameters[PointInfo::Parameter::APPEAR_IN_FILES];
  if (!appear_in_file.isEmpty()) {
    addToAppearInFiles(appear_in_file);
  }
}

const bool& Point::isInDBID() const {
  return is_in_dbid;
}

const bool& Point::isInSRC() const {
  return is_in_src;
}

const bool& Point::isInXML() const {
  return is_in_xml;
}

const bool& Point::isInOPHXML() const {
  return is_in_ophxml;
}

const QString Point::operator[](const PointInfo::Parameter& parameter) const {
  return parameters_[parameter];
}

QString& Point::operator[](const PointInfo::Parameter& parameter) {
  return parameters_[parameter];
}

void Point::addParameters(QHash<PointInfo::Parameter, QString> parameters) {
  auto kks = parameters[PointInfo::Parameter::KKS];
  if (!kks.isEmpty() && parameters_[PointInfo::Parameter::KKS] == kks) {
    for (auto it = parameters.keyValueBegin();
         it != parameters.keyValueEnd(); ++it) {
      auto parameter = (*it).first;
      if (parameter != PointInfo::Parameter::KKS) {
        auto value = (*it).second;
        if (parameter == PointInfo::Parameter::APPEAR_IN_FILES) {
          addToAppearInFiles(value);
        } else {
          parameters_[parameter] = value;
        }
      }

    }
  }
}


void Point::addToAppearInFiles(const QString& appear_in_file) {
  if (!appear_in_files_.contains(appear_in_file)) {
    if (!is_in_dbid && appear_in_file == "DBID.imp") {
      is_in_dbid = true;
    } else if (!is_in_src
               && QStringRef(&appear_in_file, appear_in_file.length() - 3, 3)
               == "src") {
      is_in_src = true;
    } else if (!is_in_xml
               && QStringRef(&appear_in_file, appear_in_file.length() - 3, 3)
               == "xml") {
      is_in_xml = true;
    } else if (!is_in_ophxml && appear_in_file == "HistorianConfig.xml") {
      is_in_ophxml = true;
    }

    appear_in_files_.append(appear_in_file);
    appear_in_files_.sort();
    auto& file_list = parameters_[PointInfo::Parameter::APPEAR_IN_FILES];
    if (is_in_dbid) {
      file_list = "DBID.imp";
    } else {
      file_list = "";
    }
    for (const auto& file : appear_in_files_) {
      if (file != "DBID.imp") {
        if (!file_list.isEmpty()) {
          file_list += ", " + file;
        } else {
          file_list = file;
        }
      }
    }
  }
}
