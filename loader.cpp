#include "loader.h"

#include <cmath>

#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QDir>
#include <QDirIterator>

#include "point.h"
#include "treeitem.h"

#include "threadrunner.h"

#include <QDebug>

Loader::Loader(QObject* parent) : QObject(parent) {}

void Loader::loadDbid(const QString& dbid_file_path) {
  emit updateStatus("Обработка DBID. Подождите...");
//  ThreadRunner::ThreadRunner([this, &dbid_file_path] {
  QFile file(dbid_file_path);
  file.open(QIODevice::ReadOnly | QIODevice::Text);
  QTextStream stream(&file);

  auto content = stream.readAll();
  auto pos = content.indexOf('\n') + 1;

  rootItem = new DbidTreeItem();
  currentItem = rootItem;
  while (dbidGetEntityType(content, pos) == dbidEntityType::object) {
    dbidReadObject(content, pos);
  }
//  });
  emit updateStatus("Обработка DBID. Подождите... Завершено");
}

Loader::PointsContainer Loader::loadSrc(const QString& src_folder_path) {
  QVector<QHash<PointInfo::Parameter, QString>> srcPoints;
  if (!src_folder_path.isEmpty()) {
    emit updateStatus("Обработка файлов графики (src). Подождите...");
//    srcPoints.clear();
    srcBackgroundErrors.clear();
    auto file_list = fileList(src_folder_path, "src");
    int current_file_count = 0;
    for (const auto& file_path : file_list) {
      QFile file(file_path);
      file.open(QIODevice::ReadOnly | QIODevice::Text);
      QTextStream stream(&file);
      auto content = stream.readAll();
      auto file_name = QFileInfo(file_path).fileName();

      removeComments(content);
      removeQuotes(content);

      auto line_positions = getLinesPositions(content);

      int background_begin_pos =
          content.indexOf("BACKGROUND", 0, Qt::CaseInsensitive);
      int background_end_pos = content.length();
      if (background_begin_pos != -1) {
        for (const auto& search_text : {"FOREGROUND",
                        "TRIGGER",
                        "MACRO_TRIGGER",
                        "KEYBOARD"}) {
          auto t_background_end_pos = content.indexOf(
                search_text, background_begin_pos, Qt::CaseInsensitive);
          if (t_background_end_pos != -1
              && (t_background_end_pos < background_end_pos)) {
            background_end_pos = t_background_end_pos;
          }
        }
      }
      QMap<int, QStringList> bg_errors;

      if (background_begin_pos != -1) {
        auto macro_pos = content.indexOf("Macro", background_begin_pos);
        while (macro_pos != -1 && macro_pos < background_end_pos) {
          QRegularExpression re("\\s+(\\d+)\\s");
          auto macro_number = re.match(
                content, macro_pos
                + QString("Macro").length()).captured(1);
          auto& line_background_errors =
              bg_errors[posToLineNumber(macro_pos, line_positions)];
          auto error = "Macro " + macro_number;
          if (!line_background_errors.contains(error)) {
            line_background_errors.append(error);
          }
          macro_pos = content.indexOf("Macro", macro_pos + 1);
        }
      }

      QSet<QString> points_kks;

      auto pos = content.indexOf('\\');
      while (pos != -1) {
        auto end_pos = content.indexOf('\\', pos + 1);
        auto kks = content.mid(pos + 1, end_pos - pos - 1).simplified();
        if (kks.contains(' ')) {
          qFatal(qUtf8Printable(kks + " has whitespaces"));
        }
        if (!kks.startsWith('$') && kks != "________") {
          if (!points_kks.contains(kks)) {
            points_kks.insert(kks);
          }
          if (background_begin_pos != -1
              && pos > background_begin_pos
              && pos < background_end_pos) {
            auto& line_background_errors =
                bg_errors[posToLineNumber(pos, line_positions)];
            auto error = "Definition \\" + kks + "\\";
            if (!line_background_errors.contains(error)) {
              line_background_errors.append(error);
            }
          }
        }
        pos = content.indexOf('\\', end_pos + 1);
      }

      if (!bg_errors.isEmpty()) {
        for (auto it = bg_errors.keyValueBegin();
             it != bg_errors.keyValueEnd(); ++it) {
          srcBackgroundErrors.append({file_name, (*it).first, (*it).second});
        }
//        srcBackgroundErrors[file_name] = background_errors;
      }

      for (const auto& kks : points_kks) {
        QHash<PointInfo::Parameter, QString> parameters;
        parameters[PointInfo::Parameter::KKS] = kks;
        parameters[PointInfo::Parameter::APPEAR_IN_FILES] = file_name;
        srcPoints.append(parameters);
      }
      emit updateProgress(
                  std::lround(100.0 * ++current_file_count / file_list.size()));
    }
    emit updateStatus("Обработка файлов графики (src). Подождите... Завершено");
  }
  return srcPoints;
}

Loader::PointsContainer Loader::loadXml(const QString& xml_folder_path) {
  QVector<QHash<PointInfo::Parameter, QString>> xmlPoints;
  if (!xml_folder_path.isEmpty()) {
    emit updateStatus("Обработка файлов логики (xml). Подождите...");
//    xmlPoints.clear();
    auto file_list = fileList(xml_folder_path, "xml");
    int current_file_count = 0;
    for (const auto& file_path : file_list) {
      QFile file(file_path);
      file.open(QIODevice::ReadOnly | QIODevice::Text);
      QTextStream stream(&file);
      auto content = stream.readAll();
      auto file_name = QFileInfo(file_path).fileName();

      QString search_text = R"(point=")";
      int pos =  content.indexOf(search_text);
      QSet<QString> points_kks;

      while (pos != -1) {
        pos += search_text.length();
        if ((content.at(pos) != '"') && (content.mid(pos, 3) != "OCB")) {
          points_kks.insert(content.mid(pos, content.indexOf('"', pos) - pos));
        }
        pos = content.indexOf(search_text, pos);
      }

      for (const auto& kks : points_kks) {
        QHash<PointInfo::Parameter, QString> parameters;
        parameters[PointInfo::Parameter::KKS] = kks;
        parameters[PointInfo::Parameter::APPEAR_IN_FILES] = file_name;
        xmlPoints.append(parameters);
      }
      emit updateProgress(
                  std::lround(100.0 * ++current_file_count / file_list.size()));
    }
    emit updateStatus("Обработка файлов логики (xml). Подождите... Завершено");
  }
  return xmlPoints;
}

Loader::PointsContainer Loader::loadOphxml(const QString& ophxml_file_path) {
  QVector<QHash<PointInfo::Parameter, QString>> ophxmlPoints;
  emit updateStatus("Обработка OPHXML файла. Подождите...");
  QFile file(ophxml_file_path);
  file.open(QIODevice::ReadOnly | QIODevice::Text);
  QTextStream stream(&file);
  auto file_name = QFileInfo(ophxml_file_path).fileName();

  auto content = stream.readAll();

  auto scangroup_freq_decl = QString(R"(ScanGroup_Frequency=")");
  auto point_name_decl = QString(R"(Point_Name=")");

  auto scangroup_freq_pos = content.indexOf(scangroup_freq_decl);
  auto point_name_pos = content.indexOf(point_name_decl);
  QString freq, point_name;


  auto pos = getFirstIndex(scangroup_freq_pos, point_name_pos);

//  ophxmlPoints.clear();
  while (pos != -1) {
    if (content[pos] == scangroup_freq_decl[0]) {
      pos += scangroup_freq_decl.length();
      freq = content.mid(pos, content.indexOf('"', pos) - pos);
    } else {
      pos += point_name_decl.length();
      point_name = content.mid(pos, content.indexOf('.', pos) - pos);
      QHash<PointInfo::Parameter, QString> parameters;
      parameters[PointInfo::Parameter::KKS] = point_name;
      parameters[PointInfo::Parameter::APPEAR_IN_FILES] = file_name;
      parameters[PointInfo::Parameter::SCANGROUP_FREQUENCY] = freq;
      ophxmlPoints.append(parameters);
    }

    scangroup_freq_pos = content.indexOf(scangroup_freq_decl, pos);
    point_name_pos = content.indexOf(point_name_decl, pos);
    pos = getFirstIndex(scangroup_freq_pos, point_name_pos);
    emit updateProgress(std::lround(100.0 * pos / content.size()));
  }

  emit updateStatus("Обработка OPHXML файла. Подождите... Завершено");
  return ophxmlPoints;
}

void Loader::clear() {
  delete rootItem;
  srcBackgroundErrors.clear();
}

Loader::DbidTreeItem* Loader::getDbidTreeRootItem() {
  return rootItem;
}

Loader::dbidEntityType Loader::dbidGetEntityType(QString& content, int& pos) {
  while (content[pos].isSpace()) {++pos;}
  auto ch = content[pos++];
  while (content[pos].isSpace()) {++pos;}
  if (ch == '(') {
    return dbidEntityType::object;
  } else if (ch == '[') {
    return dbidEntityType::array;
  } else {
    return dbidEntityType::undefined;
  }
}

void Loader::dbidReadObject(QString& content, int& pos) {

  auto type_pair = dbidReadParameter(content, pos);
  auto type_keyword = type_pair.first;
  auto type = type_pair.second;

  auto name_pair = dbidReadParameter(content, pos);
  auto name_keyword = name_pair.first;
  auto name = name_pair.second;

  auto parent = currentItem;
  currentItem->children.append(new DbidTreeItem());
  currentItem = currentItem->children.last();
  currentItem->parameter = name;
  currentItem->value = type;

  while (true) {
    if (content[pos] == ')') {
      ++pos;
      while (content[pos].isSpace()) {++pos;}
      break;
    }
    auto operation = dbidGetEntityType(content, pos);
    if (operation == dbidEntityType::array) {
      int row = 0;
      while (true) {
        if (content[pos] == ']') {
          ++pos;
          while (content[pos].isSpace()) {++pos;}
          break;
        }
        currentItem->children.append(new DbidTreeItem());
        auto item = currentItem->children.last();

        auto parameter_pair = dbidReadParameter(content, pos);
        auto parameter_name = parameter_pair.first;
        auto parameter_value = parameter_pair.second;
        item->parameter = parameter_name;
        item->value = parameter_value;
        ++row;
      }
    } else if (operation == dbidEntityType::object) {
      dbidReadObject(content, pos);
    }
  }

  currentItem = parent;

  emit updateProgress(std::lround(100.0 * pos / content.size()));
}

QPair<QString, QString> Loader::dbidReadParameter(QString& content, int& pos) {
  auto parameter_name = dbidReadKeyword(content, pos);
  if (parameter_name.isEmpty()) {
    qFatal("Parameter name is empty");
  }
  while (content[pos].isSpace()) {++pos;}
  if (content[pos++] != '=') {
    qFatal("Unexpected character. Expected '='");
  }
  while (content[pos].isSpace()) {++pos;}
  auto parameter_value = dbidReadValue(content, pos);
  while (content[pos].isSpace()) {++pos;}
  return QPair(parameter_name, parameter_value);
}

QString Loader::dbidReadKeyword(QString& content, int& pos) {
  QString keyword;
  QChar ch;

  while (static_cast<void>(ch = content[pos]),
         ch.isLetter()
         || ch.isDigit()
         || ch == '.'
         || ch == '_'
         || ch == '-') {
    keyword += ch;
    ++pos;
  }

  return keyword;
}

QString Loader::dbidReadValue(QString& content, int& pos) {
  if (content[pos++] != '"') {
    qFatal("Error. Expected quote");
  }
  QString value;
  while (content[pos] != '"') {
    value += content[pos++];
  }
  ++pos;
  return value;
}

QVector<QString> Loader::fileList(
        const QString& path, const QString& extension) {
  QDir dir(path);
  QVector<QString> container;
  dir.setNameFilters(QStringList() << "*." + extension);
  QDirIterator dir_iterator(dir);
  if (dir_iterator.hasNext()) {
    while (!dir_iterator.next().isEmpty()) {
      auto file_info = dir_iterator.fileInfo();
      if (file_info.isFile()) {
        container.append(file_info.filePath());
      }
    }
  }
  return container;
}

QVector<int> Loader::getLinesPositions(QString& content) {
  QVector<int> result;
  int pos = 0;
  while (pos != -1) {
    result.append(pos + 1);
    pos = content.indexOf('\n', pos + 1);
  }
  return result;
}

int Loader::posToLineNumber(int pos, const QVector<int>& linesPositions) {
  int line = 0;
  for (const auto line_position : linesPositions) {
    if (pos < line_position) {
      return line;
    }
    line++;
  }
  qFatal("pos out of range");
}

void Loader::removeComments(QString& content) {
  int pos = content.indexOf('*');

  while (pos != -1) {
    auto end_pos = content.indexOf('\n', pos);
    if (end_pos == -1) {
      end_pos = content.size();
    }
    content.remove(pos, end_pos - pos);
    pos = content.indexOf('*', pos + 1);
  }
}

void Loader::removeQuotes(QString& content) {
  auto pos = getFirstIndex(content.indexOf('"'), content.indexOf('\''));

  while (pos != -1) {
    QChar quote = content[pos];
    auto next_quote_pos = content.indexOf(quote, pos + 1);
    if (next_quote_pos == -1) {
      qFatal("Not closed quotes");
    }
    auto new_line_count = content.midRef(pos, next_quote_pos - pos).count('\n');
    content.remove(pos + 1, next_quote_pos - pos - 1);
    for (int i = 0; i < new_line_count; ++i) {
      content.insert(pos + 1, '\n');
    }
    pos = getFirstIndex(
                content.indexOf('"', pos + new_line_count + 2),
                content.indexOf('\'', pos + new_line_count + 2));
  }
}

int Loader::getFirstIndex(const int& left, const int& right) {
  if (left == -1 && right == -1) {
    return -1;
  } else if (left != -1 && right != -1) {
    return std::min(left, right);
  } else if (left == -1) {
    return right;
  } else if (right == -1) {
    return left;
  } else {
    Q_ASSERT(true);
    return -1;
  }
}
