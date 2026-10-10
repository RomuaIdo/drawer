#include <QtTest>

#include "data/Database.h"
#include "data/ItemSearch.h"
#include "data/SqliteCabinetRepository.h"
#include "data/SqliteDrawerRepository.h"
#include "data/SqliteItemRepository.h"

class SearchTest : public QObject
{
    Q_OBJECT

private slots:
    void init()
    {
        m_db = std::make_unique<Database>(":memory:");
        m_items = std::make_unique<SqliteItemRepository>(m_db->connection());
        SqliteCabinetRepository cabinets(m_db->connection());
        SqliteDrawerRepository drawers(m_db->connection());
        DrawerGroup g;
        g.cabinetId = cabinets.add({0, "C", ""}).id;
        g.name = "G";
        g.cols = 8;
        const QList<Drawer> d = drawers.drawersOf(drawers.addGroup(g).id);
        auto add = [&](int drawer, const char *name, const char *category, const char *value) {
            m_items->add({0, d[drawer].id, name, category, value, "", 1, ""});
        };
        add(0, "Cap A", "Capacitor", "1u");
        add(1, "Cap B", "Capacitor", "1000nF");
        add(2, "Cap C", "capacitor ", "1µF");
        add(3, "Cap D", "Capacitor", "100n");
        add(4, "Res A", "Resistor", "10k");
        add(5, "Res B", "Resistor", "10kΩ");
        add(6, "Res C", "Resistor", "4k7");
        add(7, "Diode", "Semiconductor", "1N4148");
    }

    void cleanup()
    {
        m_items.reset();
        m_db.reset();
    }

    void aValueFindsEveryWayOfWritingIt()
    {
        QCOMPARE(names({"1uF", ""}), (QStringList{"Cap A", "Cap B", "Cap C"}));
        QCOMPARE(names({"1u", ""}), (QStringList{"Cap A", "Cap B", "Cap C"}));
        QCOMPARE(names({"0.001m", ""}), (QStringList{"Cap A", "Cap B", "Cap C"}));
        QCOMPARE(names({"10k", ""}), (QStringList{"Res A", "Res B"}));
        QCOMPARE(names({"4k7", ""}), (QStringList{"Res C"}));
        QCOMPARE(names({"4.7k", ""}), (QStringList{"Res C"}));
    }

    void aValueDoesNotMatchOtherValues()
    {
        QVERIFY(!names({"1uF", ""}).contains("Cap D")); // 100n is another value.
        QVERIFY(names({"2.2u", ""}).isEmpty());
    }

    void textStillMatchesTheTextFields()
    {
        QCOMPARE(names({"diode", ""}), (QStringList{"Diode"}));
        QCOMPARE(names({"resistor", ""}), (QStringList{"Res A", "Res B", "Res C"}));
        QCOMPARE(names({"1N41", ""}), (QStringList{"Diode"}));
        QVERIFY(names({"nothing like this", ""}).isEmpty());
    }

    void aTextThatIsAlsoAValueCountsEachItemOnce()
    {
        // "1u" matches "1u" as text and as a value; Cap A must not appear twice.
        const QList<Item> found = ItemSearch::run(*m_items, {"1u", ""});
        QCOMPARE(found.size(), 3);
        QSet<int> ids;
        for (const Item &item : found)
            ids.insert(item.id);
        QCOMPARE(ids.size(), 3);
        QCOMPARE(names({"100n", ""}), (QStringList{"Cap D"}));
    }

    void categoryAloneListsItsItems()
    {
        QCOMPARE(names({"", "Resistor"}), (QStringList{"Res A", "Res B", "Res C"}));
        QCOMPARE(names({"", "RESISTOR"}), (QStringList{"Res A", "Res B", "Res C"}));
        QCOMPARE(names({"", "Capacitor"}).size(), 4); // Stray spaces and case do not split it.
    }

    void categoryRestrictsTheText()
    {
        QCOMPARE(names({"10k", "Resistor"}), (QStringList{"Res A", "Res B"}));
        QVERIFY(names({"10k", "Capacitor"}).isEmpty());
        QCOMPARE(names({"1uF", "Capacitor"}).size(), 3);
        QCOMPARE(names({"Cap", "Resistor"}), QStringList());
    }

    void emptyQueryMatchesNothing()
    {
        QVERIFY(ItemSearch::run(*m_items, {"", ""}).isEmpty());
        QVERIFY(ItemSearch::run(*m_items, {"  ", " "}).isEmpty());
    }

    void categoriesAreDistinctAndSorted()
    {
        QCOMPARE(m_items->categories(), (QStringList{"Capacitor", "Resistor", "Semiconductor"}));
    }

private:
    QStringList names(const SearchQuery &query)
    {
        QStringList result;
        for (const Item &item : ItemSearch::run(*m_items, query))
            result << item.name;
        result.sort();
        return result;
    }

    std::unique_ptr<Database> m_db;
    std::unique_ptr<SqliteItemRepository> m_items;
};

QTEST_MAIN(SearchTest)
#include "tst_search.moc"
