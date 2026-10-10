#include <QtTest>

#include "tools/SmdCode.h"

class SmdCodeTest : public QObject
{
    Q_OBJECT

private slots:
    void decodes()
    {
        QCOMPARE(*SmdCode::decode("472"), 4700.0);
        QCOMPARE(*SmdCode::decode("1002"), 10000.0);
        QCOMPARE(*SmdCode::decode("000"), 0.0);
        QCOMPARE(*SmdCode::decode("4R7"), 4.7);
        QCOMPARE(*SmdCode::decode("R47"), 0.47);
        QCOMPARE(*SmdCode::decode("10R0"), 10.0);
    }

    void decodesEia96()
    {
        QCOMPARE(*SmdCode::decode("01A"), 100.0);
        QCOMPARE(*SmdCode::decode("01C"), 10000.0);
        QCOMPARE(*SmdCode::decode("68X"), 49.9); // 68 -> 499, X = 0.1
        QCOMPARE(*SmdCode::decode("96F"), 97600000.0);
        QCOMPARE(*SmdCode::decode("01z"), 0.1);
        QCOMPARE(*SmdCode::decode("47R"), 3.01); // Letter wins over R notation.
        QCOMPARE(*SmdCode::decode("10B"), 1240.0);
    }

    void eia96TableIsComplete()
    {
        const QList<int> &values = SmdCode::e96Values();
        QCOMPARE(values.size(), 96);
        QCOMPARE(values.first(), 100);
        QCOMPARE(values.last(), 976);
        for (int i = 1; i < values.size(); ++i)
            QVERIFY(values[i] > values[i - 1]);
    }

    void rejectsInvalid()
    {
        QVERIFY(!SmdCode::decode(""));
        QVERIFY(!SmdCode::decode("47"));
        QVERIFY(!SmdCode::decode("00C")); // No code 00.
        QVERIFY(!SmdCode::decode("97C")); // Table ends at 96.
        QVERIFY(!SmdCode::decode("01G")); // Unknown multiplier letter.
    }
};

QTEST_MAIN(SmdCodeTest)
#include "tst_smdcode.moc"
