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
#include "tools/CapacitorCode.h"
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

    void capacitorDrawingFollowsTheValueUntilTheTypeIsFixed()
    {
        using T = CapacitorCode::CapacitorType;
        CapacitorWidget widget;
        widget.show();
        auto *strip = widget.findChild<CapacitorStrip *>();
        auto *input = widget.findChild<QLineEdit *>();
        auto *type = widget.findChild<QComboBox *>();
        QVERIFY(strip && input && type);

        input->setText("104");
        QCOMPARE(strip->type(), T::CeramicDisc);
        input->setText("470n");
        QCOMPARE(strip->type(), T::Film);
        input->setText("4u7");
        QCOMPARE(strip->type(), T::Electrolytic);

        type->setCurrentIndex(type->findData(int(T::Tantalum))); // Fixed by hand.
        QCOMPARE(strip->type(), T::Tantalum);
        input->setText("104");
        QCOMPARE(strip->type(), T::Tantalum);

        type->setCurrentIndex(0); // Back to automatic.
        QCOMPARE(strip->type(), T::CeramicDisc);

        // Every type paints.
        for (int i = 1; i < type->count(); ++i) {
            type->setCurrentIndex(i);
            QVERIFY(!widget.grab().isNull());
        }
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
        QComboBox *bandCount = widget.findChildren<QComboBox *>()[1];
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

    void metalFilmResistorsGetFiveBandsAndTheBlueBody()
    {
        ResistorWidget widget;
        widget.show();
        auto *strip = widget.findChild<ColorBandStrip *>();
        auto combos = widget.findChildren<QComboBox *>();
        QComboBox *bandCount = combos[1]; // Created after the type selector.
        QComboBox *type = combos[0];
        QCOMPARE(type->itemText(0), QString("Carbon film"));
        QCOMPARE(type->itemText(1), QString("Metal film"));
        QCOMPARE(strip->type(), ResistorCode::ResistorType::CarbonFilm);
        QCOMPARE(bandCount->currentData().toInt(), 4);

        widget.findChild<QLineEdit *>()->setText("4.7k");
        type->setCurrentIndex(1);
        QCOMPARE(strip->type(), ResistorCode::ResistorType::MetalFilm);
        QCOMPARE(bandCount->currentData().toInt(), 5);
        QStringList texts;
        for (QLabel *label : widget.findChildren<QLabel *>())
            texts << label->text();
        QVERIFY2(texts.join("|").contains("4.7 kΩ  ±1%"), qPrintable(texts.join("|")));

        // The body color differs from the carbon film one.
        const QImage metal = strip->grab().toImage();
        type->setCurrentIndex(0);
        QVERIFY(strip->grab().toImage() != metal);
    }

    void resistorBandsSurviveBandCountChangeWithoutTypedValue()
    {
        ResistorWidget widget;
        widget.show();
        QComboBox *bandCount = widget.findChildren<QComboBox *>()[1];
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

    void quantityTooltipListsTheDrawersHoldingTheItem()
    {
        auto locations = [](int) {
            QList<ItemLocation> list;
            list.append({1, 10, "Resistors", "R1C1", 4}); // The entry itself.
            for (int i = 0; i < 7; ++i)
                list.append({2 + i, 20 + i, "Hardware", QString("R%1C1").arg(i + 1), 1});
            return list;
        };
        ItemTableModel model;
        model.setLocationProvider(locations);
        Item item{1, 10, "Resistor", "", "", "", 4, "", 20};
        item.totalQuantity = 11;
        model.setItems({item});

        // The cell itself reads "this drawer/total"; the tooltip names the drawers.
        QCOMPARE(model.data(model.index(0, 4), Qt::DisplayRole).toString(), QString("4/11"));
        const QString tip = model.data(model.index(0, 4), Qt::ToolTipRole).toString();
        const QStringList lines = tip.split('\n');
        QCOMPARE(lines[0], QString("Resistors/R1C1 (4)")); // The current drawer first.
        QCOMPARE(lines[1], QString("Hardware/R1C1 (1)"));
        QCOMPARE(lines[5], QString("Hardware/R5C1 (1)")); // Five other drawers at most...
        QCOMPARE(lines[6], QString("+2 more")); // ...then a count of the rest.
        QVERIFY(!tip.contains("4/11")); // The ratio is not repeated.
        QVERIFY(tip.contains("Low stock")); // 11 <= 20.
    }

    void quantityTooltipIsEmptyForAnItemInOneDrawer()
    {
        ItemTableModel model;
        Item item{1, 10, "Resistor", "", "", "", 4, ""};
        item.totalQuantity = 4;
        model.setItems({item});
        QVERIFY(model.data(model.index(0, 4), Qt::ToolTipRole).isNull());
        QCOMPARE(model.data(model.index(0, 4), Qt::DisplayRole).toInt(), 4); // Just the quantity.
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
        auto *bandCount = dialog.findChildren<QComboBox *>()[1]; // After the type selector.
        QVERIFY(bandCount);
        for (int i = 0; i < bandCount->count(); ++i)
            bandCount->setCurrentIndex(i);
    }
};

QTEST_MAIN(SmokeTest)
#include "tst_smoke.moc"
