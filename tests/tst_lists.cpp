#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalSpy>
#include <QTableView>
#include <QTimer>
#include <QtTest>

#include "data/Database.h"
#include "data/SqliteCabinetRepository.h"
#include "data/SqliteDrawerRepository.h"
#include "data/SqliteItemRepository.h"
#include "data/SqliteListRepository.h"
#include "ui/CabinetView.h"
#include "ui/ItemDialog.h"
#include "ui/ListModel.h"
#include "ui/ListPanel.h"

namespace {

// A message box is modal; the item dialog is not, so the cabinet stays clickable.
QWidget *openDialog()
{
    if (QWidget *modal = QApplication::activeModalWidget())
        return modal;
    for (QWidget *w : QApplication::topLevelWidgets()) {
        if (w->isVisible() && qobject_cast<ItemFormDialog *>(w))
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

void answer(QWidget *box, QMessageBox::StandardButton button)
{
    qobject_cast<QMessageBox *>(box)->button(button)->click();
}

QPushButton *buttonText(QWidget *parent, const QString &text)
{
    for (QPushButton *button : parent->findChildren<QPushButton *>()) {
        if (button->text() == text)
            return button;
    }
    return nullptr;
}

struct Fixture {
    Database db{":memory:"};
    SqliteCabinetRepository cabinets{db.connection()};
    SqliteDrawerRepository drawers{db.connection()};
    SqliteItemRepository items{db.connection()};
    SqliteListRepository lists{db.connection()};
    QList<Drawer> drawer;
    DrawerLayouts layouts;
    Item screw, nut;

    Fixture()
    {
        DrawerGroup g;
        g.cabinetId = cabinets.add({0, "C", ""}).id;
        g.name = "G";
        g.cols = 2;
        g = drawers.addGroup(g);
        drawer = drawers.drawersOf(g.id);
        layouts = {{g, drawer}};
        screw = items.add({0, drawer[0].id, "Screw", "Hardware", "M3", "", 50, ""});
        nut = items.add({0, drawer[1].id, "Nut", "Hardware", "M3", "", 3, ""});
    }
};

} // namespace

class ListsTest : public QObject
{
    Q_OBJECT

private slots:
    void showsTheLinesWithStockAndWhatWillBeLeft()
    {
        Fixture f;
        f.lists.add(ListKind::Pick, {f.screw.id, f.nut.id});
        for (const ListLine &line : f.lists.lines(ListKind::Pick))
            f.lists.setQuantity(line.id, line.item.name == "Screw" ? 8 : 5); // 5 nuts, only 3.

        ListPanel panel(ListKind::Pick, f.lists, f.items);
        auto *table = panel.findChild<QTableView *>();
        QCOMPARE(table->model()->rowCount(), 2);
        auto cell = [&](int row, int column) {
            return table->model()->index(row, column).data().toString();
        };
        QCOMPARE(cell(0, ListModel::Name), QString("Nut")); // Sorted by name.
        QCOMPARE(cell(0, ListModel::Location), QString("G/R1C2"));
        QCOMPARE(cell(0, ListModel::InStock), QString("3"));
        QCOMPARE(cell(0, ListModel::Quantity), QString("5"));
        QCOMPARE(cell(0, ListModel::After), QString("0")); // Never negative.
        QCOMPARE(cell(1, ListModel::After), QString("42"));
        QVERIFY(table->model()->index(0, 0).data(Qt::BackgroundRole).isValid()); // Short.
        QVERIFY(!table->model()->index(1, 0).data(Qt::BackgroundRole).isValid());
        QVERIFY(panel.findChild<QLabel *>()->text().contains("1 line(s) ask for more"));
    }

    void restockShowsWhatTheStockWillBe()
    {
        Fixture f;
        f.lists.add(ListKind::Restock, {f.nut.id});
        f.lists.setQuantity(f.lists.lines(ListKind::Restock).first().id, 20);
        ListPanel panel(ListKind::Restock, f.lists, f.items);
        auto *table = panel.findChild<QTableView *>();
        QCOMPARE(table->model()->index(0, ListModel::After).data().toInt(), 23);
        QCOMPARE(table->model()->headerData(ListModel::Quantity, Qt::Horizontal).toString(),
                 QString("To add"));
        QVERIFY(buttonText(&panel, "Restock all"));
    }

    void anEmptyListCannotBeApplied()
    {
        Fixture f;
        ListPanel panel(ListKind::Pick, f.lists, f.items);
        QVERIFY(!buttonText(&panel, "Take all from stock")->isEnabled());
        QVERIFY(!buttonText(&panel, "Clear")->isEnabled());
        QVERIFY(!buttonText(&panel, "Remove line")->isEnabled());
        QVERIFY(panel.findChild<QLabel *>()->text().contains("empty"));
    }

    void editingTheQuantityCellSavesIt()
    {
        Fixture f;
        f.lists.add(ListKind::Pick, {f.screw.id});
        ListPanel panel(ListKind::Pick, f.lists, f.items);
        QSignalSpy changed(&panel, &ListPanel::listChanged);
        auto *table = panel.findChild<QTableView *>();

        QVERIFY(table->model()->setData(table->model()->index(0, ListModel::Quantity), 12));
        QCOMPARE(f.lists.lines(ListKind::Pick).first().quantity, 12);
        QCOMPARE(changed.count(), 1);
        QVERIFY(!table->model()->setData(table->model()->index(0, ListModel::Quantity), 0));
        QVERIFY(!table->model()->setData(table->model()->index(0, ListModel::Name), "x"));
        QCOMPARE(f.lists.lines(ListKind::Pick).first().quantity, 12);
    }

    void removeAndClearEditTheList()
    {
        Fixture f;
        f.lists.add(ListKind::Pick, {f.screw.id, f.nut.id});
        ListPanel panel(ListKind::Pick, f.lists, f.items);
        QSignalSpy changed(&panel, &ListPanel::listChanged);
        auto *table = panel.findChild<QTableView *>();

        table->selectRow(0); // Nut.
        buttonText(&panel, "Remove line")->click();
        QCOMPARE(f.lists.lines(ListKind::Pick).size(), 1);
        QCOMPARE(f.lists.lines(ListKind::Pick).first().item.name, QString("Screw"));

        buttonText(&panel, "Clear")->click();
        QVERIFY(f.lists.lines(ListKind::Pick).isEmpty());
        QCOMPARE(changed.count(), 2);
        QCOMPARE(panel.lineCount(), 0);
    }

    void applyingAPickListTakesEverythingAfterConfirming()
    {
        Fixture f;
        f.lists.add(ListKind::Pick, {f.screw.id, f.nut.id});
        for (const ListLine &line : f.lists.lines(ListKind::Pick))
            f.lists.setQuantity(line.id, 2);
        ListPanel panel(ListKind::Pick, f.lists, f.items);
        QSignalSpy applied(&panel, &ListPanel::applied);

        onNextDialog([](QWidget *box) { answer(box, QMessageBox::No); });
        buttonText(&panel, "Take all from stock")->click();
        QCOMPARE(f.items.find(f.screw.id)->quantity, 50); // Declined: nothing happens.
        QCOMPARE(applied.count(), 0);

        QString question;
        onNextDialog([&question](QWidget *box) {
            question = qobject_cast<QMessageBox *>(box)->text();
            answer(box, QMessageBox::Yes);
        });
        buttonText(&panel, "Take all from stock")->click();
        QVERIFY(question.contains("Take 4 piece(s) in 2 line(s)"));
        QCOMPARE(f.items.find(f.screw.id)->quantity, 48);
        QCOMPARE(f.items.find(f.nut.id)->quantity, 1);
        QCOMPARE(applied.count(), 1);
        QCOMPARE(panel.lineCount(), 0); // The list is emptied.
    }

    void applyingWarnsAboutLinesThatAskForTooMuch()
    {
        Fixture f;
        f.lists.add(ListKind::Pick, {f.nut.id});
        f.lists.setQuantity(f.lists.lines(ListKind::Pick).first().id, 10); // Only 3 in stock.
        ListPanel panel(ListKind::Pick, f.lists, f.items);

        QString question;
        onNextDialog([&question](QWidget *box) {
            question = qobject_cast<QMessageBox *>(box)->text();
            answer(box, QMessageBox::Yes);
        });
        buttonText(&panel, "Take all from stock")->click();
        QVERIFY(question.contains("ask for more than is in stock"));
        QCOMPARE(f.items.find(f.nut.id)->quantity, 0);
    }

    void applyingARestockListAddsEverything()
    {
        Fixture f;
        f.lists.add(ListKind::Restock, {f.screw.id, f.nut.id});
        for (const ListLine &line : f.lists.lines(ListKind::Restock))
            f.lists.setQuantity(line.id, 10);
        ListPanel panel(ListKind::Restock, f.lists, f.items);
        QSignalSpy applied(&panel, &ListPanel::applied);

        onNextDialog([](QWidget *box) { answer(box, QMessageBox::Yes); });
        buttonText(&panel, "Restock all")->click();
        QCOMPARE(f.items.find(f.screw.id)->quantity, 60);
        QCOMPARE(f.items.find(f.nut.id)->quantity, 13);
        QCOMPARE(applied.count(), 1);
    }

    void onlyTheRestockListHasAnAddButton()
    {
        Fixture f;
        ListPanel pick(ListKind::Pick, f.lists, f.items);
        ListPanel restock(ListKind::Restock, f.lists, f.items);
        pick.show();
        restock.show();
        QVERIFY(!buttonText(&pick, "Add")->isVisible());
        QVERIFY(buttonText(&restock, "Add")->isVisible());
        QVERIFY(!buttonText(&restock, "Add")->isEnabled()); // No drawers to put it in yet.
        restock.setDestinations(f.layouts);
        QVERIFY(buttonText(&restock, "Add")->isEnabled());
    }

    void addCreatesAnEmptyItemInTheClickedDrawerAndListsIt()
    {
        Fixture f;
        ListPanel panel(ListKind::Restock, f.lists, f.items);
        panel.setDestinations(f.layouts);
        CabinetView view;
        view.setLayouts(f.layouts);
        view.show();
        panel.setPickView(&view);
        panel.setDefaultDrawer(f.drawer[0].id);
        panel.show();
        QSignalSpy changed(&panel, &ListPanel::itemsChanged);

        QString drawerText;
        onNextDialog([&](QWidget *dialog) {
            dialog->findChildren<QLineEdit *>()[0]->setText("Zener 5V1");
            CabinetView *picker = &view;
            // Click the second drawer on the main cabinet view.
            QTest::mouseClick(picker, Qt::LeftButton, Qt::NoModifier,
                              picker->drawerRect(f.drawer[1].id).center());
            for (QLabel *label : dialog->findChildren<QLabel *>()) {
                if (label->text().startsWith("G /"))
                    drawerText = label->text();
            }
            dialog->findChild<QSpinBox *>()->setValue(25); // Quantity to add.
            buttonText(dialog, "OK")->click();
        });
        buttonText(&panel, "Add")->click();

        QCOMPARE(drawerText, QString("G / R1C2"));
        const QList<Item> inSecond = f.items.inDrawer(f.drawer[1].id);
        QCOMPARE(inSecond.size(), 2); // The nut was there already.
        Item created;
        for (const Item &item : inSecond) {
            if (item.name == "Zener 5V1")
                created = item;
        }
        QCOMPARE(created.quantity, 0); // Nothing is in stock until the list is applied.
        const QList<ListLine> lines = f.lists.lines(ListKind::Restock);
        QCOMPARE(lines.size(), 1);
        QCOMPARE(lines[0].item.id, created.id);
        QCOMPARE(lines[0].quantity, 25);
        QCOMPARE(changed.count(), 1);

        onNextDialog([](QWidget *box) { answer(box, QMessageBox::Yes); });
        buttonText(&panel, "Restock all")->click();
        QCOMPARE(f.items.find(created.id)->quantity, 25);
    }

    void cancellingTheAddDialogChangesNothing()
    {
        Fixture f;
        ListPanel panel(ListKind::Restock, f.lists, f.items);
        panel.setDestinations(f.layouts);
        CabinetView view;
        view.setLayouts(f.layouts);
        view.show();
        panel.setPickView(&view);
        panel.show();
        onNextDialog([](QWidget *dialog) { buttonText(dialog, "Cancel")->click(); });
        buttonText(&panel, "Add")->click();
        QVERIFY(f.lists.lines(ListKind::Restock).isEmpty());
        QCOMPARE(f.items.inDrawer(f.drawer[0].id).size(), 1);
    }

    void theDialogNeedsADrawerAndAName()
    {
        Fixture f;
        ListPanel panel(ListKind::Restock, f.lists, f.items);
        panel.setDestinations(f.layouts);
        CabinetView view;
        view.setLayouts(f.layouts);
        view.show();
        panel.setPickView(&view); // No default drawer chosen.
        panel.show();
        bool okAfterNameOnly = true;
        bool okAfterDrawer = false;
        onNextDialog([&](QWidget *dialog) {
            dialog->findChildren<QLineEdit *>()[0]->setText("Thing");
            okAfterNameOnly = buttonText(dialog, "OK")->isEnabled();
            CabinetView *picker = &view;
            QTest::mouseClick(picker, Qt::LeftButton, Qt::NoModifier,
                              picker->drawerRect(f.drawer[0].id).center());
            okAfterDrawer = buttonText(dialog, "OK")->isEnabled();
            buttonText(dialog, "Cancel")->click();
        });
        buttonText(&panel, "Add")->click();
        QVERIFY(!okAfterNameOnly); // Name filled, but no drawer picked yet.
        QVERIFY(okAfterDrawer);
    }

    void selectingLinesReportsTheirDrawers()
    {
        Fixture f;
        f.lists.add(ListKind::Pick, {f.screw.id, f.nut.id});
        ListPanel panel(ListKind::Pick, f.lists, f.items);
        QSignalSpy chosen(&panel, &ListPanel::drawersChosen);
        panel.findChild<QTableView *>()->selectAll();
        QCOMPARE(panel.selectedDrawerIds().size(), 2);
        QVERIFY(chosen.count() >= 1);
        panel.clearSelection();
        QVERIFY(panel.selectedDrawerIds().isEmpty());
    }
};

QTEST_MAIN(ListsTest)
#include "tst_lists.moc"
