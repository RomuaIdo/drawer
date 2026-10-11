#include <QCheckBox>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTimer>
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
#include "ui/ItemDialog.h"

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

// Runs `action` on the next modal dialog that appears (message boxes, item dialogs).
// A message box is modal; the item dialog is not, so the cabinet stays clickable.
QWidget *openDialog()
{
    if (QWidget *modal = QApplication::activeModalWidget())
        return modal;
    for (QWidget *w : QApplication::topLevelWidgets()) {
        if (w->isVisible() && qobject_cast<ItemDialog *>(w))
            return w;
    }
    return nullptr;
}

void onNextDialog(std::function<void(QWidget *)> action)
{
    auto *timer = new QTimer;
    timer->setInterval(5);
    QObject::connect(timer, &QTimer::timeout, [timer, action] {
        if (QWidget *dialog = openDialog()) {
            timer->stop();
            timer->deleteLater();
            action(dialog);
        }
    });
    timer->start();
}

void answerYes(QWidget *box)
{
    qobject_cast<QMessageBox *>(box)->button(QMessageBox::Yes)->click();
}

QPushButton *buttonText(QWidget *parent, const QString &text)
{
    for (QPushButton *button : parent->findChildren<QPushButton *>()) {
        if (button->text() == text)
            return button;
    }
    return nullptr;
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
    // A cabinet with one 3-drawer block, and a panel on it. Screw lives in drawers 0 and 2.
    struct Fixture {
        Database db{":memory:"};
        SqliteCabinetRepository cabinets{db.connection()};
        SqliteDrawerRepository drawers{db.connection()};
        SqliteItemRepository items{db.connection()};
        int cabinetId = 0;
        QList<Drawer> drawer;
        std::unique_ptr<AllItemsPanel> panel;
        CabinetView view; // The main window's cabinet, where drawers are clicked.
        QTableView *table = nullptr;

        Fixture()
        {
            DrawerGroup g;
            g.cabinetId = cabinetId = cabinets.add({0, "C", ""}).id;
            g.name = "G";
            g.cols = 3;
            drawer = drawers.drawersOf(drawers.addGroup(g).id);
            items.add({0, drawer[0].id, "Screw", "Hardware", "M3", "", 5, "", 20});
            items.add({0, drawer[2].id, "Screw", "Hardware", "M3", "", 7, ""});
            items.add({0, drawer[1].id, "Washer", "Hardware", "M3", "", 1, ""});

            panel = std::make_unique<AllItemsPanel>(items);
            CabinetView::GroupLayout layout;
            layout.group = drawers.groupsOf(cabinetId).first();
            layout.drawers = drawer;
            panel->setDestinations({layout});
            view.setLayouts({layout});
            view.show();
            panel->setPickView(&view);
            panel->setCabinet(cabinetId);
            panel->show();
            table = panel->findChild<QTableView *>();
        }

        int quantityIn(int drawerIndex, const QString &name)
        {
            for (const Item &item : items.inDrawer(drawer[drawerIndex].id)) {
                if (item.name == name)
                    return item.quantity;
            }
            return -1;
        }
    };

private slots:
    void plusAndMinusAdjustTheSelectedEntry()
    {
        Fixture f;
        QSignalSpy changed(f.panel.get(), &AllItemsPanel::itemsChanged);
        f.table->selectRow(0); // Screw in the first drawer.
        QPushButton *plus = buttonText(f.panel.get(), "+");
        QPushButton *minus = buttonText(f.panel.get(), QString::fromUtf8("\u2212"));
        QVERIFY(plus->isEnabled() && minus->isEnabled());

        plus->click();
        plus->click();
        QCOMPARE(f.quantityIn(0, "Screw"), 7);
        minus->click();
        QCOMPARE(f.quantityIn(0, "Screw"), 6);
        QCOMPARE(f.quantityIn(2, "Screw"), 7); // The other entry is untouched.
        QCOMPARE(changed.count(), 3);
        QVERIFY(f.table->selectionModel()->hasSelection()); // Still selected for the next click.
        QCOMPARE(changed.last()[0].toString(), QString("Adjust quantity"));
    }

    void plusAndMinusAreOffWhileDuplicatesAreGrouped()
    {
        Fixture f;
        f.panel->findChild<QCheckBox *>()->setChecked(true);
        f.table->selectRow(0);
        QVERIFY(!buttonText(f.panel.get(), "+")->isEnabled());
        QVERIFY(!buttonText(f.panel.get(), QString::fromUtf8("\u2212"))->isEnabled());
        QVERIFY(buttonText(f.panel.get(), "Edit")->isEnabled());
        QVERIFY(buttonText(f.panel.get(), "Remove")->isEnabled());
        f.panel->adjustQuantity(+1); // Ignored even when called directly.
        QCOMPARE(f.quantityIn(0, "Screw"), 5);

        f.panel->findChild<QCheckBox *>()->setChecked(false);
        f.table->selectRow(0);
        QVERIFY(buttonText(f.panel.get(), "+")->isEnabled());
    }

    void addCreatesAnItemInTheChosenDrawer()
    {
        Fixture f;
        QSignalSpy changed(f.panel.get(), &AllItemsPanel::itemsChanged);
        f.panel->setDefaultDrawer(f.drawer[1].id);
        onNextDialog([](QWidget *dialog) {
            dialog->findChildren<QLineEdit *>()[0]->setText("Nut");
            dialog->findChild<QSpinBox *>()->setValue(40);
            buttonText(dialog, "OK")->click();
        });
        buttonText(f.panel.get(), "Add")->click();

        QCOMPARE(f.quantityIn(1, "Nut"), 40); // In the default drawer.
        QCOMPARE(f.table->model()->rowCount(), 4);
        QCOMPARE(changed.count(), 1);
        QCOMPARE(changed[0][0].toString(), QString("Add item"));
    }

    void theAddDialogPicksTheDrawerByClickingIt()
    {
        Fixture f;
        f.panel->setDefaultDrawer(f.drawer[0].id);
        onNextDialog([&](QWidget *dialog) {
            dialog->findChildren<QLineEdit *>()[0]->setText("Bolt");
            CabinetView *picker = &f.view;
            QTest::mouseClick(picker, Qt::LeftButton, Qt::NoModifier,
                              picker->drawerRect(f.drawer[2].id).center()); // Not the default.
            buttonText(dialog, "OK")->click();
        });
        buttonText(f.panel.get(), "Add")->click();
        QCOMPARE(f.quantityIn(2, "Bolt"), 0);
        QCOMPARE(f.quantityIn(0, "Bolt"), -1);
    }

    void theEditDialogCanMoveTheItemByClickingAnotherDrawer()
    {
        Fixture f;
        f.table->selectRow(2); // Washer, in the middle drawer.
        onNextDialog([&](QWidget *dialog) {
            CabinetView *picker = &f.view;
            QTest::mouseClick(picker, Qt::LeftButton, Qt::NoModifier,
                              picker->drawerRect(f.drawer[2].id).center());
            buttonText(dialog, "OK")->click();
        });
        buttonText(f.panel.get(), "Edit")->click();
        QCOMPARE(f.quantityIn(1, "Washer"), -1);
        QCOMPARE(f.quantityIn(2, "Washer"), 1);
    }

    void editChangesOneEntry()
    {
        Fixture f;
        f.table->selectRow(0);
        onNextDialog([](QWidget *dialog) {
            dialog->findChildren<QLineEdit *>()[0]->setText("Screw M3");
            buttonText(dialog, "OK")->click();
        });
        buttonText(f.panel.get(), "Edit")->click();
        QCOMPARE(f.items.inDrawer(f.drawer[0].id)[0].name, QString("Screw M3"));
        QCOMPARE(f.items.inDrawer(f.drawer[2].id)[0].name, QString("Screw")); // Only that entry.
    }

    void editOfAGroupedItemShowsAQuantityTableForEveryOccurrence()
    {
        Fixture f;
        f.panel->findChild<QCheckBox *>()->setChecked(true);
        f.table->selectRow(0); // Screw, in two drawers.
        int occurrences = 0;
        onNextDialog([&occurrences](QWidget *dialog) {
            auto *sub = dialog->findChild<QTableWidget *>();
            occurrences = sub->rowCount();
            qobject_cast<QSpinBox *>(sub->cellWidget(0, 1))->setValue(11);
            qobject_cast<QSpinBox *>(sub->cellWidget(1, 1))->setValue(2);
            dialog->findChildren<QLineEdit *>()[0]->setText("Screw M3"); // Common field.
            buttonText(dialog, "OK")->click();
        });
        buttonText(f.panel.get(), "Edit")->click();

        QCOMPARE(occurrences, 2);
        QCOMPARE(f.quantityIn(0, "Screw M3"), 11);
        QCOMPARE(f.quantityIn(2, "Screw M3"), 2);
        QVERIFY(f.table->selectionModel()->hasSelection());
        QCOMPARE(f.table->model()->rowCount(), 2); // Still one row for the item, plus Washer.
        QCOMPARE(f.items.inDrawer(f.drawer[0].id)[0].totalQuantity, 13);
    }

    void groupedEditAppliesTheMinimumStockToEveryEntry()
    {
        Fixture f;
        f.panel->findChild<QCheckBox *>()->setChecked(true);
        f.table->selectRow(0);
        onNextDialog([](QWidget *dialog) {
            auto *alert = dialog->findChild<QCheckBox *>();
            alert->setChecked(false); // Remove the alert set on the first entry.
            buttonText(dialog, "OK")->click();
        });
        buttonText(f.panel.get(), "Edit")->click();
        QVERIFY(!f.items.inDrawer(f.drawer[0].id)[0].minStock);
        QVERIFY(!f.items.inDrawer(f.drawer[2].id)[0].minStock);
    }

    void removeAsksAndDeletesTheEntry()
    {
        Fixture f;
        f.table->selectRow(0);
        onNextDialog(answerYes);
        buttonText(f.panel.get(), "Remove")->click();
        QVERIFY(f.items.inDrawer(f.drawer[0].id).isEmpty());
        QCOMPARE(f.items.inDrawer(f.drawer[2].id).size(), 1);
        QCOMPARE(f.table->model()->rowCount(), 2);
    }

    void removeOfAGroupedItemRemovesEveryEntry()
    {
        Fixture f;
        f.panel->findChild<QCheckBox *>()->setChecked(true);
        f.table->selectRow(0);
        onNextDialog(answerYes);
        buttonText(f.panel.get(), "Remove")->click();
        QVERIFY(f.items.inDrawer(f.drawer[0].id).isEmpty());
        QVERIFY(f.items.inDrawer(f.drawer[2].id).isEmpty());
        QCOMPARE(f.items.inDrawer(f.drawer[1].id).size(), 1); // Washer stays.
    }

    void declinedRemoveKeepsEverything()
    {
        Fixture f;
        f.table->selectRow(0);
        onNextDialog([](QWidget *box) {
            qobject_cast<QMessageBox *>(box)->button(QMessageBox::No)->click();
        });
        buttonText(f.panel.get(), "Remove")->click();
        QCOMPARE(f.items.inDrawer(f.drawer[0].id).size(), 1);
    }

private:
    static QString text(const AllItemsModel &model, int row, int column)
    {
        return model.data(model.index(row, column), Qt::DisplayRole).toString();
    }
};

QTEST_MAIN(AllItemsTest)
#include "tst_allitems.moc"
