#pragma once

#include <QSortFilterProxyModel>

#include <QComboBox>
#include <QVBoxLayout>
#include <QDialog>

class CompareModelData;

class CompareModel : public QSortFilterProxyModel
{
  Q_OBJECT
public:
  CompareModel(const QList<QAbstractTableModel*> &tableModelList,
               QObject *parent = nullptr);
//  QVariant data(const QModelIndex& index,
//                int role = Qt::DisplayRole) const override;
  QVariant headerData(int section,
                      Qt::Orientation orientation,
                      int role) const override;
  void runComparition();

protected:
  bool filterAcceptsRow(int source_row,
                        const QModelIndex &source_parent) const override;
  bool filterAcceptsColumn(int source_column,
                           const QModelIndex &source_parent) const override;

private:
  CompareModelData* compareModelData;
};

class CompareModelData : public QAbstractTableModel
{
  Q_OBJECT
public:
  CompareModelData(const QList<QAbstractTableModel*> &tableModelList,
               QObject *parent = nullptr);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  QVariant headerData(int section,
                      Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const override;

  void clear();

  void runComparition();

private:

  QList<QList<QString>> m_parameterList;
  QList<QPair<QString, QVector<int>>*> m_pointList;
  //                   kks       rows
  QMap<QString, QPair<QString, QVector<int>>*> m_pointByKks;

  QList<QAbstractTableModel*> m_tableModelList;

  friend class CompareModel;
  friend class CompareParameterChooser;
};

class CompareParameterChooser : public QDialog
{
  Q_OBJECT
public:
  CompareParameterChooser(CompareModelData* compareModel);

private:
  CompareModelData *m_compareModel;

  void addComboBoxesWidget(QVBoxLayout* scrollLayout);
  void addComboBoxes(QVBoxLayout* scrollLayout);
  void addComboBoxes(
      QVBoxLayout* scrollLayout,
      const QList<QString>& parameterList);

  QList<QVector<QComboBox*>> m_comboBoxVectorList;
};
