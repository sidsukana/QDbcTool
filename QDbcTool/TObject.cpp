#include "TObject.h"
#include "Defines.h"

TObject::TObject(quint8 id, DTForm* form)
    : m_id(id), m_form(form)
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
            m_form->GenerateTable();
            break;
        default:
            break;
    }
}
