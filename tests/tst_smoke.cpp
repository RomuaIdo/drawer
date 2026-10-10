#include <QComboBox>
#include <QLineEdit>
#include <QtTest>

#include "data/Database.h"
#include "data/SqliteCabinetRepository.h"
#include "data/SqliteDrawerRepository.h"
#include "data/SqliteItemRepository.h"
#include "data/SqliteMappingRepository.h"
#include "data/SqliteOrganizationRepository.h"
#include "export/CsvExporter.h"
#include "tools/CapacitorStrip.h"
#include "tools/CapacitorWidget.h"
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

    void capacitorDrawingFollowsInput()
    {
        CapacitorWidget widget;
        widget.show();
        auto *strip = widget.findChild<CapacitorStrip *>();
        auto inputs = widget.findChildren<QLineEdit *>();
        QVERIFY(strip);
        QCOMPARE(inputs.size(), 2);

        inputs[0]->setText("104k");
        QCOMPARE(strip->marking(), QString("104K"));
        inputs[0]->setText("zzz");
        QVERIFY(strip->marking().isEmpty());

        inputs[1]->setText("22n");
        QCOMPARE(strip->marking(), QString("223"));
        QVERIFY(!strip->grab().isNull()); // Painting does not crash.
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
