#ifndef EVENT_H
#define EVENT_H

#include <QtCore/QEvent>
#include <DTForm.h>
#include "QtGui/QStandardItemModel"

class DTForm;
class DBCTableModel;
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
