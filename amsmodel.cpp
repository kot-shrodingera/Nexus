#include "amsmodel.h"

#include <QDomDocument>
#include <QFile>

#include <QDebug>

AmsModel::AmsModel(QObject *parent) : QAbstractTableModel(parent)
{

}

int AmsModel::rowCount([[maybe_unused]] const QModelIndex &parent) const
{
  return points.size();
}

int AmsModel::columnCount([[maybe_unused]] const QModelIndex &parent) const
{
  return headers.size();
}

QVariant AmsModel::data(const QModelIndex &index, int role) const
{
  if (role == Qt::DisplayRole) {
    auto row = index.row();
    auto column = index.column();
    return points[row][headers[column]];
  }
  return QVariant();
}

QVariant AmsModel::headerData(int section,
                              Qt::Orientation orientation,
                              int role) const
{
  if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
    return headers[section];
  }
  return QAbstractTableModel::headerData(section, orientation, role);
}

void AmsModel::load(const QString &amsPath)
{
  if (!amsPath.isEmpty()) {
    beginResetModel();
    QDomDocument doc;
    QFile file(amsPath);
    if (!file.open(QIODevice::ReadOnly) || !doc.setContent(&file)) {
      return;
    }

    auto rootElem = doc.documentElement();

    std::function<void(const QDomElement&,
                       QMap<QString, QString>&)> readParameters
        = [this, &readParameters](const QDomElement &element,
                                  QMap<QString, QString>& parameters) {
      if (element.hasAttributes()) {
        for (int i = 0; i < element.attributes().count(); ++i) {
          auto attr = element.attributes().item(i).toAttr();
          auto name =  element.nodeName() + '.' + attr.name();
          auto value = attr.value();
          if (!headers.contains(name)) {
            headers.append(name);
          }
          parameters[name] = value;
        }
      }
      for (int i = 0; i < element.childNodes().count(); ++i) {
        auto child_node = element.childNodes().at(i);
        auto child_node_type = child_node.nodeType();
        if (child_node_type == QDomNode::NodeType::ElementNode) {
          readParameters(child_node.toElement(), parameters);
        } else if (child_node_type == QDomNode::NodeType::TextNode) {
          auto name =  element.nodeName();
          auto value = child_node.toText().nodeValue();
          if (!headers.contains(name)) {
            headers.append(name);
          }
          parameters[name] = value;
        }
      }
    };

    auto readDevice = [this, &readParameters](const QDomElement &element) {
      QMap<QString, QString> parameters;
      readParameters(element, parameters);
      points.append(parameters);
    };

    if (rootElem.nodeName() == "Root") {
      auto root_childs = rootElem.childNodes();
      if (root_childs.count() == 1) {
        auto deviceList = root_childs.at(0).toElement();
        if (deviceList.nodeName() == "DeviceList") {
          for (int i = 0; i < deviceList.childNodes().count(); ++i) {
            auto child_item = deviceList.childNodes().at(i);
            if (child_item.nodeType() == QDomNode::NodeType::ElementNode) {
              auto child_node_element = child_item.toElement();
              if (child_node_element.nodeName() == "Device") {
                readDevice(child_node_element);
              }
            }
          }
        }
      }
    }
    auto amsTagIndex = headers.indexOf("Device.AMSTag");
    Q_ASSERT(amsTagIndex != -1);
    headers.swap(0, amsTagIndex);
    endResetModel();
  }
}

void AmsModel::clear()
{
  beginResetModel();
  headers.clear();
  points.clear();
  endResetModel();
}
