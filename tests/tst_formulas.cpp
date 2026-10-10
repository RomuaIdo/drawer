#include <QtTest>

#include "tools/Formulas.h"

using namespace Formulas;

class FormulasTest : public QObject
{
    Q_OBJECT

private slots:
    void ohmFromVoltageAndCurrent()
    {
        const auto r = solveOhm(10.0, 0.5, std::nullopt, std::nullopt);
        QVERIFY(r);
        QCOMPARE(r->ohms, 20.0);
        QCOMPARE(r->watts, 5.0);
    }

    void ohmFromPowerAndResistance()
    {
        const auto r = solveOhm(std::nullopt, std::nullopt, 100.0, 1.0);
        QVERIFY(r);
        QVERIFY(qFuzzyCompare(r->amps, 0.1));
        QVERIFY(qFuzzyCompare(r->volts, 10.0));
    }

    void ohmFromVoltageAndPower()
    {
        const auto r = solveOhm(12.0, std::nullopt, std::nullopt, 24.0);
        QVERIFY(r);
        QCOMPARE(r->amps, 2.0);
        QCOMPARE(r->ohms, 6.0);
    }

    void ohmNeedsTwoValuesAndAvoidsDivisionByZero()
    {
        QVERIFY(!solveOhm(5.0, std::nullopt, std::nullopt, std::nullopt));
        QVERIFY(!solveOhm(5.0, 0.0, std::nullopt, std::nullopt));
        QVERIFY(!solveOhm(5.0, std::nullopt, 0.0, std::nullopt));
    }

    void divider()
    {
        QCOMPARE(*dividerOutput(10, 1000, 1000), 5.0);
        QVERIFY(!dividerOutput(10, 0, 0));
    }

    void led()
    {
        const auto r = ledResistor(5.0, 2.0, 0.02);
        QVERIFY(r);
        QVERIFY(qFuzzyCompare(r->ohms, 150.0));
        QVERIFY(qFuzzyCompare(r->watts, 0.06));
        QVERIFY(!ledResistor(1.8, 2.0, 0.02));
        QVERIFY(!ledResistor(5.0, 2.0, 0.0));
    }

    void standardValues()
    {
        QCOMPARE(*nearestStandard(150, 24), 150.0);
        QCOMPARE(*nearestStandard(151, 24), 160.0);
        QCOMPARE(*nearestStandard(4100, 12), 4700.0);
        QCOMPARE(*nearestStandard(9500, 24), 10000.0); // Rolls into the next decade.
        QVERIFY(qFuzzyCompare(*nearestStandard(0.47, 12), 0.47));
        QVERIFY(!nearestStandard(100, 7));
        QVERIFY(!nearestStandard(0, 12));
    }
};

QTEST_MAIN(FormulasTest)
#include "tst_formulas.moc"
