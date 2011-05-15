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

class DTObject
{
    public:

        DTObject(DTForm* form = NULL);
        ~DTObject();

        void Load();
        inline QChar GetFieldType(quint32 field);

        quint32 GetRecordCount() { return m_recordCount; }
        quint32 GetFieldCount() { return m_fieldCount; }
        quint32 GetRecordSize() { return m_recordSize; }
        quint32 GetStringSize() { return m_stringSize; }
        QString GetFileName() { return m_fileName; }
        void SetFileName(QString name) { m_fileName = name; }
        void SetSaveFileName(QString name) { m_saveFileName = name; }
        void SetBuild(QString build) { m_build = build; }
        void LoadFormats();

        void ThreadBegin(quint8 id);
        void ThreadSet(quint8 id) { ThreadSemaphore[id] = true; }
        void ThreadUnset(quint8 id) { ThreadSemaphore[id] = false; }
        bool ThreadExist(quint8 id) { return ThreadSemaphore[id]; }

        // Export methods
        void ExportAsSQL();
        void ExportAsCSV();

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

        QStringList m_fieldNames;
        QStringList m_fieldTypes;

        QDomDocument m_dbcFormats;

        bool ThreadSemaphore[MAX_THREAD];
};

#endif // DTOBJECT_H