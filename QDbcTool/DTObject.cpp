#include "DTObject.h"
#include "Defines.h"

#include <QtGui/QFileDialog>

DTObject::DTObject(DTForm *form)
    : m_form(form)
{
    m_recordCount = 0;
    m_fieldCount = 0;
    m_recordSize = 0;
    m_stringSize = 0;

    m_fileName = "";

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

QChar DTObject::GetColumnFormat(quint32 field)
{
    // debug Spell.dbc
    // iiiiiiiiiiiiiii SkillLineAbility.dbc
    // iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiifiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiffffffiiiiiiiiiiiiiiiiiiiiifffiiiiiiiiiiiifffiiiiissssssssissssssssissssssssissssssssiiiiiiiiiiiffffiii
    QString format = QString("iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiifiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiffffffiiiiiiiiiiiiiiiiiiiiifffiiiiiiiiiiiifffiiiiissssssssissssssssissssssssissssssssiiiiiiiiiiiffffiii");
    
    if (!format.isEmpty())
        return format.at(field);

    return QChar();
}

void DTObject::Load()
{
    ThreadSet(THREAD_OPENFILE);

    quint32 header;

    m_fileName = QFileDialog::getOpenFileName();
    QFile file(m_fileName);
        
    if (!file.open(QIODevice::ReadOnly))
        return;

    QDataStream stream(&file);
    //stream.setByteOrder(QDataStream::ByteOrder(QSW_ENDIAN));
    stream.setByteOrder(QDataStream::ByteOrder(1));

    stream >> header >> m_recordCount >> m_fieldCount >> m_recordSize >> m_stringSize;

    // Check 'WDBC'
    if (header != 0x43424457)
        return; 

    DBCTableModel* model = new DBCTableModel(m_form, this);

    model->insertRows(0, m_recordCount);
    model->insertColumns(0, m_fieldCount);

    quint32 offset = 20;
    quint32 strBegin = m_recordSize * m_recordCount + 20;
    QByteArray bytes;

    quint32 step = 0;

    QApplication::postEvent(m_form, new ProgressBar(m_fieldCount * m_recordCount, BAR_SIZE));
    QApplication::postEvent(m_form, new ProgressBar(step, BAR_STEP));

    for (quint32 i = 0; i < m_recordCount; i++)
    {
        for (quint32 j = 0; j < m_fieldCount; j++)
        {
            switch (GetColumnFormat(j).toAscii())
            {
                case 'i':
                {
                    step++;
                    file.seek(offset);
                    bytes = file.read(sizeof(quint32));
                    quint32 value = *reinterpret_cast<quint32*>(bytes.data());
                    QString data = QString("%0").arg(value);
                    QModelIndex index = model->index(i, j);
                    model->setData(index, data, Qt::EditRole);
                    offset += sizeof(quint32);
                    QApplication::postEvent(m_form, new ProgressBar(step, BAR_STEP));
                }
                break;
                case 'f':
                {
                    step++;
                    file.seek(offset);
                    bytes = file.read(sizeof(float));
                    float value = *reinterpret_cast<float*>(bytes.data());
                    QString data = QString("%0").arg(value);
                    QModelIndex index = model->index(i, j);
                    model->setData(index, data, Qt::EditRole);
                    offset += sizeof(float);
                    QApplication::postEvent(m_form, new ProgressBar(step, BAR_STEP));
                }
                break;
                case 's':
                {
                    step++;
                    file.seek(offset);
                    bytes = file.read(sizeof(quint32));
                    quint32 value = *reinterpret_cast<quint32*>(bytes.data());

                    if (i < m_recordCount-1)
                    {
                        if (value != 0)
                        {
                            quint32 value2 = 0;

                            quint32 nextOffset = offset;

                            while (value2 == 0)
                            {
                                nextOffset += m_recordSize;
                                
                                file.seek(nextOffset);
                                bytes = file.read(sizeof(quint32));
                                value2 = *reinterpret_cast<quint32*>(bytes.data());
                            }

                            file.seek(strBegin + value);
                            bytes = file.read(value2 - 2);

                            QString data = QString("%0").arg(bytes.data());
                            QModelIndex index = model->index(i, j);
                            model->setData(index, data, Qt::EditRole);
                            offset += sizeof(char*);
                            QApplication::postEvent(m_form, new ProgressBar(step, BAR_STEP));
                        }
                        else
                        {
                            QString data = QString("");
                            QModelIndex index = model->index(i, j);
                            model->setData(index, data, Qt::EditRole);
                            offset += sizeof(char*);
                            QApplication::postEvent(m_form, new ProgressBar(step, BAR_STEP));
                        }
                    }
                    else
                    {
                        if (value != 0)
                        {
                            file.seek(strBegin + value);
                            bytes = file.read(m_stringSize - value - 1);

                            QString data = QString("%0").arg(bytes.data());
                            QModelIndex index = model->index(i, j);
                            model->setData(index, data, Qt::EditRole);
                            offset += sizeof(char*);
                            QApplication::postEvent(m_form, new ProgressBar(step, BAR_STEP));
                        }
                        else
                        {
                            QString data = QString("");
                            QModelIndex index = model->index(i, j);
                            model->setData(index, data, Qt::EditRole);
                            offset += sizeof(char*);
                            QApplication::postEvent(m_form, new ProgressBar(step, BAR_STEP));
                        }
                    }
                }
                break;
                default:
                {
                    step++;
                    file.seek(offset);
                    bytes = file.read(sizeof(quint32));
                    quint32 value = *reinterpret_cast<quint32*>(bytes.data());
                    QString data = QString("%0").arg(value);
                    QModelIndex index = model->index(i, j);
                    model->setData(index, data, Qt::EditRole);
                    offset += sizeof(quint32);
                    QApplication::postEvent(m_form, new ProgressBar(step, BAR_STEP));
                }
                break;
            }
        }
    }

    QApplication::postEvent(m_form, new SendModel(m_form, model));

    file.close();

    ThreadUnset(THREAD_OPENFILE);
}