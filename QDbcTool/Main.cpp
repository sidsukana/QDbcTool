#include <QApplication>
#include <QDesktopWidget>

#include "MainForm.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MainForm form;

    qRegisterMetaType<QList<bool>>("QList<bool>");

    // Set position to center about desktop widget
    QRect frameRect = form.frameGeometry();
    frameRect.moveCenter(QDesktopWidget().availableGeometry().center());
    form.move(frameRect.topLeft());

    form.show();
    return app.exec();
}
