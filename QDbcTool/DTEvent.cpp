#include "DTEvent.h"

SendModel::SendModel(DTForm *form, DBCTableModel* obj)
    : QEvent(QEvent::Type(SendModel::TypeId)), m_obj(obj), m_form(form)
{
}

SendModel::~SendModel()
{
}

ProgressBar::ProgressBar(quint32 value, quint8 id)
    : QEvent(QEvent::Type(ProgressBar::TypeId)), bar_step(value), bar_size(value), op_id(id)
{
}

ProgressBar::~ProgressBar()
{
}




