#include <QComboBox>
#include <QLabel>
#include <QMimeData>
#include <QPushButton>
#include <QSignalSpy>
#include <QTableView>
#include <QLineEdit>
#include <QtTest>

#include "data/Database.h"
#include "data/SqliteCabinetRepository.h"
#include "data/SqliteDrawerRepository.h"
#include "data/SqliteItemRepository.h"
#include "data/SqliteMappingRepository.h"
#include "data/SqliteOrganizationRepository.h"
#include "data/UndoLog.h"
#include "export/CsvExporter.h"
#include "ui/ItemMime.h"
#include "ui/ItemPanel.h"
#include "ui/ItemTableModel.h"
#include "tools/CapacitorStrip.h"
#include "tools/ColorBandStrip.h"
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
        MainWindow window({cabinets, drawers, items, mapping, organization, db.undoLog()}, exporter);
        QCOMPARE(window.windowTitle(), QString("Drawer"));
    }

    void capacitorToolPaints()
    {
        CapacitorWidget widget;
        widget.show();
        QVERIFY(widget.findChild<CapacitorStrip *>());
        QVERIFY(!widget.grab().isNull()); // Painting does not crash.
    }

    void resistorKeepsItsSizeInALargeWidget()
    {
        ColorBandStrip strip;
        strip.setBands({ResistorCode::Color::Brown, ResistorCode::Color::Black,
                        ResistorCode::Color::Red, ResistorCode::Color::Gold});
        strip.resize(1600, 200);
        const QImage image = strip.grab().toImage();
        const QColor background = image.pixelColor(0, 0);
        // Far left and right of the part are untouched: neither body nor leads grow.
        for (int x : {5, 400, 1200, 1595})
            QCOMPARE(image.pixelColor(x, 100), background);
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

    void quickQuantityButtonsAdjustTheSelectedItems()
    {
        Database db(":memory:");
        SqliteCabinetRepository cabinets(db.connection());
        SqliteDrawerRepository drawers(db.connection());
        SqliteItemRepository items(db.connection());
        DrawerGroup g;
        g.cabinetId = cabinets.add({0, "C", ""}).id;
        g.name = "G";
        g = drawers.addGroup(g);
        const Drawer drawer = drawers.drawersOf(g.id).first();
        items.add({0, drawer.id, "Resistor", "", "", "", 3, "", 2});

        ItemPanel panel(items);
        panel.setDrawer(drawer, "G", {});
        panel.show();
        QSignalSpy changed(&panel, &ItemPanel::itemsChanged);
        auto *table = panel.findChild<QTableView *>();
        table->selectRow(0);

        QPushButton *decrease = nullptr;
        QPushButton *increase = nullptr;
        for (QPushButton *button : panel.findChildren<QPushButton *>()) {
            if (button->text() == "+")
                increase = button;
            else if (button->toolTip().startsWith("Take"))
                decrease = button;
        }
        QVERIFY(increase && decrease);

        decrease->click();
        QCOMPARE(items.inDrawer(drawer.id).first().quantity, 2);
        QVERIFY(items.inDrawer(drawer.id).first().isLowStock());
        increase->click();
        increase->click();
        QCOMPARE(items.inDrawer(drawer.id).first().quantity, 4);
        QCOMPARE(changed.count(), 3);
        QVERIFY(table->selectionModel()->hasSelection()); // Selection survives the reload.
    }

    void draggingItemRowsProducesTheItemMimeData()
    {
        ItemTableModel model;
        model.setItems({Item{11, 1, "A", "", "", "", 1, ""}, Item{12, 1, "B", "", "", "", 1, ""}});
        QVERIFY(model.flags(model.index(0, 0)) & Qt::ItemIsDragEnabled);
        QVERIFY(model.supportedDragActions().testFlag(Qt::CopyAction));
        std::unique_ptr<QMimeData> mime(
            model.mimeData({model.index(1, 0), model.index(1, 2), model.index(0, 0)}));
        QCOMPARE(ItemMime::decode(mime.get()), (QList<int>{12, 11})); // One id per row.
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
