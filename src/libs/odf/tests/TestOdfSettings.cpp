// clazy:excludeall=qstring-arg
#include <QTest>

#include <QObject>

#include <KoXmlReader.h>
#include <KoOasisSettings.h>

class TestOdfSettings : public QObject
{
    Q_OBJECT
public:
    TestOdfSettings() { }

private Q_SLOTS:
    void initTestCase();
    void testParseConfigItemString();
    void testSelectItemSet();
    void testIndexedMap();
    void testNamedMap();

private:
    KoXmlDocument doc;
    KoOasisSettings *settings;
};

void TestOdfSettings::initTestCase()
{
    const QString xml = QStringLiteral(
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<office:document-settings xmlns:office=\"urn:oasis:names:tc:opendocument:xmlns:office:1.0\""
                " xmlns:config=\"urn:oasis:names:tc:opendocument:xmlns:config:1.0\">"
        "<office:settings>"
        "<config:config-item-set config:name=\"view-settings\">"
        "<config:config-item config:name=\"unit\" config:type=\"string\">mm</config:config-item>"
        "<config:config-item-map-indexed config:name=\"Views\">"
        "<config:config-item-map-entry>"
        "<config:config-item config:name=\"ZoomFactor\" config:type=\"short\">100</config:config-item>"
        "</config:config-item-map-entry>"
        "</config:config-item-map-indexed>"
        "<config:config-item-map-named config:name=\"NamedMap\">"
        "<config:config-item-map-entry config:name=\"foo\">"
        "<config:config-item config:name=\"ZoomFactor\" config:type=\"int\">100</config:config-item>"
        "</config:config-item-map-entry>"
        "</config:config-item-map-named>"
        "</config:config-item-set>"
        "</office:settings>"
        "</office:document-settings>");

    bool ok = doc.setContent(xml, true /* namespace processing */);
    QVERIFY(ok);
    settings = new KoOasisSettings(doc);
}

void TestOdfSettings::testSelectItemSet()
{
    KoOasisSettings::Items items = settings->itemSet(QStringLiteral("notexist"));
    QVERIFY(items.isNull());
    items = settings->itemSet(QStringLiteral("view-settings"));
    QVERIFY(!items.isNull());
}

void TestOdfSettings::testParseConfigItemString()
{
    KoOasisSettings::Items viewSettings = settings->itemSet(QStringLiteral("view-settings"));
    const QString unit = viewSettings.parseConfigItemString(QStringLiteral("unit"));
    QCOMPARE(unit, QString(QStringLiteral("mm")));
}

void TestOdfSettings::testIndexedMap()
{
    KoOasisSettings::Items viewSettings = settings->itemSet(QStringLiteral("view-settings"));
    QVERIFY(!viewSettings.isNull());
    KoOasisSettings::IndexedMap viewMap = viewSettings.indexedMap(QStringLiteral("Views"));
    QVERIFY(!viewMap.isNull());
    KoOasisSettings::Items firstView = viewMap.entry(0);
    QVERIFY(!firstView.isNull());
    const short zoomFactor = firstView.parseConfigItemShort(QStringLiteral("ZoomFactor"));
    QCOMPARE(zoomFactor, (short) 100);
    KoOasisSettings::Items secondView = viewMap.entry(1);
    QVERIFY(secondView.isNull());
}

void TestOdfSettings::testNamedMap()
{
    KoOasisSettings::Items viewSettings = settings->itemSet(QStringLiteral("view-settings"));
    QVERIFY(!viewSettings.isNull());
    KoOasisSettings::NamedMap viewMap = viewSettings.namedMap(QStringLiteral("NamedMap"));
    QVERIFY(!viewMap.isNull());
    KoOasisSettings::Items foo = viewMap.entry(QStringLiteral("foo"));
    QVERIFY(!foo.isNull());
    const int zoomFactor = foo.parseConfigItemShort(QStringLiteral("ZoomFactor"));
    QCOMPARE(zoomFactor, 100);
    KoOasisSettings::Items secondView = viewMap.entry(QStringLiteral("foobar"));
    QVERIFY(secondView.isNull());
}

QTEST_GUILESS_MAIN(TestOdfSettings)

#include <TestOdfSettings.moc>
