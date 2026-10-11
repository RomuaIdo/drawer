#include <QtTest>

#include "tools/Screws.h"

class ScrewsTest : public QObject
{
    Q_OBJECT

private slots:
    void listsTheSupportedSizes()
    {
        QStringList names;
        for (const auto &screw : Screws::all())
            names << screw.name;
        QCOMPARE(names, QStringList({"M2", "M2.5", "M3", "M4"}));
    }

    void m3Values()
    {
        const auto m3 = Screws::find("m3");
        QVERIFY(m3);
        QCOMPARE(m3->diameter, 3.0);
        QCOMPARE(m3->tapDrill, 2.5);
        QCOMPARE(m3->clearanceNormal, 3.4);
    }

    void unknownSizeIsRejected()
    {
        QVERIFY(!Screws::find("M5"));
    }

    void tapDrillIsPitchBelowDiameter()
    {
        for (const auto &s : Screws::all())
            QVERIFY(qAbs(s.diameter - s.pitch - s.tapDrill) < 0.06);
    }

    void clearanceGrowsWithTheFit()
    {
        for (const auto &s : Screws::all()) {
            QVERIFY(s.diameter < s.clearanceClose);
            QVERIFY(s.clearanceClose < s.clearanceNormal);
            QVERIFY(s.clearanceNormal < s.clearanceLoose);
        }
    }
};

QTEST_MAIN(ScrewsTest)
#include "tst_screws.moc"
