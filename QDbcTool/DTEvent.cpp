#include "DTEvent.h"

SendModel::SendModel(DTForm *form, DBCTableModel* obj)
    : QEvent(QEvent::Type(SendModel::TypeId)), m_obj(obj), m_form(form)
{
}

SendModel::~SendModel()
{
}

SendText::SendText(DTForm *form, quint8 id, QString str)
    : QEvent(QEvent::Type(SendText::TypeId)), m_str(str), m_form(form), m_id(id)
{
}

SendText::~SendText()
{
}

SendHiden::SendHiden(quint8 id, quint32 value, bool ok)
    : QEvent(QEvent::Type(SendHiden::TypeId)), m_ok(ok), m_id(id), m_value(value)
{
}

SendHiden::~SendHiden()
{
}

ProgressBar::ProgressBar(quint32 value, quint8 id)
    : QEvent(QEvent::Type(ProgressBar::TypeId)), bar_step(value), bar_size(value), op_id(id)
{
}

ProgressBar::~ProgressBar()
{
}




