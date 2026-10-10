#include <QAction>
#include <QMenuBar>
#include <QPushButton>
#include <QSignalSpy>
#include <QTableView>
#include <QtTest>

#include "data/Database.h"
#include "data/SqliteCabinetRepository.h"
#include "data/SqliteDrawerRepository.h"
#include "data/SqliteItemRepository.h"
#include "data/SqliteMappingRepository.h"
#include "data/SqliteOrganizationRepository.h"
#include "data/UndoLog.h"
#include "export/CsvExporter.h"
#include "ui/CabinetView.h"
#include "ui/ItemPanel.h"
#include "ui/MainWindow.h"

// Edit menu behavior: shortcuts, undo/redo, cut/copy/paste and select all.
class MainWindowTest : public QObject
{
    Q_OBJECT

private slots:
    void init()
    {
        m_db = std::make_unique<Database>(":memory:");
        m_cabinets = std::make_unique<SqliteCabinetRepository>(m_db->connection());
        m_drawers = std::make_unique<SqliteDrawerRepository>(m_db->connection());
        m_items = std::make_unique<SqliteItemRepository>(m_db->connection());
        m_mapping = std::make_unique<SqliteMappingRepository>(m_db->connection());
        m_org = std::make_unique<SqliteOrganizationRepository>(m_db->connection());

        // One cabinet with two single-row blocks; the first drawer holds an item.
        const int cabinet = m_cabinets->add({0, "Bench", ""}).id;
        for (int x : {0, 3}) {
            DrawerGroup g;
            g.cabinetId = cabinet;
            g.name = x ? "Right" : "Left";
            g.rows = 1;
            g.cols = 2;
            g.posX = x;
            m_drawers->addGroup(g);
        }
        m_cabinetId = cabinet;
        m_groups = m_drawers->groupsOf(cabinet);
        m_left = m_drawers->drawersOf(m_groups[0].id);
        m_right = m_drawers->drawersOf(m_groups[1].id);
        m_items->add({0, m_left[0].id, "Resistor", "", "", "", 5, ""});
        m_db->undoLog().clear(); // The setup is not part of the history under test.

        m_window = std::make_unique<MainWindow>(
            Repositories{*m_cabinets, *m_drawers, *m_items, *m_mapping, *m_org, m_db->undoLog()},
            m_exporter);
        m_window->show();
        m_view = m_window->findChild<CabinetView *>();
        m_panel = m_window->findChild<ItemPanel *>();
        QVERIFY(m_view && m_panel);
    }

    void cleanup()
    {
        m_window.reset();
        m_org.reset();
        m_mapping.reset();
        m_items.reset();
        m_drawers.reset();
        m_cabinets.reset();
        m_db.reset();
    }

    void editMenuHasTheStandardShortcuts()
    {
        QCOMPARE(action("Undo")->shortcut(), QKeySequence(QKeySequence::Undo));
        QVERIFY(action("Redo")->shortcuts().contains(QKeySequence(Qt::CTRL | Qt::Key_Y)));
        QCOMPARE(action("Cut")->shortcut(), QKeySequence(QKeySequence::Cut));
        QCOMPARE(action("Copy")->shortcut(), QKeySequence(QKeySequence::Copy));
        QCOMPARE(action("Paste")->shortcut(), QKeySequence(QKeySequence::Paste));
        QCOMPARE(action("Select all")->shortcut(), QKeySequence(QKeySequence::SelectAll));
        QVERIFY(!action("Undo")->isEnabled());
        QVERIFY(!action("Redo")->isEnabled());
    }

    void undoAndRedoAGroupMove()
    {
        DrawerGroup moved = m_groups[0];
        moved.posY = 5;
        emit m_view->groupsMoved({moved});
        QCOMPARE(groupY(m_groups[0].id), 5.0);
        QVERIFY(action("Undo")->isEnabled());
        QVERIFY(action("Undo")->text().contains("Move drawer groups"));

        action("Undo")->trigger();
        QCOMPARE(groupY(m_groups[0].id), 0.0);
        QVERIFY(!action("Undo")->isEnabled());
        QVERIFY(action("Redo")->isEnabled());

        action("Redo")->trigger();
        QCOMPARE(groupY(m_groups[0].id), 5.0);
    }

    void cutAndPasteMovesTheItemAndCanBeUndone()
    {
        selectDrawer(m_left[0]);
        panelTable()->selectRow(0);
        action("Cut")->trigger();
        QVERIFY(m_items->inDrawer(m_left[0].id).size() == 1); // Nothing moves until paste.

        selectDrawer(m_right[1]);
        action("Paste")->trigger();
        QVERIFY(m_items->inDrawer(m_left[0].id).isEmpty());
        QCOMPARE(m_items->inDrawer(m_right[1].id).size(), 1);

        action("Paste")->trigger(); // Cut items can only be pasted once.
        QCOMPARE(m_items->inDrawer(m_right[1].id).size(), 1);

        action("Undo")->trigger();
        QCOMPARE(m_items->inDrawer(m_left[0].id).size(), 1);
        QVERIFY(m_items->inDrawer(m_right[1].id).isEmpty());
    }

    void copyAndPasteDuplicatesTheItemAndCanBeRepeated()
    {
        selectDrawer(m_left[0]);
        panelTable()->selectRow(0);
        action("Copy")->trigger();

        selectDrawer(m_right[0]);
        action("Paste")->trigger();
        action("Paste")->trigger();
        QCOMPARE(m_items->inDrawer(m_left[0].id).size(), 1);
        QCOMPARE(m_items->inDrawer(m_right[0].id).size(), 2);

        action("Undo")->trigger(); // One paste at a time.
        QCOMPARE(m_items->inDrawer(m_right[0].id).size(), 1);
        action("Undo")->trigger();
        QVERIFY(m_items->inDrawer(m_right[0].id).isEmpty());
    }

    void itemEditsAndQuantityChangesAreUndoable()
    {
        selectDrawer(m_left[0]);
        panelTable()->selectRow(0);
        QPushButton *increase = nullptr;
        for (QPushButton *button : m_panel->findChildren<QPushButton *>()) {
            if (button->text() == "+")
                increase = button;
        }
        QVERIFY(increase);
        increase->click();
        QCOMPARE(m_items->inDrawer(m_left[0].id).first().quantity, 6);
        QVERIFY(action("Undo")->text().contains("Adjust quantity"));
        action("Undo")->trigger();
        QCOMPARE(m_items->inDrawer(m_left[0].id).first().quantity, 5);
    }

    void selectAllSelectsEveryGroupWhenTheCabinetHasTheFocus()
    {
        action("Select all")->trigger();
        QCOMPARE(m_view->selectedGroups().size(), 2);
    }

    void selectAllSelectsTheItemsWhenTheListHasTheFocus()
    {
        m_items->add({0, m_left[0].id, "Capacitor", "", "", "", 1, ""});
        selectDrawer(m_left[0]);
        m_window->activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(m_window.get()));
        panelTable()->setFocus();
        QTRY_VERIFY(panelTable()->hasFocus());

        action("Select all")->trigger();
        QCOMPARE(panelTable()->selectionModel()->selectedRows().size(), 2);
        QVERIFY(m_view->selectedGroups().isEmpty());
    }

private:
    QAction *action(const QString &name)
    {
        for (QAction *a : m_window->findChildren<QAction *>()) {
            if (QString(a->text()).remove('&').startsWith(name))
                return a;
        }
        return nullptr;
    }

    double groupY(int groupId)
    {
        for (const DrawerGroup &g : m_drawers->groupsOf(m_cabinetId)) {
            if (g.id == groupId)
                return g.posY;
        }
        return -1;
    }

    QTableView *panelTable() { return m_panel->findChild<QTableView *>(); }

    void selectDrawer(const Drawer &drawer) { emit m_view->drawerSelected(drawer); }

    std::unique_ptr<Database> m_db;
    std::unique_ptr<SqliteCabinetRepository> m_cabinets;
    std::unique_ptr<SqliteDrawerRepository> m_drawers;
    std::unique_ptr<SqliteItemRepository> m_items;
    std::unique_ptr<SqliteMappingRepository> m_mapping;
    std::unique_ptr<SqliteOrganizationRepository> m_org;
    CsvExporter m_exporter;
    std::unique_ptr<MainWindow> m_window;
    CabinetView *m_view = nullptr;
    ItemPanel *m_panel = nullptr;
    int m_cabinetId = 0;
    QList<DrawerGroup> m_groups;
    QList<Drawer> m_left;
    QList<Drawer> m_right;
};

QTEST_MAIN(MainWindowTest)
#include "tst_mainwindow.moc"
