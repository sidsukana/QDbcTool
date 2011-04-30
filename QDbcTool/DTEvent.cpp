#include "DTEvent.h"

SendModel::SendModel(DTForm *form, DBCTableModel* obj)
    : QEvent(QEvent::Type(SendModel::TypeId)), m_obj(obj), m_form(form)
{
}

SendModel::~SendModel()
{
}





