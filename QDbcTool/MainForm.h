#pragma once

#include <QMainWindow>
#include <QDialog>
#include <QLineEdit>
#include <QStatusBar>
#include <QToolButton>
#include <QProgressBar>
#include <QLabel>
#include <QAbstractTableModel>
#include <QSignalMapper>
#include <QSortFilterProxyModel>
#include <QFutureWatcher>

#include "ui_MainForm.h"
#include "ui_DTBuild.h"

#include "ui_AboutForm.h"
#include "Defines.h"
#include "DTObject.h"

class TObject;
class DBCTableModel;
class DBCSortedModel;
class RecordTableModel;
class DTObject;
class DBCFormat;

class MainForm : public QMainWindow, public Ui::MainFormUI
{
    Q_OBJECT

    public:
        MainForm(QWidget *parent = 0);
        ~MainForm();
    
    public slots:
        void slotSetVisible(QAction*);
        void slotOpenFile();
        void slotExportAsSQL();
        void slotExportAsCSV();
        void slotWriteDBC();
        void slotAbout();
        void slotSearch();
        void slotFileLoaded(QAbstractItemModel *model);
        void slotLoadingNote(QString note);
        void slotLoadingStep(quint32 step);
        void slotLoadingStart(quint32 size);
        void slotSearchDone(QList<bool> rowStates);

    private:
        void applyFilter();

        QFutureWatcher<void> _watcher;

        Ui::MainFormUI ui;
        DTObject* dbc;
        DBCFormat* format;
        DBCSortedModel* proxyModel;

        QProgressBar* progressBar;
        QToolButton* fieldBox;
        QLabel* statusText;
        QLabel* dbcInfo;
};

class DBCSortedModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    DBCSortedModel(QObject *parent = 0);

    bool lessThan(const QModelIndex &left, const QModelIndex &right) const;
};

class DTBuild : public QDialog, public Ui::DTBuildUI
{
    Q_OBJECT

    public:
        DTBuild(QWidget *parent = 0, MainForm* form = NULL);
        ~DTBuild();

private:
    Ui::DTBuildUI ui;
    MainForm* m_form;
};

class AboutForm : public QDialog, public Ui::AboutFormUI
{
    Q_OBJECT

public:
    AboutForm(QWidget *parent = 0);
    ~AboutForm();

private:
    Ui::AboutFormUI ui;
};

typedef QList<QStringList> DBCList;

class DBCTableModel : public QAbstractTableModel
{
    Q_OBJECT
    
public:
    DBCTableModel(QObject *parent = 0, DTObject *dbc = NULL);
    DBCTableModel(DBCList dbcList, QObject *parent = 0, DTObject *dbc = NULL);

    bool insertRows(int position, int rows, const QModelIndex &index = QModelIndex());
    bool removeRows(int position, int rows, const QModelIndex &index = QModelIndex());
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
    void setFieldNames(QStringList strl) { m_fieldNames = strl; }
    Qt::ItemFlags flags(const QModelIndex &index) const;
    void appendRecord(QStringList strl) { m_dbcList << strl; }
    DBCList getDbcList() { return m_dbcList; }
    QStringList getRecord(quint32 row) const;
    void clear();

private:

    DBCList m_dbcList;
    QStringList m_fieldNames;

    DTObject* m_dbc;
};

class RecordTableModel : public QAbstractTableModel
{
    Q_OBJECT
    
public:
    RecordTableModel(QObject *parent = 0) {}

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
    Qt::ItemFlags flags(const QModelIndex &index) const;
    void setRowCount(quint32 count) { m_rowCount = count; }
    void appendVar(QPair<QString, QString> var) { m_recordVars << var; }
    QString getValue(quint32 field) const { return m_recordVars.at(field).second; }
    void setValue(quint32 field, QString value, const QModelIndex& index);
    quint32 getRowCount() const { return m_rowCount; }

private:
    QList<QPair<QString, QString> > m_recordVars;
    quint32 m_rowCount;

};
