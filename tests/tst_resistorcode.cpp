#include <QtTest>

#include "tools/ResistorCode.h"

using namespace ResistorCode;
using C = Color;

class ResistorCodeTest : public QObject
{
    Q_OBJECT

private slots:
    void decodeFourBand()
    {
        const auto r = decode({C::Brown, C::Black, C::Red, C::Gold});
        QVERIFY(r);
        QCOMPARE(r->ohms, 1000.0);
        QCOMPARE(r->tolerancePercent, 5.0);
        QVERIFY(!r->tempcoPpm);
    }

    void decodeFiveBand()
    {
        const auto r = decode({C::Yellow, C::Violet, C::Black, C::Red, C::Brown});
        QVERIFY(r);
        QCOMPARE(r->ohms, 47000.0);
        QCOMPARE(r->tolerancePercent, 1.0);
    }

    void decodeSixBandWithTempco()
    {
        const auto r = decode({C::Red, C::Red, C::Black, C::Black, C::Brown, C::Red});
        QVERIFY(r);
        QCOMPARE(r->ohms, 220.0);
        QCOMPARE(*r->tempcoPpm, 50);
    }

    void decodeFractionalMultipliers()
    {
        QCOMPARE(decode({C::Yellow, C::Violet, C::Gold, C::Gold})->ohms, 4.7);
        QCOMPARE(decode({C::Brown, C::Black, C::Silver, C::Gold})->ohms, 0.1);
    }

    void decodeRejectsInvalidRoles()
    {
        QVERIFY(!decode({C::Gold, C::Black, C::Red, C::Gold})); // Gold is not a digit.
        QVERIFY(!decode({C::Brown, C::Black, C::Red, C::Orange})); // Orange is not a tolerance.
        QVERIFY(!decode({C::Brown, C::Black, C::Red})); // Too few bands.
    }

    void encodeRoundTrips()
    {
        for (double ohms : {0.47, 4.7, 10.0, 330.0, 4700.0, 100000.0, 2200000.0}) {
            const auto bands = encode(ohms, 5, 4);
            QVERIFY2(bands, qPrintable(QString::number(ohms)));
            QCOMPARE(decode(*bands)->ohms, ohms);
        }
    }

    void encodeKnownValues()
    {
        QCOMPARE(*encode(1000, 5, 4), (QList<Color>{C::Brown, C::Black, C::Red, C::Gold}));
        QCOMPARE(*encode(47000, 1, 5), (QList<Color>{C::Yellow, C::Violet, C::Black, C::Red, C::Brown}));
        QCOMPARE(*encode(220, 2, 6, 50),
                 (QList<Color>{C::Red, C::Red, C::Black, C::Black, C::Red, C::Red}));
    }

    void encodeRejectsUnrepresentable()
    {
        QVERIFY(!encode(1234, 5, 4)); // Needs 4 significant digits.
        QVERIFY(!encode(100, 3, 4)); // No 3% band.
        QVERIFY(!encode(100, 5, 3)); // Invalid band count.
    }
};

QTEST_MAIN(ResistorCodeTest)
#include "tst_resistorcode.moc"
