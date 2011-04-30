#include "DTForm.h"

#include <QtCore/QFile>
#include <QtCore/QDataStream>
#include <QtGui/QTableView>

DTForm::DTForm(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi(this);

    dbc = new DTObject(this);

    connect(actionOpen, SIGNAL(triggered()), this, SLOT(SlotOpenFile()));
}

DTForm::~DTForm()
{
}

void DTForm::SlotOpenFile()
{
    dbc->ThreadBegin(THREAD_OPENFILE);
}

bool DTForm::event(QEvent *ev)
{
    switch (ev->type())
    {
        case ProgressBar::TypeId:
        {
            ProgressBar* bar = (ProgressBar*)ev;
            
            switch (bar->GetId())
            {
                case BAR_STEP:
                {
                    progressBar->setValue(bar->GetStep());
                    return true;
                }
                case BAR_SIZE:
                {
                    progressBar->setMaximum(bar->GetSize());
                    return true;
                }
            }

            break;
        }
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

DBCTableModel::DBCTableModel(QObject *parent, DTObject *dbc)
    : QAbstractTableModel(parent), m_dbc(dbc)
{
}

DBCTableModel::DBCTableModel(QMap<quint32, QMap<quint32, QString>> data, QObject *parent, DTObject *dbc)
    : QAbstractTableModel(parent), m_dbc(dbc)
{
    dataMap = data;
}

int DBCTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_dbc->GetRecordCount();
}

int DBCTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_dbc->GetFieldCount();
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
        for (quint32 col = 0; col < m_dbc->GetFieldCount(); col++)
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