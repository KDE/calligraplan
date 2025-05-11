/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2004-2006 David Faure <faure@kde.org>
   SPDX-FileCopyrightText: 2007 Thorsten Zachmann <zachmann@kde.org>
   SPDX-FileCopyrightText: 2008 Girish Ramakrishnan <girish@forwardbias.in>
   SPDX-FileCopyrightText: 2009 Thomas Zander <zander@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "TestKoGenStyles.h"

#include <KoGenStyles.h>
#include <KoXmlWriter.h>
#include <OdfDebug.h>
#include <QBuffer>

#include <QTest>

#define TEST_BEGIN(publicId,systemId) \
    { \
        QByteArray cstr; \
        QBuffer buffer(&cstr); \
        buffer.open(QIODevice::WriteOnly); \
        { \
            KoXmlWriter writer(&buffer); \
            writer.startDocument("r", publicId, systemId); \
            writer.startElement("r")

#define TEST_END_QTTEST(expected) \
            writer.endElement(); \
            writer.endDocument(); \
        } \
        buffer.putChar('\0'); /*null-terminate*/ \
        QString expectedFull = QString::fromLatin1("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"); \
        expectedFull += expected; \
        QString s1 = QString::fromLatin1(cstr); \
        QCOMPARE(expectedFull, s1); \
    }


void TestKoGenStyles::testLookup()
{
    debugOdf ;
    KoGenStyles coll;

    QMultiMap<QString, QString> map1;
    map1.insert(QStringLiteral("map1key"), QStringLiteral("map1value"));
    QMultiMap<QString, QString> map2;
    map2.insert(QStringLiteral("map2key1"), QStringLiteral("map2value1"));
    map2.insert(QStringLiteral("map2key2"), QStringLiteral("map2value2"));

    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    KoXmlWriter childWriter(&buffer);
    childWriter.startElement("child");
    childWriter.addAttribute("test:foo", "bar");
    childWriter.endElement();
    QString childContents = QString::fromUtf8(buffer.buffer().constData(), buffer.buffer().size());

    QBuffer buffer2;
    buffer2.open(QIODevice::WriteOnly);
    KoXmlWriter childWriter2(&buffer2);
    childWriter2.startElement("child2");
    childWriter2.addAttribute("test:foo", "bar");
    childWriter2.endElement();
    QString childContents2 = QString::fromUtf8(buffer2.buffer().constData(), buffer2.buffer().size());

    KoGenStyle first(KoGenStyle::ParagraphAutoStyle, "paragraph");
    first.addAttribute("style:master-page-name", "Standard");
    first.addProperty("style:page-number", "0");
    first.addProperty("style:foobar", "2", KoGenStyle::TextType);
    first.addStyleMap(map1);
    first.addStyleMap(map2);
    first.addChildElement("test", childContents);
    first.addChildElement("test", childContents2, KoGenStyle::TextType);

    QString firstName = coll.insert(first);
    debugOdf << "The first style got assigned the name" << firstName;
    QVERIFY(!firstName.isEmpty());
    QCOMPARE(first.type(), KoGenStyle::ParagraphAutoStyle);

    KoGenStyle second(KoGenStyle::ParagraphAutoStyle, "paragraph");
    second.addAttribute("style:master-page-name", "Standard");
    second.addProperty("style:page-number", "0");
    second.addProperty("style:foobar", "2", KoGenStyle::TextType);
    second.addStyleMap(map1);
    second.addStyleMap(map2);
    second.addChildElement("test", childContents);
    second.addChildElement("test", childContents2, KoGenStyle::TextType);

    QString secondName = coll.insert(second);
    debugOdf << "The second style got assigned the name" << secondName;

    QCOMPARE(firstName, secondName);   // check that sharing works
    QCOMPARE(first, second);   // check that operator== works :)

    const KoGenStyle* s = coll.style(firstName, "paragraph");   // check insert of existing style
    QVERIFY(s != nullptr);
    QCOMPARE(*s, first);
    s = coll.style(QStringLiteral("foobarblah"), "paragraph");   // check insert of non-existing style
    QVERIFY(s == nullptr);

    KoGenStyle third(KoGenStyle::ParagraphAutoStyle, "paragraph", secondName);   // inherited style
    third.addProperty("style:margin-left", "1.249cm");
    third.addProperty("style:page-number", "0");   // same as parent
    third.addProperty("style:foobar", "3", KoGenStyle::TextType);   // different from parent
    QCOMPARE(third.parentName(), secondName);

    QString thirdName = coll.insert(third, QStringLiteral("P"));
    debugOdf << "The third style got assigned the name" << thirdName;
    QVERIFY(thirdName != firstName);
    QVERIFY(!thirdName.isEmpty());

    KoGenStyle user(KoGenStyle::ParagraphStyle, "paragraph");   // differs from third since it doesn't inherit second, and has a different type
    user.addProperty("style:margin-left", "1.249cm");

    QString userStyleName = coll.insert(user, QStringLiteral("User"), KoGenStyles::DontAddNumberToName);
    debugOdf << "The user style got assigned the name" << userStyleName;
    QCOMPARE(userStyleName, QStringLiteral("User"));

    KoGenStyle sameAsParent(KoGenStyle::ParagraphAutoStyle, "paragraph", secondName);   // inherited style
    sameAsParent.addAttribute("style:master-page-name", "Standard");
    sameAsParent.addProperty("style:page-number", "0");
    sameAsParent.addProperty("style:foobar", "2", KoGenStyle::TextType);
    sameAsParent.addStyleMap(map1);
    sameAsParent.addStyleMap(map2);
    sameAsParent.addChildElement("test", childContents);
    sameAsParent.addChildElement("test", childContents2, KoGenStyle::TextType);
    QString sapName = coll.insert(sameAsParent, QStringLiteral("foobar"));
    debugOdf << "The 'same as parent' style got assigned the name" << sapName;

    QCOMPARE(sapName, secondName);
    QCOMPARE(coll.styles().count(), 3);

    // OK, now add a style marked as for styles.xml; it looks like the above style, but
    // since it's marked for styles.xml it shouldn't be shared with it.
    KoGenStyle headerStyle(KoGenStyle::ParagraphAutoStyle, "paragraph");
    headerStyle.addAttribute("style:master-page-name", "Standard");
    headerStyle.addProperty("style:page-number", "0");
    headerStyle.addProperty("style:foobar", "2", KoGenStyle::TextType);
    headerStyle.addStyleMap(map1);
    headerStyle.addStyleMap(map2);
    headerStyle.setAutoStyleInStylesDotXml(true);
    coll.insert(headerStyle, QStringLiteral("foobar"));

    QCOMPARE(coll.styles().count(), 4);
    //QCOMPARE(coll.styles(KoGenStyle::ParagraphAutoStyle).count(), 2);
    //QCOMPARE(coll.styles(KoGenStyle::ParagraphStyle).count(), 1);

    // XML for first/second style
    TEST_BEGIN(nullptr, nullptr);
    first.writeStyle(&writer, coll, "style:style", firstName, "style:paragraph-properties");


    TEST_END_QTTEST(QStringLiteral("<r>\n <style:style style:name=\"") + firstName + QStringLiteral("\" style:family=\"paragraph\" "
        "style:master-page-name=\"Standard\">\n  <style:paragraph-properties style:page-number=\"0\">\n"
        "   <child test:foo=\"bar\"/>\n  </style:paragraph-properties>\n  <style:text-properties style:foobar=\"2\">\n"
        "   <child2 test:foo=\"bar\"/>\n  </style:text-properties>\n"
        "  <style:map map1key=\"map1value\"/>\n  <style:map map2key1=\"map2value1\" map2key2=\"map2value2\"/>\n"
        " </style:style>\n</r>\n"));

    // XML for third style
    TEST_BEGIN(nullptr, nullptr);
    third.writeStyle(&writer, coll, "style:style", thirdName, "style:paragraph-properties");
    TEST_END_QTTEST(QStringLiteral("<r>\n <style:style style:name=\"") + thirdName + QStringLiteral("\""
        " style:parent-style-name=\"") + firstName + QStringLiteral("\" style:family=\"paragraph\">\n"
        "  <style:paragraph-properties style:margin-left=\"1.249cm\"/>\n"
        "  <style:text-properties style:foobar=\"3\"/>\n </style:style>\n</r>\n"));
}

void TestKoGenStyles::testLookupFlags()
{
    KoGenStyles coll;

    KoGenStyle first(KoGenStyle::ParagraphAutoStyle, "paragraph");
    first.addAttribute("style:master-page-name", "Standard");
    first.addProperty("style:page-number", "0");

    QString styleName = coll.insert(first, QStringLiteral("P"), KoGenStyles::DontAddNumberToName);
    QCOMPARE(styleName, QStringLiteral("P"));

    styleName = coll.insert(first, QStringLiteral("P"));
    QCOMPARE(styleName, QStringLiteral("P"));

    KoGenStyle second(KoGenStyle::ParagraphAutoStyle, "paragraph");
    second.addProperty("fo:text-align", "left");

    styleName = coll.insert(second, QStringLiteral("P"));
    QCOMPARE(styleName, QStringLiteral("P1"));

    styleName = coll.insert(second, QStringLiteral("P"), KoGenStyles::AllowDuplicates);
    QCOMPARE(styleName, QStringLiteral("P2"));

    styleName = coll.insert(second, QStringLiteral("P"), KoGenStyles::AllowDuplicates);
    QCOMPARE(styleName, QStringLiteral("P3"));

    styleName = coll.insert(second, QStringLiteral("P"), KoGenStyles::AllowDuplicates | KoGenStyles::DontAddNumberToName);
    QCOMPARE(styleName, QStringLiteral("P4"));
}

void TestKoGenStyles::testWriteStyle()
{
    debugOdf;
    KoGenStyles coll;

    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    KoXmlWriter styleChildWriter(&buffer);
    styleChildWriter.startElement("styleChild");
    styleChildWriter.addAttribute("foo", "bar");
    styleChildWriter.endElement();
    QString styleChildContents = QString::fromUtf8(buffer.buffer().constData(), buffer.buffer().size());

    KoGenStyle style(KoGenStyle::ParagraphStyle, "paragraph");
    style.addProperty("style:foo", "bar");
    style.addProperty("style:paragraph", "property", KoGenStyle::ParagraphType);
    style.addProperty("style:graphic", "property", KoGenStyle::GraphicType);
    style.addProperty("styleChild", styleChildContents, KoGenStyle::StyleChildElement);
    QString styleName = coll.insert(style, QStringLiteral("P"));

    // XML for style
    TEST_BEGIN(nullptr, nullptr);
    style.writeStyle(&writer, coll, "style:style", styleName, "style:paragraph-properties");
    TEST_END_QTTEST(QStringLiteral("<r>\n <style:style style:name=\"P1\" style:family=\"paragraph\">\n  <style:paragraph-properties style:foo=\"bar\" style:paragraph=\"property\"/>\n  <style:graphic-properties style:graphic=\"property\"/>\n  <styleChild foo=\"bar\"/>\n </style:style>\n</r>\n"));

    KoGenStyle pageLayoutStyle(KoGenStyle::PageLayoutStyle);
    pageLayoutStyle.addProperty("style:print-orientation", "portrait");
    QString pageLayoutStyleName = coll.insert(pageLayoutStyle, QStringLiteral("pm"));

    // XML for page layout style
    TEST_BEGIN(nullptr, nullptr);
    pageLayoutStyle.writeStyle(&writer, coll, "style:page-layout", pageLayoutStyleName, "style:page-layout-properties");
    TEST_END_QTTEST(QStringLiteral("<r>\n <style:page-layout style:name=\"pm1\">\n  <style:page-layout-properties style:print-orientation=\"portrait\"/>\n </style:page-layout>\n</r>\n"));

    KoGenStyle listStyle(KoGenStyle::ListStyle);
    QString listStyleName = coll.insert(listStyle, "L");
    // XML for list layout style
    TEST_BEGIN(nullptr, nullptr);
    listStyle.writeStyle(&writer, coll, "text:list-style", listStyleName, nullptr);
    TEST_END_QTTEST(QStringLiteral("<r>\n <text:list-style style:name=\"L1\"/>\n</r>\n"));
}

void TestKoGenStyles::testDefaultStyle()
{
    debugOdf ;
    /* Create a default style,
     * and then an auto style with exactly the same attributes
     * -> the insert gives the default style.
     *
     * Also checks how the default style gets written out to XML.
     */
    KoGenStyles coll;

    KoGenStyle defaultStyle(KoGenStyle::ParagraphStyle, "paragraph");
    defaultStyle.addAttribute("style:master-page-name", "Standard");
    defaultStyle.addProperty("myfont", "isBold");
    defaultStyle.setDefaultStyle(true);
    QString defaultStyleName = coll.insert(defaultStyle);
    // default styles don't get a name
    QVERIFY(defaultStyleName.isEmpty());
    QCOMPARE(defaultStyle.type(), KoGenStyle::ParagraphStyle);
    QVERIFY(defaultStyle.isDefaultStyle());

    KoGenStyle anotherStyle(KoGenStyle::ParagraphStyle, "paragraph");
    anotherStyle.addAttribute("style:master-page-name", "Standard");
    anotherStyle.addProperty("myfont", "isBold");
    QString anotherStyleName = coll.insert(anotherStyle);
    QVERIFY(anotherStyleName != defaultStyleName);

    QCOMPARE(coll.styles().count(), 1);

    // XML for default style
    TEST_BEGIN(nullptr, nullptr);
    defaultStyle.writeStyle(&writer, coll, "style:default-style", defaultStyleName, "style:paragraph-properties");
    TEST_END_QTTEST(QStringLiteral("<r>\n <style:default-style style:family=\"paragraph\" style:master-page-name=\"Standard\">\n  <style:paragraph-properties myfont=\"isBold\"/>\n </style:default-style>\n</r>\n"));

    // The Calligra Sheets case: not writing out all properties, only if they differ
    // from the default style.
    // KoGenStyles doesn't fetch info from the parent style when testing
    // for equality, so Calligra Sheets uses isEmpty() to check for equality-to-parent.
    KoGenStyle dataStyle(KoGenStyle::ParagraphStyle, "paragraph", defaultStyleName);
    QVERIFY(dataStyle.isEmpty());
    // and then it doesn't look up the auto style, but rather uses the parent style directly.
}

void TestKoGenStyles:: testUserStyles()
{
    debugOdf ;
    /* Two user styles with exactly the same attributes+properties will not get merged, since
     * they don't have exactly the same attributes after all: the display-name obviously differs :)
     */
    KoGenStyles coll;

    KoGenStyle user1(KoGenStyle::ParagraphStyle, "paragraph");
    user1.addAttribute("style:display-name", "User 1");
    user1.addProperty("myfont", "isBold");

    QString user1StyleName = coll.insert(user1, QStringLiteral("User1"), KoGenStyles::DontAddNumberToName);
    debugOdf << "The user style got assigned the name" << user1StyleName;
    QCOMPARE(user1StyleName, QStringLiteral("User1"));

    KoGenStyle user2(KoGenStyle::ParagraphStyle, "paragraph");
    user2.addAttribute("style:display-name", "User 2");
    user2.addProperty("myfont", "isBold");

    QString user2StyleName = coll.insert(user2, QStringLiteral("User2"), KoGenStyles::DontAddNumberToName);
    debugOdf << "The user style got assigned the name" << user2StyleName;
    QCOMPARE(user2StyleName, QStringLiteral("User2"));

    // And now, what if the data uses that style?
    // This is like sameAsParent in the other test, but this time the
    // parent is a STYLE_USER...
    KoGenStyle dataStyle(KoGenStyle::ParagraphAutoStyle, "paragraph", user2StyleName);
    dataStyle.addProperty("myfont", "isBold");

    QString dataStyleName = coll.insert(dataStyle, QStringLiteral("DataStyle"));
    debugOdf << "The auto style got assigned the name" << dataStyleName;
    QCOMPARE(dataStyleName, QStringLiteral("User2"));     // it found the parent as equal

    // Let's do the opposite test, just to make sure
    KoGenStyle dataStyle2(KoGenStyle::ParagraphAutoStyle, "paragraph", user2StyleName);
    dataStyle2.addProperty("myfont", "isNotBold");

    QString dataStyle2Name = coll.insert(dataStyle2, QStringLiteral("DataStyle"));
    debugOdf << "The different auto style got assigned the name" << dataStyle2Name;
    QCOMPARE(dataStyle2Name, QStringLiteral("DataStyle1"));

    QCOMPARE(coll.styles().count(), 3);

    // XML for user style 1
    TEST_BEGIN(nullptr, nullptr);
    user1.writeStyle(&writer, coll, "style:style", user1StyleName, "style:paragraph-properties");
    TEST_END_QTTEST(QStringLiteral("<r>\n <style:style style:name=\"User1\" style:display-name=\"User 1\" style:family=\"paragraph\">\n  <style:paragraph-properties myfont=\"isBold\"/>\n </style:style>\n</r>\n"));

    // XML for user style 2
    TEST_BEGIN(nullptr, nullptr);
    user2.writeStyle(&writer, coll, "style:style", user2StyleName, "style:paragraph-properties");
    TEST_END_QTTEST(QStringLiteral("<r>\n <style:style style:name=\"User2\" style:display-name=\"User 2\" style:family=\"paragraph\">\n  <style:paragraph-properties myfont=\"isBold\"/>\n </style:style>\n</r>\n"));
}

void TestKoGenStyles::testStylesDotXml()
{
    debugOdf ;
    KoGenStyles coll;

    // Check that an autostyle-in-style.xml and an autostyle-in-content.xml
    // don't get the same name. It confuses KoGenStyle's named-based maps.
    KoGenStyle headerStyle(KoGenStyle::ParagraphAutoStyle, "paragraph");
    headerStyle.addAttribute("style:master-page-name", "Standard");
    headerStyle.addProperty("style:page-number", "0");
    headerStyle.setAutoStyleInStylesDotXml(true);
    QString headerStyleName = coll.insert(headerStyle, QStringLiteral("P"));
    QCOMPARE(headerStyleName, QStringLiteral("P1"));

    //debugOdf << coll;

    KoGenStyle first(KoGenStyle::ParagraphAutoStyle, "paragraph");
    first.addAttribute("style:master-page-name", "Standard");
    QString firstName = coll.insert(first, QStringLiteral("P"));
    debugOdf << "The auto style got assigned the name" << firstName;
    QCOMPARE(firstName, QStringLiteral("P2"));     // anything but not P1.
}

QTEST_MAIN(TestKoGenStyles)
