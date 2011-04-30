#ifndef DTFORM_H
#define DTFORM_H

#include <QtGui/QMainWindow>
#include <QtCore/QAbstractTableModel>
#include "ui_DTForm.h"
#include "Defines.h"
#include "TObject.h"
#include "DTEvent.h"

class TObject;
class DBCTableModel;
class DTForm : public QMainWindow, public Ui::DTFormUI
{
    Q_OBJECT

    public:
        DTForm(QWidget *parent = 0);
        ~DTForm();

        void GenerateTable();
        QChar GetColumnFormat(quint32 field);

        void ThreadBegin(quint8 id);
        void ThreadSet(quint8 id) { ThreadSemaphore[id] = true; }
        void ThreadUnset(quint8 id) { ThreadSemaphore[id] = false; }
        bool ThreadExist(quint8 id) { return ThreadSemaphore[id]; }

        bool event(QEvent *ev);

        quint32 GetRecordCount() { return m_recordCount; }
        quint32 GetFieldCount() { return m_fieldCount; }
        quint32 GetRecordSize() { return m_recordSize; }
        quint32 GetStringSize() { return m_stringSize; }
    
    public slots:
        void SlotOpenFile();

private:
    Ui::DTFormUI ui;

    quint32 m_recordCount;
    quint32 m_fieldCount;
    quint32 m_recordSize;
    quint32 m_stringSize;

    DBCTableModel* model;
    QString filename;
    bool ThreadSemaphore[MAX_THREAD];
};

class DBCTableModel : public QAbstractTableModel
{
    Q_OBJECT
    
public:
    DBCTableModel(QObject *parent = 0, DTForm *form = NULL);
    DBCTableModel(QMap<quint32, QMap<quint32, QString>> dataMap, QObject *parent = 0, DTForm *form = NULL);

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
    QMap<quint32, QMap<quint32, QString>> getMap();

private:
    QMap<quint32, QMap<quint32, QString>> dataMap;
    DTForm* m_form;
};

#endif // DTFORM_H
