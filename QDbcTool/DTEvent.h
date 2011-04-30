#ifndef EVENT_H
#define EVENT_H

#include <QtCore/QEvent>
#include <DTForm.h>
#include "QtGui/QStandardItemModel"

class DTForm;
class DBCTableModel;

class ProgressBar : public QEvent
{
    public:
        enum { TypeId = QEvent::User + 1 };
        ProgressBar(quint32 value, quint8 id);
        ~ProgressBar();

        quint8 GetId() const { return op_id; }
        quint32 GetStep() const { return bar_step; }
        quint32 GetSize() const { return bar_size; }
    private:
        quint32 bar_step;
        quint32 bar_size; 
        quint8 op_id; 
};

class SendModel : public QEvent
{
    public:
        enum { TypeId = QEvent::User + 2 };
        SendModel(DTForm *form, DBCTableModel* obj);
        ~SendModel();

        DBCTableModel* GetObject() { return m_obj; }

    private:
        DTForm *m_form;
        DBCTableModel* m_obj;
};

#endif // EVENTMGR_H
