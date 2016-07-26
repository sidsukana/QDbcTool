#include "DTObject.h"
#include "Defines.h"

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QTime>
#include <QtCore/QTextStream>

DTObject::DTObject(DTForm *form, DBCFormat* format)
    : m_form(form), m_format(format)
{
    m_fileName = "";
    m_saveFileName = "";
    m_build = "";

    for (quint8 i = 0; i < MAX_THREAD; i++)
        ThreadSemaphore[i] = false;
}

DTObject::~DTObject()
{
}

void DTObject::ThreadBegin(quint8 id)
{
    if (!ThreadExist(id))
    {
        TObject *thread = new TObject(id, this);
        thread->start();
    }
}

void DTObject::Set(QString dbcName, QString dbcBuild)
{
    m_fileName = dbcName;
    m_build = dbcBuild;
    m_saveFileName = "";
}

void DTObject::Search()
{
    ThreadSet(THREAD_SEARCH);

    int index = m_form->fontComboBox->currentIndex();
    bool isText = false;

    char fieldType = m_format->GetFieldType(index);

    if (fieldType == 's')
        isText = true;

    QString searchValue = m_form->lineEdit->text();

    DBCSortedModel* smodel = static_cast<DBCSortedModel*>(m_form->tableView->model());
    DBCTableModel* model = static_cast<DBCTableModel*>(smodel->sourceModel());
    
    QApplication::postEvent(m_form, new ProgressBar(dbc->m_recordCount - 1, BAR_SIZE));

    for (quint32 i = 0; i < dbc->m_recordCount; i++)
    {
        QStringList record = model->getRecord(i);

        if (searchValue.isEmpty())
        {
            QApplication::postEvent(m_form, new SendHiden(0, i, false));
            continue;
        }

        if (isText)
            QApplication::postEvent(m_form, new SendHiden(0, i, !record.at(index).contains(searchValue, Qt::CaseInsensitive)));
        else
            QApplication::postEvent(m_form, new SendHiden(0, i, record.at(index) != searchValue));

        QApplication::postEvent(m_form, new ProgressBar(i, BAR_STEP));
    }

    ThreadUnset(THREAD_SEARCH);
}

void DTObject::Load()
{
    ThreadSet(THREAD_OPENFILE);

    // Timer
    QTime m_time;
    m_time.start();

    QFile m_file(m_fileName);
        
    if (!m_file.open(QIODevice::ReadOnly))
    {
        ThreadUnset(THREAD_OPENFILE);
        return;
    }

    dbc = new DBC;
    dbc->m_header = *reinterpret_cast<quint32*>(m_file.read(4).data());


    // Check 'WDBC'
    if (dbc->m_header != 0x43424457)
    {
        ThreadUnset(THREAD_OPENFILE);
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

    QApplication::postEvent(m_form, new ProgressBar(dbc->m_recordCount - 1, BAR_SIZE));

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
                default:  recordList << QString("%0").arg(dbc->m_dataBlock[i][j]); break;
                
            }
        }
        dbcList << recordList;
        QApplication::postEvent(m_form, new ProgressBar(i, BAR_STEP));
    }

    DBCTableModel* model = new DBCTableModel(dbcList, m_form, this);
    model->setFieldNames(m_format->GetFieldNames());

    QApplication::postEvent(m_form, new SendModel(m_form, model));

    m_file.close();

    QString stime(QString("Load time (ms): %0").arg(m_time.elapsed()));

    QApplication::postEvent(m_form, new SendText(m_form, 1, stime));

    ThreadUnset(THREAD_OPENFILE);
}

void DTObject::WriteDBC()
{
    ThreadSet(THREAD_WRITE_DBC);

    DBCSortedModel* smodel = static_cast<DBCSortedModel*>(m_form->tableView->model());
    DBCTableModel* model = static_cast<DBCTableModel*>(smodel->sourceModel());
    if (!model)
        return;

    QFileInfo finfo(m_fileName);

    QFile exportFile(m_saveFileName);
    exportFile.open(QIODevice::WriteOnly | QIODevice::Truncate);

    QDataStream stream(&exportFile);
    stream.setByteOrder(QDataStream::LittleEndian);

    QApplication::postEvent(m_form, new ProgressBar(dbc->m_recordCount, BAR_SIZE));
    quint32 step = 0;

    QList<QStringList> dbcList = model->getDbcList();

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
        QApplication::postEvent(m_form, new ProgressBar(step, BAR_STEP));
    }

    for (quint32 i = 0; i < stringBytes.size(); i++)
        stream << quint8(stringBytes.at(i));

    exportFile.close();

    QApplication::postEvent(m_form, new SendText(m_form, 1, QString("Done!")));

    ThreadUnset(THREAD_WRITE_DBC);
}

void DTObject::ExportAsCSV()
{
    ThreadSet(THREAD_EXPORT_CSV);

    DBCSortedModel* smodel = static_cast<DBCSortedModel*>(m_form->tableView->model());
    DBCTableModel* model = static_cast<DBCTableModel*>(smodel->sourceModel());
    if (!model)
        return;

    QFileInfo finfo(m_fileName);

    QFile exportFile(m_saveFileName);
    exportFile.open(QIODevice::WriteOnly | QIODevice::Truncate);

    QTextStream stream(&exportFile);

    QApplication::postEvent(m_form, new ProgressBar(dbc->m_recordCount, BAR_SIZE));
    quint32 step = 0;

    QStringList fieldNames = m_format->GetFieldNames();

    for (quint32 f = 0; f < dbc->m_fieldCount; f++)
        stream << fieldNames.at(f) + ";";

    stream << "\n";

    QList<QStringList> dbcList = model->getDbcList();

    for (quint32 i = 0; i < dbc->m_recordCount; i++)
    {
        QStringList dataList = dbcList.at(i);

        for (quint32 j = 0; j < dbc->m_fieldCount; j++)
        {
            if (!m_format->IsVisible(j))
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
        QApplication::postEvent(m_form, new ProgressBar(step, BAR_STEP));
    }

    exportFile.close();

    QApplication::postEvent(m_form, new SendText(m_form, 1, QString("Done!")));

    ThreadUnset(THREAD_EXPORT_CSV);
}

void DTObject::ExportAsSQL()
{
    ThreadSet(THREAD_EXPORT_SQL);

    DBCSortedModel* smodel = static_cast<DBCSortedModel*>(m_form->tableView->model());
    DBCTableModel* model = static_cast<DBCTableModel*>(smodel->sourceModel());
    if (!model)
        return;

    QFileInfo finfo(m_fileName);

    QFile exportFile(m_saveFileName);
    exportFile.open(QIODevice::WriteOnly | QIODevice::Truncate);

    QTextStream stream(&exportFile);

    QApplication::postEvent(m_form, new ProgressBar(dbc->m_fieldCount + dbc->m_recordCount, BAR_SIZE));
    quint32 step = 0;

    QStringList fieldNames = m_format->GetFieldNames();

    stream << "CREATE TABLE `" + finfo.baseName() + "_dbc` (\n";
    for (quint32 i = 0; i < dbc->m_fieldCount; i++)
    {
        if (!m_format->IsVisible(i))
            continue;

        QString endl = i < dbc->m_fieldCount-1 ? ",\n" : "\n";
        switch (m_format->GetFieldType(i))
        {
            case 'u':
            case 'i':
                stream << "\t`" + fieldNames.at(i) + "` bigint(20) NOT NULL default '0'" + endl;
                break;
            case 'f':
                stream << "\t`" + fieldNames.at(i) + "` float NOT NULL default '0'" + endl;
                break;
            case 's':
                stream << "\t`" + fieldNames.at(i) + "` text NOT NULL" + endl;
                break;
            default:
                stream << "\t`" + fieldNames.at(i) + "` bigint(20) NOT NULL default '0'" + endl;
                break;
        }
        step++;
        QApplication::postEvent(m_form, new ProgressBar(step, BAR_STEP));
    }
    stream << ") ENGINE = MyISAM DEFAULT CHARSET = utf8 COMMENT = 'Data from " + finfo.fileName() + "';\n\n";

    QList<QStringList> dbcList = model->getDbcList();
    for (quint32 i = 0; i < dbc->m_recordCount; i++)
    {
        stream << "INSERT INTO `" + finfo.baseName() + "_dbc` (";
        for (quint32 f = 0; f < dbc->m_fieldCount; f++)
        {
            if (!m_format->IsVisible(f))
                continue;

            QString endl = f < dbc->m_fieldCount-1 ? "`, " : "`) VALUES (";
            stream << "`" + fieldNames.at(f) + endl;
        }
        QStringList dataList = dbcList.at(i);

        for (quint32 d = 0; d < dataList.size(); d++)
        {
            if (!m_format->IsVisible(d))
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
            if (!m_format->IsVisible(j))
                continue;

            if (j < dbc->m_fieldCount-1)
                stream << "'" + dataList.at(j) + "', ";
            else
                stream << "'" + dataList.at(j) + "');\n";
        }
        step++;
        QApplication::postEvent(m_form, new ProgressBar(step, BAR_STEP));
    }

    exportFile.close();

    QApplication::postEvent(m_form, new SendText(m_form, 1, QString("Done!")));

    ThreadUnset(THREAD_EXPORT_SQL);
}

DBCFormat::DBCFormat(QString xmlFileName)
{
    QFile xmlFile(xmlFileName);
    m_fileName = xmlFileName;
    xmlFile.open(QIODevice::ReadOnly);
    m_xmlData.setContent(&xmlFile);
    xmlFile.close();
}

DBCFormat::~DBCFormat()
{
}

QStringList DBCFormat::GetBuildList(QString fileName)
{
    QDomNodeList dbcNodes = m_xmlData.childNodes();
    QStringList buildList;

    buildList.append("Default");

    for (quint32 i = 0; i < dbcNodes.count(); i++)
        if (!m_xmlData.elementsByTagName(fileName).isEmpty())
            buildList.append(m_xmlData.elementsByTagName(fileName).item(i).toElement().attribute("build"));

    return buildList;
}

void DBCFormat::LoadFormat(QString dbcName, quint32 fieldCount)
{
    m_dbcName = dbcName;
    m_dbcBuild = "Default";

    m_dbcFields.clear();

    for (quint32 i = 0; i < fieldCount; i++)
    {
        DBCField field;
        field.type = "uint";
        field.name = QString("Field%0").arg(i+1);
        field.visible = true;
        m_dbcFields.append(field);
    }
}

void DBCFormat::LoadFormat(QString dbcName, QString dbcBuild)
{
    QDomNodeList dbcNodes = m_xmlData.childNodes();

    m_dbcName = dbcName;
    m_dbcBuild = dbcBuild;

    m_dbcFields.clear();

    for (quint32 i = 0; i < dbcNodes.count(); i++)
    {
        QDomNodeList dbcExisted = m_xmlData.elementsByTagName(dbcName);
        if (!dbcExisted.isEmpty())
        {
            if (dbcExisted.item(i).toElement().attribute("build") == dbcBuild)
            {
                QDomNodeList fieldNodes = m_xmlData.elementsByTagName(dbcName).item(i).childNodes();
                for (quint32 j = 0; j < fieldNodes.count(); j++)
                {
                    DBCField field;
                    field.type = fieldNodes.item(j).toElement().attribute("type", "uint");
                    field.name = fieldNodes.item(j).toElement().attribute("name", QString("Field%0").arg(j+1));
                    field.visible = fieldNodes.item(j).toElement().attribute("visible", "true") == QString("true") ? true : false;
                    m_dbcFields.append(field);
                }
            }
        }
    }
}

QStringList DBCFormat::GetFieldNames()
{
    QStringList fieldNames;
    for (QList<DBCField>::const_iterator itr = m_dbcFields.begin(); itr != m_dbcFields.end(); ++itr)
        fieldNames.append(itr->name);

    return fieldNames;
}

QStringList DBCFormat::GetFieldTypes()
{
    QStringList fieldTypes;
    for (QList<DBCField>::const_iterator itr = m_dbcFields.begin(); itr != m_dbcFields.end(); ++itr)
        fieldTypes.append(itr->type);

    return fieldTypes;
}

void DBCFormat::SetFieldAttribute(quint32 field, QString attr, QString value)
{
    if (m_dbcBuild == "Default")
        return;

    // Set in QDocument
    QDomNodeList dbcNodes = m_xmlData.childNodes();

    for (quint32 i = 0; i < dbcNodes.count(); i++)
    {
        QDomNodeList dbcExisted = m_xmlData.elementsByTagName(m_dbcName);
        if (!dbcExisted.isEmpty())
        {
            if (dbcExisted.item(i).toElement().attribute("build") == m_dbcBuild)
            {
                QDomNodeList fieldNodes = m_xmlData.elementsByTagName(m_dbcName).item(i).childNodes();
                fieldNodes.item(field).toElement().setAttribute(attr, value);
                break;
            }
        }
    }

    // Save to file
    QFile xmlFile(m_fileName);
    if (xmlFile.open(QIODevice::WriteOnly))
    {
        QTextStream stream(&xmlFile);
        m_xmlData.save(stream, 0);
        xmlFile.close();
    }
}
