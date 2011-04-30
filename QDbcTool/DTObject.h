#ifndef DTOBJECT_H
#define DTOBJECT_H

#include "DTForm.h"
#include "DTEvent.h"
#include "TObject.h"

class DTForm;

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

        bool ThreadSemaphore[MAX_THREAD];
};

#endif // DTOBJECT_H