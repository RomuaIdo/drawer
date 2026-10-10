#include <QtTest>

#include "data/Database.h"
#include "data/SqliteCabinetRepository.h"
#include "data/SqliteDrawerRepository.h"
#include "data/SqliteItemRepository.h"
#include "data/SqliteMappingRepository.h"
#include "export/CsvExporter.h"
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
        SqliteItemRepository items(db.connection());
        SqliteMappingRepository mapping(db.connection());
        CsvExporter exporter;
        MainWindow window({cabinets, drawers, items, mapping}, exporter);
        QCOMPARE(window.windowTitle(), QString("Drawer"));
    }
};

QTEST_MAIN(SmokeTest)
#include "tst_smoke.moc"
