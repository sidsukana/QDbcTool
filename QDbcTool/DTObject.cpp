#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonObject>

#include "DTObject.h"
#include "Defines.h"

DTObject::DTObject(MainForm *form, DBCFormat* format, QObject* parent)
    : _form(form), _format(format), _model(nullptr), QObject(parent)
{
    _fileName = "";
    _saveFileName = "";
    _version = "";
}

DTObject::~DTObject()
{
}

QString escapedString(QString str)
{
    str = str.replace("Interface\\Icons\\", "");
    str = str.replace("\r", "");
    str = str.replace("\n", "");
    str = str.replace("\\", "\\\\");
    str = str.replace("\"", "\\\"");
    return str;
}

void DTObject::set(QString fileName, QString version)
{
    _fileName = fileName;
    _version = version;
    _saveFileName = "";
}

void DTObject::search()
{
    int index = _form->fontComboBox->currentIndex();
    bool isText = false;

    char fieldType = _format->getFieldType(index);

    if (fieldType == 's')
        isText = true;

    QString searchValue = _form->lineEdit->text();

    DBCSortedModel* smodel = static_cast<DBCSortedModel*>(_form->tableView->model());
    DBCTableModel* model = static_cast<DBCTableModel*>(smodel->sourceModel());
    
    emit loadingStart(_dbc->m_recordCount - 1);

    QList<bool> rowStates;
    for (quint32 i = 0; i < _dbc->m_recordCount; i++)
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

void DTObject::load()
{
    QElapsedTimer timer;
    timer.start();

    QFile m_file(_fileName);
        
    if (!m_file.open(QIODevice::ReadOnly))
    {
        emit loadingNote(QString("Can't open file %0").arg(_fileName));
        return;
    }

    _dbc = new DBC;
    _dbc->m_header = *reinterpret_cast<quint32*>(m_file.read(4).data());


    // Check 'WDBC'
    if (_dbc->m_header != 0x43424457)
    {
        emit loadingNote(QString("Incorrect DBC header!"));
        return;
    }

    _dbc->m_recordCount = *reinterpret_cast<quint32*>(m_file.read(4).data());
    _dbc->m_fieldCount = *reinterpret_cast<quint32*>(m_file.read(4).data());
    _dbc->m_recordSize = *reinterpret_cast<quint32*>(m_file.read(4).data());
    _dbc->m_stringSize = *reinterpret_cast<quint32*>(m_file.read(4).data());

    _dbc->m_dataBlock.resize(_dbc->m_recordCount);

    for (quint32 i = 0; i < _dbc->m_recordCount; i++)
    {
        _dbc->m_dataBlock[i].resize(_dbc->m_fieldCount);
        m_file.read((char*)&_dbc->m_dataBlock[i][0], _dbc->m_recordSize);
    }

    QHash<quint32, quint32> hash;
    quint32 i = 0;
    for (QVector<QVector<quint32> >::iterator itr = _dbc->m_dataBlock.begin(); itr != _dbc->m_dataBlock.end(); ++itr)
    {
        hash[itr->at(0)] = i;
        i++;
    }

    QByteArray strings = m_file.readAll().right(_dbc->m_stringSize);

    QList<QByteArray> stringsList = strings.split('\0');
    QHash<quint32, QString> stringsMap;
    qint32 off = -1;

    for (QList<QByteArray>::iterator itr = stringsList.begin(); itr != stringsList.end(); ++itr)
    {
        stringsMap[off + 1] = QString::fromUtf8((*itr).data());
        off += (*itr).size() + 1;
    }

    // Load format
    QFileInfo finfo(_fileName);
    if (_version != "Default")
        _format->loadFormat(finfo.baseName(), _version);
    else
        _format->loadFormat(finfo.baseName(), _dbc->m_fieldCount);

    QStringList recordList;
    QList<QStringList> dbcList;

    emit loadingStart(_dbc->m_recordCount - 1);

    auto combine = [](quint32 high, quint32 low) { return quint64(quint64(high) << 32) | quint64(low); };

    for (quint32 i = 0; i < _dbc->m_recordCount; i++)
    {
        recordList.clear();
        for (quint32 j = 0; j < _dbc->m_fieldCount; j++)
        {
            switch (_format->getFieldType(j))
            {
                case 'u': recordList << QString("%0").arg(_dbc->m_dataBlock[i][j]); break;
                case 'i': recordList << QString("%0").arg((qint32)_dbc->m_dataBlock[i][j]); break;
                case 'f': recordList << QString("%0").arg((float&)_dbc->m_dataBlock[i][j]); break;
                case 's': recordList << stringsMap[_dbc->m_dataBlock[i][j]]; break;
                case 'l':
                {
                    if (j + 1 < _dbc->m_fieldCount)
                    {
                        recordList << QString("%0").arg(combine(_dbc->m_dataBlock[i][j + 1], _dbc->m_dataBlock[i][j]));
                        break;
                    }
                }
                case '!': recordList << QString(""); break;
                default:  recordList << QString("%0").arg(_dbc->m_dataBlock[i][j]); break;
                
            }
        }
        dbcList << recordList;
        emit loadingStep(i);
    }

    _model = new DBCTableModel(dbcList, _form, this);
    _model->setFieldNames(_format->getFieldNames());

    m_file.close();

    emit loadingNote(QString("Load time (ms): %0").arg(timer.elapsed()));
    emit loadingDone(_model);
}

void DTObject::writeDBC()
{
    DBCSortedModel* smodel = static_cast<DBCSortedModel*>(_form->tableView->model());
    DBCTableModel* model = static_cast<DBCTableModel*>(smodel->sourceModel());
    if (!model)
        return;

    QFileInfo finfo(_fileName);

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
            switch (_format->getFieldType(j))
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
            switch (_format->getFieldType(j))
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

DBCFormat::DBCFormat(QString jsonFileName)
{
    QFile jsonFile(jsonFileName);
    _fileName = jsonFileName;
    jsonFile.open(QIODevice::ReadOnly);

    _json = QJsonDocument::fromJson(jsonFile.readAll());

    jsonFile.close();
}

DBCFormat::DBCFormat(QJsonDocument json) : _json(json)
{
}

DBCFormat::~DBCFormat()
{
}

QStringList DBCFormat::getVersionList(QString fileName)
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

void DBCFormat::loadFormat(QString name, quint32 fieldCount)
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
        field.custom = false;
        _fields.append(field);
    }
}

void DBCFormat::loadFormat(QString name, QString version)
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
                    QString fieldType = field.contains("type") ? field["type"].toString() : "uint";
                    if (fieldType == "array")
                    {
                        QStringList props = field["props"].toString().split("|");
                        QString type = props.at(0);
                        quint32 size = props.at(1).toUInt();
                        quint32 index = props.size() == 3 ? props.at(2).toUInt() : 0;
                        for (quint32 i = 0; i < size; ++i)
                        {
                            DBCField f;
                            f.type = type;
                            f.name = field.contains("name") ? field["name"].toString() + QString::number(index++) : QString("Field%0").arg(++j);
                            f.hiden = field.contains("hiden") ? field["hiden"].toBool() : false;
                            f.ref = field.contains("ref") ? field["ref"].toString() : "";
                            f.custom = field.contains("custom") ? field["custom"].toBool() : false;
                            f.value = field.contains("value") ? field["value"].toString() : "";
                            _fields.append(f);
                        }
                    }
                    else if (fieldType == "long")
                    {
                        DBCField f;
                        f.type = fieldType;
                        f.name = field.contains("name") ? field["name"].toString() : QString("Field%0").arg(++j);
                        f.hiden = field.contains("hiden") ? field["hiden"].toBool() : false;
                        f.ref = field.contains("ref") ? field["ref"].toString() : "";
                        f.custom = field.contains("custom") ? field["custom"].toBool() : false;
                        f.value = QString::number(field.contains("value") ? field["value"].toInt() : 0);
                        _fields.append(f);
                        f.type = "!";
                        f.name = field.contains("name") ? field["name"].toString() + "_hiden" : QString("Field%0_hiden").arg(++j);
                        f.hiden = true;
                        f.ref = field.contains("ref") ? field["ref"].toString() : "";
                        f.custom = field.contains("custom") ? field["custom"].toBool() : false;
                        _fields.append(f);
                    }
                    else
                    {
                        DBCField f;
                        f.type = fieldType;
                        f.name = field.contains("name") ? field["name"].toString() : QString("Field%0").arg(++j);
                        f.hiden = field.contains("hiden") ? field["hiden"].toBool() : false;
                        f.ref = field.contains("ref") ? field["ref"].toString() : "";
                        f.custom = field.contains("custom") ? field["custom"].toBool() : false;
                        f.value = QString::number(field.contains("value") ? field["value"].toInt() : 0);
                        _fields.append(f);
                    }
                }
            }
        }
    }
}

QStringList DBCFormat::getFieldNames()
{
    QStringList fieldNames;
    for (DBCField& field : _fields)
        if (!field.custom)
            fieldNames.append(field.name);

    return fieldNames;
}

QStringList DBCFormat::getFieldTypes()
{
    QStringList fieldTypes;
    for (DBCField& field : _fields)
        if (!field.custom)
            fieldTypes.append(field.type);

    return fieldTypes;
}

void DBCFormat::setFieldAttribute(quint32 field, QString attr, QString value)
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
