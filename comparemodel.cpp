#include "comparemodel.h"

#include <QPushButton>
#include <QScrollArea>

#include <QDebug>

#include "globalsettings.h"

CompareModel::CompareModel(const QList<QAbstractTableModel*> &tableModelList,
                           QObject *parent)
  : QSortFilterProxyModel(parent),
    compareModelData(new CompareModelData(tableModelList, parent))
{
  setSourceModel(compareModelData);
}

QVariant CompareModel::headerData(int section,
                                  Qt::Orientation orientation,
                                  int role) const
{
  if(role == Qt::DisplayRole) {
    if (orientation == Qt::Vertical) {
      return section + 1;
    }
  }
  return QSortFilterProxyModel::headerData(section, orientation, role);
}

void CompareModel::runComparition() {
  compareModelData->runComparition();
}

bool CompareModel::filterAcceptsRow
(int source_row, [[maybe_unused]] const QModelIndex &source_parent) const
{
  auto model = sourceModel();
  auto compareModelCount = compareModelData->m_tableModelList.size();
  for (int i = 0; i < compareModelData->m_parameterList.size(); ++i) {
    auto value = QVariant();
    for (int j = 0; j < compareModelCount; ++j) {
      if (compareModelData->m_tableModelList[j]->rowCount() != 0) {
        auto stringValue
            = model->data(model->index(source_row,
                                       1 + i * compareModelCount + j))
                                 .toString();
        bool convOk = false;
        auto doubleValue = QString::number(stringValue.toDouble(&convOk));
        if (value.isValid()) {
          if ((convOk && value != doubleValue)
              || (!convOk && value != stringValue)) {
            return true;
          }
        } else {
          if (convOk) {
            value = doubleValue;
          } else {
            value = stringValue;
          }
        }
      }

//    auto firstValueString
//        = model->data(model->index(source_row,
//                                   1 + i * compareModelCount)).toString();
//    bool convOk = false;
//    auto firstValue = QString::number(firstValueString.toDouble(&convOk));
//    if (!convOk) {
//      firstValue = firstValueString;
//    }
//    for (int j = 1; j < compareModelCount; ++j) {
//      auto otherValueString
//          = model->data(model->index(source_row,
//                                     1 + i * compareModelCount + j)).toString();
//      auto otherValue = QString::number(otherValueString.toDouble(&convOk));
//      if (!convOk) {
//        otherValue = otherValueString;
//      }
//      if (firstValue != otherValue) {
//        return true;
//      }
    }
  }
  return false;
}

bool CompareModel::filterAcceptsColumn
(int source_column, [[maybe_unused]] const QModelIndex &source_parent) const
{
  if (source_column != 0) {
    int modelCount = compareModelData->m_tableModelList.size();
    int modelNumber = (source_column - 1) % modelCount;
    if (compareModelData->m_tableModelList[modelNumber]->rowCount() == 0) {
      return false;
    }
  }
  return true;
}

CompareModelData::CompareModelData
(const QList<QAbstractTableModel*> &tableModelList, QObject *parent)
  : QAbstractTableModel(parent),
    m_tableModelList(tableModelList)
{
  auto compareDefaultParameters = global_settings["CompareDefaultParameters"];
  Q_ASSERT(compareDefaultParameters.isArray());
  for (auto defaultParameters : compareDefaultParameters.toArray()) {
    Q_ASSERT(defaultParameters.isArray());
    m_parameterList.append(QList<QString>{});
    for (auto parameter : defaultParameters.toArray()) {
      m_parameterList.last().append(parameter.toString());
    }
  }

  for (auto model : m_tableModelList) {
    connect(model, &QAbstractTableModel::modelAboutToBeReset,
            this, &CompareModelData::clear);
  }
}

int CompareModelData::rowCount([[maybe_unused]] const QModelIndex &parent) const
{
  return m_pointList.size();
}

int CompareModelData::columnCount
([[maybe_unused]] const QModelIndex &parent) const
{
  return 1 + m_tableModelList.size() * m_parameterList.size();
}

QVariant CompareModelData::headerData(int section,
                                  Qt::Orientation orientation,
                                  int role) const
{
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
    if (section == 0) {
      return "KKS";
    } else {
      int parameterNumber = (section - 1) / m_tableModelList.size();
      int modelNumber = (section - 1) % m_tableModelList.size();
      return m_parameterList[parameterNumber][modelNumber];
    }
  }
  return QAbstractTableModel::headerData(section, orientation, role);
}

void CompareModelData::clear()
{
  beginResetModel();
//  if (clearParameters) {
//    m_parameterList.clear();
//  }
  m_pointList.clear();
  m_pointByKks.clear();
  endResetModel();
}

QVariant CompareModelData::data(const QModelIndex &index, int role) const
{
  if (role == Qt::DisplayRole) {
    auto row = index.row();
    auto column = index.column();
    if (column == 0) {
      return m_pointList[row]->first;
    } else {
      int parameterNumber = (column - 1) / m_tableModelList.size();
      int modelNumber = (column - 1) % m_tableModelList.size();
      auto model = m_tableModelList.at(modelNumber);
      auto modelRow = m_pointList[row]->second[modelNumber];
      if (modelRow == -1) {
        return QVariant();
      }
      QStringList modelHeaders;
      for (int i = 0; i < model->columnCount(); ++i) {
        modelHeaders += model->headerData(i, Qt::Horizontal).toString();
      }
      auto modelColumn
          = modelHeaders.indexOf(m_parameterList[parameterNumber][modelNumber]);
      auto modelIndex = model->index(modelRow, modelColumn);
      return model->data(modelIndex);
    }
  }
  return QVariant();
}

void CompareModelData::runComparition()
{
  if (CompareParameterChooser(this).exec() == QDialog::Accepted) {
    beginResetModel();
    for(int modelNumber = 0;
        modelNumber < m_tableModelList.size();
        ++modelNumber) {
      auto model = m_tableModelList[modelNumber];
      auto rowCount = model->rowCount();
      for (int row = 0; row < rowCount; ++row) {
        auto kksIndex = model->index(row, 0);
        auto kks = model->data(kksIndex).toString();
        if (!m_pointByKks.contains(kks)) {
          auto data = new QPair<QString, QVector<int>>
              (kks, QVector<int>(m_tableModelList.size(), -1));
          m_pointList.append(data);
          m_pointList.last()->second[modelNumber] = row;
          m_pointByKks.insert(kks, m_pointList.last());
        } else {
          m_pointByKks[kks]->second[modelNumber] = row;
        }
      }
    }
    for (int i = 0; i < m_pointList.size();) {
      auto kks = m_pointList[i]->first;
      auto rows = m_pointList[i]->second;
      int existingRowsCount = 0;
      for (auto row : rows) {
        if (row != -1) {
          ++existingRowsCount;
        }
        if (existingRowsCount > 1) {
          ++i;
          goto skipRemove;
        }
      }
      m_pointList.removeOne(m_pointByKks[kks]);
      m_pointByKks.remove(kks);
      skipRemove:;
    }
    endResetModel();
  }
}

CompareParameterChooser::CompareParameterChooser(CompareModelData *compareModel)
  : QDialog(), m_compareModel(compareModel)
{
  setWindowTitle("Выбор сравниваемых параметров");
//  resize(600, 400);

  auto layout = new QVBoxLayout(this);

  auto scrollArea = new QScrollArea(this);
  scrollArea->setWidgetResizable(true);
  scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  auto scrollWidget = new QWidget(scrollArea);
  auto scrollLayout = new QVBoxLayout(scrollWidget);
  scrollLayout->setAlignment(Qt::AlignTop);
  scrollArea->setWidget(scrollWidget);

  layout->addWidget(scrollArea);

  for (const auto& parameterList : compareModel->m_parameterList) {
    addComboBoxes(scrollLayout, parameterList);
  }

  auto addButton = new QPushButton("Добавить", this);
  layout->addWidget(addButton);

  connect(addButton, &QPushButton::clicked,
          this, [this, scrollLayout] {
    addComboBoxes(scrollLayout);

  });

  auto applyButton = new QPushButton("Применить", this);
  layout->addWidget(applyButton);

  connect(applyButton, &QPushButton::clicked, this, &QDialog::accept);

  connect(this, &QDialog::accepted,
          this, [this] {
//    while (!m_comboBoxVectorList.isEmpty()
//        && std::all_of(m_comboBoxVectorList.last().begin(),
//                       m_comboBoxVectorList.last().end(),
//                       [](QComboBox* comboBox) {
//        return comboBox->currentIndex() == 0;
//      })) {
//      m_comboBoxVectorList.removeLast();
//    }
//    m_compareModel->m_parameterList.clear();
//    for (const auto& comboBoxVector : m_comboBoxVectorList) {
//      QList<QString> parameterList;
//      for (auto comboBox : comboBoxVector) {
//        parameterList.append(comboBox->currentText());
//      }
//      m_compareModel->m_parameterList.append(parameterList);
//    }
  });
}

void CompareParameterChooser::addComboBoxesWidget(QVBoxLayout *scrollLayout)
{
  if (m_comboBoxVectorList.isEmpty()
      || std::any_of(m_comboBoxVectorList.last().begin(),
                     m_comboBoxVectorList.last().end(),
                     [](QComboBox* comboBox) {
      return comboBox->currentIndex() != 0;
    }))
  {
    auto comboBoxesWidget = new QWidget(this);
    auto comboBoxesLayout = new QHBoxLayout(comboBoxesWidget);

    auto modelCount =  m_compareModel->m_tableModelList.count();
    QVector<QComboBox*> comboBoxVector;


    for (int modelNumber = 0;
         modelNumber < modelCount;
         ++modelNumber) {
      auto comboBox = new QComboBox(this);

      auto model = m_compareModel->m_tableModelList[modelNumber];
      for (auto column = 0; column < model->columnCount(); ++column) {
        auto modelHeader = model->headerData(column, Qt::Horizontal).toString();
        comboBox->addItem(modelHeader);
      }

      comboBoxesLayout->addWidget(comboBox);

      comboBoxVector.append(comboBox);

      connect(comboBox, qOverload<int>(&QComboBox::currentIndexChanged),
              this, [this,
                     scrollLayout,
                     comboBox,
                     comboBoxesWidget,
                     comboBoxesLayout](int index) {
        auto parameterListIndex = scrollLayout->indexOf(comboBoxesWidget);
        auto parameterIndex = comboBoxesLayout->indexOf(comboBox);
        m_compareModel
        ->m_parameterList[parameterListIndex][parameterIndex] =
            comboBox->itemText(index);
      });
    }

    auto deleteButton = new QPushButton("Удалить", this);
    comboBoxesLayout->addWidget(deleteButton);

    connect(deleteButton, &QPushButton::clicked,
            this, [this,
                   scrollLayout,
                   comboBoxesWidget] {
      auto index = scrollLayout->indexOf(comboBoxesWidget);
      scrollLayout->removeWidget(comboBoxesWidget);
//      scrollLayout->removeItem(scrollLayout->itemAt(index));
      delete comboBoxesWidget;
      m_comboBoxVectorList.removeAt(index);
      m_compareModel->m_parameterList.removeAt(index);
    });

    scrollLayout->addWidget(comboBoxesWidget);

    m_comboBoxVectorList.append(comboBoxVector);
  }
}

void CompareParameterChooser::addComboBoxes(QVBoxLayout* scrollLayout) {
  addComboBoxesWidget(scrollLayout);
  auto comboBoxes = m_comboBoxVectorList.last();
  QList<QString> parametersList;
  for (auto comboBox : m_comboBoxVectorList.last()) {
    parametersList.append(comboBox->currentText());
  }
  m_compareModel->m_parameterList.append(parametersList);
}

void CompareParameterChooser::addComboBoxes(QVBoxLayout *scrollLayout,
                                            const QList<QString>& parameterList)
{
  addComboBoxesWidget(scrollLayout);
  Q_ASSERT(!m_comboBoxVectorList.isEmpty());
  for (int i = 0; i < parameterList.count(); ++i) {
    auto comboBox = m_comboBoxVectorList.last()[i];
    auto model = m_compareModel->m_tableModelList[i];
    for (int column = 0; column < model->columnCount(); ++column) {
      if (model->headerData(column, Qt::Horizontal).toString()
          == parameterList[i]) {
        comboBox->setCurrentIndex(column);
        break;
      }
    }
  }
}
