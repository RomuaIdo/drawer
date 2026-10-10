#include <QtTest>

#include "data/Database.h"
#include "data/SqliteCabinetRepository.h"
#include "data/SqliteDrawerRepository.h"
#include "ui/MainWindow.h"

class SmokeTest : public QObject
{
    Q_OBJECT

private slots:
    void mainWindowConstructs()
    {
        Database db(":memory:");
        SqliteCabinetRepository cabinets(db.connection());
        SqliteDrawerRepository drawers(db.connection());
        MainWindow window(cabinets, drawers);
        QCOMPARE(window.windowTitle(), QString("Drawer"));
    }
};

QTEST_MAIN(SmokeTest)
#include "tst_smoke.moc"
