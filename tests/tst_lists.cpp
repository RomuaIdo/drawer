#include <QLabel>
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
#include "ui/ListModel.h"
#include "ui/ListPanel.h"

namespace {

void onNextDialog(std::function<void(QWidget *)> action)
{
    auto *timer = new QTimer;
    timer->setInterval(5);
    QObject::connect(timer, &QTimer::timeout, [timer, action] {
        if (QWidget *dialog = QApplication::activeModalWidget()) {
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
    Item screw, nut;

    Fixture()
    {
        DrawerGroup g;
        g.cabinetId = cabinets.add({0, "C", ""}).id;
        g.name = "G";
        g.cols = 2;
        drawer = drawers.drawersOf(drawers.addGroup(g).id);
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

        ListPanel panel(ListKind::Pick, f.lists);
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
        ListPanel panel(ListKind::Restock, f.lists);
        auto *table = panel.findChild<QTableView *>();
        QCOMPARE(table->model()->index(0, ListModel::After).data().toInt(), 23);
        QCOMPARE(table->model()->headerData(ListModel::Quantity, Qt::Horizontal).toString(),
                 QString("To add"));
        QVERIFY(buttonText(&panel, "Restock all"));
    }

    void anEmptyListCannotBeApplied()
    {
        Fixture f;
        ListPanel panel(ListKind::Pick, f.lists);
        QVERIFY(!buttonText(&panel, "Take all from stock")->isEnabled());
        QVERIFY(!buttonText(&panel, "Clear")->isEnabled());
        QVERIFY(!buttonText(&panel, "Remove line")->isEnabled());
        QVERIFY(panel.findChild<QLabel *>()->text().contains("empty"));
    }

    void editingTheQuantityCellSavesIt()
    {
        Fixture f;
        f.lists.add(ListKind::Pick, {f.screw.id});
        ListPanel panel(ListKind::Pick, f.lists);
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
        ListPanel panel(ListKind::Pick, f.lists);
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
        ListPanel panel(ListKind::Pick, f.lists);
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
        ListPanel panel(ListKind::Pick, f.lists);

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
        ListPanel panel(ListKind::Restock, f.lists);
        QSignalSpy applied(&panel, &ListPanel::applied);

        onNextDialog([](QWidget *box) { answer(box, QMessageBox::Yes); });
        buttonText(&panel, "Restock all")->click();
        QCOMPARE(f.items.find(f.screw.id)->quantity, 60);
        QCOMPARE(f.items.find(f.nut.id)->quantity, 13);
        QCOMPARE(applied.count(), 1);
    }

    void selectingLinesReportsTheirDrawers()
    {
        Fixture f;
        f.lists.add(ListKind::Pick, {f.screw.id, f.nut.id});
        ListPanel panel(ListKind::Pick, f.lists);
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
