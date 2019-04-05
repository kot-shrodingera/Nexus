#pragma once

#include <QAbstractTableModel>

#include <QCheckBox>
#include <QDialog>

class ExcelPointsModel : public QAbstractTableModel
{
  Q_OBJECT
public:
  ExcelPointsModel(QObject *parent = nullptr);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant headerData(int section,
                      Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const override;
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;

  void loadExcel(const QString& path);

  void openHeadersChooser();

  void clear();

  QList<QString> using_headers_list;

signals:
  void updateStatus(const QString& status, int timeout = 0);
  void updateProgress(int percent);
  void requestHeadersChooser();
  void closedHeadersChooser();

private:
  QList<QString> full_headers_list;
  QVector<QVector<QString>> points_list;

  class HeadersChooser : public QDialog {
  public:
    HeadersChooser(const QList<QString>& full_headers_list,
                   QList<QString>& using_headers_list);
  private:
    QList<QCheckBox*> checkBox_list;
  };
};
