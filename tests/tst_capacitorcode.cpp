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

    void readsMarkingsAndValuesAlike_data()
    {
        QTest::addColumn<QString>("input");
        QTest::addColumn<double>("picofarads");
        QTest::addColumn<QString>("marking");
        QTest::newRow("marking") << "104" << 100000.0 << "104";
        QTest::newRow("nano") << "100n" << 100000.0 << "104";
        QTest::newRow("nano unit") << "100nF" << 100000.0 << "104";
        QTest::newRow("micro") << "1uF" << 1e6 << "105";
        QTest::newRow("infix") << "4u7" << 4.7e6 << "475";
        QTest::newRow("pico") << "22p" << 22.0 << "220";
        QTest::newRow("decimal") << "0.1u" << 100000.0 << "104";
        QTest::newRow("tolerance letter") << "104K" << 100000.0 << "104K";
        QTest::newRow("lowercase letter") << "223j" << 22000.0 << "223J";
        QTest::newRow("small") << "4R7" << 4.7 << "4R7";
        QTest::newRow("letter wins over farads") << "100F" << 10.0 << "100F";
    }

    void readsMarkingsAndValuesAlike()
    {
        QFETCH(QString, input);
        QFETCH(double, picofarads);
        QFETCH(QString, marking);
        const auto reading = CapacitorCode::read(input);
        QVERIFY(reading);
        QVERIFY(qFuzzyCompare(reading->capacitance.picofarads, picofarads));
        QCOMPARE(reading->marking, marking);
    }

    void readKeepsTheTolerance()
    {
        QCOMPARE(*CapacitorCode::read("104K")->capacitance.tolerancePercent, 10.0);
        QCOMPARE(*CapacitorCode::read("4R7C")->capacitance.tolerancePf, 0.25);
        QVERIFY(!CapacitorCode::read("104")->capacitance.tolerancePercent);
        QVERIFY(!CapacitorCode::read("100n")->capacitance.tolerancePercent);
    }

    void readLeavesTheMarkingEmptyWhenNoCodeFits()
    {
        const auto reading = CapacitorCode::read("2200u"); // Too large for three digits.
        QVERIFY(reading);
        QVERIFY(reading->marking.isEmpty());
        QVERIFY(!CapacitorCode::read("abc"));
        QVERIFY(!CapacitorCode::read(""));
        QVERIFY(!CapacitorCode::read("0"));
    }

    void defaultTypeFollowsTheValueRange()
    {
        using T = CapacitorCode::CapacitorType;
        QCOMPARE(CapacitorCode::defaultTypeFor(22), T::CeramicDisc);
        QCOMPARE(CapacitorCode::defaultTypeFor(CapacitorCode::read("100n")->capacitance.picofarads),
                 T::CeramicDisc); // The boundary belongs to the disc.
        QCOMPARE(CapacitorCode::defaultTypeFor(CapacitorCode::read("220n")->capacitance.picofarads),
                 T::Film);
        QCOMPARE(CapacitorCode::defaultTypeFor(CapacitorCode::read("1u")->capacitance.picofarads),
                 T::Film);
        QCOMPARE(CapacitorCode::defaultTypeFor(CapacitorCode::read("4u7")->capacitance.picofarads),
                 T::Electrolytic);
        QCOMPARE(CapacitorCode::defaultTypeFor(CapacitorCode::read("1000u")->capacitance.picofarads),
                 T::Electrolytic);
    }

    void valuesArePrintedTheWayPartsCarryThem_data()
    {
        QTest::addColumn<double>("picofarads");
        QTest::addColumn<QString>("compact");
        QTest::addColumn<QString>("withUnit");
        QTest::newRow("pico") << 22.0 << "22p" << "22pF";
        QTest::newRow("nano") << 100000.0 << "100n" << "100nF";
        QTest::newRow("fractional micro") << 4.7e6 << "4.7\u00b5" << "4.7\u00b5F";
        QTest::newRow("large micro") << 2.2e9 << "2200\u00b5" << "2200\u00b5F";
        QTest::newRow("milli") << 1.5e10 << "15m" << "15mF";
        QTest::newRow("kilo-pico boundary") << 1000.0 << "1n" << "1nF";
    }

    void valuesArePrintedTheWayPartsCarryThem()
    {
        QFETCH(double, picofarads);
        QFETCH(QString, compact);
        QFETCH(QString, withUnit);
        QCOMPARE(CapacitorCode::compactValue(picofarads, false), compact);
        QCOMPARE(CapacitorCode::compactValue(picofarads, true), withUnit);
    }

    void voltagesParseAndFormat()
    {
        QCOMPARE(*CapacitorCode::parseVoltage("50"), 50.0);
        QCOMPARE(*CapacitorCode::parseVoltage("50V"), 50.0);
        QCOMPARE(*CapacitorCode::parseVoltage(" 6.3v "), 6.3);
        QCOMPARE(*CapacitorCode::parseVoltage("1kV"), 1000.0);
        QVERIFY(!CapacitorCode::parseVoltage(""));
        QVERIFY(!CapacitorCode::parseVoltage("abc"));
        QVERIFY(!CapacitorCode::parseVoltage("0"));
        QVERIFY(!CapacitorCode::parseVoltage("-5"));
        QCOMPARE(CapacitorCode::formatVoltage(50), QString("50V"));
        QCOMPARE(CapacitorCode::formatVoltage(6.3), QString("6.3V"));
        QCOMPARE(CapacitorCode::formatVoltage(1500), QString("1.5kV"));
    }

    void printedTextDependsOnTheType()
    {
        using T = CapacitorCode::CapacitorType;
        const auto reading = *CapacitorCode::read("104K");
        const std::optional<double> volts = 50;
        QCOMPARE(CapacitorCode::printedLines(T::CeramicDisc, reading, volts),
                 (QStringList{"104K", "50V"})); // The code, as on disc ceramics.
        QCOMPARE(CapacitorCode::printedLines(T::Film, reading, volts),
                 (QStringList{"100n", "50V"}));
        QCOMPARE(CapacitorCode::printedLines(T::Electrolytic, reading, volts),
                 (QStringList{"100nF", "50V"}));
        QCOMPARE(CapacitorCode::printedLines(T::Tantalum, reading, std::nullopt),
                 (QStringList{"100n"})); // No voltage given: no voltage line.
        QVERIFY(CapacitorCode::printedLines(T::CeramicChip, reading, volts).isEmpty()); // Unmarked.
    }

    void discsWithoutACodePrintTheValue()
    {
        using T = CapacitorCode::CapacitorType;
        const auto reading = *CapacitorCode::read("2200u"); // No 3-digit code fits.
        QCOMPARE(CapacitorCode::printedLines(T::CeramicDisc, reading, std::nullopt),
                 (QStringList{"2200\u00b5"}));
    }

    void higherVoltageMeansABiggerPart()
    {
        QCOMPARE(CapacitorCode::voltageScale(std::nullopt), 1.0);
        QVERIFY(CapacitorCode::voltageScale(6.3) < CapacitorCode::voltageScale(25));
        QVERIFY(CapacitorCode::voltageScale(25) < CapacitorCode::voltageScale(50));
        QVERIFY(CapacitorCode::voltageScale(50) < CapacitorCode::voltageScale(100));
        QVERIFY(CapacitorCode::voltageScale(100) < CapacitorCode::voltageScale(400));
    }

    void roundTrip()
    {
        for (const QString code : {"104", "223", "471", "331", "105"})
            QCOMPARE(*encode(decode(code)->picofarads), code);
    }
};

QTEST_MAIN(CapacitorCodeTest)
#include "tst_capacitorcode.moc"
