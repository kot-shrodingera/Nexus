#include "treemodel.h"
#include "treeitem.h"

#include <QFile>

#include <QDebug>

TreeModel::TreeModel(const QStringList& headers, QObject* parent)
    : QAbstractItemModel(parent), headers(headers) {
  QVector<QVariant> rootData;
  for (auto header : headers) {
    rootData << header;
  }

  root_item = new TreeItem(rootData);
}

TreeModel::TreeModel(const TreeModel &treeModel, QObject *parent)
  : QAbstractItemModel(parent), headers(treeModel.headers) {
  QVector<QVariant> rootData;
  for (auto header : headers) {
    rootData << header;
  }

  root_item = new TreeItem(*treeModel.root_item);
}

//TreeModel::~TreeModel() {
//    delete rootItem;
//}

int TreeModel::columnCount([[maybe_unused]] const QModelIndex& parent) const {
  return root_item->columnCount();
}

QVariant TreeModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid())
    return QVariant();

  if (role != Qt::DisplayRole && role != Qt::EditRole)
    return QVariant();

  TreeItem* item = getItem(index);

  return item->data(index.column());
}

Qt::ItemFlags TreeModel::flags(const QModelIndex& index) const {
  if (!index.isValid())
    return Qt::NoItemFlags;

  return Qt::ItemIsEditable | QAbstractItemModel::flags(index);
}

TreeItem* TreeModel::getItem(const QModelIndex& index) const {
  if (index.isValid()) {
    auto item = static_cast<TreeItem*>(index.internalPointer());
    if (item)
      return item;
  }
  return root_item;
}

QVariant TreeModel::headerData(int section,
                               Qt::Orientation orientation,
                               int role) const {
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    return root_item->data(section);

  return QVariant();
}

QModelIndex TreeModel::index(int row,
                             int column,
                             const QModelIndex& parent) const {
  if (parent.isValid() && parent.column() != 0)
    return QModelIndex();

  auto parent_item = getItem(parent);

  auto child_item = parent_item->child(row);
  if (child_item)
    return createIndex(row, column, child_item);
  else
    return QModelIndex();
}

bool TreeModel::insertColumns(int position,
                              int columns,
                              const QModelIndex& parent) {
  bool success;

  beginInsertColumns(parent, position, position + columns - 1);
  success = root_item->insertColumns(position, columns);
  endInsertColumns();

  return success;
}

bool TreeModel::insertRows(int position, int rows, const QModelIndex& parent) {
  auto parent_item = getItem(parent);
  bool success;

  beginInsertRows(parent, position, position + rows - 1);
  success = parent_item->insertChildren(position,
                                        rows,
                                        root_item->columnCount());
  endInsertRows();

  return success;
}

QModelIndex TreeModel::parent(const QModelIndex& index) const {
  if (!index.isValid())
    return QModelIndex();

  auto child_item = getItem(index);
  auto parent_item = child_item->parent();

  if (parent_item == root_item)
    return QModelIndex();

  return createIndex(parent_item->childNumber(), 0, parent_item);
}

bool TreeModel::removeColumns(int position,
                              int columns,
                              const QModelIndex& parent) {
  bool success;

  beginRemoveColumns(parent, position, position + columns - 1);
  success = root_item->removeColumns(position, columns);
  endRemoveColumns();

  if (root_item->columnCount() == 0)
    removeRows(0, rowCount());

  return success;
}

bool TreeModel::removeRows(int position, int rows, const QModelIndex& parent) {
  auto parent_item = getItem(parent);
  bool success = true;

  beginRemoveRows(parent, position, position + rows - 1);
  success = parent_item->removeChildren(position, rows);
  endRemoveRows();

  return success;
}

void TreeModel::loadFromDbidTree(Loader::DbidTreeItem* dbid_root_item) {
  emit updateStatus("Создание дерева DBID. Подождите...");
  beginResetModel();
  delete root_item;
  QVector<QVariant> rootData;
  for(const auto& header : headers)
    rootData << header;

  root_item = new TreeItem(rootData);
  std::function<void (TreeItem*, Loader::DbidTreeItem*)> readItem =
          [this, &readItem](TreeItem* current_item,
                            Loader::DbidTreeItem* dbid_item) {
    if (current_item != root_item) {
      current_item->setData(0, dbid_item->parameter);
      current_item->setData(1, dbid_item->value);
    }
    int row = 0;
    for (auto childDbidItem : dbid_item->children) {
      current_item->insertChildren(row, 1, 2);
      readItem(current_item->child(row), childDbidItem);
      ++row;
    }
  };
  readItem(root_item, dbid_root_item);
  endResetModel();
  emit updateStatus("Создание дерева DBID. Подождите... Завершено");
}

QPair<QVariant, QVariant>
TreeModel::getNameValue(int row,const QModelIndex& parent) const {
  auto item = getItem(parent);
  return {item->child(row)->data(0), item->child(row)->data(1)};
}

void TreeModel::soeCheck()
{
  auto getItem
      = [](TreeItem* item, const QPair<QString, QString>& data) -> TreeItem* {
    for (int i = 0; i < item->childCount(); ++i) {
      auto child = item->child(i);
      if (child->data(0).toString().startsWith(data.first)
          && child->data(1).toString().startsWith(data.second)) {
        return child;
      }
    }
    return nullptr;
  };

  auto getItems
      = [](TreeItem* item, const QPair<QString, QString>& data)
      -> QList<TreeItem*> {
    QList<TreeItem*> list;
    for (int i = 0; i < item->childCount(); ++i) {
      auto child = item->child(i);
      if (child->data(0).toString().startsWith(data.first)
          && child->data(1).toString().startsWith(data.second)) {
        list.append(child);
      }
    }
    return list;
  };

  QMap<QString, QPair<TreeItem*, QList<TreeItem*>>> moduleStructures;

  auto systemItem = getItem(root_item, {"", "System"});
  Q_ASSERT(systemItem);
  auto networkItem = getItem(systemItem, {"", "Network"});
  Q_ASSERT(networkItem);
  auto unitItem = getItem(networkItem, {"UNIT", "Unit"});
  Q_ASSERT(unitItem);
  auto dropItems = getItems(unitItem, {"DROP", "Drop"});
  Q_ASSERT(!dropItems.isEmpty());
  for (auto dropItem : dropItems) {
    QRegularExpression rx("DROP(\\d{2})/DROP\\d{2}");
    auto match = rx.match(dropItem->data(0).toString());
    if (match.hasMatch()) {
      auto dropNumber = match.captured(1);
      auto ioDeviceItem = getItem(dropItem, {"I/O Device 0 IOIC", "IoDevice"});
      Q_ASSERT(ioDeviceItem);
      auto ioInterfaceItems
          = getItems(ioDeviceItem, {"I/O Interface", "IoDevice"});
      Q_ASSERT(!ioInterfaceItems.isEmpty());
      for (auto ioInterfaceItem : ioInterfaceItems) {
        auto interfaceNumber
            = ioInterfaceItem
              ->data(0).toString().mid(QString("I/O Interface ").length(), 1);
        if (interfaceNumber == "1" || interfaceNumber == "2") {
          auto branchItems = getItems(ioInterfaceItem, {"Branch", "Branch"});
          if (!branchItems.isEmpty()) {
            for (auto branchItem : branchItems) {
              auto branchNumber
                  = branchItem
                    ->data(0).toString().mid(QString("Branch ").length(), 1);
              auto slotItems = getItems(branchItem, {"Slot", "RSlot"});
              Q_ASSERT(!slotItems.isEmpty());
              for (auto slotItem : slotItems) {
                auto slotNumber
                    = slotItem
                      ->data(0).toString().mid(QString("Slot ").length(), 1);
                auto moduleItem = getItem(slotItem, {"", "RModule"});
                if (moduleItem) {
                  auto moduleReference = (QStringList()
                                          << dropNumber
                                          << interfaceNumber
                                          << branchNumber
                                          << slotNumber).join('.');
                  moduleStructures.insert(moduleReference, {moduleItem, {}});
                }
              }
            }
          }
        }
      }
      auto pointItems
          = getItems(dropItem,
                     {"", PointInfo::toString(PointInfo::Type::DigitalPoint)});
      for (auto pointItem : pointItems) {
        auto ioLocationItem = getItem(pointItem, {"IO_LOCATION", ""});
        if (ioLocationItem) {
          Q_ASSERT(getItem(pointItem, {"IO_CHANNEL", ""}));
          auto ioLocation = ioLocationItem->data(1).toString();
          auto moduleReference
              = (QStringList() << dropNumber << ioLocation).join('.');
          Q_ASSERT(moduleStructures.contains(moduleReference));
          moduleStructures[moduleReference].second.append(pointItem);
        }
      }
    }
  }

  for (auto structure : moduleStructures) {
    auto key = moduleStructures.key(structure);
//    if (key == "28.1.3.8") {
//      qDebug() << key;
//    }
    auto moduleItem = structure.first;
    auto pointItems = structure.second;
    uint uEventTaggingEnable = 0;
    for (auto pointItem : pointItems) {
//      if (key == "28.1.3.8") {
//        qDebug() << pointItem->data(0);
//      }
      auto soePointItem = getItem(pointItem, {"SOE_POINT", ""});
      auto soeEnabledItem = getItem(pointItem, {"SOE_ENABLED", ""});
      if (soePointItem && soeEnabledItem) {
        if (soePointItem->data(1) == soeEnabledItem->data(1)
            && soePointItem->data(1) == "1") {
          auto ioChannel
              = getItem(pointItem, {"IO_CHANNEL", ""})->data(1).toString();
          uEventTaggingEnable |= 1 << (ioChannel.toInt() - 1);
        }
      }
//      if (key == "28.1.3.8") {
//        qDebug() << uEventTaggingEnable;
//      }
    }
    auto eventTaggingEnabledItem
        = getItem(moduleItem, {"EVENT_TAGGING_ENABLE", ""});
    QString qsEventTaggingEnable
        = "0x" + QString::number(uEventTaggingEnable, 16)
                 .rightJustified(4, '0');
    if (eventTaggingEnabledItem) {
      eventTaggingEnabledItem->setData(1, qsEventTaggingEnable);
    } else {
      moduleItem->insertChildren(moduleItem->childCount(), 1, 2);
      eventTaggingEnabledItem = moduleItem->child(moduleItem->childCount() - 1);
      eventTaggingEnabledItem->setData(0, "EVENT_TAGGING_ENABLE");
      eventTaggingEnabledItem->setData(1, qsEventTaggingEnable);
    }

  }
}

void TreeModel::saveDbid(const QString &path)
{
  auto model = new TreeModel(*this);
  model->soeCheck();
  QString data;
  data += "OVPT_FORMAT=2.1\n";

  auto offset = [](int depth) -> QString {
    QString offset;
    for (int i = 0; i < depth; ++i) {
      offset += " ";
    }
    return offset;
  };

  std::function<QString (TreeItem*, int)> objectToString
      = [&objectToString, &offset](TreeItem* item,int depth) -> QString {

    QString data;
    int i = 0;
    QStringList array;
    for (; i < item->childCount() && item->child(i)->childCount() == 0; ++i) {
      array += QString("%1=\"%2\"")
               .arg(item->child(i)->data(0).toString())
               .arg(item->child(i)->data(1).toString());
    }

    QString children;
    for (; i < item->childCount(); ++i) {
      children += objectToString(item->child(i), depth + 1);
    }

    bool isLast
        = item == item->parent()->child(item->parent()->childCount() - 1);
    data += QString("%1(TYPE=\"%2\" NAME=\"%3\"\n%4[%5]\n%6%7)\n")
           .arg(offset(depth))
           .arg(item->data(1).toString())
           .arg(item->data(0).toString())
           .arg(offset(depth + 1))
           .arg(array.join('\n' + offset(depth + 1)))
           .arg(children)
           .arg(offset((depth == 0 && !isLast) ? 1 : depth));

    return data;
  };

  for (int i = 0; i < model->root_item->childCount(); ++i) {
    data += objectToString(model->root_item->child(i), 0);
  }

  QFile output(path);
  output.open(QIODevice::WriteOnly | QIODevice::Text);

  output.write(data.toLocal8Bit().toStdString().c_str());

}

void TreeModel::clear() {
  beginResetModel();
  delete root_item;
  QVector<QVariant> rootData;
  for(const auto& header : headers)
    rootData << header;

  root_item = new TreeItem(rootData);
  endResetModel();
}

int TreeModel::rowCount(const QModelIndex& parent) const {
  auto parent_item = getItem(parent);

  return parent_item->childCount();
}

bool TreeModel::setData(const QModelIndex& index,
                        const QVariant &value,
                        int role) {
  if (role != Qt::EditRole)
    return false;

  auto item = getItem(index);
  bool result = item->setData(index.column(), value);

  if (result)
    emit dataChanged(index, index, {role});

  return result;
}

bool TreeModel::setData(const QModelIndex& index,
                        const QList<QVariant>& data,
                        int role) {
  if (role != Qt::EditRole)
    return false;

  auto item = getItem(index);
  bool result = true;
  for (int i = 0; i < data.size(); ++i) {
    if (!item->setData(index.column() + i, data)) {
      result = false;
    }
  }

  if (result)
    emit dataChanged(index,
                     this->index(index.row(), index.column() + data.size()),
                     {role});

  return result;
}

bool TreeModel::setHeaderData(int section,
                              Qt::Orientation orientation,
                              const QVariant& value,
                              int role) {
  if (role != Qt::EditRole || orientation != Qt::Horizontal)
    return false;

  bool result = root_item->setData(section, value);

  if (result)
    emit headerDataChanged(orientation, section, section);

  return result;
}
