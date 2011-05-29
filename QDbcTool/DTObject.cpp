#include "DTObject.h"
#include "Defines.h"

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QTime>
#include <QtCore/QTextStream>

DTObject::DTObject(DTForm *form)
    : m_form(form)
{
    m_recordCount = 0;
    m_fieldCount = 0;
    m_recordSize = 0;
    m_stringSize = 0;

    m_saveFileName = "";
    m_fileName = "";
    m_build = "";

    m_fieldNames.clear();
    m_fieldTypes.clear();
    m_dbcFormats.clear();

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

void DTObject::LoadFormats()
{
    if (!m_build.isEmpty())
    {
        QFileInfo finfo(m_fileName);

        QFile xmlFile("dbcFormats.xml");
        xmlFile.open(QIODevice::ReadOnly);
        m_dbcFormats.setContent(&xmlFile);

        QDomNodeList dbcNodes = m_dbcFormats.childNodes();

        m_fieldNames.clear();
        m_fieldTypes.clear();

        for (quint32 i = 0; i < dbcNodes.count(); i++)
	    {
            if (m_dbcFormats.elementsByTagName(finfo.baseName()).item(i).toElement().attribute("build") == m_build)
            {
                QDomNodeList fieldNodes = m_dbcFormats.elementsByTagName(finfo.baseName()).item(i).childNodes();
                for (quint32 j = 0; j < fieldNodes.count(); j++)
                {
                    m_fieldTypes.append(fieldNodes.item(j).toElement().attribute("type", "uint"));
                    m_fieldNames.append(fieldNodes.item(j).toElement().attribute("name", QString("Field%0").arg(j+1)));
                }
            }
	    }
    }
}

inline QChar DTObject::GetFieldType(quint32 field)
{
    if (!m_fieldTypes.isEmpty())
        return m_fieldTypes.at(field).at(0);

    return QChar();
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

    // Head bytes
    QByteArray head;
    head = m_file.read(20);

    quint32 m_header;

    m_header        = *reinterpret_cast<quint32*>(head.mid(0, 4).data());
    m_recordCount   = *reinterpret_cast<quint32*>(head.mid(4, 4).data());
    m_fieldCount    = *reinterpret_cast<quint32*>(head.mid(8, 4).data());
    m_recordSize    = *reinterpret_cast<quint32*>(head.mid(12, 4).data());
    m_stringSize    = *reinterpret_cast<quint32*>(head.mid(16, 4).data());

    // Check 'WDBC'
    if (m_header != 0x43424457)
    {
        ThreadUnset(THREAD_OPENFILE);
        return;
    }

    QByteArray  dataBytes;
    QByteArray  stringBytes;
    QStringList recordList;
    quint32 offset = 0;

    // Data bytes
    m_file.seek(20);
    dataBytes = m_file.read(m_recordSize * m_recordCount);

    // String bytes
    m_file.seek(20 + m_recordSize * m_recordCount);
    stringBytes = m_file.read(m_stringSize);

    DBCTableModel* model = new DBCTableModel(m_form, this);
    model->clear();
    model->setFieldNames(m_fieldNames);

    QApplication::postEvent(m_form, new ProgressBar(m_recordCount - 1, BAR_SIZE));

    for (quint32 i = 0; i < m_recordCount; i++)
    {
        recordList.clear();
        for (quint32 j = 0; j < m_fieldCount; j++)
        {
            switch (GetFieldType(j).toAscii())
            {
                case 'u':
                {
                    quint32 value = *reinterpret_cast<quint32*>(dataBytes.mid(offset, 4).data());
                    recordList.append(QString("%0").arg(value));
                    offset += 4;
                }
                break;
                case 'i':
                {
                    qint32 value = *reinterpret_cast<qint32*>(dataBytes.mid(offset, 4).data());
                    recordList.append(QString("%0").arg(value));
                    offset += 4;
                }
                break;
                case 'f':
                {
                    float value = *reinterpret_cast<float*>(dataBytes.mid(offset, 4).data());
                    recordList.append(QString("%0").arg(value));
                    offset += 4;
                }
                break;
                case 's':
                {
                    quint32 value = *reinterpret_cast<quint32*>(dataBytes.mid(offset, 4).data());

                    if (value)
                    {
                        QChar ch(' ');

                        quint32 length = 0;

                        while (!ch.isNull())
                        {
                            ch = stringBytes.at(value + length);
                            if (!ch.isNull())
                                length++;
                        }

                        recordList.append(QString("%0").arg(stringBytes.mid(value, length).data()));
                    }
                    else
                        recordList.append("");

                    offset += 4;
                }
                break;
                default:
                {
                    quint32 value = *reinterpret_cast<quint32*>(dataBytes.mid(offset, 4).data());
                    recordList.append(QString("%0").arg(value));
                    offset += 4;
                }
                break;
            }
        }
        model->appendRecord(recordList);
        QApplication::postEvent(m_form, new ProgressBar(i, BAR_STEP));
    }

    QApplication::postEvent(m_form, new SendModel(m_form, model));

    m_file.close();

    QString stime(QString("Load time (ms): %0").arg(m_time.elapsed()));

    QApplication::postEvent(m_form, new SendText(m_form, 1, stime));

    ThreadUnset(THREAD_OPENFILE);
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

    QString key = m_build + "/" + finfo.fileName();

    QApplication::postEvent(m_form, new ProgressBar(m_recordCount, BAR_SIZE));
    quint32 step = 0;

    for (quint32 f = 0; f < m_fieldCount; f++)
        stream << m_fieldNames.at(f) + ";";

    stream << "\n";

    QList<QStringList> dbcList = model->getDbcList();

    for (quint32 i = 0; i < m_recordCount; i++)
    {
        QStringList dataList = dbcList.at(i);

        for (quint32 j = 0; j < m_fieldCount; j++)
        {
            switch (GetFieldType(j).toAscii())
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

    QString key = m_build + "/" + finfo.fileName();

    QApplication::postEvent(m_form, new ProgressBar(m_fieldCount+m_recordCount, BAR_SIZE));
    quint32 step = 0;

    stream << "CREATE TABLE `" + finfo.baseName() + "_dbc` (\n";
    for (quint32 i = 0; i < m_fieldCount; i++)
    {
        QString endl = i < m_fieldCount-1 ? ",\n" : "\n";
        switch (GetFieldType(i).toAscii())
        {
            case 'u':
            case 'i':
                stream << "\t`" + m_fieldNames.at(i) + "` bigint(20) NOT NULL default '0'" + endl;
                break;
            case 'f':
                stream << "\t`" + m_fieldNames.at(i) + "` float NOT NULL default '0'" + endl;
                break;
            case 's':
                stream << "\t`" + m_fieldNames.at(i) + "` text NOT NULL" + endl;
                break;
            default:
                stream << "\t`" + m_fieldNames.at(i) + "` bigint(20) NOT NULL default '0'" + endl;
                break;
        }
        step++;
        QApplication::postEvent(m_form, new ProgressBar(step, BAR_STEP));
    }
    stream << ") ENGINE = MyISAM DEFAULT CHARSET = utf8 COMMENT = 'Data from " + finfo.fileName() + "';\n\n";

    QList<QStringList> dbcList = model->getDbcList();
    for (quint32 i = 0; i < m_recordCount; i++)
    {
        stream << "INSERT INTO `" + finfo.baseName() + "_dbc` (";
        for (quint32 f = 0; f < m_fieldCount; f++)
        {
            QString endl = f < m_fieldCount-1 ? "`, " : "`) VALUES (";
            stream << "`" + m_fieldNames.at(f) + endl;
        }
        QStringList dataList = dbcList.at(i);

        for (quint32 d = 0; d < dataList.size(); d++)
        {
            if (dataList.at(d).contains("'"))
            {
                QString data = dataList.at(d);
                data.replace("'", "\\'");
                dataList.replace(d, data);
            }
        }

        for (quint32 j = 0; j < m_fieldCount; j++)
        {
            if (j < m_fieldCount-1)
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