#include <QtTest>

#include "tools/CapacitorCode.h"

using namespace CapacitorCode;

class CapacitorCodeTest : public QObject
{
    Q_OBJECT

private slots:
    void decodeThreeDigit_data()
    {
        QTest::addColumn<QString>("marking");
        QTest::addColumn<double>("picofarads");
        QTest::newRow("104") << "104" << 100000.0;
        QTest::newRow("223") << "223" << 22000.0;
        QTest::newRow("471") << "471" << 470.0;
        QTest::newRow("100") << "100" << 10.0;
        QTest::newRow("479") << "479" << 4.7;
        QTest::newRow("4R7") << "4R7" << 4.7;
        QTest::newRow("22 plain") << "22" << 22.0;
    }

    void decodeThreeDigit()
    {
        QFETCH(QString, marking);
        QFETCH(double, picofarads);
        const auto c = decode(marking);
        QVERIFY(c);
        QVERIFY(qFuzzyCompare(c->picofarads, picofarads));
    }

    void decodeTolerance()
    {
        QCOMPARE(*decode("104K")->tolerancePercent, 10.0);
        QCOMPARE(*decode("104z")->tolerancePercent, 80.0);
        QCOMPARE(*decode("4R7C")->tolerancePf, 0.25);
        QVERIFY(!decode("104")->tolerancePercent);
    }

    void decodeRejectsInvalid()
    {
        QVERIFY(!decode(""));
        QVERIFY(!decode("abc"));
        QVERIFY(!decode("104Q")); // Unknown tolerance letter.
        QVERIFY(!decode("1047"));
    }

    void encodeValues()
    {
        QCOMPARE(*encode(100000), QString("104"));
        QCOMPARE(*encode(22000), QString("223"));
        QCOMPARE(*encode(47), QString("470"));
        QCOMPARE(*encode(4.7), QString("4R7"));
        QVERIFY(!encode(123456)); // Needs more than two significant digits.
    }

    void roundTrip()
    {
        for (const QString code : {"104", "223", "471", "331", "105"})
            QCOMPARE(*encode(decode(code)->picofarads), code);
    }
};

QTEST_MAIN(CapacitorCodeTest)
#include "tst_capacitorcode.moc"
