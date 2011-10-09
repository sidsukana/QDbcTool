#ifndef DTOBJECT_H
#define DTOBJECT_H

#include "DTForm.h"
#include "DTEvent.h"
#include "TObject.h"

#include <QtXml/QDomElement>
#include <QtXml/QDomDocument>
#include <QtXml/QDomAttr>
#include <QtXml/QDomNodeList>

class DTForm;
class DBCTableModel;
class DBCFormat;

class DTObject
{
    public:

        DTObject(DTForm* form, DBCFormat* format);
        ~DTObject();

        void Set(QString dbcName, QString dbcBuild = "Default");
        void Load();

        void SetRecordCount(quint32 count) { m_recordCount = count; }
        quint32 GetRecordCount() { return m_recordCount; }
        quint32 GetFieldCount() { return m_fieldCount; }
        quint32 GetRecordSize() { return m_recordSize; }
        quint32 GetStringSize() { return m_stringSize; }
        QString GetFileName() { return m_fileName; }
        void SetSaveFileName(QString name) { m_saveFileName = name; }

        void ThreadBegin(quint8 id);
        void ThreadSet(quint8 id) { ThreadSemaphore[id] = true; }
        void ThreadUnset(quint8 id) { ThreadSemaphore[id] = false; }
        bool ThreadExist(quint8 id) { return ThreadSemaphore[id]; }

        // Export methods
        void ExportAsSQL();
        void ExportAsCSV();
        void WriteDBC();

        bool isEmpty() { return (m_fileName.isEmpty() && m_build.isEmpty()); }
    private:
        DTForm* m_form;

        quint32 m_recordCount;
        quint32 m_fieldCount;
        quint32 m_recordSize;
        quint32 m_stringSize;

        QString m_fileName;
        QString m_saveFileName;
        QString m_build;

        DBCFormat* m_format;

        bool ThreadSemaphore[MAX_THREAD];
};

struct DBCField
{
    QString name;
    QString type;
    bool visible;
};

class DBCFormat
{
    public:
        DBCFormat(QString xmlFileName = QString());
        ~DBCFormat();

        void LoadFormat(QString dbcName, QString dbcBuild);
        void LoadFormat(QString dbcName, quint32 fieldCount);
        QStringList GetBuildList(QString fileName);
        QStringList GetFieldNames();
        QStringList GetFieldTypes();
        bool IsVisible(quint32 field) { return m_dbcFields.at(field).visible; }
        char GetFieldType(quint32 field) { return m_dbcFields.at(field).type.at(0).toAscii(); }
        QString GetFieldName(quint32 field) { return m_dbcFields.at(field).name; }
        void SetFieldAttribute(quint32 field, QString attr, QString value);

    private:
        QDomDocument m_xmlData;

        QString m_fileName;
        QString m_dbcName;
        QString m_dbcBuild;

        QList<DBCField> m_dbcFields;

};

#endif // DTOBJECT_H