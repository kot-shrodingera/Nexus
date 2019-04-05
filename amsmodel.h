#pragma once

#include <QAbstractTableModel>

class AmsModel : public QAbstractTableModel
{
  Q_OBJECT
public:
  AmsModel(QObject *parent = nullptr);

  int rowCount(const QModelIndex &parent) const override;
  int columnCount(const QModelIndex &parent) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  QVariant headerData(int section,
                      Qt::Orientation orientation,
                      int role) const override;

  void load(const QString& amsPath);
  void clear();

private:
  QStringList headers;
  QList<QMap<QString, QString>> points;
};
