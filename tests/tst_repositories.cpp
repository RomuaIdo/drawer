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
