#ifndef DTFORM_H
#define DTFORM_H

#include <QtGui/QMainWindow>
#include <QtCore/QAbstractTableModel>
#include "ui_DTForm.h"
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
    DTObject* dbc;
};

typedef QMap<quint32, QString> FieldMap;
typedef QMap<quint32, FieldMap> DbcMap;

class DBCTableModel : public QAbstractTableModel
{
    Q_OBJECT
    
public:
    DBCTableModel(QObject *parent = 0, DTObject *dbc = NULL);
    DBCTableModel(DbcMap dbcMap, QObject *parent = 0, DTObject *dbc = NULL);

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    //QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role=Qt::EditRole);
    bool insertRows(int position, int rows, const QModelIndex &index = QModelIndex());
    //bool insertColumns(int position, int columns, const QModelIndex &index = QModelIndex());
    bool removeRows(int position, int rows, const QModelIndex &index = QModelIndex());
    //bool removeColumns(int position, int columns, const QModelIndex &index = QModelIndex());
    DbcMap getMap();

private:

    DbcMap m_dbcMap;
    FieldMap m_fieldMap;

    DTObject* m_dbc;
};

#endif // DTFORM_H
