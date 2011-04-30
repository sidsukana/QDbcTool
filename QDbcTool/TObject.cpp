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
        default:
            break;
    }
}
