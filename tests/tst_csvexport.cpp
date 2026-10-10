#include <QBuffer>
#include <QtTest>

#include "data/Database.h"
#include "data/SqliteCabinetRepository.h"
#include "data/SqliteDrawerRepository.h"
#include "data/SqliteItemRepository.h"
#include "data/SqliteMappingRepository.h"
#include "export/CsvExporter.h"

class CsvExportTest : public QObject
{
    Q_OBJECT

private slots:
    void escapesSpecialCharacters()
    {
        QCOMPARE(CsvExporter::escape("plain"), QString("plain"));
        QCOMPARE(CsvExporter::escape("a,b"), QString("\"a,b\""));
        QCOMPARE(CsvExporter::escape("say \"hi\""), QString("\"say \"\"hi\"\"\""));
        QCOMPARE(CsvExporter::escape("two\nlines"), QString("\"two\nlines\""));
    }

    void exportsItemsAndEmptyDrawers()
    {
        Database db(":memory:");
        {
            SqliteCabinetRepository cabinets(db.connection());
            SqliteDrawerRepository drawers(db.connection());
            SqliteItemRepository items(db.connection());
            SqliteMappingRepository mapping(db.connection());

            const int cab = cabinets.add({0, "Bench", ""}).id;
            DrawerGroup g;
            g.cabinetId = cab;
            g.name = "Block";
            g.rows = 1;
            g.cols = 2;
            g.defaultSize = DrawerSize::Small;
            g = drawers.addGroup(g);
            const Drawer first = drawers.drawersOf(g.id).first();
            items.add({0, first.id, "Resistor, 1%", "Passive", "4.7k", "0603", 50, ""});

            QByteArray bytes;
            QBuffer buffer(&bytes);
            buffer.open(QIODevice::WriteOnly);
            CsvExporter(false).write(mapping.rows(), buffer);

            const QStringList lines = QString::fromUtf8(bytes).split("\r\n", Qt::SkipEmptyParts);
            QCOMPARE(lines.size(), 3); // Header, item row, empty drawer row.
            QCOMPARE(lines[1], QString("Bench,Block,1,1,R1C1,small,\"Resistor, 1%\",Passive,4.7k,0603,50,"));
            QCOMPARE(lines[2], QString("Bench,Block,1,2,R1C2,small,,,,,,"));
        }
    }

    void writesBomWhenRequested()
    {
        QByteArray bytes;
        QBuffer buffer(&bytes);
        buffer.open(QIODevice::WriteOnly);
        CsvExporter(true).write({}, buffer);
        QVERIFY(bytes.startsWith("\xEF\xBB\xBF"));
    }
};

QTEST_MAIN(CsvExportTest)
#include "tst_csvexport.moc"
