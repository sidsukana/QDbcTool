#ifndef DTFORM_H
#define DTFORM_H

#include <QtGui/QMainWindow>
#include <QtGui/QDialog>
#include <QtGui/QStatusBar>
#include <QtGui/QToolButton>
#include <QtGui/QProgressBar>
#include <QtGui/QLabel>
#include <QtCore/QAbstractTableModel>
#include <QtCore/QSignalMapper>
#include <QtGui/QSortFilterProxyModel>
//#include <QtCore/QSettings>
#include "ui_DTForm.h"
#include "ui_DTBuild.h"
#include "ui_AboutFormUI.h"
#include "Defines.h"
#include "TObject.h"
#include "DTEvent.h"
#include "DTObject.h"

class TObject;
class DBCTableModel;
class DBCSortedModel;
class DTObject;
class DBCFormat;

class DTForm : public QMainWindow, public Ui::DTFormUI
{
    Q_OBJECT

    public:
        DTForm(QWidget *parent = 0);
        ~DTForm();

        //QSettings* GetConfig() { return config; }

        bool event(QEvent *ev);
    
    public slots:
        void SlotSetVisible(QAction*);
        void SlotOpenFile();
        void SlotExportAsSQL();
        void SlotExportAsCSV();
        void SlotAbout();

    private:
        void ApplyFilter();

        Ui::DTFormUI ui;
        DTObject* dbc;
        DBCFormat* format;
        //QSettings* config;
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
        DTBuild(QWidget *parent = 0, DTForm* form = NULL);
        ~DTBuild();

private:
    Ui::DTBuildUI ui;
    DTForm* m_form;
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

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    void setFieldNames(QStringList strl) { m_fieldNames = strl; }
    Qt::ItemFlags flags(const QModelIndex &index) const;
    void appendRecord(QStringList strl) { m_dbcList << strl; }
    DBCList getDbcList() { return m_dbcList; }
    void clear();

private:

    DBCList m_dbcList;
    QStringList m_fieldNames;

    DTObject* m_dbc;
};

#endif // DTFORM_H
