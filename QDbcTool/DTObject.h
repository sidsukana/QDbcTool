#pragma once

#include <QJsonDocument>

#include "MainForm.h"

class MainForm;
class DBCTableModel;
class DBCFormat;


struct DBC
{
    quint32 m_header;
    quint32 m_recordCount;
    quint32 m_fieldCount;
    quint32 m_recordSize;
    quint32 m_stringSize;
    QVector<QVector<quint32> > m_dataBlock;
    char* m_stringBlock;
};

QString escapedString(QString str);

class DTObject : public QObject
{
    Q_OBJECT
    public:

        DTObject(MainForm* form, DBCFormat* format, QObject* parent = nullptr);
        ~DTObject();

        void set(QString fileName, QString version = "Default");
        void load();
        void search();

        void setRecordCount(quint32 count) { _dbc->m_recordCount = count; }
        quint32 getRecordCount() { return _dbc->m_recordCount; }
        quint32 getFieldCount() { return _dbc->m_fieldCount; }
        quint32 getRecordSize() { return _dbc->m_recordSize; }
        quint32 getStringSize() { return _dbc->m_stringSize; }
        QString getFileName() { return _fileName; }
        void setSaveFileName(QString name) { _saveFileName = name; }

        DBCTableModel* getModel() const { return _model; }

        // Export methods
        void exportAsJSON();
        void exportAsSQL();
        void exportAsCSV();
        void writeDBC();

        bool isEmpty() { return (_fileName.isEmpty() && _version.isEmpty()); }

    signals:
        void loadingStart(quint32);
        void loadingStep(quint32);
        void loadingNote(QString);
        void loadingDone(QAbstractItemModel*);
        void searchDone(QList<bool>);

    private:
        DBCTableModel* _model;
        MainForm* _form;
        DBC* _dbc;

        QString _fileName;
        QString _saveFileName;
        QString _version;

        DBCFormat* _format;
};

struct DBCField
{
    QString name;
    QString type;
    bool hiden;
    QString ref;
    bool custom;
    QString value;
};

class DBCFormat
{
    public:
        DBCFormat(QString jsonFileName = QString());
        DBCFormat(QJsonDocument json);
        ~DBCFormat();

        void loadFormat(QString name, QString version);
        void loadFormat(QString name, quint32 fieldCount);
        QStringList getVersionList(QString fileName);
        QStringList getFieldNames();
        QStringList getFieldTypes();
        quint32 getFieldCount() const { return quint32(_fields.size()); }
        bool isHiden(quint32 field) const { return _fields.at(field).hiden; }
        bool isCustom(quint32 field) const { return _fields.at(field).custom; }
        QString getValue(quint32 field) const { return _fields.at(field).value; }
        char getFieldType(quint32 field) const { return _fields.at(field).type.at(0).toLatin1(); }
        QString getFieldName(quint32 field) const { return _fields.at(field).name; }
        QString getFieldRef(quint32 field) const { return _fields.at(field).ref; }
        void setFieldAttribute(quint32 field, QString attr, QString value);

        QJsonDocument getJson() const { return _json; }

    private:
        QJsonDocument _json;

        QString _fileName;
        QString _name;
        QString _version;

        QList<DBCField> _fields;

};
