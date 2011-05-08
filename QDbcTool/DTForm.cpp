#include "DTForm.h"
#include <QtCore/QDataStream>
#include <QtGui/QTableView>
#include <QtGui/QFileDialog>

DTForm::DTForm(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi(this);

    dbc = new DTObject(this);

    statusBar = new QStatusBar(this);
    setStatusBar(statusBar);
    statusBar->showMessage("Ready");

    connect(actionOpen, SIGNAL(triggered()), this, SLOT(SlotOpenFile()));
}

DTForm::~DTForm()
{
}

DTBuild::DTBuild(QWidget *parent, DTForm* form)
    : QDialog(parent), m_form(form)
{
    setupUi(this);
}

DTBuild::~DTBuild()
{
}

void DTForm::SlotOpenFile()
{
    dbc->SetFileName(QFileDialog::getOpenFileName());

    DTBuild* build = new DTBuild;
    build->show();
    build->comboBox->clear();
    build->comboBox->addItems(dbc->GetConfig()->childGroups());

    if (build->exec() == QDialog::Accepted)
    {
        dbc->SetBuild(build->comboBox->currentText());
        dbc->LoadConfig();
        dbc->ThreadBegin(THREAD_OPENFILE);
    }
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
        case SendText::TypeId:
        {
            SendText* m_ev = (SendText*)ev;
            switch (m_ev->GetId())
            {
                case 1:
                    statusBar->showMessage(m_ev->GetText());
                    break;
                default:
                    break;
            }
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
    m_dbcList.clear();
}

DBCTableModel::DBCTableModel(DBCList dbcList, QObject *parent, DTObject *dbc)
    : QAbstractTableModel(parent), m_dbc(dbc)
{
    m_dbcList = dbcList;
}

void DBCTableModel::clear()
{
    beginResetModel();
    m_dbcList.clear();
    m_fieldsNames.clear();
    endResetModel();
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

    if (index.row() >= m_dbcList.size() || index.row() < 0)
        return QVariant();

    if (role == Qt::DisplayRole)
        return m_dbcList.at(index.row()).at(index.column());

    return QVariant();
}

QVariant DBCTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal)
        return m_fieldsNames.at(section);

    return QVariant();
}

void DBCTableModel::setFieldsNames(QStringList strl)
{
    m_fieldsNames = strl;
}

void DBCTableModel::insertRecord(QStringList strl)
{
    m_dbcList.append(strl);
}

DBCList DBCTableModel::getDbcList()
{
    return m_dbcList;
}

Qt::ItemFlags DBCTableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;

    return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
}