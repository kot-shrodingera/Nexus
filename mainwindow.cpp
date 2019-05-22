#include "mainwindow.h"

#include <QFileDialog>
#include <QToolButton>
#include <QHeaderView>
#include <QTimer>
#include <QJsonDocument>

#include <xlsxdocument.h>

#include "threadrunner.h"
#include "loader.h"
#include "point.h"

#include "globalsettings.h"
#include <QJsonArray>

#include <QDebug>

#include <QApplication>
#include <QScreen>
#include <QDesktopWidget>
#include <QDomDocument>

QJsonObject global_settings;

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
  QFile settingsFile("settings.json");
  if (settingsFile.open(QIODevice::ReadOnly)) {
    global_settings
        = QJsonDocument::fromJson(settingsFile.readAll()).object();
  }
  setupModels();
  loader = new Loader(this);
  filter_option_dialogs_ = {
    {PointsTableModel::FilterMode::CHARACTERISTICS_ERRORS,
     new CharacteristicsDialog(tableModel, this)},
    {PointsTableModel::FilterMode::ANCILLARY_ERRORS,
     new AncillaryOrderDialog(tableModel, this)},
    {PointsTableModel::FilterMode::LIMITS_PRIORITY_ERRORS,
     new AlarmsDialog(tableModel, this)}
  };
  setupUi();
  connectSignals();

  exportExcelDialog = new ExportExcelDialog(this);
}

void MainWindow::setupModels() {
  tableModel = new PointsTableModel(this);
  treeModel = new TreeModel(QStringList() << "Parameter" << "Value");
  proxyModel = new PointsSortFilterProxyModel(this);
  proxyModel->setSourceModel(tableModel);
  srcBGProxyModel = new SrcBGProxyModel(this);
//  srcBackgroundErrorsTableModel = new QStandardItemModel(this);
  excelPointsModel = new ExcelPointsModel(this);
  amsModel = new AmsModel(this);
  compareModel = new CompareModel({tableModel,
                                   excelPointsModel,
                                   amsModel}, this);
}

void MainWindow::setupUi() {
  resize(1000, 600);
  centralWidget = new QWidget(this);
  setCentralWidget(centralWidget);
  mainLayout = new QGridLayout();
  centralWidget->setLayout(mainLayout);
  mainLayout->setContentsMargins(5, 5, 5, 5);
  mainLayout->setSpacing(5);

  setStatusBar(statusBar = new QStatusBar(this));
  statusBar->setSizeGripEnabled(false);

  progressBar = new QProgressBar(this);
  statusBar->addPermanentWidget(progressBar);
  progressBar->setAlignment(Qt::AlignCenter);
  statusBar->setStyleSheet("margin: 0 5px 3px");

  leftSideLayout = new QGridLayout();
  leftSideLayout->setSpacing(0);
  leftSideLayout->setContentsMargins(0, 0, 0, 0);

  tabBar = new QTabBar(this);
  tabBar->addTab("Обзор точек");
  tabBar->addTab("Дерево");
  tabBar->addTab("Ошибки BACKGROUND");
  tabBar->addTab("БД Excel");
  tabBar->addTab("AMS");
  tabBar->addTab("Сравнение");
  for (int i = 1; i < tabBar->count(); ++i) {
    tabBar->setTabEnabled(i, false);
  }

  leftSideLayout->addWidget(tabBar, 0, 0);

  stackedWidget = new QStackedWidget(this);

  tableView = new TableView(proxyModel, this);
  stackedWidget->addWidget(tableView);

  treeView = new QTreeView(this);
  treeView->setModel(treeModel);
  treeView->setColumnWidth(0, 350);
  stackedWidget->addWidget(treeView);

  srcBackgroundErrorsWidget = new QWidget(this);
  srcBackgroundErrorsWidget->setLayout(new QGridLayout());
  srcBackgroundErrorsWidget->layout()->setContentsMargins(0, 0, 0, 5);
  srcBackgroundErrorsTableView =
      new TableView(srcBGProxyModel, this);
  srcBackgroundErrorsTableView->horizontalHeader()
      ->setSectionResizeMode(QHeaderView::ResizeToContents);
  srcBackgroundErrorsWidget->layout()->addWidget(srcBackgroundErrorsTableView);

  srcBackgroundErrorsWidget->layout()
      ->addWidget(srcBGProxyModel->optionsButton);

  stackedWidget->addWidget(srcBackgroundErrorsWidget);

  excelTableView = new TableView(excelPointsModel, this);
  stackedWidget->addWidget(excelTableView);

  amsView = new TableView(amsModel, this);
  stackedWidget->addWidget(amsView);

  compareView = new TableView(compareModel, this);
  stackedWidget->addWidget(compareView);

  leftSideLayout->addWidget(stackedWidget, 1, 0);

  textBrowser = new QTextBrowser(this);
  textBrowser->setSizePolicy(textBrowser->sizePolicy().horizontalPolicy(),
                             QSizePolicy::Preferred);
  textBrowser->setMaximumHeight(100);
  leftSideLayout->addWidget(textBrowser, 2, 0);

  mainLayout->addLayout(leftSideLayout, 0, 0);

  setupSideArea();

  mainLayout->addWidget(sideArea, 0, 1);
}

void MainWindow::setupSideArea() {
  sideArea = new QScrollArea(this);
  sideArea->setWidgetResizable(true);
  sideArea->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
  sideArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  sideWidget = new QWidget(this);
  sideArea->setWidget(sideWidget);
  sideLayout = new QGridLayout(sideWidget);

  pathGroupBox = new QGroupBox("Пути к исходным данным", this);
  pathGroupBoxLayout = new QGridLayout(pathGroupBox);

  for (auto tuple :
       QList<std::tuple<QCheckBox**, QLineEdit**, QPushButton**, QString>>({
  {&dbidCheckBox, &dbidPathLineEdit, &dbidPathButton, "DBID"},
  {&srcCheckBox, &srcPathLineEdit, &srcPathButton, "SRC"},
  {&xmlCheckBox, &xmlPathLineEdit, &xmlPathButton, "XML"},
  {&ophxmlCheckBox, &ophxmlPathLineEdit, &ophxmlPathButton, "OPHXML"},
  {&excelCheckBox, &excelPathLineEdit, &excelPathButton, "Excel"},
  {&amsCheckBox, &amsPathLineEdit, &amsPathButton, "AMS"}
  })) {
    auto& checkBox = *std::get<0>(tuple);
    auto& lineEdit = *std::get<1>(tuple);
    auto& pushButton = *std::get<2>(tuple);
    const auto& pushButtonLabel = std::get<3>(tuple);

    checkBox = new QCheckBox(this);
    checkBox->setChecked(true);
    pathGroupBoxLayout->addWidget(checkBox,
                                  pathGroupBoxLayout->rowCount(), 0);

    lineEdit = new QLineEdit(this);
    pathGroupBoxLayout->addWidget(lineEdit,
                                  pathGroupBoxLayout->rowCount() - 1, 1);

    pushButton = new QPushButton(pushButtonLabel, this);
    pathGroupBoxLayout->addWidget(pushButton,
                                  pathGroupBoxLayout->rowCount() - 1, 2);
  }

  backupPathButton = new QPushButton("Путь к папке Backup", this);
  pathGroupBoxLayout->addWidget(backupPathButton,
                                pathGroupBoxLayout->rowCount(), 0, 1, -1);

  loadButton = new QPushButton("Загрузить исходные данные", this);
  pathGroupBoxLayout->addWidget(loadButton,
                                pathGroupBoxLayout->rowCount(), 0, 1, -1);

  compareButton = new QPushButton("Сравнение DBID|Excel|AMS", this);
  pathGroupBoxLayout->addWidget(compareButton,
                                pathGroupBoxLayout->rowCount(), 0, 1, -1);

  soeButton = new QPushButton("Автонастройка модулей SOE", this);
  soeButton->setToolTip("Простановка настроек SOE в модулях "
                        "на основе данных в точках");
  pathGroupBoxLayout->addWidget(soeButton,
                                pathGroupBoxLayout->rowCount(), 0, 1, -1);

  sideLayout->addWidget(pathGroupBox, sideLayout->rowCount(), 0);

  setupFilters();

  sideLayout->addWidget(filtersGroupBox, sideLayout->rowCount(), 0);

  exportExcelButton = new QPushButton("Экспорт в Excel", this);
  sideLayout->addWidget(exportExcelButton, sideLayout->rowCount(), 0);

  sideLayout->addItem(new QSpacerItem(0, 0,
                                      QSizePolicy::Expanding,
                                      QSizePolicy::Expanding),
                      sideLayout->rowCount(), 0);
}

void MainWindow::setupFilters() {

  filtersGroupBox = new QGroupBox("Фильтрация", this);
  filtersGroupBoxLayout = new QGridLayout(filtersGroupBox);

  kksFilterLineEdit = new QLineEdit(this);
  kksFilterLineEdit->setPlaceholderText("Фильтр KKS");
  filtersGroupBoxLayout->addWidget(kksFilterLineEdit,
                                   filtersGroupBoxLayout->rowCount(), 0, 1, -1);

  for (auto filter_info : PointsTableModel::filters) {
    auto mode = filter_info.mode;
    auto description = filter_info.description;
    auto radioButton = new QRadioButton(description, this);
    if (mode != PointsTableModel::FilterMode::ALL) {
      radioButton->setDisabled(true);
    }
    filtersButtons.append(radioButton);
    if (mode == PointsTableModel::FilterMode::ALL) {
      radioButton->setChecked(true);
    }
    filtersGroupBoxLayout->addWidget(radioButton,
                                     filtersGroupBoxLayout->rowCount(), 1);
    if (filter_option_dialogs_.contains(mode)) {
      auto optionButton = new QToolButton(this);
      optionButton->setText("...");
      filtersGroupBoxLayout->addWidget(optionButton,
                                       filtersGroupBoxLayout->rowCount() - 1, 2);
      connect(optionButton, &QToolButton::clicked, this, [this, mode]() {
        filter_option_dialogs_[mode]->exec();
      });
    }
    if (!filter_info.details.isEmpty()) {
      auto detailsButton = new QToolButton(this);
      detailsButton->setText("?");
      filtersGroupBoxLayout->addWidget(detailsButton,
                                       filtersGroupBoxLayout->rowCount() - 1, 3);
      filter_details_dialogs_[mode] =
              new FilterInfoDialog(filter_info.details, this);
      connect(detailsButton, &QToolButton::clicked, this, [this, mode]() {
        filter_details_dialogs_[mode]->exec();
      });
    }
  }
}

void MainWindow::connectSignals() {
  connect(tabBar, &QTabBar::currentChanged,
          stackedWidget, &QStackedWidget::setCurrentIndex);

  connect(dbidPathButton, &QPushButton::clicked, [this]() {
     auto dialog = new QFileDialog(this);
     dialog->setFileMode(QFileDialog::ExistingFile);
     dialog->setNameFilter("DBID file (*.imp *.exp)");
     if (dialog->exec()) {
       dbidPathLineEdit->setText(dialog->selectedFiles().first());
     }
  });

  connect(srcPathButton, &QPushButton::clicked, [this]() {
     auto dialog = new QFileDialog(this);
     dialog->setFileMode(QFileDialog::Directory);
     if (dialog->exec()) {
       srcPathLineEdit->setText(dialog->selectedFiles().first());
     }
  });

  connect(xmlPathButton, &QPushButton::clicked, [this]() {
     auto dialog = new QFileDialog(this);
     dialog->setFileMode(QFileDialog::Directory);
     if (dialog->exec()) {
       xmlPathLineEdit->setText(dialog->selectedFiles().first());
     }
  });

  connect(ophxmlPathButton, &QPushButton::clicked, [this]() {
     auto dialog = new QFileDialog(this);
     dialog->setFileMode(QFileDialog::ExistingFile);
     dialog->setNameFilter("*.xml");
     if (dialog->exec()) {
       ophxmlPathLineEdit->setText(dialog->selectedFiles().first());
     }
  });

  connect(excelPathButton, &QPushButton::clicked,
          this, [this] {
    auto dialog = new QFileDialog(this);
    dialog->setFileMode(QFileDialog::ExistingFile);
    dialog->setNameFilter("*.xlsx *.xlsm");
    if (dialog->exec()) {
      excelPathLineEdit->setText(dialog->selectedFiles().first());
    }
  });

  connect(amsPathButton, &QPushButton::clicked,
          this, [this] {
    auto dialog = new QFileDialog(this);
    dialog->setFileMode(QFileDialog::ExistingFile);
    dialog->setNameFilter("*.xml");
    if (dialog->exec()) {
      amsPathLineEdit->setText(dialog->selectedFiles().first());
    }
  });

  connect(backupPathButton, &QPushButton::clicked, [this]() {
    auto dialog = new QFileDialog(this);
    dialog->setFileMode(QFileDialog::Directory);
    dialog->setOption(QFileDialog::ShowDirsOnly, true);
    if (dialog->exec()) {
      auto path = dialog->selectedFiles().first();
      if (QFileInfo::exists(path + "/Backup")) {
        path += "/Backup";
      }
      if (QFileInfo::exists(path + "/Temp")) {
        path += "/Temp";
      }

      QFileInfo dbid_info(path + "/DBID.imp");
      if (dbid_info.exists() && dbid_info.isFile()) {
        dbidPathLineEdit->setText(dbid_info.filePath());
      }

      QFileInfo src_info(path + "/OvptSvr/Aksu/Graphics/");
      if (src_info.exists() && src_info.isDir()) {
        srcPathLineEdit->setText(src_info.path());
      }

      QDir xml_dir(path + "/OvptSvr/Aksu/NET0/");
      xml_dir.setNameFilters(QStringList() << "UNIT?");
      if (xml_dir.count() == 1) {
        QFileInfo xml_info(xml_dir.entryInfoList().first().filePath()
                   + "/ControlFunctions");
        if (xml_info.exists() && xml_info.isDir()) {
          xmlPathLineEdit->setText(xml_info.filePath());
        }
      }

      QFileInfo ophxml_info(path + "/HistorianConfig.xml");
      if (ophxml_info.exists() && ophxml_info.isFile()) {
        ophxmlPathLineEdit->setText(ophxml_info.filePath());
      }
    }
  });

  connect(loadButton, &QPushButton::clicked, this, [this]() {
//    sideWidget->setDisabled(true);
//    loadButton->setDisabled(true);
    for (int i = 0; i < sideLayout->count(); ++i) {
      auto widget = sideLayout->itemAt(i)->widget();
      if (widget != nullptr) {
        widget->setDisabled(true);
      }
    }
    ThreadRunner::ThreadRunner([this] {
      auto dbid_path = dbidPathLineEdit->text();
      auto src_path = srcPathLineEdit->text();
      auto xml_path = xmlPathLineEdit->text();
      auto ophxml_path = ophxmlPathLineEdit->text();
      auto excel_path = excelPathLineEdit->text();
      auto amsPath = amsPathLineEdit->text();
      auto dbid_enabled = !dbid_path.isEmpty() && dbidCheckBox->isChecked();
      auto src_enabled = !src_path.isEmpty() && srcCheckBox->isChecked();
      auto xml_enabled = !xml_path.isEmpty() && xmlCheckBox->isChecked();
      auto ophxml_enabled
          = !ophxml_path.isEmpty() && ophxmlCheckBox->isChecked();
      auto excel_enabled = !excel_path.isEmpty() && excelCheckBox->isChecked();
      auto ams_enabled = !amsPath.isEmpty() && amsCheckBox->isChecked();
      QVector<QHash<PointInfo::Parameter, QString>> container;
      if (dbid_enabled
          || src_enabled
          || xml_enabled
          || ophxml_enabled
          || excel_enabled) {
        emit updateStatus("Сброс данных. Подождите...");
        loader->clear();
        tableModel->clear();
        treeModel->clear();
        srcBGProxyModel->dataModel->clear();
        excelPointsModel->clear();
        amsModel->clear();
        emit updateStatus("Сброс данных. Подождите... Завершено");
      }
      if (dbid_enabled) {
        loader->loadDbid(dbid_path);
        treeModel->loadFromDbidTree(loader->getDbidTreeRootItem());
        container += tableModel->loadDbidRootItem(loader->getDbidTreeRootItem());
      }
      if (src_enabled) {
        container += loader->loadSrc(src_path);
        srcBGProxyModel->dataModel->setBGErrors(loader->srcBackgroundErrors);
      }
      if (xml_enabled) {
        container += loader->loadXml(xml_path);
      }
      if (ophxml_enabled) {
        container += loader->loadOphxml(ophxml_path);
      }
      tableModel->loadPoints(container);
      updateStatus("After tableModel->loadPoints(container)");
      if (excel_enabled) {
        qDebug() << "Before load Excel";
        excelPointsModel->loadExcel(excel_path);
      }
      if (ams_enabled) {
        amsModel->load(amsPathLineEdit->text());
      }
      emit loadComplete(dbid_enabled,
                        src_enabled,
                        xml_enabled,
                        ophxml_enabled,
                        excel_enabled,
                        ams_enabled);
    });
  });

  connect(compareButton, &QPushButton::clicked, this, [this] {
    compareModel->runComparition();
  });

  connect(soeButton, &QPushButton::clicked, this, [this] {
    auto path
        = QFileDialog::getSaveFileName(this, "Save DBID", "", "*.imp");
    if (!path.isEmpty()) {
      ThreadRunner::ThreadRunner([this, path] {
        treeModel->saveDbid(path);
      });
    }
  });

  connect(exportExcelButton, &QPushButton::clicked, this, [this]() {
    exportExcelDialog->exec();
  });

  connect(excelPointsModel, &ExcelPointsModel::requestHeadersChooser,
          this, [this] {
    excelPointsModel->openHeadersChooser();
  }, Qt::ConnectionType::BlockingQueuedConnection);

  connect(this, &MainWindow::updateProgress,
          progressBar, &QProgressBar::setValue);
  connect(this, &MainWindow::updateStatus,
          statusBar, &QStatusBar::showMessage);
  connect(this, &MainWindow::loadComplete,
          this, [this] (bool dbid_enabled,
                        bool src_enabled,
                        bool xml_enabled,
                        bool ophxml_enabled,
                        bool excel_enabled,
                        bool ams_enabled) {
    for (int i = 0; i < sideLayout->count(); ++i) {
      auto widget = sideLayout->itemAt(i)->widget();
      if (widget != nullptr) {
        widget->setDisabled(false);
      }
    }
    int i = 0;
    for (auto filter_info : PointsTableModel::filters) {
      auto enabled = filter_info.enabled(dbid_enabled,
                                         src_enabled,
                                         xml_enabled,
                                         ophxml_enabled);
      filtersButtons[i]->setDisabled(!enabled);
      ++i;
    }
    tabBar->setTabEnabled(0, dbid_enabled
                             || src_enabled
                             || xml_enabled
                             || ophxml_enabled);
    tabBar->setTabEnabled(1, dbid_enabled);
    tabBar->setTabEnabled(2, src_enabled);
    tabBar->setTabEnabled(3, excel_enabled);
    tabBar->setTabEnabled(4, ams_enabled);
    tabBar->setTabEnabled(5, QVector{dbid_enabled,
                                     excel_enabled,
                                     ams_enabled}.count(true) > 1);
    if (!tabBar->isTabEnabled(tabBar->currentIndex())) {
      for (int i = 0; i < tabBar->count(); ++i) {
        if (tabBar->isTabEnabled(i)) {
          tabBar->setCurrentIndex(i);
          return;
        }
      }
    }
    emit updateStatus("Загрузка завершена");
  });
  connect(loader, &Loader::updateProgress,
          progressBar, &QProgressBar::setValue);
  connect(loader, &Loader::updateStatus,
          statusBar, &QStatusBar::showMessage);
  connect(tableModel, &PointsTableModel::updateProgress,
          progressBar, &QProgressBar::setValue);
  connect(tableModel, &PointsTableModel::updateStatus,
          statusBar, &QStatusBar::showMessage);
//  connect(tableModel, &PointsTableModel::filteringUpdated,
//          proxyModel, &PointsSortFilterProxyModel::invalidate,
//          Qt::ConnectionType::BlockingQueuedConnection);
  connect(excelPointsModel, &ExcelPointsModel::updateStatus,
          statusBar, &QStatusBar::showMessage);

  connect(tableView->selectionModel(), &QItemSelectionModel::selectionChanged,
          this, [this] (const QItemSelection& selection) {
    if (selection.isEmpty()) {
      textBrowser->clear();
      return;
    }
    auto row = selection.indexes().first().row();
    auto kks = proxyModel->data(proxyModel->index(row, 0)).toString();
    auto filter_mode =
            static_cast<PointsTableModel::FilterMode>(
                proxyModel->getFilterMode());
    if (tableModel->filtering.hasError(kks, filter_mode)) {
      textBrowser->setText(tableModel->filtering.getErrorInfo(
                               kks, filter_mode).join('\n'));
    } else {
      textBrowser->clear();
    }
  });

  int i = 0;
  for (auto filterButton : filtersButtons) {
    connect(filterButton, &QRadioButton::toggled,
            this, [this, i](bool toggled) {
      if (toggled) {
        proxyModel->setFilterMode(i);
      }
    });
    ++i;
  }

  connect(kksFilterLineEdit, &QLineEdit::textChanged,
          this, [this](const QString& text) {
    proxyModel->setFilterWildcard(text);
  });
}

void MainWindow::setDefaults()
{
  if (global_settings["DBIDDefaultPath"].isString()) {
    dbidPathLineEdit->setText(global_settings["DBIDDefaultPath"].toString());
  }
  if (global_settings["SRCDefaultPath"].isString()) {
    srcPathLineEdit->setText(global_settings["SRCDefaultPath"].toString());
  }
  if (global_settings["XMLDefaultPath"].isString()) {
    xmlPathLineEdit->setText(global_settings["XMLDefaultPath"].toString());
  }
  if (global_settings["OPHXMLDefaultPath"].isString()) {
    ophxmlPathLineEdit->setText(global_settings["OPHXMLDefaultPath"].toString());
  }
  if (global_settings["ExcelDefaultPath"].isString()) {
    excelPathLineEdit->setText(global_settings["ExcelDefaultPath"].toString());
  }
  if (global_settings["AMSDefaultPath"].isString()) {
    amsPathLineEdit->setText(global_settings["AMSDefaultPath"].toString());
  }
}

void MainWindow::paintEvent(QPaintEvent* event) {
  QMainWindow::paintEvent(event);
  if (init) {
    init = false;
    setDefaults();
  }
}

FilterInfoDialog::FilterInfoDialog(const QString& details, QWidget* parent)
    : QDialog(parent) {
  setMinimumSize(200, 100);
  layout = new QGridLayout(this);
  label = new QLabel(details, this);
  layout->addWidget(label);
}

CharacteristicsDialog::CharacteristicsDialog(PointsTableModel* tableModel,
                                             QWidget* parent)
    : QDialog(parent) {
  setWindowTitle("Характеристика");
  setMinimumSize(100, 100);
  auto layout = new QGridLayout(this);
  auto infoLabel = new QLabel("? - один любой символ\n"
                              "* - любое количество любых символов\n"
                              "[abc] - один из указанных символов", this);
  layout->addWidget(infoLabel, layout->rowCount(), 0, 1, -1);
  auto equalRadioButton = new QRadioButton("Равно", this);
  layout->addWidget(equalRadioButton, layout->rowCount(), 0);
  auto notEqualRadioButton = new QRadioButton("Не равно", this);
  layout->addWidget(notEqualRadioButton, layout->rowCount() - 1, 1);
  if (tableModel->characteristicsFilter.compare_equal) {
    equalRadioButton->setChecked(true);
  } else {
    notEqualRadioButton->setChecked(true);
  }
  auto maskLineEdit = new QLineEdit(this);
  maskLineEdit->setText(tableModel->characteristicsFilter.mask);
  layout->addWidget(maskLineEdit, layout->rowCount(), 0, 1, -1);
  auto applyButton = new QPushButton("Применить", this);
  layout->addWidget(applyButton, layout->rowCount(), 0, 1, -1);
  connect(applyButton, &QPushButton::clicked,
          this, [tableModel, maskLineEdit, equalRadioButton]() {
    tableModel->characteristicsFilter.mask = maskLineEdit->text();
    tableModel->characteristicsFilter.compare_equal =
            equalRadioButton->isChecked();
    tableModel->updateFiltering(
    {PointsTableModel::FilterMode::CHARACTERISTICS_ERRORS});
  });
}

AncillaryOrderDialog::AncillaryOrderDialog(PointsTableModel* tableModel,
                                           QWidget* parent)
    : QDialog(parent) {
  setWindowTitle("Ancillary");
  setMinimumSize(100, 100);
  auto layout = new QGridLayout(this);
  auto infoLabel =
          new QLabel("Указанные поля будут проверяться на соответствие", this);
  layout->addWidget(infoLabel, layout->rowCount(), 0, 1, -1);
  const auto& order = tableModel->ancillaryFilter.order;
  for (const auto& parameter: order.keys()) {
    auto checkBox = new QCheckBox(PointInfo::toString(parameter), this);
    checkBox->setChecked(order[parameter].first);
    layout->addWidget(checkBox, layout->rowCount(), 0);
    auto comboBox = new QComboBox(this);
    for (auto anc : anc_list) {
      comboBox->addItem(PointInfo::toString(anc));
    }
    comboBox->setCurrentIndex(anc_list.indexOf(order[parameter].second));
    layout->addWidget(comboBox, layout->rowCount() - 1, 1);
    structure_list.append({parameter, checkBox, comboBox});
  }
  auto applyButton = new QPushButton("Применить", this);
  layout->addWidget(applyButton, layout->rowCount(), 0, 1, -1);
  connect(applyButton, &QPushButton::clicked, this, [this, tableModel]() {
    for (const auto& s : structure_list) {
      tableModel->ancillaryFilter.order[s.parameter] =
      {s.checkBox->isChecked(), anc_list[s.comboBox->currentIndex()]};
    }
    tableModel->updateFiltering(
    {PointsTableModel::FilterMode::ANCILLARY_ERRORS});
  });
}

AlarmsDialog::AlarmsDialog(PointsTableModel* tableModel,
                           QWidget* parent)
    : QDialog(parent) {
  setWindowTitle("Приоритеты пределов");
  setMinimumSize(100, 100);
  auto layout = new QGridLayout(this);
  const auto& priority = tableModel->alarmsFilter.priority;
  QList<QString> values;
  for (int i = 1; i <= 8; ++i) {
    values.append(QString::number(i));
  }
  for (auto it = priority.keyValueBegin(); it != priority.keyValueEnd(); ++it) {
    auto parameter = (*it).first;
    auto enabled = (*it).second.first;
    auto value = (*it).second.second;
    auto checkBox = new QCheckBox(PointInfo::toString(parameter), this);
    checkBox->setChecked(enabled);
    layout->addWidget(checkBox, layout->rowCount(), 0);
    auto comboBox = new QComboBox(this);
    comboBox->addItems(values);
    comboBox->setCurrentIndex(values.indexOf(QString::number(value)));
    layout->addWidget(comboBox, layout->rowCount() - 1, 1);
    structure_list.append({parameter, checkBox, comboBox});
  }
  auto applyButton = new QPushButton("Применить", this);
  layout->addWidget(applyButton, layout->rowCount(), 0, 1, -1);
  connect(applyButton, &QPushButton::clicked, this, [this, tableModel]() {
    auto& priority = tableModel->alarmsFilter.priority;
    for (const auto& s : structure_list) {
      priority[s.parameter] =
      {s.checkBox->isChecked(), s.comboBox->currentText().toInt()};
    }
    tableModel->updateFiltering(
    {PointsTableModel::FilterMode::LIMITS_PRIORITY_ERRORS});
  });
}

ExportExcelDialog::ExportExcelDialog(MainWindow *parent)
  : QDialog(parent)
{
  connect(this, &ExportExcelDialog::updateStatus,
          parent, &MainWindow::updateStatus);
  connect(this, &ExportExcelDialog::updateProgress,
          parent, &MainWindow::updateProgress);

  setWindowTitle("Экспорт в Excel");
  setMinimumSize(100, 100);
  auto layout = new QVBoxLayout(this);
  auto pathWidget = new QWidget(this);
  auto pathLayout = new QHBoxLayout(pathWidget);
  pathLayout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(pathWidget);
  auto pathLabel = new QLabel("Путь", this);
  pathLayout->addWidget(pathLabel);
  auto pathLineEdit = new QLineEdit(this);
  pathLineEdit->setReadOnly(true);
  pathLayout->addWidget(pathLineEdit);
  auto pathButton = new QPushButton("Обзор...", this);
  pathLayout->addWidget(pathButton);
  auto radioCurrent = new QRadioButton("Текущий фильтр", this);
  radioCurrent->setChecked(true);
  layout->addWidget(radioCurrent);
  auto radioMulti = new QRadioButton("Выбранные фильтры:", this);
  layout->addWidget(radioMulti);
  auto groupBox = new QGroupBox(this);
  layout->addWidget(groupBox);
  auto groupBoxLayout = new QVBoxLayout(groupBox);

  for (auto filterButton : parent->filtersButtons) {
    auto checkBox = new QCheckBox(filterButton->text(), this);
    groupBoxLayout->addWidget(checkBox);
    filterButtons.append(checkBox);
    connect(checkBox, &QCheckBox::clicked, this, [radioMulti]() {
      if (!radioMulti->isChecked()) {
        radioMulti->setChecked(true);
      }
    });
  }

  auto applyButton = new QPushButton("Экспорт", this);
  layout->addWidget(applyButton);

  connect(pathButton, &QPushButton::clicked, this, [this, pathLineEdit] {
    pathLineEdit->setText(
          QFileDialog::getSaveFileName(this, "Экспорт в Excel", "",  "*.xlsx"));
  });

  connect(radioCurrent, &QRadioButton::toggled, this, [this](bool checked) {
    if (checked) {
      filterMode = FilterMode::CURRENT;
    }
  });

  connect(radioMulti, &QRadioButton::toggled, this, [this](bool checked) {
    if (checked) {
      filterMode = FilterMode::MULTI;
    }
  });

  connect(applyButton, &QPushButton::clicked, this, [this, pathLineEdit] {
    ThreadRunner::ThreadRunner([this, pathLineEdit]() {
      exportExcel(pathLineEdit->text());
    });
  });
}

void ExportExcelDialog::exportExcel(const QString& file_name) {
  Q_ASSERT_X(!file_name.isEmpty(), Q_FUNC_INFO, "file_name is empty");
  auto mainWindow = qobject_cast<MainWindow*>(parent());
  emit updateProgress(0);
  emit updateStatus("Генерация Excel-файла");

  QXlsx::Document xlsx;
  QXlsx::Format header_format;
  header_format.setFillPattern(QXlsx::Format::FillPattern::PatternSolid);
  header_format.setPatternBackgroundColor(QColor(65, 157, 241, 255));
  QXlsx::Format red_background_format;
  red_background_format.setFillPattern(QXlsx::Format::FillPattern::PatternSolid);
  red_background_format.setPatternBackgroundColor(QColor(255, 0, 0, 255));
  QXlsx::Format gray_background_format;
  gray_background_format.setFillPattern(QXlsx::Format::FillPattern::PatternSolid);
  gray_background_format.setPatternBackgroundColor(QColor(160, 160, 164, 255));
  auto model = new PointsSortFilterProxyModel(mainWindow->proxyModel);
  QList<int> modes;
  if (filterMode == FilterMode::CURRENT) {
    for (const auto& radioButton : mainWindow->filtersButtons) {
      if (radioButton->isChecked()) {
        modes.append(mainWindow->filtersButtons.indexOf(radioButton));
      }
    }
  } else {
    for (int i = 0; i < filterButtons.size(); ++i) {
      if (filterButtons[i]->isChecked()) {
        modes.append(i);
      }
    }
  }
  int i = 1;
  for (int mode : modes) {
    emit updateStatus("Генерация Excel-файла. Этап "
                      + QString::number(i++) + "/"
                      + QString::number(modes.size()));
    const QString& filter_name = mainWindow->filtersButtons[mode]->text();
    xlsx.addSheet(filter_name);
    model->setFilterMode(mode);

    for (int col = 0; col < model->columnCount(); col++) {
      QString data =
              model->headerData(col, Qt::Horizontal, Qt::DisplayRole).toString();
      xlsx.write(1, col + 1, data, header_format);
      if (data == "APPEAR_IN_FILES") {
        xlsx.setColumnWidth(col + 1, 80);
      } else if (data == "TYPE") {
        xlsx.setColumnWidth(col + 1, 20);
      } else {
        xlsx.setColumnWidth(col + 1, 30);
      }
    }

    int row_count = model->rowCount();
    int column_count = model->columnCount();
    for (int row = 0; row < row_count; row++) {
      emit updateProgress(100 * row / (row_count - 1));
      for (int col = 0; col < column_count; col++) {
        QXlsx::CellReference cell(row + 2, col + 1);
        xlsx.write(cell, model->data(model->index(row, col)));
        if (model->data(model->index(row, col),
                        Qt::BackgroundColorRole) == QColor(Qt::red)) {
          xlsx.write(cell, xlsx.cellAt(cell)->value(), red_background_format);
        } else if (model->data(model->index(row, col),
                               Qt::BackgroundColorRole) == QColor(Qt::gray)) {
          xlsx.write(cell, xlsx.cellAt(cell)->value(), gray_background_format);
        }
      }
    }
  }
  emit updateStatus("Сохранение Excel-файла. Подождите...");
  xlsx.saveAs(file_name);
  emit updateStatus("Сохранение Excel-файла. Подождите... Завершено");
}
