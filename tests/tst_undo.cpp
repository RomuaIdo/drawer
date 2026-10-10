#include <QtTest>

#include "data/Database.h"
#include "data/SqliteCabinetRepository.h"
#include "data/SqliteDrawerRepository.h"
#include "data/SqliteItemRepository.h"
#include "data/SqliteOrganizationRepository.h"
#include "data/UndoLog.h"
#include "export/OrganizationJson.h"

class UndoTest : public QObject
{
    Q_OBJECT

private slots:
    void init()
    {
        m_db = std::make_unique<Database>(":memory:");
        m_cabinets = std::make_unique<SqliteCabinetRepository>(m_db->connection());
        m_drawers = std::make_unique<SqliteDrawerRepository>(m_db->connection());
        m_items = std::make_unique<SqliteItemRepository>(m_db->connection());
        m_org = std::make_unique<SqliteOrganizationRepository>(m_db->connection());
    }

    void cleanup()
    {
        m_org.reset();
        m_items.reset();
        m_drawers.reset();
        m_cabinets.reset();
        m_db.reset();
    }

    void nothingToUndoAtFirst()
    {
        QVERIFY(!history().canUndo());
        QVERIFY(!history().canRedo());
        history().commit("Empty"); // No changes: no step.
        QVERIFY(!history().canUndo());
    }

    void undoAndRedoAnItemEdit()
    {
        const Drawer d = drawerWithItem();
        history().commit("Setup");
        Item item = m_items->inDrawer(d.id).first();
        item.quantity = 99;
        item.name = "Renamed";
        m_items->update(item);
        history().commit("Edit item");

        QCOMPARE(history().undoLabel(), QString("Edit item"));
        history().undo();
        QCOMPARE(m_items->inDrawer(d.id).first().quantity, 5);
        QCOMPARE(m_items->inDrawer(d.id).first().name, QString("Resistor"));
        QCOMPARE(history().redoLabel(), QString("Edit item"));

        history().redo();
        QCOMPARE(m_items->inDrawer(d.id).first().quantity, 99);
        QCOMPARE(m_items->inDrawer(d.id).first().name, QString("Renamed"));
    }

    void undoRestoresADeletedItemWithItsId()
    {
        const Drawer d = drawerWithItem();
        history().commit("Setup");
        const Item before = m_items->inDrawer(d.id).first();
        m_items->remove(before.id);
        history().commit("Remove item");
        QVERIFY(m_items->inDrawer(d.id).isEmpty());

        history().undo();
        const QList<Item> restored = m_items->inDrawer(d.id);
        QCOMPARE(restored.size(), 1);
        QCOMPARE(restored[0].id, before.id);
        QCOMPARE(*restored[0].minStock, 2);
        QCOMPARE(restored[0].notes, before.notes);
    }

    void undoRestoresADeletedCabinetWithEverythingInside()
    {
        drawerWithItem();
        history().commit("Setup");
        const QByteArray before = snapshot();

        m_cabinets->remove(m_cabinets->all().first().id); // Cascades to groups, drawers, items.
        history().commit("Delete cabinet");
        QVERIFY(m_cabinets->all().isEmpty());
        QVERIFY(m_items->search("Resistor").isEmpty());

        history().undo();
        QCOMPARE(snapshot(), before);
        QCOMPARE(m_items->search("Resistor").size(), 1);

        history().redo();
        QVERIFY(m_cabinets->all().isEmpty());
        history().undo();
        QCOMPARE(snapshot(), before);
    }

    void undoRestoresDrawersAndItemsRemovedByAShrink()
    {
        const int cab = m_cabinets->add({0, "A", ""}).id;
        DrawerGroup g;
        g.cabinetId = cab;
        g.name = "Block";
        g.rows = 2;
        g.cols = 2;
        g = m_drawers->addGroup(g);
        const QList<Drawer> drawers = m_drawers->drawersOf(g.id);
        m_items->add({0, drawers[3].id, "In the corner", "", "", "", 1, ""});
        history().commit("Setup");
        const QByteArray before = snapshot();

        g.rows = 1;
        m_drawers->resizeGroup(g, {});
        history().commit("Resize group");
        QCOMPARE(m_drawers->drawersOf(g.id).size(), 2);

        history().undo();
        QCOMPARE(snapshot(), before);
    }

    void undoRestoresGroupPositionsExactly()
    {
        const int cab = m_cabinets->add({0, "A", ""}).id;
        DrawerGroup g;
        g.cabinetId = cab;
        g.name = "G";
        g.posX = 0.1 + 0.2;
        g = m_drawers->addGroup(g);
        history().commit("Setup");

        g.posX = 7;
        g.posY = 3.5;
        m_drawers->moveGroups({g});
        history().commit("Move");

        history().undo();
        QCOMPARE(m_drawers->groupsOf(cab).first().posX, 0.1 + 0.2);
        QCOMPARE(m_drawers->groupsOf(cab).first().posY, 0.0);
    }

    void severalStepsUndoInOrderAndNewWorkClearsRedo()
    {
        const Drawer d = drawerWithItem();
        const int itemId = m_items->inDrawer(d.id).first().id;
        history().commit("Setup");

        m_items->adjustQuantity({itemId}, +1);
        history().commit("Plus");
        m_items->adjustQuantity({itemId}, +1);
        history().commit("Plus again");
        QCOMPARE(m_items->inDrawer(d.id).first().quantity, 7);

        history().undo();
        QCOMPARE(m_items->inDrawer(d.id).first().quantity, 6);
        history().undo();
        QCOMPARE(m_items->inDrawer(d.id).first().quantity, 5);
        QCOMPARE(history().undoLabel(), QString("Setup"));
        QVERIFY(history().canRedo());

        m_items->adjustQuantity({itemId}, -2);
        history().commit("Minus");
        QVERIFY(!history().canRedo()); // The undone steps are gone.
        QCOMPARE(m_items->inDrawer(d.id).first().quantity, 3);
    }

    void undoRevertsAnImportAsOneStep()
    {
        drawerWithItem();
        history().commit("Setup");
        const QByteArray before = snapshot();

        m_org->import(m_org->load());
        history().commit("Import");
        QCOMPARE(m_cabinets->all().size(), 2);

        history().undo();
        QCOMPARE(snapshot(), before);
        history().redo();
        QCOMPARE(m_cabinets->all().size(), 2);
    }

    void uncommittedChangesAreUndoneFirst()
    {
        const Drawer d = drawerWithItem();
        history().commit("Setup");
        m_items->add({0, d.id, "Forgot to commit", "", "", "", 1, ""});
        history().undo();
        QVERIFY(m_items->search("Forgot").isEmpty());
        QCOMPARE(m_items->search("Resistor").size(), 1);
    }

private:
    UndoLog &history() { return m_db->undoLog(); }

    QByteArray snapshot() { return OrganizationJson::write(m_org->load()); }

    Drawer drawerWithItem()
    {
        const int cab = m_cabinets->add({0, "Bench", "d"}).id;
        DrawerGroup g;
        g.cabinetId = cab;
        g.name = "Block";
        g.color = "#3366cc";
        g = m_drawers->addGroup(g);
        const Drawer d = m_drawers->drawersOf(g.id).first();
        m_items->add({0, d.id, "Resistor", "Passive", "4.7k", "0603", 5, "it's \"quoted\"", 2});
        return d;
    }

    std::unique_ptr<Database> m_db;
    std::unique_ptr<SqliteCabinetRepository> m_cabinets;
    std::unique_ptr<SqliteDrawerRepository> m_drawers;
    std::unique_ptr<SqliteItemRepository> m_items;
    std::unique_ptr<SqliteOrganizationRepository> m_org;
};

QTEST_MAIN(UndoTest)
#include "tst_undo.moc"
