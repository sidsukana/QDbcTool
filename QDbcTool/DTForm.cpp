#include "DTForm.h"
#include <QtCore/QDataStream>
#include <QtGui/QTableView>
#include <QtGui/QFileDialog>

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
    dbc->SetFileName(QFileDialog::getOpenFileName());
    dbc->LoadConfig();
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
    m_dbcMap.clear();
    m_fieldMap.clear();
}

DBCTableModel::DBCTableModel(DbcMap dbcMap, QObject *parent, DTObject *dbc)
    : QAbstractTableModel(parent), m_dbc(dbc)
{
    m_dbcMap = dbcMap;
    m_fieldMap.clear();
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

    if (index.row() >= m_dbcMap.size() || index.row() < 0)
        return QVariant();

    if (role == Qt::DisplayRole)
    {
        FieldMap fmap = m_dbcMap[index.row()];

        return fmap[index.column()];
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

    for (quint32 row = 0; row < rows; row++)
    {
        m_fieldMap.clear();
        for (quint32 col = 0; col < m_dbc->GetFieldCount(); col++)
            m_fieldMap.insert(col, " ");

        m_dbcMap.insert(row, m_fieldMap);
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

        m_fieldMap = m_dbcMap[row];
        m_fieldMap[column] = value.toString();

        m_dbcMap[row] = m_fieldMap;

        emit(dataChanged(index, index));

        return true;
    }

    return false;
}

DbcMap DBCTableModel::getMap()
{
    return m_dbcMap;
}

Qt::ItemFlags DBCTableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;

    return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
}