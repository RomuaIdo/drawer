#include <QComboBox>
#include <QtTest>

#include "data/Database.h"
#include "data/SqliteCabinetRepository.h"
#include "data/SqliteDrawerRepository.h"
#include "data/SqliteItemRepository.h"
#include "data/SqliteMappingRepository.h"
#include "data/SqliteOrganizationRepository.h"
#include "export/CsvExporter.h"
#include "tools/ToolRegistry.h"
#include "tools/ToolsDialog.h"
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
        SqliteOrganizationRepository organization(db.connection());
        CsvExporter exporter;
        MainWindow window({cabinets, drawers, items, mapping, organization}, exporter);
        QCOMPARE(window.windowTitle(), QString("Drawer"));
    }

    void toolsDialogBuildsAllTools()
    {
        const ToolRegistry registry = ToolRegistry::withDefaultTools();
        ToolsDialog dialog(registry);
        QVERIFY(!registry.tools().empty());

        // Switching the band count rebuilds the selectors without crashing.
        auto *bandCount = dialog.findChild<QComboBox *>();
        QVERIFY(bandCount);
        for (int i = 0; i < bandCount->count(); ++i)
            bandCount->setCurrentIndex(i);
    }
};

QTEST_MAIN(SmokeTest)
#include "tst_smoke.moc"
