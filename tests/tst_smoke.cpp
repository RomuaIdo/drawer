#include <QtTest>

#include "ui/MainWindow.h"

class SmokeTest : public QObject
{
    Q_OBJECT

private slots:
    void mainWindowConstructs()
    {
        MainWindow window;
        QCOMPARE(window.windowTitle(), QString("Drawer"));
    }
};

QTEST_MAIN(SmokeTest)
#include "tst_smoke.moc"
