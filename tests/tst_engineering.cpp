#include <QtTest>

#include "tools/Engineering.h"

class EngineeringTest : public QObject
{
    Q_OBJECT

private slots:
    void parseValues_data()
    {
        QTest::addColumn<QString>("text");
        QTest::addColumn<double>("expected");
        QTest::newRow("plain") << "330" << 330.0;
        QTest::newRow("kilo") << "4.7k" << 4700.0;
        QTest::newRow("infix") << "4k7" << 4700.0;
        QTest::newRow("mega") << "2.2M" << 2.2e6;
        QTest::newRow("milli") << "10m" << 0.01;
        QTest::newRow("r infix") << "4R7" << 4.7;
        QTest::newRow("unit") << "100 kΩ" << 1e5;
        QTest::newRow("nano") << "100nF" << 1e-7;
        QTest::newRow("micro infix") << "2u2" << 2.2e-6;
        QTest::newRow("micro sign") << "1µF" << 1e-6;
        QTest::newRow("greek mu") << "1μF" << 1e-6;
    }

    void parseValues()
    {
        QFETCH(QString, text);
        QFETCH(double, expected);
        const auto value = Engineering::parse(text);
        QVERIFY(value);
        QVERIFY(qFuzzyCompare(*value, expected));
    }

    void parseRejectsGarbage()
    {
        QVERIFY(!Engineering::parse(""));
        QVERIFY(!Engineering::parse("abc"));
        QVERIFY(!Engineering::parse("4.7x"));
    }

    void formatUsesPrefixes()
    {
        QCOMPARE(Engineering::format(4700, "Ω"), QString("4.7 kΩ"));
        QCOMPARE(Engineering::format(330, "Ω"), QString("330 Ω"));
        QCOMPARE(Engineering::format(1e-7, "F"), QString("100 nF"));
        QCOMPARE(Engineering::format(2.2e6, "Ω"), QString("2.2 MΩ"));
    }
};

QTEST_MAIN(EngineeringTest)
#include "tst_engineering.moc"
