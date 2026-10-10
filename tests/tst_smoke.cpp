#include <QComboBox>
#include <QLabel>
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
#include "tools/FormulaWidgets.h"
#include "tools/ResistorWidget.h"
#include "tools/SmdStrip.h"
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

    void capacitorToolPaints()
    {
        CapacitorWidget widget;
        widget.show();
        QVERIFY(widget.findChild<CapacitorStrip *>());
        QVERIFY(!widget.grab().isNull()); // Painting does not crash.
    }

    void smdToolDrawsMarking()
    {
        SmdWidget widget;
        widget.show();
        auto *strip = widget.findChild<SmdStrip *>();
        QVERIFY(strip);
        widget.findChild<QLineEdit *>()->setText("01c");
        QCOMPARE(strip->marking(), QString("01C"));
        widget.findChild<QLineEdit *>()->setText("zz9");
        QVERIFY(strip->marking().isEmpty());
        QVERIFY(!widget.grab().isNull());
    }

    void resistorValueSurvivesBandCountChange()
    {
        ResistorWidget widget;
        widget.show();
        QComboBox *bandCount = widget.findChildren<QComboBox *>().first();
        QLineEdit *value = widget.findChild<QLineEdit *>();
        const auto resultText = [&] {
            QStringList texts;
            for (QLabel *label : widget.findChildren<QLabel *>())
                texts << label->text();
            return texts.join("|");
        };

        value->setText("4.7k");
        QVERIFY(resultText().contains("4.7 kΩ"));
        for (int index : {1, 2, 0}) { // 5, 6, then back to 4 bands.
            bandCount->setCurrentIndex(index);
            QVERIFY2(resultText().contains("4.7 kΩ"), qPrintable(resultText()));
            QCOMPARE(value->text(), QString("4.7k"));
        }
    }

    void resistorBandsSurviveBandCountChangeWithoutTypedValue()
    {
        ResistorWidget widget;
        widget.show();
        QComboBox *bandCount = widget.findChildren<QComboBox *>().first();
        // Defaults decode to 10 ohm; changing the count must not reset or lose that.
        bandCount->setCurrentIndex(1);
        bool found = false;
        for (QLabel *label : widget.findChildren<QLabel *>())
            found = found || label->text().startsWith("10 Ω");
        QVERIFY(found);
    }

    void toolsDialogBuildsAllTools()
    {
        const ToolRegistry registry = ToolRegistry::withDefaultTools();
        ToolsDialog dialog(registry);
        QVERIFY(!registry.tools().empty());
        QVERIFY(!registry.toolsIn(ToolCategory::ComponentReading).empty());
        QVERIFY(!registry.toolsIn(ToolCategory::Auxiliary).empty());

        // Switching the band count rebuilds the selectors without crashing.
        auto *bandCount = dialog.findChild<QComboBox *>();
        QVERIFY(bandCount);
        for (int i = 0; i < bandCount->count(); ++i)
            bandCount->setCurrentIndex(i);
    }
};

QTEST_MAIN(SmokeTest)
#include "tst_smoke.moc"
