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

    m_fieldsNames.clear();

    config = new QSettings("config.ini", QSettings::IniFormat, m_form);

    LoadConfig();

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

void DTObject::LoadConfig()
{
    config->sync();

    if (!m_build.isEmpty())
    {
        QFileInfo finfo(m_fileName);

        QString key = m_build + "/" + finfo.fileName();
        m_format = config->value(key + "/Format", "None").toString();

        m_fieldsNames.clear();
        for (quint32 i = 0; i < m_format.length(); i++)
        {
            QString fieldName(QString("%0/Field%1").arg(key).arg(i+1));
            m_fieldsNames.append(config->value(fieldName, QString("Field%0").arg(i+1)).toString());
        }
    }
}

QChar DTObject::GetColumnFormat(quint32 field)
{
    if (!m_format.isEmpty())
        return m_format.at(field);

    return QChar();
}

void DTObject::Load()
{
    ThreadSet(THREAD_OPENFILE);

    // Timer
    QTime m_time;
    m_time.start();

    QFile m_file(m_fileName);

    quint32 m_header;
        
    if (!m_file.open(QIODevice::ReadOnly))
    {
        ThreadUnset(THREAD_OPENFILE);
        return;
    }

    // Head bytes
    QByteArray head;
    head = m_file.read(20);

    m_header = *reinterpret_cast<quint32*>(head.mid(0, 4).data());
    m_recordCount = *reinterpret_cast<quint32*>(head.mid(4, 4).data());
    m_fieldCount = *reinterpret_cast<quint32*>(head.mid(8, 4).data());
    m_recordSize = *reinterpret_cast<quint32*>(head.mid(12, 4).data());
    m_stringSize = *reinterpret_cast<quint32*>(head.mid(16, 4).data());

    // Check 'WDBC'
    if (m_header != 0x43424457)
    {
        ThreadUnset(THREAD_OPENFILE);
        return;
    }

    QByteArray dataBytes;
    QByteArray stringBytes;

    QStringList strl;

    quint32 offset = 0;

    // Data bytes
    m_file.seek(20);
    dataBytes = m_file.read(m_recordSize * m_recordCount);

    // String bytes
    m_file.seek(20 + m_recordSize * m_recordCount);
    stringBytes = m_file.read(m_stringSize);

    DBCTableModel* model = new DBCTableModel(m_form, this);
    model->clear();
    model->setFieldsNames(m_fieldsNames);

    QApplication::postEvent(m_form, new ProgressBar(m_recordCount - 1, BAR_SIZE));

    for (quint32 i = 0; i < m_recordCount; i++)
    {
        strl.clear();
        for (quint32 j = 0; j < m_fieldCount; j++)
        {
            switch (GetColumnFormat(j).toAscii())
            {
                case 'u':
                {
                    quint32 value = *reinterpret_cast<quint32*>(dataBytes.mid(offset, 4).data());
                    QString data = QString("%0").arg(value);
                    strl.append(data);
                    offset += 4;
                }
                break;
                case 'i':
                {
                    qint32 value = *reinterpret_cast<qint32*>(dataBytes.mid(offset, 4).data());
                    QString data = QString("%0").arg(value);
                    strl.append(data);
                    offset += 4;
                }
                break;
                case 'f':
                {
                    float value = *reinterpret_cast<float*>(dataBytes.mid(offset, 4).data());
                    QString data = QString("%0").arg(value);
                    strl.append(data);
                    offset += 4;
                }
                break;
                case 's':
                {
                    quint32 value = *reinterpret_cast<quint32*>(dataBytes.mid(offset, 4).data());

                    if (value)
                    {
                        char* ch = new char[1];

                        quint32 length = 0;

                        while (ch[0] != 0)
                        {
                            ch[0] = stringBytes.at(value+length);
                            if (ch[0] != 0)
                                length++;
                        }

                        QString data = QString("%0").arg(stringBytes.mid(value, length).data());
                        strl.append(data);
                    }
                    else
                    {
                        QString data = QString("");
                        strl.append(data);
                    }

                    offset += 4;
                }
                break;
                default:
                {
                    quint32 value = *reinterpret_cast<quint32*>(dataBytes.mid(offset, 4).data());
                    QString data = QString("%0").arg(value);
                    strl.append(data);
                    offset += 4;
                }
                break;
            }
        }
        model->insertRecord(strl);
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

    DBCTableModel* model = static_cast<DBCTableModel*>(m_form->tableView->model());
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
        stream << m_fieldsNames.at(f) + ";";

    stream << "\n";

    QList<QStringList> dbcList = model->getDbcList();

    for (quint32 i = 0; i < m_recordCount; i++)
    {
        QStringList dataList = dbcList.at(i);

        for (quint32 j = 0; j < m_fieldCount; j++)
        {
            switch (GetColumnFormat(j).toAscii())
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

    DBCTableModel* model = static_cast<DBCTableModel*>(m_form->tableView->model());
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
        switch (GetColumnFormat(i).toAscii())
        {
            case 'u':
            case 'i':
                stream << "\t`" + m_fieldsNames.at(i) + "` bigint(20) NOT NULL default '0'" + endl;
                break;
            case 'f':
                stream << "\t`" + m_fieldsNames.at(i) + "` float NOT NULL default '0'" + endl;
                break;
            case 's':
                stream << "\t`" + m_fieldsNames.at(i) + "` text NOT NULL" + endl;
                break;
            default:
                stream << "\t`" + m_fieldsNames.at(i) + "` bigint(20) NOT NULL default '0'" + endl;
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
            stream << "`" + m_fieldsNames.at(f) + endl;
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