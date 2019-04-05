#pragma once

#include "point.h"

#include "srcbgproxymodel.h"

class Loader : public QObject {
  Q_OBJECT
public:
  Loader(QObject *parent = nullptr);

  void loadDbid(const QString &dbid_file_path);
  using PointsContainer = QVector<QHash<PointInfo::Parameter, QString>>;
  PointsContainer loadSrc(const QString &src_folder_path);
  PointsContainer loadXml(const QString &xml_folder_path);
  PointsContainer loadOphxml(const QString &ophxml_folder_path);
  void clear();

  struct DbidTreeItem {
    QString parameter, value;
    QList<DbidTreeItem*> children;
  };

  DbidTreeItem* getDbidTreeRootItem();
//  QVector<QHash<PointInfo::Parameter, QString>> getAllPoints();

//  QVector<QHash<PointInfo::Parameter, QString>> pointsContainerFromDbidTreeModel();

//  QMap<QString, QMap<int, QStringList>> srcBackgroundErrors = {};
  QList<SrcBGProxyModel::DataModel::Data> srcBackgroundErrors = {};

//  QVector<QHash<PointInfo::Parameter, QString>> srcPoints = {};
//  QVector<QHash<PointInfo::Parameter, QString>> xmlPoints = {};
//  QVector<QHash<PointInfo::Parameter, QString>> ophxmlPoints = {};

signals:
  void updateStatus(const QString& status, int timeout = 0);
  void updateProgress(int percent);

private:

  enum class dbidEntityType {
    object, array, undefined
  };

  DbidTreeItem *rootItem = nullptr, *currentItem = nullptr;


  dbidEntityType dbidGetEntityType(QString& content, int& pos);
  void dbidReadObject(QString& content, int& pos);
  QPair<QString, QString> dbidReadParameter(QString& content, int& pos);
  QString dbidReadKeyword(QString& content, int& pos);
  QString dbidReadValue(QString& content, int& pos);

  QVector<int> getLinesPositions(QString& content);
  int posToLineNumber(int pos, const QVector<int>& linesPositions);
  void removeComments(QString& content);
  void removeQuotes(QString& content);
  int getFirstIndex(const int& left, const int& right);

  QVector<QString> fileList(const QString& path, const QString& extension);

};

