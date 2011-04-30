#ifndef THREADOBJ_H
#define THREADOBJ_H

#include <QtCore/QThread>
#include "DTForm.h"

class DTForm;
class TObject: public QThread
{
public:
    
    TObject(quint8 id = 0, DTForm* form = NULL);
    ~TObject();

    virtual void run();
    quint8 GetId() { return m_id; }
private:

    DTForm* m_form;
    quint8 m_id;
};

#endif
