#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonObject>

#include "DTObject.h"
#include "Defines.h"

DTObject::DTObject(MainForm *form, DBCFormat* format, QObject* parent)
    : m_form(form), m_format(format), QObject(parent)
{
    m_fileName = "";
    _saveFileName = "";
    m_build = "";
}

DTObject::~DTObject()
{
}

void DTObject::set(QString dbcName, QString dbcBuild)
{
    m_fileName = dbcName;
    m_build = dbcBuild;
    _saveFileName = "";
}

void DTObject::search()
{
    int index = m_form->fontComboBox->currentIndex();
    bool isText = false;

    char fieldType = m_format->GetFieldType(index);

    if (fieldType == 's')
        isText = true;

    QString searchValue = m_form->lineEdit->text();

    DBCSortedModel* smodel = static_cast<DBCSortedModel*>(m_form->tableView->model());
    DBCTableModel* model = static_cast<DBCTableModel*>(smodel->sourceModel());
    
    emit loadingStart(dbc->m_recordCount - 1);

    QList<bool> rowStates;
    for (quint32 i = 0; i < dbc->m_recordCount; i++)
    {
        QStringList record = model->getRecord(i);

        if (searchValue.isEmpty())
        {
            rowStates.append(false);
            continue;
        }

        if (isText)
            rowStates.append(!record.at(index).contains(searchValue, Qt::CaseInsensitive));
        else
            rowStates.append(record.at(index) != searchValue);

        emit loadingStep(i);
    }

    emit searchDone(rowStates);
}

void DTObject::exportAsJSON()
{
    DBCSortedModel* smodel = static_cast<DBCSortedModel*>(m_form->tableView->model());
    DBCTableModel* model = static_cast<DBCTableModel*>(smodel->sourceModel());
    if (!model)
        return;

    QFile exportFile(_saveFileName);
    exportFile.open(QIODevice::WriteOnly | QIODevice::Truncate);

    QTextStream stream(&exportFile);

    quint32 step = 0;

    QList<QStringList> dbcList = model->getDbcList();
    emit loadingStart(dbcList.size());

    QStringList fieldNames = m_format->GetFieldNames();

    auto hasNextVisibleField = [this](quint32 pos) {
      for (quint32 i = pos + 1; i < dbc->m_fieldCount; ++i)
          if (!m_format->IsHiden(i))
              return true;
      return false;
    };

    for (quint32 i = 0; i < dbc->m_recordCount; i++)
    {
        QStringList dataList = dbcList.at(i);

        stream << "{ ";
        for (quint32 j = 0; j < dbc->m_fieldCount; j++)
        {
            if (m_format->IsHiden(j))
                continue;

            stream << "\"" << fieldNames.at(j) << "\": ";
            QString endl = hasNextVisibleField(j) ? ", " : "";
            switch (m_format->GetFieldType(j))
            {
                case 'u':
                case 'i':
                case 'f':
                    stream << dataList.at(j) + endl;
                    break;
                case 's':
                    stream << "\"" + dataList.at(j) + "\"" + endl;
                    break;
                default:
                    stream << dataList.at(j) + endl;
                    break;
            }
        }

        stream << " }\n";

        step++;
        emit loadingStep(step);
    }

    exportFile.close();

    emit loadingNote(QString("Done!"));
}

void DTObject::load()
{
    QElapsedTimer timer;
    timer.start();

    QFile m_file(m_fileName);
        
    if (!m_file.open(QIODevice::ReadOnly))
    {
        emit loadingNote(QString("Can't open file %0").arg(m_fileName));
        return;
    }

    dbc = new DBC;
    dbc->m_header = *reinterpret_cast<quint32*>(m_file.read(4).data());


    // Check 'WDBC'
    if (dbc->m_header != 0x43424457)
    {
        emit loadingNote(QString("Incorrect DBC header!"));
        return;
    }

    dbc->m_recordCount = *reinterpret_cast<quint32*>(m_file.read(4).data());
    dbc->m_fieldCount = *reinterpret_cast<quint32*>(m_file.read(4).data());
    dbc->m_recordSize = *reinterpret_cast<quint32*>(m_file.read(4).data());
    dbc->m_stringSize = *reinterpret_cast<quint32*>(m_file.read(4).data());

    dbc->m_dataBlock.resize(dbc->m_recordCount);

    for (quint32 i = 0; i < dbc->m_recordCount; i++)
    {
        dbc->m_dataBlock[i].resize(dbc->m_fieldCount);
        m_file.read((char*)&dbc->m_dataBlock[i][0], dbc->m_recordSize);
    }

    QHash<quint32, quint32> hash;
    quint32 i = 0;
    for (QVector<QVector<quint32> >::iterator itr = dbc->m_dataBlock.begin(); itr != dbc->m_dataBlock.end(); ++itr)
    {
        hash[itr->at(0)] = i;
        i++;
    }

    QByteArray strings = m_file.readAll().right(dbc->m_stringSize);

    QList<QByteArray> stringsList = strings.split('\0');
    QHash<quint32, QString> stringsMap;
    qint32 off = -1;

    for (QList<QByteArray>::iterator itr = stringsList.begin(); itr != stringsList.end(); ++itr)
    {
        stringsMap[off + 1] = QString::fromUtf8((*itr).data());
        off += (*itr).size() + 1;
    }

    // Load format
    QFileInfo finfo(m_fileName);
    if (m_build != "Default")
        m_format->LoadFormat(finfo.baseName(), m_build);
    else
        m_format->LoadFormat(finfo.baseName(), dbc->m_fieldCount);

    QStringList recordList;
    QList<QStringList> dbcList;

    emit loadingStart(dbc->m_recordCount - 1);

    auto combine = [](quint32 high, quint32 low) { return quint64(quint64(high) << 32) | quint64(low); };

    for (quint32 i = 0; i < dbc->m_recordCount; i++)
    {
        recordList.clear();
        for (quint32 j = 0; j < dbc->m_fieldCount; j++)
        {
            switch (m_format->GetFieldType(j))
            {
                case 'u': recordList << QString("%0").arg(dbc->m_dataBlock[i][j]); break;
                case 'i': recordList << QString("%0").arg((qint32)dbc->m_dataBlock[i][j]); break;
                case 'f': recordList << QString("%0").arg((float&)dbc->m_dataBlock[i][j]); break;
                case 's': recordList << stringsMap[dbc->m_dataBlock[i][j]]; break;
                case 'l':
                {
                    if (j + 1 < dbc->m_fieldCount)
                    {
                        recordList << QString("%0").arg(combine(dbc->m_dataBlock[i][j + 1], dbc->m_dataBlock[i][j]));
                        break;
                    }
                }
                case '!': recordList << QString(""); break;
                default:  recordList << QString("%0").arg(dbc->m_dataBlock[i][j]); break;
                
            }
        }
        dbcList << recordList;
        emit loadingStep(i);
    }

    DBCTableModel* model = new DBCTableModel(dbcList, m_form, this);
    model->setFieldNames(m_format->GetFieldNames());

    m_file.close();

    emit loadingNote(QString("Load time (ms): %0").arg(timer.elapsed()));
    emit loadingDone(model);
}

void DTObject::writeDBC()
{
    DBCSortedModel* smodel = static_cast<DBCSortedModel*>(m_form->tableView->model());
    DBCTableModel* model = static_cast<DBCTableModel*>(smodel->sourceModel());
    if (!model)
        return;

    QFileInfo finfo(m_fileName);

    QFile exportFile(_saveFileName);
    exportFile.open(QIODevice::WriteOnly | QIODevice::Truncate);

    QDataStream stream(&exportFile);
    stream.setByteOrder(QDataStream::LittleEndian);

    quint32 step = 0;

    QList<QStringList> dbcList = model->getDbcList();

    emit loadingStart(dbcList.size());

    // <String value, Offset value>
    QMap<QString, quint32> stringMap;

    QByteArray stringBytes;
    stringBytes.append('\0');

    quint32 recordCount = dbcList.size();
    quint32 fieldCount = dbcList.at(0).size();
    quint32 recordSize = fieldCount * 4;

    stream << quint32(0x43424457);
    stream << quint32(recordCount);
    stream << quint32(fieldCount);
    stream << quint32(recordSize);

    for (quint32 i = 0; i < recordCount; i++)
    {
        QStringList dataList = dbcList.at(i);

        for (quint32 j = 0; j < fieldCount; j++)
        {
            switch (m_format->GetFieldType(j))
            {
                case 's':
                {
                    if (dataList.at(j).isEmpty())
                        continue;

                    if (!stringMap.contains(dataList.at(j)))
                    {
                        stringMap[dataList.at(j)] = stringBytes.size();
                        stringBytes.append(dataList.at(j).toUtf8());
                        stringBytes.append('\0');
                    }
                    else
                        continue;
                    break;
                }
                default:
                    break;
            }
        }
    }

    stream << quint32(stringBytes.size());

    for (quint32 i = 0; i < recordCount; i++)
    {
        QStringList dataList = dbcList.at(i);

        for (quint32 j = 0; j < fieldCount; j++)
        {
            switch (m_format->GetFieldType(j))
            {
                case 'u':
                    stream << quint32(dataList.at(j).toUInt());
                    break;
                case 'i':
                    stream << quint32(dataList.at(j).toInt());
                    break;
                case 'f':
                {
                    float value = dataList.at(j).toFloat();
                    stream << (quint32&)value;
                    break;
                }
                case 's':
                    if (dataList.at(j).isEmpty())
                        stream << quint32(0);
                    else
                        stream << quint32(stringMap.value(dataList.at(j)));
                    break;
                default:
                    stream << quint32(dataList.at(j).toUInt());
                    break;
            }
        }

        step++;
        emit loadingStep(step);

    }

    for (quint32 i = 0; i < stringBytes.size(); i++)
        stream << quint8(stringBytes.at(i));

    exportFile.close();

    emit loadingNote(QString("Done!"));
}

void DTObject::exportAsCSV()
{
    DBCSortedModel* smodel = static_cast<DBCSortedModel*>(m_form->tableView->model());
    DBCTableModel* model = static_cast<DBCTableModel*>(smodel->sourceModel());
    if (!model)
        return;

    QFile exportFile(_saveFileName);
    exportFile.open(QIODevice::WriteOnly | QIODevice::Truncate);

    QTextStream stream(&exportFile);

    quint32 step = 0;

    QStringList fieldNames = m_format->GetFieldNames();

    for (quint32 f = 0; f < dbc->m_fieldCount; f++)
        stream << fieldNames.at(f) + ";";

    stream << "\n";

    QList<QStringList> dbcList = model->getDbcList();
    emit loadingStart(dbcList.size());

    for (quint32 i = 0; i < dbc->m_recordCount; i++)
    {
        QStringList dataList = dbcList.at(i);

        for (quint32 j = 0; j < dbc->m_fieldCount; j++)
        {
            if (m_format->IsHiden(j))
                continue;

            switch (m_format->GetFieldType(j))
            {
                case 'u':
                case 'i':
                case 'f':
                    stream << dataList.at(j) + ";";
                    break;
                case 's':
                    stream << "\"" + dataList.at(j) + "\";";
                    break;
                default:
                    stream << dataList.at(j) + ";";
                    break;
            }
        }

        stream << "\n";

        step++;
        emit loadingStep(step);
    }

    exportFile.close();

    emit loadingNote(QString("Done!"));
}

void DTObject::exportAsSQL()
{
    DBCSortedModel* smodel = static_cast<DBCSortedModel*>(m_form->tableView->model());
    DBCTableModel* model = static_cast<DBCTableModel*>(smodel->sourceModel());
    if (!model)
        return;

    QFileInfo finfo(m_fileName);

    QFile exportFile(_saveFileName);
    exportFile.open(QIODevice::WriteOnly | QIODevice::Truncate);

    QTextStream stream(&exportFile);

    emit loadingStart(dbc->m_fieldCount + dbc->m_recordCount);
    quint32 step = 0;

    QStringList fieldNames = m_format->GetFieldNames();

    auto hasNextVisibleField = [this](quint32 pos) {
      for (quint32 i = pos + 1; i < dbc->m_fieldCount; ++i)
          if (!m_format->IsHiden(i))
              return true;
      return false;
    };

    stream << "CREATE TABLE `" + finfo.baseName() + "_dbc` (\n";
    for (quint32 i = 0; i < dbc->m_fieldCount; i++)
    {
        if (m_format->IsHiden(i))
            continue;

        QString endl = hasNextVisibleField(i) ? ",\n" : "\n";
        switch (m_format->GetFieldType(i))
        {
            case 'u':
            case 'i':
            case 'l':
                stream << "\t`" + fieldNames.at(i) + "` bigint(20) NOT NULL default '0'" + endl;
                break;
            case 'f':
                stream << "\t`" + fieldNames.at(i) + "` float NOT NULL default '0'" + endl;
                break;
            case 's':
                stream << "\t`" + fieldNames.at(i) + "` text NOT NULL" + endl;
                break;
            case '!':
                break;
            default:
                stream << "\t`" + fieldNames.at(i) + "` bigint(20) NOT NULL default '0'" + endl;
                break;
        }
        step++;
        emit loadingStep(step);
    }
    stream << ") ENGINE = MyISAM DEFAULT CHARSET = utf8 COMMENT = 'Data from " + finfo.fileName() + "';\n\n";

    QList<QStringList> dbcList = model->getDbcList();
    for (quint32 i = 0; i < dbc->m_recordCount; i++)
    {
        stream << "INSERT INTO `" + finfo.baseName() + "_dbc` (";
        for (quint32 f = 0; f < dbc->m_fieldCount; f++)
        {
            if (m_format->IsHiden(f))
                continue;

            QString endl = hasNextVisibleField(f) ? "`, " : "`) VALUES (";
            stream << "`" + fieldNames.at(f) + endl;
        }
        QStringList dataList = dbcList.at(i);

        for (quint32 d = 0; d < dbc->m_fieldCount; d++)
        {
            if (m_format->IsHiden(d))
                continue;

            if (dataList.at(d).contains("'"))
            {
                QString data = dataList.at(d);
                data.replace("'", "\\'");
                dataList.replace(d, data);
            }
        }

        for (quint32 j = 0; j < dbc->m_fieldCount; j++)
        {
            if (m_format->IsHiden(j))
                continue;

            QString endl = hasNextVisibleField(j) ? "', " : "');\n";
            stream << "'" + dataList.at(j) + endl;
        }
        step++;
        emit loadingStep(step);
    }

    exportFile.close();

    emit loadingNote(QString("Done!"));
}

DBCFormat::DBCFormat(QString jsonFileName)
{
    QFile jsonFile(jsonFileName);
    _fileName = jsonFileName;
    jsonFile.open(QIODevice::ReadOnly);

    _json = QJsonDocument::fromJson(jsonFile.readAll());

    jsonFile.close();
}

DBCFormat::~DBCFormat()
{
}

QStringList DBCFormat::GetBuildList(QString fileName)
{
    QStringList buildList;

    buildList.append("Default");

    if (_json.isArray())
    {
        QJsonArray array = _json.array();
        for (QJsonValue v : array)
        {
            QJsonObject format = v.toObject();
            if (format["name"].toString() == fileName)
            {
                buildList.append(format["version"].toString());
            }
        }
    }

    return buildList;
}

void DBCFormat::LoadFormat(QString name, quint32 fieldCount)
{
    _name = name;
    _version = "Default";

    _fields.clear();

    for (quint32 i = 0; i < fieldCount; i++)
    {
        DBCField field;
        field.type = "uint";
        field.name = QString("Field%0").arg(i+1);
        field.hiden = false;
        _fields.append(field);
    }
}

void DBCFormat::LoadFormat(QString name, QString version)
{
    _name = name;
    _version = version;

    _fields.clear();

    if (_json.isArray())
    {
        QJsonArray array = _json.array();
        for (QJsonValue v : array)
        {
            QJsonObject format = v.toObject();
            if (format["name"].toString() == name && format["version"].toString() == version)
            {
                array = format["fields"].toArray();
                quint32 j = 0;
                for (QJsonValue v : array)
                {
                    QJsonObject field = v.toObject();
                    DBCField f;
                    f.type = field.contains("type") ? field["type"].toString() : "uint";
                    f.name = field.contains("name") ? field["name"].toString() : QString("Field%0").arg(++j);
                    f.hiden = field.contains("hiden") ? field["hiden"].toBool() : false;
                    _fields.append(f);
                }
            }
        }
    }
}

QStringList DBCFormat::GetFieldNames()
{
    QStringList fieldNames;
    for (QList<DBCField>::const_iterator itr = _fields.begin(); itr != _fields.end(); ++itr)
        fieldNames.append(itr->name);

    return fieldNames;
}

QStringList DBCFormat::GetFieldTypes()
{
    QStringList fieldTypes;
    for (QList<DBCField>::const_iterator itr = _fields.begin(); itr != _fields.end(); ++itr)
        fieldTypes.append(itr->type);

    return fieldTypes;
}

void DBCFormat::SetFieldAttribute(quint32 field, QString attr, QString value)
{
    if (_version == "Default")
        return;

    // Set in QDocument
//    QDomNodeList dbcNodes = m_xmlData.childNodes();

//    for (quint32 i = 0; i < dbcNodes.count(); i++)
//    {
//        QDomNodeList dbcExisted = m_xmlData.elementsByTagName(_name);
//        if (!dbcExisted.isEmpty())
//        {
//            if (dbcExisted.item(i).toElement().attribute("build") == _version)
//            {
//                QDomNodeList fieldNodes = m_xmlData.elementsByTagName(_name).item(i).childNodes();
//                fieldNodes.item(field).toElement().setAttribute(attr, value);
//                break;
//            }
//        }
//    }

//    // Save to file
//    QFile xmlFile(_fileName);
//    if (xmlFile.open(QIODevice::WriteOnly))
//    {
//        QTextStream stream(&xmlFile);
//        m_xmlData.save(stream, 0);
//        xmlFile.close();
//    }
}
