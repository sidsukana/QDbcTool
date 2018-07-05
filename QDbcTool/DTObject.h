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


class DTObject : public QObject
{
    Q_OBJECT
    public:

        DTObject(MainForm* form, DBCFormat* format, QObject* parent = nullptr);
        ~DTObject();

        void set(QString dbcName, QString dbcBuild = "Default");
        void load();
        void search();

        void SetRecordCount(quint32 count) { dbc->m_recordCount = count; }
        quint32 GetRecordCount() { return dbc->m_recordCount; }
        quint32 GetFieldCount() { return dbc->m_fieldCount; }
        quint32 GetRecordSize() { return dbc->m_recordSize; }
        quint32 GetStringSize() { return dbc->m_stringSize; }
        QString GetFileName() { return m_fileName; }
        void SetSaveFileName(QString name) { _saveFileName = name; }

        // Export methods
        void exportAsJSON();
        void exportAsSQL();
        void exportAsCSV();
        void writeDBC();

        bool isEmpty() { return (m_fileName.isEmpty() && m_build.isEmpty()); }

    signals:
        void loadingStart(quint32);
        void loadingStep(quint32);
        void loadingNote(QString);
        void loadingDone(QAbstractItemModel*);
        void searchDone(QList<bool>);

    private:
        MainForm* m_form;
        DBC* dbc;

        QString m_fileName;
        QString _saveFileName;
        QString m_build;

        DBCFormat* m_format;
};

struct DBCField
{
    QString name;
    QString type;
    bool hiden;
};

class DBCFormat
{
    public:
        DBCFormat(QString jsonFileName = QString());
        ~DBCFormat();

        void LoadFormat(QString name, QString version);
        void LoadFormat(QString name, quint32 fieldCount);
        QStringList GetBuildList(QString fileName);
        QStringList GetFieldNames();
        QStringList GetFieldTypes();
        bool IsHiden(quint32 field) { return _fields.at(field).hiden; }
        char GetFieldType(quint32 field) { return _fields.at(field).type.at(0).toLatin1(); }
        QString GetFieldName(quint32 field) { return _fields.at(field).name; }
        void SetFieldAttribute(quint32 field, QString attr, QString value);

    private:
        QJsonDocument _json;

        QString _fileName;
        QString _name;
        QString _version;

        QList<DBCField> _fields;

};
