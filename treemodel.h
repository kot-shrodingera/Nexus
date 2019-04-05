#pragma once

#include <QAbstractItemModel>

#include "loader.h"

class TreeItem;

class TreeModel : public QAbstractItemModel {
  Q_OBJECT

public:
  TreeModel(const QStringList &headers, QObject *parent = nullptr);
  TreeModel(const TreeModel &treeModel, QObject *parent = nullptr);
//  ~TreeModel();

  QVariant data(const QModelIndex& index, int role) const override;
  QVariant headerData(int section,
                      Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const override;

  QModelIndex index(int row,
                    int column,
                    const QModelIndex &parent = QModelIndex()) const override;
  QModelIndex parent(const QModelIndex& index) const override;

  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  int columnCount(const QModelIndex& parent = QModelIndex()) const override;

  Qt::ItemFlags flags(const QModelIndex& index) const override;
  bool setData(const QModelIndex& index,
               const QVariant& value,
               int role = Qt::EditRole) override;
  bool setData(const QModelIndex& index,
               const QList<QVariant>& data,
               int role = Qt::EditRole);
  bool setHeaderData(int section,
                     Qt::Orientation orientation,
                     const QVariant& value,
                     int role = Qt::EditRole) override;

  bool insertColumns(int position,
                     int columns,
                     const QModelIndex& parent = QModelIndex()) override;
  bool removeColumns(int position,
                     int columns,
                     const QModelIndex& parent = QModelIndex()) override;
  bool insertRows(int position,
                  int rows,
                  const QModelIndex& parent = QModelIndex()) override;
  bool removeRows(int position,
                  int rows,
                  const QModelIndex& parent = QModelIndex()) override;

  void loadFromDbidTree(Loader::DbidTreeItem* dbid_root_item);

  QPair<QVariant, QVariant> getNameValue(int row,
                                         const QModelIndex& parent
                                           = QModelIndex()) const;

  void soeCheck();
  void saveDbid(const QString& path);

  void clear();

signals:
  void updateStatus(const QString& status, int timeout = 0);
  void updateProgress(int percent);

private:
  TreeItem* getItem(const QModelIndex& index) const;

  TreeItem* root_item;
  QStringList headers;
};
