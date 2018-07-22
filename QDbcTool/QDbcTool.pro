QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = qdbctool
TEMPLATE = app


SOURCES += Main.cpp\
    Alphanum.cpp \
    DTObject.cpp \
    MainForm.cpp \
    export_sql.cpp \
    export_csv.cpp \
    export_json.cpp

HEADERS  += \
    Alphanum.h \
    Defines.h \
    DTObject.h \
    MainForm.h

FORMS    += \
    DTBuild.ui \
    MainForm.ui \
    AboutForm.ui

RESOURCES += \
    DbcTool.qrc
