#include "TObject.h"
#include "Defines.h"

TObject::TObject(quint8 id, DTObject* dbc)
    : m_id(id), m_dbc(dbc)
{
    moveToThread(this);
}

TObject::~TObject()
{
}

void TObject::run()
{
    switch(m_id)
    {
        case THREAD_OPENFILE:
            m_dbc->Load();
            break;
        case THREAD_EXPORT_SQL:
            m_dbc->ExportAsSQL();
            break;
        case THREAD_EXPORT_CSV:
            m_dbc->ExportAsCSV();
            break;
        case THREAD_WRITE_DBC:
            m_dbc->WriteDBC();
            break;
        case THREAD_SEARCH:
            m_dbc->Search();
            break;
        default:
            break;
    }
}
