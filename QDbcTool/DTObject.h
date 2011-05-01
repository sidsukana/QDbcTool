#ifndef DTOBJECT_H
#define DTOBJECT_H

#include "DTForm.h"
#include "DTEvent.h"
#include "TObject.h"

#include <QtCore/QSettings>

class DTForm;
class DBCTableModel;

class DTObject
{
    public:

        DTObject(DTForm* form = NULL);
        ~DTObject();

        void Load();
        QChar GetColumnFormat(quint32 field);

        quint32 GetRecordCount() { return m_recordCount; }
        quint32 GetFieldCount() { return m_fieldCount; }
        quint32 GetRecordSize() { return m_recordSize; }
        quint32 GetStringSize() { return m_stringSize; }
        QString GetFileName() { return m_fileName; }
        void SetFileName(QString name) { m_fileName = name; }
        void LoadConfig();

        void ThreadBegin(quint8 id);
        void ThreadSet(quint8 id) { ThreadSemaphore[id] = true; }
        void ThreadUnset(quint8 id) { ThreadSemaphore[id] = false; }
        bool ThreadExist(quint8 id) { return ThreadSemaphore[id]; }

    private:
        quint32 m_recordCount;
        quint32 m_fieldCount;
        quint32 m_recordSize;
        quint32 m_stringSize;

        DTForm* m_form;
        QString m_fileName;
        QSettings* config;
        QString m_format;

        DBCTableModel* model;

        bool ThreadSemaphore[MAX_THREAD];
};

#endif // DTOBJECT_H