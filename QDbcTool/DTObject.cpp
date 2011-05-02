#include "DTObject.h"
#include "Defines.h"

#include <QtCore/QFile>

DTObject::DTObject(DTForm *form)
    : m_form(form)
{
    m_recordCount = 0;
    m_fieldCount = 0;
    m_recordSize = 0;
    m_stringSize = 0;

    m_fileName = "";

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
    // debug Spell.dbc
    // iiiiiiiiiiiiiii SkillLineAbility.dbc
    // iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiifiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiffffffiiiiiiiiiiiiiiiiiiiiifffiiiiiiiiiiiifffiiiiissssssssissssssssissssssssissssssssiiiiiiiiiiiffffiii
    m_format = config->value("1.12.x/Format", "None").toString();
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

    quint32 header;

    QFile file(m_fileName);
        
    if (!file.open(QIODevice::ReadOnly))
    {
        ThreadUnset(THREAD_OPENFILE);
        return;
    }

    QDataStream stream(&file);
    //stream.setByteOrder(QDataStream::ByteOrder(QSW_ENDIAN));
    stream.setByteOrder(QDataStream::ByteOrder(1));

    stream >> header >> m_recordCount >> m_fieldCount >> m_recordSize >> m_stringSize;

    // Check 'WDBC'
    if (header != 0x43424457)
    {
        ThreadUnset(THREAD_OPENFILE);
        return;
    }

    model->clear();

    QByteArray bytes;
    QStringList strl;

    quint32 offset = 20;
    quint32 strBegin = m_recordSize * m_recordCount + 20;
    

    QApplication::postEvent(m_form, new ProgressBar(m_recordCount-1, BAR_SIZE));

    for (quint32 i = 0; i < m_recordCount; i++)
    {
        strl.clear();
        for (quint32 j = 0; j < m_fieldCount; j++)
        {
            switch (GetColumnFormat(j).toAscii())
            {
                case 'i':
                {
                    file.seek(offset);
                    bytes = file.read(sizeof(quint32));
                    quint32 value = *reinterpret_cast<quint32*>(bytes.data());
                    QString data = QString("%0").arg(value);
                    strl.append(data);
                    offset += sizeof(quint32);
                }
                break;
                case 'f':
                {
                    file.seek(offset);
                    bytes = file.read(sizeof(float));
                    float value = *reinterpret_cast<float*>(bytes.data());
                    QString data = QString("%0").arg(value);
                    strl.append(data);
                    offset += sizeof(float);
                }
                break;
                case 's':
                {
                    file.seek(offset);
                    bytes = file.read(sizeof(quint32));
                    quint32 value = *reinterpret_cast<quint32*>(bytes.data());

                    if (i < m_recordCount-1)
                    {
                        if (value != 0)
                        {
                            quint32 value2 = 0;

                            quint32 safeOffset = i;

                            quint32 nextOffset = offset;

                            while (value2 == 0)
                            {
                                safeOffset++;

                                if (safeOffset < m_recordCount)
                                {
                                    nextOffset += m_recordSize;

                                    file.seek(nextOffset);
                                    bytes = file.read(sizeof(quint32));
                                    value2 = *reinterpret_cast<quint32*>(bytes.data());
                                }
                                else
                                {
                                    value2 = 0;
                                    break;
                                }
                            }

                            if (value2 != 0)
                            {
                                file.seek(strBegin + value);
                                bytes = file.read(value2 - 2);
                                QString data = QString("%0").arg(bytes.data());
                                strl.append(data);
                                offset += sizeof(char*);
                            }
                            else
                            {
                                QString data = QString("");
                                offset += sizeof(char*);
                                strl.append(data);
                            }
                        }
                        else
                        {
                            QString data = QString("");
                            offset += sizeof(char*);
                            strl.append(data);
                        }
                    }
                    else
                    {
                        if (value != 0)
                        {
                            file.seek(strBegin + value);
                            bytes = file.read(m_stringSize - value - 1);
                            QString data = QString("%0").arg(bytes.data());
                            strl.append(data);
                            offset += sizeof(char*);
                        }
                        else
                        {
                            QString data = QString("");
                            offset += sizeof(char*);
                            strl.append(data);
                        }
                    }
                }
                break;
                default:
                {
                    file.seek(offset);
                    bytes = file.read(sizeof(quint32));
                    quint32 value = *reinterpret_cast<quint32*>(bytes.data());
                    QString data = QString("%0").arg(value);
                    strl.append(data);
                    offset += sizeof(quint32);
                }
                break;
            }
        }
        model->insertRecord(strl);
        QApplication::postEvent(m_form, new ProgressBar(i, BAR_STEP));
    }

    QApplication::postEvent(m_form, new SendModel(m_form, model));

    file.close();

    ThreadUnset(THREAD_OPENFILE);
}