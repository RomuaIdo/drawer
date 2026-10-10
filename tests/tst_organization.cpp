#include <QtTest>

#include "data/Database.h"
#include "data/DatabaseError.h"
#include "data/SqliteCabinetRepository.h"
#include "data/SqliteDrawerRepository.h"
#include "data/SqliteItemRepository.h"
#include "data/SqliteOrganizationRepository.h"
#include "export/OrganizationJson.h"

namespace {

// Builds a small organization through the repositories.
void populate(Database &db)
{
    SqliteCabinetRepository cabinets(db.connection());
    SqliteDrawerRepository drawers(db.connection());
    SqliteItemRepository items(db.connection());

    const int cab = cabinets.add({0, "Bench", "Main \"cabinet\""}).id;
    DrawerGroup block;
    block.cabinetId = cab;
    block.name = "Block";
    block.rows = 2;
    block.cols = 2;
    block.defaultSize = DrawerSize::Small;
    block.posX = 1.5;
    block.color = "#3366cc";
    block = drawers.addGroup(block);

    Drawer first = drawers.drawersOf(block.id).first();
    first.size = DrawerSize::Large;
    first.label = "Big one";
    drawers.updateDrawer(first);
    items.add({0, first.id, "Resistor", "Passive", "4.7k", "0603", 25, "ünïcode, comma", 10});
}

QJsonObject minimalFile()
{
    return QJsonObject{{"format", "drawer-organization"}, {"version", 1},
                       {"cabinets", QJsonArray{QJsonObject{{"name", "C"}}}}};
}

QByteArray bytes(const QJsonObject &o) { return QJsonDocument(o).toJson(); }

} // namespace

class OrganizationTest : public QObject
{
    Q_OBJECT

private slots:
    void roundTripThroughFile()
    {
        Database source(":memory:");
        populate(source);
        const Organization original = SqliteOrganizationRepository(source.connection()).load();

        const QByteArray file = OrganizationJson::write(original);
        const Organization parsed = OrganizationJson::read(file);

        Database target(":memory:");
        SqliteOrganizationRepository repo(target.connection());
        repo.import(parsed);

        // Exporting the imported data reproduces the same file.
        QCOMPARE(OrganizationJson::write(repo.load()), file);

        const Organization loaded = repo.load();
        QCOMPARE(loaded.cabinets.size(), 1);
        const GroupEntry &group = loaded.cabinets[0].groups[0];
        QCOMPARE(group.group.posX, 1.5);
        QCOMPARE(group.group.color, QString("#3366cc"));
        QCOMPARE(group.drawers.size(), 4);
        QCOMPARE(group.drawers[0].drawer.size, DrawerSize::Large);
        QCOMPARE(group.drawers[0].drawer.label, QString("Big one"));
        QCOMPARE(group.drawers[0].items[0].notes, QString("ünïcode, comma"));
        QCOMPARE(*group.drawers[0].items[0].minStock, 10);
        QCOMPARE(group.drawers[1].drawer.size, DrawerSize::Small);
    }

    void importAddsToExistingData()
    {
        Database db(":memory:");
        populate(db);
        SqliteOrganizationRepository repo(db.connection());
        repo.import(repo.load());
        QCOMPARE(repo.load().cabinets.size(), 2);
    }

    void missingCellsGetDefaults()
    {
        QJsonObject file = minimalFile();
        QJsonObject group{{"name", "G"}, {"rows", 2}, {"cols", 2}, {"posX", 0}, {"posY", 0},
                          {"defaultSize", "medium"}};
        QJsonObject cabinet = file["cabinets"].toArray()[0].toObject();
        cabinet["groups"] = QJsonArray{group};
        file["cabinets"] = QJsonArray{cabinet};

        const Organization org = OrganizationJson::read(bytes(file));
        const auto &drawers = org.cabinets[0].groups[0].drawers;
        QCOMPARE(drawers.size(), 4);
        QCOMPARE(drawers[3].drawer.label, QString("R2C2"));
        QCOMPARE(drawers[3].drawer.size, DrawerSize::Medium);
    }

    void rejectsInvalidFiles_data()
    {
        QTest::addColumn<QByteArray>("data");

        QTest::newRow("not json") << QByteArray("hello");
        QTest::newRow("wrong format") << bytes({{"format", "other"}, {"version", 1}});
        QJsonObject version = minimalFile();
        version["version"] = 99;
        QTest::newRow("wrong version") << bytes(version);

        auto withGroup = [](QJsonObject group) {
            QJsonObject file = minimalFile();
            QJsonObject cabinet = file["cabinets"].toArray()[0].toObject();
            cabinet["groups"] = QJsonArray{group};
            file["cabinets"] = QJsonArray{cabinet};
            return bytes(file);
        };
        const QJsonObject good{{"name", "G"}, {"rows", 1}, {"cols", 1}, {"posX", 0}, {"posY", 0},
                               {"defaultSize", "small"}};

        QJsonObject g = good;
        g["rows"] = 0;
        QTest::newRow("zero rows") << withGroup(g);
        g = good;
        g["defaultSize"] = "huge";
        QTest::newRow("unknown size") << withGroup(g);
        g = good;
        g["color"] = "not-a-color";
        QTest::newRow("invalid color") << withGroup(g);
        g = good;
        g["posX"] = -1;
        QTest::newRow("negative position") << withGroup(g);
        g = good;
        g["drawers"] = QJsonArray{QJsonObject{{"row", 3}, {"col", 0}, {"size", "small"}}};
        QTest::newRow("drawer out of range") << withGroup(g);
        g = good;
        g["drawers"] = QJsonArray{
            QJsonObject{{"row", 0}, {"col", 0}, {"size", "small"}},
            QJsonObject{{"row", 0}, {"col", 0}, {"size", "small"}}};
        QTest::newRow("duplicate drawer") << withGroup(g);
        g = good;
        g["drawers"] = QJsonArray{QJsonObject{
            {"row", 0}, {"col", 0}, {"size", "small"},
            {"items", QJsonArray{QJsonObject{{"name", ""}}}}}};
        QTest::newRow("empty item name") << withGroup(g);
        g = good;
        g["drawers"] = QJsonArray{QJsonObject{
            {"row", 0}, {"col", 0}, {"size", "small"},
            {"items", QJsonArray{QJsonObject{{"name", "X"}, {"minStock", -3}}}}}};
        QTest::newRow("negative min stock") << withGroup(g);
    }

    void rejectsInvalidFiles()
    {
        QFETCH(QByteArray, data);
        QVERIFY_EXCEPTION_THROWN(OrganizationJson::read(data), OrganizationFormatError);
    }

    void rejectsOverlappingGroups()
    {
        const QJsonObject g{{"name", "G"}, {"rows", 2}, {"cols", 2}, {"posX", 0}, {"posY", 0},
                            {"defaultSize", "small"}};
        QJsonObject file = minimalFile();
        QJsonObject cabinet = file["cabinets"].toArray()[0].toObject();
        cabinet["groups"] = QJsonArray{g, g};
        file["cabinets"] = QJsonArray{cabinet};
        QVERIFY_EXCEPTION_THROWN(OrganizationJson::read(bytes(file)), OrganizationFormatError);
    }

    void holesSurviveAFileRoundTrip()
    {
        Database source(":memory:");
        populate(source);
        SqliteDrawerRepository drawers(source.connection());
        SqliteCabinetRepository cabinets(source.connection());
        const DrawerGroup group = drawers.groupsOf(cabinets.all().first().id).first();
        drawers.removeDrawer(drawers.drawersOf(group.id)[1].id); // Delete R1C2 of the 2x2 block.

        const QByteArray file = OrganizationJson::write(SqliteOrganizationRepository(source.connection()).load());
        Database target(":memory:");
        SqliteOrganizationRepository repo(target.connection());
        repo.import(OrganizationJson::read(file));

        const GroupEntry &imported = repo.load().cabinets[0].groups[0];
        QCOMPARE(imported.drawers.size(), 3);
        QCOMPARE(OrganizationJson::write(repo.load()), file);
    }

    void failedImportChangesNothing()
    {
        Database db(":memory:");
        SqliteOrganizationRepository repo(db.connection());

        Organization bad;
        CabinetEntry cabinet{{0, "Broken", ""}, {}};
        GroupEntry group;
        group.group.name = "G";
        group.group.rows = group.group.cols = 1;
        // Same cell twice violates UNIQUE(group_id, row, col) midway through the import.
        group.drawers = {DrawerEntry{{0, 0, 0, 0, DrawerSize::Small, "a"}, {}},
                         DrawerEntry{{0, 0, 0, 0, DrawerSize::Small, "b"}, {}}};
        cabinet.groups.append(group);
        bad.cabinets.append(cabinet);

        QVERIFY_EXCEPTION_THROWN(repo.import(bad), DatabaseError);
        QVERIFY(repo.load().cabinets.isEmpty());
    }
};

QTEST_MAIN(OrganizationTest)
#include "tst_organization.moc"
