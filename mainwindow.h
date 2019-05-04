#pragma once

#include <QMainWindow>

#include <QGridLayout>

#include <QTreeView>

#include <QStatusBar>
#include <QProgressBar>

#include <QScrollArea>
#include <QGroupBox>
#include <QLineEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QStandardItemModel>
#include <QLabel>
#include <QTextBrowser>

#include <QCheckBox>
#include <QRadioButton>
#include <QDialog>
#include <QComboBox>

#include "tableview.h"
#include "treemodel.h"
#include "pointstablemodel.h"
#include "pointssortfilterproxymodel.h"
#include "srcbgproxymodel.h"
#include "excelpointsmodel.h"
#include "comparemodel.h"
#include "amsmodel.h"

class Loader;

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget* parent = nullptr);

  void setupModels();
  void setupUi();
  void setupSideArea();
  void setupFilters();

  void connectSignals();

  void setDefaults();

  void exportExcel(const QString& file_name);

signals:
  void updateStatus(const QString& status, int timeout = 0);
  void updateProgress(int percent);
  void loadComplete(bool dbid_enabled,
                    bool src_enabled,
                    bool xml_enabled,
                    bool ophxml_enabled,
                    bool excel_enabled,
                    bool ams_enabled);

protected:
  void paintEvent(QPaintEvent *event) override;

private:

  Loader* loader;

  PointsTableModel* tableModel;
  TreeModel* treeModel;
  PointsSortFilterProxyModel* proxyModel;
//  SrcBGErrorsModel* srcBackgroundErrorsTableModel;
  SrcBGProxyModel* srcBGProxyModel;
//  QStandardItemModel* srcBackgroundErrorsTableModel;
  ExcelPointsModel *excelPointsModel;
  CompareModel * compareModel;
  AmsModel *amsModel;

  QWidget* centralWidget;
  QGridLayout* mainLayout;

  QStatusBar* statusBar;
  QProgressBar* progressBar;

  QGridLayout* leftSideLayout;

  QTabBar* tabBar;
  QStackedWidget* stackedWidget;

  TableView* tableView;
  TableView* excelTableView;
  QTreeView* treeView;
  TableView *compareView;
  TableView *amsView;

  QWidget* srcBackgroundErrorsWidget;
  TableView* srcBackgroundErrorsTableView;

  QTextBrowser* textBrowser;

  QScrollArea* sideArea;
  QWidget* sideWidget;
  QGridLayout* sideLayout;
  QGroupBox* pathGroupBox;
  QGridLayout* pathGroupBoxLayout;
  QCheckBox* dbidCheckBox;
  QLineEdit* dbidPathLineEdit;
  QPushButton* dbidPathButton;
  QCheckBox* srcCheckBox;
  QLineEdit* srcPathLineEdit;
  QPushButton* srcPathButton;
  QCheckBox* xmlCheckBox;
  QLineEdit* xmlPathLineEdit;
  QPushButton* xmlPathButton;
  QCheckBox* ophxmlCheckBox;
  QLineEdit* ophxmlPathLineEdit;
  QPushButton* ophxmlPathButton;
  QCheckBox* excelCheckBox;
  QLineEdit* excelPathLineEdit;
  QPushButton* excelPathButton;
  QCheckBox* amsCheckBox;
  QLineEdit* amsPathLineEdit;
  QPushButton* amsPathButton;
  QPushButton* backupPathButton;
  QPushButton* loadButton;

  QPushButton *compareButton;
  QPushButton *soeButton;

  QGroupBox* filtersGroupBox;
  QGridLayout* filtersGroupBoxLayout;
  QList<QPair<QCheckBox*, QRadioButton*>> filtersButtons;
  QLineEdit* kksFilterLineEdit;

  QMap<PointsTableModel::FilterMode, QDialog*> filter_option_dialogs_;
  QMap<PointsTableModel::FilterMode, QDialog*> filter_details_dialogs_;

  QPushButton* exportExcelButton;

  bool init = true;
};

class FilterInfoDialog : public QDialog {
  Q_OBJECT
public:
  FilterInfoDialog(const QString& details, QWidget* parent = nullptr);
private:
  QGridLayout* layout;
  QLabel* label;
};

class CharacteristicsDialog : public QDialog {
  Q_OBJECT
public:
  CharacteristicsDialog(PointsTableModel* tableModel, QWidget* parent = nullptr);
};

class AncillaryOrderDialog : public QDialog {
  Q_OBJECT
public:
  AncillaryOrderDialog(PointsTableModel* tableModel, QWidget* parent = nullptr);
private:
  QList<PointInfo::Parameter> anc_list = {
      PointInfo::Parameter::ANC_1,
      PointInfo::Parameter::ANC_2,
      PointInfo::Parameter::ANC_3,
      PointInfo::Parameter::ANC_4,
      PointInfo::Parameter::ANC_5,
      PointInfo::Parameter::ANC_6,
      PointInfo::Parameter::ANC_7
  };
  struct structure {
    PointInfo::Parameter parameter;
    QCheckBox* checkBox;
    QComboBox* comboBox;
  };
  QList<structure> structure_list;
};

class AlarmsDialog : public QDialog {
  Q_OBJECT
public:
  AlarmsDialog(PointsTableModel* tableModel, QWidget* parent = nullptr);
private:
  struct structure {
    PointInfo::Parameter parameter;
    QCheckBox* checkBox;
    QComboBox* comboBox;
  };
  QList<structure> structure_list;
};
