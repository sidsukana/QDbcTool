QT       += core gui xml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = qdbctool
TEMPLATE = app


SOURCES += Main.cpp\
        DTForm.cpp \
    Alphanum.cpp \
    DTObject.cpp \
    DTEvent.cpp \
    TObject.cpp

HEADERS  += DTForm.h \
    Alphanum.h \
    Defines.h \
    DTObject.h \
    DTEvent.h \
    TObject.h

FORMS    += DTForm.ui \
    DTBuild.ui \
    AboutFormUI.ui

RESOURCES += \
    DbcTool.qrc
