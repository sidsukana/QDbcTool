#ifndef DTFORM_H
#define DTFORM_H

#include <QtGui/QMainWindow>
#include <QtGui/QDialog>
#include <QtGui/QStatusBar>
#include <QtCore/QAbstractTableModel>
#include "ui_DTForm.h"
#include "ui_DTBuild.h"
#include "Defines.h"
#include "TObject.h"
#include "DTEvent.h"
#include "DTObject.h"

class TObject;
class DBCTableModel;
class DTObject;
class DTForm : public QMainWindow, public Ui::DTFormUI
{
    Q_OBJECT

    public:
        DTForm(QWidget *parent = 0);
        ~DTForm();

        bool event(QEvent *ev);
    
    public slots:
        void SlotOpenFile();

private:
    Ui::DTFormUI ui;
    QStatusBar* statusBar;
    DTObject* dbc;
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
    //QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    void insertRecord(QStringList strl);
    DBCList getDbcList();
    void clear();

private:

    DBCList m_dbcList;

    DTObject* m_dbc;
};

#endif // DTFORM_H
