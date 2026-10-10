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

    void rejectsInvalid()
    {
        QVERIFY(!SmdCode::decode(""));
        QVERIFY(!SmdCode::decode("47"));
        QVERIFY(!SmdCode::decode("01C"));
    }
};

QTEST_MAIN(SmdCodeTest)
#include "tst_smdcode.moc"
