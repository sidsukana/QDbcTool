#ifndef THREADOBJ_H
#define THREADOBJ_H

#include <QtCore/QThread>
#include "DTForm.h"
#include "DTObject.h"

class DTForm;
class DTObject;

class TObject: public QThread
{
public:
    
    TObject(quint8 id = 0, DTObject* dbc = NULL);
    ~TObject();

    virtual void run();
    quint8 GetId() { return m_id; }
private:

    DTObject* m_dbc;
    quint8 m_id;
};

#endif
