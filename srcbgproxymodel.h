#pragma once

#include <QSortFilterProxyModel>

#include <QDialog>
#include <QListWidget>

class SrcBGProxyModel : public QSortFilterProxyModel {
  Q_OBJECT
public:
  SrcBGProxyModel(QObject *parent = nullptr);

  class DataModel : public QAbstractTableModel {
  public:
    DataModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent) const override;
    int columnCount(const QModelIndex& parent) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role) const override;

    struct Data {
      QString file_name;
      int line;
      QStringList errors;
    };

    void setBGErrors(const QList<Data>& new_bg_errors);

    void clear();

  private:

    QList<Data> bg_errors;
  } *dataModel;

  QPushButton *optionsButton;

  class OptionsDialog : public QDialog {
  public:
    OptionsDialog(SrcBGProxyModel *srcBGProxyModel/*,
                  QWidget *parent = nullptr,
                  Qt::WindowFlags f = Qt::WindowFlags()*/);

    QListWidget* listWidget;
  } *optionsDialog;

  void updateOptionsButtonText();

protected:
  bool filterAcceptsRow(int source_row,
                        const QModelIndex &source_parent) const override;
};
