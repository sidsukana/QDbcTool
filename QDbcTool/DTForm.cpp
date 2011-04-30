#include "DTForm.h"

#include <QtCore/QFile>
#include <QtCore/QDataStream>
#include <QtGui/QTableView>
#include <QtGui/QFileDialog>

DTForm::DTForm(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi(this);

    m_recordCount = 0;
    m_fieldCount = 0;
    m_recordSize = 0;
    m_stringSize = 0;

    model = new DBCTableModel(this, this);

    connect(actionOpen, SIGNAL(triggered()), this, SLOT(SlotOpenFile()));
}

DTForm::~DTForm()
{
}

void DTForm::ThreadBegin(quint8 id)
{
    if (model)
    {
        // don't know how clear...
        //model->removeRows(0, m_recordCount);
        //model->removeColumns(0, m_fieldCount);
        model->getMap().clear();
    }

    // WTF sometimes existed...
    //if (!ThreadExist(id))
    //{
        filename = QFileDialog::getOpenFileName();
        TObject *thread = new TObject(id, this);
        thread->start();
    //}
}

void DTForm::SlotOpenFile()
{
    ThreadBegin(THREAD_OPENFILE);
}

char DTForm::GetColumnFormat(quint32 field)
{
    // debug Spell.dbc
    char* format = "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiifiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiffffffiiiiiiiiiiiiiiiiiiiiifffiiiiiiiiiiiifffiiiiissssssssissssssssissssssssissssssssiiiiiiiiiiiffffiii";
    for (quint32 i = 0; i < strlen(format); i++)
        if (i == field)
            return format[i];

    return '0';
}

void DTForm::GenerateTable()
{
    ThreadSet(THREAD_OPENFILE);

    quint32 header;

    QFile file(filename);
        
    if (!file.open(QIODevice::ReadOnly))
        return;

    QDataStream stream(&file);
    //stream.setByteOrder(QDataStream::ByteOrder(QSW_ENDIAN));
    stream.setByteOrder(QDataStream::ByteOrder(1));

    stream >> header >> m_recordCount >> m_fieldCount >> m_recordSize >> m_stringSize;

    // Check 'WDBC'
    if (header != 0x43424457)
        return; 

    model->insertRows(0, m_recordCount);
    model->insertColumns(0, m_fieldCount);
    
    quint32 offset = 20;
    quint32 strBegin = m_recordSize * m_recordCount + 20;
    QByteArray bytes;

    for (quint32 i = 0; i < m_recordCount; i++)
    {
        for (quint32 j = 0; j < m_fieldCount; j++)
        {
            switch (GetColumnFormat(j))
            {
                case 'i':
                {
                    file.seek(offset);
                    bytes = file.read(sizeof(quint32));
                    quint32 value = *reinterpret_cast<quint32*>(bytes.data());
                    QString data = QString("%0").arg(value);
                    QModelIndex index = model->index(i, j);
                    model->setData(index, data, Qt::EditRole);
                    offset += sizeof(quint32);
                }
                break;
                case 'f':
                {
                    file.seek(offset);
                    bytes = file.read(sizeof(float));
                    float value = *reinterpret_cast<float*>(bytes.data());
                    QString data = QString("%0").arg(value);
                    QModelIndex index = model->index(i, j);
                    model->setData(index, data, Qt::EditRole);
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
                        }
                        else
                        {
                            QString data = QString("");
                            QModelIndex index = model->index(i, j);
                            model->setData(index, data, Qt::EditRole);
                            offset += sizeof(char*);
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
                        }
                        else
                        {
                            QString data = QString("");
                            QModelIndex index = model->index(i, j);
                            model->setData(index, data, Qt::EditRole);
                            offset += sizeof(char*);
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
                    QModelIndex index = model->index(i, j);
                    model->setData(index, data, Qt::EditRole);
                    offset += sizeof(quint32);
                }
                break;
            }
        }
    }

    QApplication::postEvent(this, new SendModel(this, model));

    file.close();

    ThreadUnset(THREAD_OPENFILE);
}

bool DTForm::event(QEvent *ev)
{
    switch (ev->type())
    {
        case SendModel::TypeId:
        {
            SendModel* m_ev = (SendModel*)ev;
            tableView->setModel(m_ev->GetObject());
            return true;
        }
        break;
        default:
            break;
    }
    

    return QWidget::event(ev);
}

DBCTableModel::DBCTableModel(QObject *parent, DTForm *form)
    : QAbstractTableModel(parent), m_form(form)
{
}

DBCTableModel::DBCTableModel(QMap<quint32, QMap<quint32, QString>> data, QObject *parent, DTForm *form)
    : QAbstractTableModel(parent), m_form(form)
{
    dataMap = data;
}

int DBCTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_form->GetRecordCount();
}

int DBCTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_form->GetFieldCount();
}

QVariant DBCTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (index.row() >= dataMap.size() || index.row() < 0)
        return QVariant();

    if (role == Qt::DisplayRole)
    {
        QMap<quint32, QString> fmap = *dataMap.find(index.row());

        return fmap.find(index.column()).value();
    }
    return QVariant();
}

/*QVariant DBCTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal) {
        switch (section) {
            case 0:
                return tr("Name");

            case 1:
                return tr("Address");

            default:
                return QVariant();
        }
    }
    return QVariant();
}*/ 

bool DBCTableModel::insertRows(int position, int rows, const QModelIndex &index)
{
    Q_UNUSED(index);
    beginInsertRows(QModelIndex(), position, position+rows-1);

    QMap<quint32, QString> fmap;
    for (quint32 row = 0; row < rows; row++)
    {
        for (quint32 col = 0; col < m_form->GetFieldCount(); col++)
            fmap.insert(col, " ");

        dataMap.insert(row, fmap);
    }

    endInsertRows();
    return true;
}

/*bool DBCTableModel::insertColumns(int position, int columns, const QModelIndex &index)
{
    Q_UNUSED(index);
    beginInsertColumns(QModelIndex(), position, position+columns-1);
    endInsertColumns();
    return true;
}

bool DBCTableModel::removeColumns(int position, int columns, const QModelIndex &index)
{
    Q_UNUSED(index);
    beginRemoveColumns(QModelIndex(), position, position+columns-1);
    endRemoveColumns();
    return true;
}*/

bool DBCTableModel::removeRows(int position, int rows, const QModelIndex &index)
{
    Q_UNUSED(index);
    beginRemoveRows(QModelIndex(), position, position+rows-1);
    endRemoveRows();
    return true;
}

bool DBCTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.isValid() && role == Qt::EditRole)
    {
        quint32 row = index.row();
        quint32 column = index.column();

        QMap<quint32, QString> fmap = *dataMap.find(row);

        fmap.remove(column);
        fmap.insert(column, value.toString());
        dataMap.remove(row);
        dataMap.insert(row, fmap);

        emit(dataChanged(index, index));

        return true;
    }

    return false;
}

QMap< quint32, QMap<quint32, QString> > DBCTableModel::getMap()
{
    return dataMap;
}

Qt::ItemFlags DBCTableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;

    return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
}