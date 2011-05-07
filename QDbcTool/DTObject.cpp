#include "DTObject.h"
#include "Defines.h"

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QTime>

DTObject::DTObject(DTForm *form)
    : m_form(form)
{
    m_recordCount = 0;
    m_fieldCount = 0;
    m_recordSize = 0;
    m_stringSize = 0;

    m_fileName = "";

    m_build = "";

    model = new DBCTableModel(m_form, this);
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
    //QTime m_time;
    //m_time.start();

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

    model->clear();

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

    QApplication::postEvent(m_form, new ProgressBar(m_recordCount - 1, BAR_SIZE));

    for (quint32 i = 0; i < m_recordCount; i++)
    {
        strl.clear();
        for (quint32 j = 0; j < m_fieldCount; j++)
        {
            switch (GetColumnFormat(j).toAscii())
            {
                case 'i':
                {
                    quint32 value = *reinterpret_cast<quint32*>(dataBytes.mid(offset, 4).data());
                    QString data = QString("%0").arg(value);
                    strl.append(data);
                    offset += 4;
                }
                break;
                case 'f':
                {
                    quint32 value = *reinterpret_cast<float*>(dataBytes.mid(offset, 4).data());
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

    //QString stime(QString("Load time (ms): %0").arg(m_time.elapsed()));

    ThreadUnset(THREAD_OPENFILE);
}