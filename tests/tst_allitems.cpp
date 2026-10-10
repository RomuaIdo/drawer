#include <QCheckBox>
#include <QSignalSpy>
#include <QTableView>
#include <QtTest>

#include "data/Database.h"
#include "data/SqliteCabinetRepository.h"
#include "data/SqliteDrawerRepository.h"
#include "data/SqliteItemRepository.h"
#include "ui/AllItemsModel.h"
#include "ui/AllItemsPanel.h"
#include "ui/CabinetView.h"

namespace {

ItemEntry entry(int id, int drawerId, const QString &name, int quantity, int total,
                const QString &identity, const QString &group, const QString &drawer,
                std::optional<int> min = std::nullopt, const QString &notes = QString())
{
    Item item{id, drawerId, name, "Cat", "10k", "0603", quantity, notes, min};
    item.totalQuantity = total;
    return {item, identity, group, drawer};
}

QList<ItemEntry> sampleEntries()
{
    return {entry(1, 11, "Diode", 2, 2, "diode", "Left", "R1C1"),
            entry(2, 21, "Resistor", 10, 14, "resistor", "Left", "R1C1", 20, "from a reel"),
            entry(3, 22, "Resistor", 4, 14, "resistor", "Left", "R2C3", 20, "cut tape")};
}

} // namespace

class AllItemsTest : public QObject
{
    Q_OBJECT

private slots:
    void ungroupedShowsOneRowPerEntryWithItsLocation()
    {
        AllItemsModel model;
        model.setEntries(sampleEntries());
        QCOMPARE(model.rowCount(), 3);
        QCOMPARE(text(model, 1, AllItemsModel::Location), QString("Left/R1C1"));
        QCOMPARE(text(model, 2, AllItemsModel::Location), QString("Left/R2C3"));
        QCOMPARE(model.headerData(AllItemsModel::Location, Qt::Horizontal).toString(),
                 QString("Location / Drawer"));
        QCOMPARE(model.drawerIdsAt(2), (QList<int>{22}));
        QVERIFY(!model.grouped()); // Off by default.
    }

    void groupedMergesDuplicatesIntoOneRow()
    {
        AllItemsModel model;
        model.setEntries(sampleEntries());
        model.setGrouped(true);
        QCOMPARE(model.rowCount(), 2);
        QCOMPARE(model.headerData(AllItemsModel::Location, Qt::Horizontal).toString(),
                 QString("Drawers"));
        QCOMPARE(text(model, 1, AllItemsModel::Name), QString("Resistor"));
        QCOMPARE(text(model, 1, AllItemsModel::Location), QString("R1C1 (10), R2C3 (4)"));
        QCOMPARE(text(model, 1, AllItemsModel::Quantity), QString("14")); // The total.
        QCOMPARE(text(model, 1, AllItemsModel::Notes), QString("from a reel; cut tape"));
        QCOMPARE(model.drawerIdsAt(1), (QList<int>{21, 22}));
        QCOMPARE(text(model, 0, AllItemsModel::Location), QString("R1C1 (2)"));
    }

    void groupedNamesTheGroupWhenDrawersSpanSeveralGroups()
    {
        QList<ItemEntry> entries = sampleEntries();
        entries[2].group = "Right";
        AllItemsModel model;
        model.setEntries(entries);
        model.setGrouped(true);
        QCOMPARE(text(model, 1, AllItemsModel::Location), QString("Left/R1C1 (10), Right/R2C3 (4)"));
    }

    void entriesInTheSameDrawerCountAsOnePlace()
    {
        QList<ItemEntry> entries = {entry(1, 11, "Nut", 3, 8, "nut", "G", "R1C1"),
                                    entry(2, 11, "nut", 5, 8, "nut", "G", "R1C1")};
        AllItemsModel model;
        model.setEntries(entries);
        model.setGrouped(true);
        QCOMPARE(model.rowCount(), 1);
        QCOMPARE(text(model, 0, AllItemsModel::Location), QString("R1C1 (8)"));
        QCOMPARE(model.drawerIdsAt(0), (QList<int>{11}));
    }

    void lowStockRowsAreTintedByTheirTotal()
    {
        AllItemsModel model;
        model.setEntries(sampleEntries()); // Resistor: 14 in total, minimum 20.
        QVERIFY(model.data(model.index(1, 0), Qt::BackgroundRole).isValid());
        QVERIFY(model.data(model.index(2, 0), Qt::BackgroundRole).isValid());
        QVERIFY(!model.data(model.index(0, 0), Qt::BackgroundRole).isValid());
        QCOMPARE(model.data(model.index(1, AllItemsModel::Quantity), Qt::DisplayRole).toString(),
                 QString("10/14")); // Stored elsewhere too: this entry over the total.
        QCOMPARE(model.data(model.index(0, AllItemsModel::Quantity), Qt::DisplayRole).toInt(), 2);
    }

    void panelReportsTheDrawersOfTheSelectedRow()
    {
        Database db(":memory:");
        SqliteCabinetRepository cabinets(db.connection());
        SqliteDrawerRepository drawers(db.connection());
        SqliteItemRepository items(db.connection());
        DrawerGroup g;
        g.cabinetId = cabinets.add({0, "C", ""}).id;
        g.name = "G";
        g.cols = 3;
        g = drawers.addGroup(g);
        const QList<Drawer> d = drawers.drawersOf(g.id);
        items.add({0, d[0].id, "Screw", "", "M3", "", 5, ""});
        items.add({0, d[2].id, "Screw", "", "M3", "", 7, ""});
        items.add({0, d[1].id, "Washer", "", "", "", 1, ""});

        AllItemsPanel panel(items);
        panel.setCabinet(g.cabinetId);
        panel.show();
        auto *table = panel.findChild<QTableView *>();
        QCOMPARE(table->model()->rowCount(), 3);

        QSignalSpy chosen(&panel, &AllItemsPanel::drawersChosen);
        table->selectRow(0); // First Screw.
        QCOMPARE(chosen.count(), 1);
        QCOMPARE(chosen[0][0].value<QList<int>>(), (QList<int>{d[0].id}));

        panel.findChild<QCheckBox *>()->setChecked(true); // Group duplicates.
        QCOMPARE(table->model()->rowCount(), 2);
        table->selectRow(0); // Screw, both drawers.
        QCOMPARE(chosen.last()[0].value<QList<int>>(), (QList<int>{d[0].id, d[2].id}));

        panel.clearSelection();
        QVERIFY(chosen.last()[0].value<QList<int>>().isEmpty());
    }

    void viewDarkensTheEmphasizedDrawers()
    {
        CabinetView view;
        CabinetView::GroupLayout l;
        l.group.id = 1;
        l.group.rows = 1;
        l.group.cols = 2;
        l.drawers = {{10, 1, 0, 0, DrawerSize::Medium, "a"}, {11, 1, 0, 1, DrawerSize::Medium, "b"}};
        view.setLayouts({l});

        auto sample = [&](int drawerId) {
            const QRect r = view.drawerRect(drawerId);
            return view.grab().toImage().pixelColor(r.left() + 8, r.bottom() - 14);
        };
        const QColor plain = sample(10);
        view.setEmphasizedDrawers({10});
        QVERIFY(sample(10).lightness() < plain.lightness());
        QCOMPARE(sample(11).rgb(), plain.rgb()); // Only the chosen drawers change.
        QVERIFY(view.drawerRect(999).isNull());
        view.setEmphasizedDrawers({});
        QCOMPARE(sample(10).rgb(), plain.rgb());
    }

private:
    static QString text(const AllItemsModel &model, int row, int column)
    {
        return model.data(model.index(row, column), Qt::DisplayRole).toString();
    }
};

QTEST_MAIN(AllItemsTest)
#include "tst_allitems.moc"
