#include <QtConcurrent/QtConcurrentRun>
#include <QFuture>

#include "MainForm.h"
#include <QDataStream>
#include <QTableView>
#include <QFileDialog>
#include <QSortFilterProxyModel>
#include <QAbstractItemModel>

#include "Alphanum.h"

MainForm::MainForm(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi(this);

    format = new DBCFormat("format.json");
    dbc = new DTObject(this, format);

    fieldBox = new QToolButton(this);
    fieldBox->setText("Fields");
    fieldBox->setPopupMode(QToolButton::InstantPopup);
    fieldBox->setFixedSize(50, 20);

    progressBar = new QProgressBar(this);
    progressBar->setValue(0);
    progressBar->setTextVisible(false);
    progressBar->setTextDirection(QProgressBar::TopToBottom);
    progressBar->setFixedSize(100, 20);

    dbcInfo = new QLabel(this);
    statusText = new QLabel("Ready!", this);
    fontComboBox->clear();

    mainToolBar->addWidget(progressBar);
    mainToolBar->addSeparator();
    mainToolBar->addWidget(fieldBox);
    mainToolBar->addSeparator();
    mainToolBar->addWidget(dbcInfo);
    mainToolBar->addSeparator();
    mainToolBar->addWidget(statusText);

    proxyModel = new DBCSortedModel(this);
    proxyModel->setDynamicSortFilter(true);
    tableView->setModel(proxyModel);

    connect(dbc, SIGNAL(loadingStart(quint32)), this, SLOT(slotLoadingStart(quint32)));
    connect(dbc, SIGNAL(loadingStep(quint32)), this, SLOT(slotLoadingStep(quint32)));
    connect(dbc, SIGNAL(loadingNote(QString)), this, SLOT(slotLoadingNote(QString)));
    connect(dbc, SIGNAL(loadingDone(QAbstractItemModel*)), this, SLOT(slotFileLoaded(QAbstractItemModel*)));
    connect(dbc, SIGNAL(searchDone(QList<bool>)), this, SLOT(slotSearchDone(QList<bool>)));

    connect(actionOpen, SIGNAL(triggered()), this, SLOT(slotOpenFile()));

    // Export actions
    connect(actionExport_as_JSON, SIGNAL(triggered()), this, SLOT(slotExportAsJSON()));
    connect(actionExport_as_SQL, SIGNAL(triggered()), this, SLOT(slotExportAsSQL()));
    connect(actionExport_as_CSV, SIGNAL(triggered()), this, SLOT(slotExportAsCSV()));

    connect(actionWrite_DBC, SIGNAL(triggered()), this, SLOT(slotWriteDBC()));

    connect(lineEdit, SIGNAL(returnPressed()), this, SLOT(slotSearch()));

    connect(tableView, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(SlotCustomContextMenu(const QPoint&)));

    connect(actionAbout, SIGNAL(triggered()), this, SLOT(slotAbout()));
}

void MainForm::slotSearch()
{
    if (!_watcher.isRunning())
        _watcher.setFuture(QtConcurrent::run(dbc, &DTObject::search));
}

void MainForm::slotFileLoaded(QAbstractItemModel *model)
{
    proxyModel->setSourceModel(model);
    fontComboBox->clear();
    fontComboBox->addItems(format->getFieldNames());
    applyFilter();
}

void MainForm::slotLoadingNote(QString note)
{
    statusText->setText(note);
}

void MainForm::slotLoadingStep(quint32 step)
{
    progressBar->setValue(step);
}

void MainForm::slotLoadingStart(quint32 size)
{
    progressBar->setMaximum(size);
}

void MainForm::slotSearchDone(QList<bool> rowStates)
{
    quint32 i = 0;
    for (bool state : rowStates)
    {
        tableView->setRowHidden(i++, state);
    }

}

bool DBCTableModel::insertRows(int position, int rows, const QModelIndex &index)
{
    Q_UNUSED(index);
    beginInsertRows(QModelIndex(), position, position+rows-1);

    endInsertRows();
    return true;
}

bool DBCTableModel::removeRows(int position, int rows, const QModelIndex &index)
{
    Q_UNUSED(index);
    beginRemoveRows(QModelIndex(), position, position + rows - 1);

    for (int row = 0; row < rows; ++row)
        m_dbcList.removeAt(position);

    m_dbc->setRecordCount(m_dbcList.size());

    endRemoveRows();
    
    return true;
}

void MainForm::applyFilter()
{
    qDeleteAll(fieldBox->actions());

    for (quint32 i = 0; i < dbc->getFieldCount(); i++)
    {
        if (format->isHiden(i))
            tableView->hideColumn(i);

        QAction* action = new QAction(this);
        action->setText(format->getFieldName(i));
        action->setCheckable(true);
        action->setChecked(format->isHiden(i));
        action->setData(i);

        fieldBox->addAction(action);
    }
    connect(fieldBox, SIGNAL(triggered(QAction*)), this, SLOT(slotSetVisible(QAction*)));
}

void MainForm::slotSetVisible(QAction* action)
{
    if (action->isChecked())
    {
        format->setFieldAttribute(action->data().toUInt(), "visible", "false");
        tableView->hideColumn(action->data().toUInt());
    }
    else
    {
        format->setFieldAttribute(action->data().toUInt(), "visible", "true");
        tableView->showColumn(action->data().toUInt());
    }
}

DBCSortedModel::DBCSortedModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
}

bool DBCSortedModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    QVariant leftData = sourceModel()->data(left);
    QVariant rightData = sourceModel()->data(right);

    if (compare(leftData.toString(), rightData.toString()) < 0)
        return true;

    return false;
}

MainForm::~MainForm()
{
}


DTBuild::DTBuild(QWidget *parent, MainForm* form)
    : QDialog(parent), m_form(form)
{
    setupUi(this);
    show();
}

DTBuild::~DTBuild()
{
}

AboutForm::AboutForm(QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);
    show();
}

AboutForm::~AboutForm()
{
}

void MainForm::slotAbout()
{
    new AboutForm;
}

void MainForm::slotExportAsJSON()
{
    if (!dbc->isEmpty())
    {
        QString fileName = QFileDialog::getSaveFileName(this, "Save as JSON file", ".", "JSON File (*.json)");

        if (!fileName.isEmpty())
        {
            dbc->setSaveFileName(fileName);
            statusText->setText("Exporting to JSON file...");
            if (!_watcher.isRunning())
                _watcher.setFuture(QtConcurrent::run(dbc, &DTObject::exportAsJSON));
        }
    }
    else
        statusText->setText("DBC not loaded! Please load DBC file!");
}

void MainForm::slotExportAsSQL()
{
    if (!dbc->isEmpty())
    {
        QString fileName = QFileDialog::getSaveFileName(this, "Save as SQL file", ".", "SQL File (*.sql)");

        if (!fileName.isEmpty())
        {
            dbc->setSaveFileName(fileName);
            statusText->setText("Exporting to SQL file...");
            if (!_watcher.isRunning())
                _watcher.setFuture(QtConcurrent::run(dbc, &DTObject::exportAsSQL));
        }
    }
    else
        statusText->setText("DBC not loaded! Please load DBC file!");

}

void MainForm::slotExportAsCSV()
{
    if (!dbc->isEmpty())
    {
        QString fileName = QFileDialog::getSaveFileName(this, "Save as CSV file", ".", "CSV File (*.csv)");

        if (!fileName.isEmpty())
        {
            dbc->setSaveFileName(fileName);
            statusText->setText("Exporting to CSV file...");
            if (!_watcher.isRunning())
                _watcher.setFuture(QtConcurrent::run(dbc, &DTObject::exportAsCSV));
        }
    }
    else
        statusText->setText("DBC not loaded! Please load DBC file!");

}

void MainForm::slotWriteDBC()
{
    if (!dbc->isEmpty())
    {
        QString fileName = QFileDialog::getSaveFileName(this, "Save as DBC file", ".", "DBC File (*.dbc)");

        if (!fileName.isEmpty())
        {
            dbc->setSaveFileName(fileName);
            statusText->setText("Writing DBC file...");
            if (!_watcher.isRunning())
                _watcher.setFuture(QtConcurrent::run(dbc, &DTObject::writeDBC));
        }
    }
    else
        statusText->setText("DBC not loaded! Please load DBC file!");

}

void MainForm::slotOpenFile()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Open DBC file", ".", "DBC Files (*.dbc)");

    if (fileName.isEmpty())
        return;

    QFileInfo finfo(fileName);

    QStringList buildList = format->getVersionList(finfo.baseName());
    if (!buildList.isEmpty())
    {
        DTBuild* build = new DTBuild;
        build->comboBox->clear();
        build->comboBox->addItems(buildList);

        if (build->exec() == QDialog::Accepted)
        {
            dbcInfo->setText(QString(" <b>Name:</b> <font color=green>%0</font> <b>Build:</b> <font color=green>%1</font> ")
                .arg(finfo.baseName())
                .arg(build->comboBox->currentText()));

            statusText->setText("Loading DBC file...");

            DBCSortedModel* smodel = static_cast<DBCSortedModel*>(tableView->model());
            DBCTableModel* model = static_cast<DBCTableModel*>(smodel->sourceModel());
            if (model)
                delete model;

            dbc->set(fileName, build->comboBox->currentText());
        }
    }
    else
    {
        dbcInfo->setText(QString(" <b>Name:</b> <font color=green>%0</font> <b>Build:</b> <font color=green>Unknown</font> ")
                .arg(finfo.baseName()));

        statusText->setText("Loading unknown DBC file with default format...");

        DBCSortedModel* smodel = static_cast<DBCSortedModel*>(tableView->model());
        DBCTableModel* model = static_cast<DBCTableModel*>(smodel->sourceModel());
        if (model)
            delete model;

        dbc->set(fileName);
    }

    if (!_watcher.isRunning())
        _watcher.setFuture(QtConcurrent::run(dbc, &DTObject::load));
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
    m_fieldNames.clear();
    endResetModel();
}

int DBCTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_dbc->getRecordCount();
}

int DBCTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_dbc->getFieldCount();
}

QVariant DBCTableModel::data(const QModelIndex &index, int role) const
{
    if (m_dbcList.isEmpty())
        return QVariant();

    if (!index.isValid())
        return QVariant();

    if (index.row() >= m_dbcList.size() || index.row() < 0)
        return QVariant();

    if (role == Qt::DisplayRole || role == Qt::EditRole)
        return m_dbcList.at(index.row()).at(index.column());

    return QVariant();
}

bool DBCTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (m_dbcList.isEmpty())
        return false;

    if (!index.isValid())
        return false;

    if (index.row() >= m_dbcList.size() || index.row() < 0)
        return false;

    if (role == Qt::EditRole)
    {
        QStringList p = m_dbcList.at(index.row());
        p.replace(index.column(), value.toString());
        m_dbcList.replace(index.row(), p);
        emit(dataChanged(index, index));

        return true;
    }

    return false;
}

QVariant DBCTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (m_fieldNames.isEmpty())
        return QVariant();

    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal)
        return m_fieldNames.at(section);
    else
        return section;

    return QVariant();
}

QStringList DBCTableModel::getRecord(quint32 row) const
{
    if (row < 0 || row >= m_dbcList.size())
        return QStringList();

    return m_dbcList.at(row);
}

Qt::ItemFlags DBCTableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;

    return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
}

int RecordTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_rowCount;
}

int RecordTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 2;
}

QVariant RecordTableModel::data(const QModelIndex &index, int role) const
{
    if (m_recordVars.isEmpty())
        return QVariant();

    if (!index.isValid())
        return QVariant();

    if (index.row() >= m_recordVars.size() || index.row() < 0)
        return QVariant();

    switch (index.column())
    {
        case 0:
        {
            if (role == Qt::DisplayRole)
                return m_recordVars.at(index.row()).first;
            break;
        }
        case 1:
        {
            if (role == Qt::DisplayRole || role == Qt::EditRole)
                return m_recordVars.at(index.row()).second;
            break;
        }
    }

    return QVariant();
}

bool RecordTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (m_recordVars.isEmpty())
        return false;

    if (!index.isValid())
        return false;

    if (index.row() >= m_recordVars.size() || index.row() < 0)
        return false;

    if (index.column() && role == Qt::EditRole)
    {
        QPair<QString, QString> p = m_recordVars.at(index.row());
        p.second = value.toString();
        m_recordVars.replace(index.row(), p);
        emit(dataChanged(index, index));

        return true;
    }

    return false;
}

QVariant RecordTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal)
    {
        switch (section)
        {
            case 0:
                return QString("Field Name");
            case 1:
                return QString("Field Value");
        }
    }

    return QVariant();
}

void RecordTableModel::setValue(quint32 field, QString value, const QModelIndex& index)
{
    QPair<QString, QString> p = m_recordVars.at(field);
    p.second = value;
    m_recordVars.replace(field, p);
    emit(dataChanged(index, index));
}

Qt::ItemFlags RecordTableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;

    if (index.column())
        return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;

    return QAbstractTableModel::flags(index);
}
