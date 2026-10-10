#include <QtTest>
#include <QSqlQuery>
#include <memory>

#include "data/Database.h"
#include "data/SqliteCabinetRepository.h"
#include "data/SqliteDrawerRepository.h"
#include "data/SqliteItemRepository.h"

class RepositoriesTest : public QObject
{
    Q_OBJECT

private slots:
    void init()
    {
        m_db = std::make_unique<Database>(":memory:");
        m_cabinets = std::make_unique<SqliteCabinetRepository>(m_db->connection());
        m_drawers = std::make_unique<SqliteDrawerRepository>(m_db->connection());
        m_items = std::make_unique<SqliteItemRepository>(m_db->connection());
    }

    void cleanup()
    {
        // Repositories hold connection handles; release them before the database.
        m_items.reset();
        m_drawers.reset();
        m_cabinets.reset();
        m_db.reset();
    }

    void cabinetCrud()
    {
        Cabinet c = m_cabinets->add({0, "Workbench", "Main"});
        QVERIFY(c.id > 0);
        c.name = "Shelf";
        m_cabinets->update(c);
        QCOMPARE(m_cabinets->all().first().name, QString("Shelf"));
        m_cabinets->remove(c.id);
        QVERIFY(m_cabinets->all().isEmpty());
    }

    void groupGeneratesDrawers()
    {
        const int cab = m_cabinets->add({0, "A", ""}).id;
        DrawerGroup g;
        g.cabinetId = cab;
        g.name = "Block";
        g.rows = 3;
        g.cols = 4;
        g.defaultSize = DrawerSize::Small;
        g = m_drawers->addGroup(g);

        const QList<Drawer> drawers = m_drawers->drawersOf(g.id);
        QCOMPARE(drawers.size(), 12);
        QCOMPARE(drawers.first().label, QString("R1C1"));
        QCOMPARE(drawers.last().label, QString("R3C4"));
        QCOMPARE(drawers.first().size, DrawerSize::Small);
        QCOMPARE(m_drawers->groupsOf(cab).size(), 1);
    }

    void groupPositionKeepsFractions()
    {
        DrawerGroup g;
        g.cabinetId = m_cabinets->add({0, "A", ""}).id;
        g.name = "Free";
        g.posX = 2.5;
        g.posY = 3.25;
        g = m_drawers->addGroup(g);
        QCOMPARE(m_drawers->groupsOf(g.cabinetId).first().posX, 2.5);
        g.posX = 4;
        m_drawers->updateGroup(g);
        QCOMPARE(m_drawers->groupsOf(g.cabinetId).first().posX, 4.0);
        QCOMPARE(m_drawers->groupsOf(g.cabinetId).first().posY, 3.25);
    }

    void resizeGrowKeepsExistingDrawers()
    {
        const Drawer first = singleDrawer();
        DrawerGroup g = m_drawers->groupsOf(m_cabinets->all().first().id).first();
        Drawer edited = first;
        edited.label = "Keep me";
        m_drawers->updateDrawer(edited);

        g.cols = 3;
        g.defaultSize = DrawerSize::Large;
        m_drawers->resizeGroup(g, {});
        const QList<Drawer> drawers = m_drawers->drawersOf(g.id);
        QCOMPARE(drawers.size(), 3);
        QCOMPARE(drawers[0].id, first.id);
        QCOMPARE(drawers[0].label, QString("Keep me"));
        QCOMPARE(drawers[2].label, QString("R1C3"));
        QCOMPARE(drawers[2].size, DrawerSize::Large);
        QCOMPARE(m_drawers->groupsOf(g.cabinetId).first().cols, 3);
    }

    void resizeShrinkDropsDrawersAndItems()
    {
        const int cab = m_cabinets->add({0, "A", ""}).id;
        DrawerGroup g;
        g.cabinetId = cab;
        g.name = "Block";
        g.rows = 2;
        g.cols = 2;
        g = m_drawers->addGroup(g);
        const QList<Drawer> before = m_drawers->drawersOf(g.id);
        m_items->add({0, before[0].id, "Stays", "", "", "", 1, ""});
        m_items->add({0, before[3].id, "Goes", "", "", "", 1, ""}); // R2C2

        QCOMPARE(m_drawers->itemsLostByResize(g.id, 1, 2), 1);
        QCOMPARE(m_drawers->itemsLostByResize(g.id, 2, 2), 0);

        g.rows = 1;
        m_drawers->resizeGroup(g, {});
        QCOMPARE(m_drawers->drawersOf(g.id).size(), 2);
        QCOMPARE(m_items->search("Stays").size(), 1);
        QVERIFY(m_items->search("Goes").isEmpty());
    }

    void resizeMovesDisplacedGroupsTogether()
    {
        const int cab = m_cabinets->add({0, "A", ""}).id;
        DrawerGroup a;
        a.cabinetId = cab;
        a.name = "A";
        DrawerGroup b = a;
        b.name = "B";
        b.posX = 1;
        a = m_drawers->addGroup(a);
        b = m_drawers->addGroup(b);

        a.cols = 2;
        b.posX = 2;
        m_drawers->resizeGroup(a, {b});
        const auto groups = m_drawers->groupsOf(cab);
        QCOMPARE(groups[0].cols, 2);
        QCOMPARE(groups[1].posX, 2.0);
    }

    void drawerSizeUpdate()
    {
        const Drawer d = singleDrawer();
        Drawer edited = d;
        edited.size = DrawerSize::Large;
        edited.label = "Big";
        m_drawers->updateDrawer(edited);
        const Drawer stored = m_drawers->drawersOf(d.groupId).first();
        QCOMPARE(stored.size, DrawerSize::Large);
        QCOMPARE(stored.label, QString("Big"));
    }

    void itemCrudAndMove()
    {
        const Drawer a = singleDrawer();
        const Drawer b = singleDrawer();
        Item item = m_items->add({0, a.id, "Resistor", "Passive", "4.7k", "0603", 100, ""});
        QCOMPARE(m_items->inDrawer(a.id).size(), 1);

        item.drawerId = b.id;
        item.quantity = 90;
        m_items->update(item);
        QVERIFY(m_items->inDrawer(a.id).isEmpty());
        QCOMPARE(m_items->inDrawer(b.id).first().quantity, 90);

        m_items->remove(item.id);
        QVERIFY(m_items->inDrawer(b.id).isEmpty());
    }

    void searchMatchesFieldsAndEscapesWildcards()
    {
        const Drawer d = singleDrawer();
        m_items->add({0, d.id, "Capacitor", "Passive", "100nF", "0805", 10, ""});
        m_items->add({0, d.id, "LED", "Optics", "red", "5mm", 5, "50% bright"});
        QCOMPARE(m_items->search("100nf").size(), 1);
        QCOMPARE(m_items->search("optics").size(), 1);
        QCOMPARE(m_items->search("%").size(), 1); // Literal percent, not a wildcard.
    }

    void deletingCabinetCascades()
    {
        const Drawer d = singleDrawer();
        m_items->add({0, d.id, "X", "", "", "", 1, ""});
        m_cabinets->remove(m_cabinets->all().first().id);
        QVERIFY(m_items->search("X").isEmpty());
    }

    void migrationSetsSchemaVersion()
    {
        QSqlQuery q(m_db->connection());
        QVERIFY(q.exec("PRAGMA user_version") && q.next());
        QCOMPARE(q.value(0).toInt(), 1);
    }

private:
    Drawer singleDrawer()
    {
        int cab = m_cabinets->all().isEmpty() ? m_cabinets->add({0, "C", ""}).id
                                              : m_cabinets->all().first().id;
        DrawerGroup g;
        g.cabinetId = cab;
        g.name = "Single";
        return m_drawers->drawersOf(m_drawers->addGroup(g).id).first();
    }

    std::unique_ptr<Database> m_db;
    std::unique_ptr<SqliteCabinetRepository> m_cabinets;
    std::unique_ptr<SqliteDrawerRepository> m_drawers;
    std::unique_ptr<SqliteItemRepository> m_items;
};

QTEST_MAIN(RepositoriesTest)
#include "tst_repositories.moc"
