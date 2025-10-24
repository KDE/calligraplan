/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2004-2006 David Faure <faure@kde.org>
   SPDX-FileCopyrightText: 2007 Jan Hambrecht <jaham@gmx.net>
   SPDX-FileCopyrightText: 2008 Thorsten Zachmann <zachmann@kde.org>

   SPDX-License-Identifier: LGPL-2.0-only
*/

// clazy:excludeall=qstring-arg
#include "KoOdfStylesReader.h"

#include "KoXmlNS.h"
#include "KoOdfNotesConfiguration.h"
#include "KoOdfNumberDefinition.h"
#include "KoOdfLineNumberingConfiguration.h"
#include "KoOdfBibliographyConfiguration.h"

#include <OdfDebug.h>

class Q_DECL_HIDDEN KoOdfStylesReader::Private
{
public:

    Private()
        : globalFootnoteConfiguration(KoOdfNotesConfiguration::Footnote)
        , globalEndnoteConfiguration(KoOdfNotesConfiguration::Endnote)
    {
    }

    QHash < QString /*family*/, QHash < QString /*name*/, KoXmlElement* > > customStyles;
    // auto-styles in content.xml
    QHash < QString /*family*/, QHash < QString /*name*/, KoXmlElement* > > contentAutoStyles;
    // auto-styles in styles.xml
    QHash < QString /*family*/, QHash < QString /*name*/, KoXmlElement* > > stylesAutoStyles;
    QHash < QString /*family*/, KoXmlElement* > defaultStyles;

    QHash < QString /*name*/, KoXmlElement* > styles; // page-layout, font-face etc.
    QHash < QString /*name*/, KoXmlElement* > masterPages;
    QHash < QString /*name*/, KoXmlElement* > presentationPageLayouts;
    QHash < QString /*drawType*/, QHash< QString /*name*/, KoXmlElement* > > drawStyles;

    QList <KoXmlElement* > tableTemplates;

    KoXmlElement           officeStyle;
    KoXmlElement           layerSet;

    DataFormatsMap         dataFormats;

    // XXX: there can also be notes configuration objects _per_ section.
    KoOdfNotesConfiguration globalFootnoteConfiguration;
    KoOdfNotesConfiguration globalEndnoteConfiguration;

    KoOdfBibliographyConfiguration globalBibliographyConfiguration;

    KoOdfLineNumberingConfiguration lineNumberingConfiguration;

};

KoOdfStylesReader::KoOdfStylesReader()
    : d(new Private)
{
}

KoOdfStylesReader::~KoOdfStylesReader()
{
    typedef QHash<QString, KoXmlElement*> AutoStylesMap;
    for (const AutoStylesMap& map : std::as_const(d->customStyles))
        qDeleteAll(map);
    for (const AutoStylesMap& map : std::as_const(d->contentAutoStyles))
        qDeleteAll(map);
    for (const AutoStylesMap& map : std::as_const(d->stylesAutoStyles))
        qDeleteAll(map);
    for (const DataFormatsMap::mapped_type& dataFormat : std::as_const(d->dataFormats))
        delete dataFormat.second;
    qDeleteAll(d->defaultStyles);
    qDeleteAll(d->styles);
    qDeleteAll(d->masterPages);
    qDeleteAll(d->presentationPageLayouts);
    qDeleteAll(d->tableTemplates);
    for (const AutoStylesMap& map : std::as_const(d->drawStyles)) {
        qDeleteAll(map);
    }
    delete d;
}

void KoOdfStylesReader::createStyleMap(const KoXmlDocument& doc, bool stylesDotXml)
{
    const KoXmlElement docElement  = doc.documentElement();
    // We used to have the office:version check here, but better let the apps do that
    KoXmlElement fontStyles = KoXml::namedItemNS(docElement, KoXmlNS::office, "font-face-decls");

    if (!fontStyles.isNull()) {
        //debugOdf <<"Starting reading in font-face-decls...";
        insertStyles(fontStyles, stylesDotXml ? AutomaticInStyles : AutomaticInContent);
    }// else
    //   debugOdf <<"No items found";

    //debugOdf <<"Starting reading in office:automatic-styles. stylesDotXml=" << stylesDotXml;

    KoXmlElement autoStyles = KoXml::namedItemNS(docElement, KoXmlNS::office, "automatic-styles");
    if (!autoStyles.isNull()) {
        insertStyles(autoStyles, stylesDotXml ? AutomaticInStyles : AutomaticInContent);
    }// else
    //    debugOdf <<"No items found";


    //debugOdf <<"Reading in master styles";

    KoXmlNode masterStyles = KoXml::namedItemNS(docElement, KoXmlNS::office, "master-styles");
    if (!masterStyles.isNull()) {
        KoXmlElement master;
        forEachElement(master, masterStyles) {
            if (master.localName() == QStringLiteral("master-page") &&
                master.namespaceURI() == KoXmlNS::style) {
                const QString name = master.attributeNS(KoXmlNS::style, "name", QString());
                debugOdf << "Master style: '" << name << "' loaded";
                d->masterPages.insert(name, new KoXmlElement(master));
            } else if (master.localName() == QStringLiteral("layer-set") && master.namespaceURI() == KoXmlNS::draw) {
                debugOdf << "Master style: layer-set loaded";
                d->layerSet = master;
            } else
                // OASIS docu mentions style:handout-master and draw:layer-set here
                warnOdf << "Unknown tag " << master.tagName() << " in office:master-styles";
        }
    }


    debugOdf << "Starting reading in office:styles";

    const KoXmlElement officeStyle = KoXml::namedItemNS(docElement, KoXmlNS::office, "styles");

    if (!officeStyle.isNull()) {
        d->officeStyle = officeStyle;
        insertOfficeStyles(officeStyle);
    }

    //debugOdf <<"Styles read in.";
}

QHash<QString, KoXmlElement*> KoOdfStylesReader::customStyles(const QString& family) const
{
    if (family.isNull())
        return QHash<QString, KoXmlElement*>();
    return d->customStyles.value(family);
}

QHash<QString, KoXmlElement*> KoOdfStylesReader::customStyles(const char *family) const
{
    return customStyles(QLatin1String(family));
}

QHash<QString, KoXmlElement*> KoOdfStylesReader::autoStyles(const QString& family, bool stylesDotXml) const
{
    if (family.isNull())
        return QHash<QString, KoXmlElement*>();
    return stylesDotXml ? d->stylesAutoStyles.value(family) : d->contentAutoStyles.value(family);
}

KoOdfStylesReader::DataFormatsMap KoOdfStylesReader::dataFormats() const
{
    return d->dataFormats;
}

KoOdfNotesConfiguration KoOdfStylesReader::globalNotesConfiguration(KoOdfNotesConfiguration::NoteClass noteClass) const
{
    switch (noteClass) {
    case (KoOdfNotesConfiguration::Endnote):
        return d->globalEndnoteConfiguration;
    case (KoOdfNotesConfiguration::Footnote):
    default:
        return d->globalFootnoteConfiguration;
    }
}

KoOdfBibliographyConfiguration KoOdfStylesReader::globalBibliographyConfiguration() const
{
    return d->globalBibliographyConfiguration;
}

KoOdfLineNumberingConfiguration KoOdfStylesReader::lineNumberingConfiguration() const
{
    return d->lineNumberingConfiguration;
}


void KoOdfStylesReader::insertOfficeStyles(const KoXmlElement& styles)
{
    KoXmlElement e;
    forEachElement(e, styles) {
        const QString localName = e.localName();
        const QString ns = e.namespaceURI();
        if ((ns == KoXmlNS::svg && (
                localName == QStringLiteral("linearGradient")
                || localName == QStringLiteral("radialGradient")))
            || (ns == KoXmlNS::draw && (
                    localName == QStringLiteral("gradient")
                    || localName == QStringLiteral("hatch")
                    || localName == QStringLiteral("fill-image")
                    || localName == QStringLiteral("marker")
                    || localName == QStringLiteral("stroke-dash")
                    || localName == QStringLiteral("opacity")))
            || (ns == KoXmlNS::calligra && (
                    localName == QStringLiteral("conicalGradient")))
            ) {
            QString drawType = localName;
            if (drawType.endsWith(QStringLiteral("Gradient"))) {
                drawType = QStringLiteral("gradient");
            }
            const QString name = e.attributeNS(KoXmlNS::draw, "name", QString());
            Q_ASSERT(!name.isEmpty());
            KoXmlElement* ep = new KoXmlElement(e);
            d->drawStyles[drawType].insert(name, ep);
        }else if(ns == KoXmlNS::table && localName == QStringLiteral("table-template")) {
            d->tableTemplates.append(new KoXmlElement(e));
        } else {
            insertStyle(e, CustomInStyles);
        }
    }
}

void KoOdfStylesReader::insertStyles(const KoXmlElement& styles, TypeAndLocation typeAndLocation)
{
    //debugOdf <<"Inserting styles from" << styles.tagName();
    KoXmlElement e;
    forEachElement(e, styles)
            insertStyle(e, typeAndLocation);
}

void KoOdfStylesReader::insertStyle(const KoXmlElement& e, TypeAndLocation typeAndLocation)
{
    const QString localName = e.localName();
    const QString ns = e.namespaceURI();
    const QString name = e.attributeNS(KoXmlNS::style, "name", QString());

    if ((ns == KoXmlNS::style && localName == QStringLiteral("style"))
        || (ns == KoXmlNS::text && localName == QStringLiteral("list-style"))) {
        const QString family = localName == QStringLiteral("list-style") ? QStringLiteral("list") : e.attributeNS(KoXmlNS::style, "family", QString());

        if (typeAndLocation == AutomaticInContent) {
            QHash<QString, KoXmlElement*>& dict = d->contentAutoStyles[ family ];
            if (dict.contains(name)) {
                debugOdf << "Auto-style: '" << name << "' already exists";
                delete dict.take(name);
            }
            dict.insert(name, new KoXmlElement(e));
            //debugOdf <<"Style: '" << name <<"' loaded as a style auto style";
        } else if (typeAndLocation == AutomaticInStyles) {
            QHash<QString, KoXmlElement*>& dict = d->stylesAutoStyles[ family ];
            if (dict.contains(name)) {
                debugOdf << "Auto-style: '" << name << "' already exists";
                delete dict.take(name);
            }
            dict.insert(name, new KoXmlElement(e));
            //debugOdf <<"Style: '" << name <<"' loaded as a style auto style";
        } else {
            QHash<QString, KoXmlElement*>& dict = d->customStyles[ family ];
            if (dict.contains(name)) {
                debugOdf << "Style: '" << name << "' already exists";
                delete dict.take(name);
            }
            dict.insert(name, new KoXmlElement(e));
            //debugOdf <<"Style: '" << name <<"' loaded";
        }
    } else if (ns == KoXmlNS::style && (
            localName == QStringLiteral("page-layout")
            || localName == QStringLiteral("font-face"))) {
        if (d->styles.contains(name)) {
            debugOdf << "Style: '" << name << "' already exists";
            delete d->styles.take(name);
        }
        d->styles.insert(name, new KoXmlElement(e));
    } else if (localName == QStringLiteral("presentation-page-layout") && ns == KoXmlNS::style) {
        if (d->presentationPageLayouts.contains(name)) {
            debugOdf << "Presentation page layout: '" << name << "' already exists";
            delete d->presentationPageLayouts.take(name);
        }
        d->presentationPageLayouts.insert(name, new KoXmlElement(e));
    } else if (localName == QStringLiteral("default-style") && ns == KoXmlNS::style) {
        const QString family = e.attributeNS(KoXmlNS::style, "family", QString());
        if (!family.isEmpty())
            d->defaultStyles.insert(family, new KoXmlElement(e));
    } else if (ns == KoXmlNS::number && (
                   localName == QStringLiteral("number-style")
                   || localName == QStringLiteral("currency-style")
                   || localName == QStringLiteral("percentage-style")
                   || localName == QStringLiteral("boolean-style")
                   || localName == QStringLiteral("text-style")
                   || localName == QStringLiteral("date-style")
                   || localName == QStringLiteral("time-style"))) {
        QPair<QString, KoOdfNumberStyles::NumericStyleFormat> numberStyle = KoOdfNumberStyles::loadOdfNumberStyle(e);
        d->dataFormats.insert(numberStyle.first, qMakePair(numberStyle.second, new KoXmlElement(e)));
    } else if (ns == KoXmlNS::text && localName == QStringLiteral("notes-configuration")) {
        if (e.attributeNS(KoXmlNS::text, "note-class", QStringLiteral("footnote")) == QStringLiteral("footnote")) {
            d->globalFootnoteConfiguration.loadOdf(e);
        } else  {
            d->globalEndnoteConfiguration.loadOdf(e);
        }
    } else if (ns == KoXmlNS::text && localName == QStringLiteral("linenumbering-configuration")) {
        d->lineNumberingConfiguration.loadOdf(e);
    } else if (ns == KoXmlNS::text && localName == QStringLiteral("bibliography-configuration")) {
        KoOdfBibliographyConfiguration bibConfiguration;
        bibConfiguration.loadOdf(e);
        d->globalBibliographyConfiguration = bibConfiguration;
    }
}

KoXmlElement *KoOdfStylesReader::defaultStyle(const QString &family) const
{
    return d->defaultStyles[family];
}

KoXmlElement KoOdfStylesReader::officeStyle() const
{
    return d->officeStyle;
}

KoXmlElement KoOdfStylesReader::layerSet() const
{
    return d->layerSet;
}

QHash<QString, KoXmlElement*> KoOdfStylesReader::masterPages() const
{
    return d->masterPages;
}

QHash<QString, KoXmlElement*> KoOdfStylesReader::presentationPageLayouts() const
{
    return d->presentationPageLayouts;
}

QHash<QString, KoXmlElement*> KoOdfStylesReader::drawStyles(const QString &drawType) const
{
    return d->drawStyles.value(drawType);
}

QHash<QString, KoXmlElement*> KoOdfStylesReader::drawStyles(const char *drawType) const
{
    return d->drawStyles.value(QLatin1String(drawType));
}

const KoXmlElement* KoOdfStylesReader::findStyle(const QString& name) const
{
    return d->styles[ name ];
}

const KoXmlElement* KoOdfStylesReader::findStyle(const QString& styleName, const QString& family) const
{
    const KoXmlElement* style = findStyleCustomStyle(styleName, family);
    if (!style)
        style = findAutoStyleStyle(styleName, family);
    if (!style)
        style = findContentAutoStyle(styleName, family);
    return style;
}

const KoXmlElement* KoOdfStylesReader::findStyle(const QString& styleName, const QString& family, bool stylesDotXml) const
{
    const KoXmlElement* style = findStyleCustomStyle(styleName, family);
    if (!style && !stylesDotXml) {
        style = findContentAutoStyle(styleName, family);
    }
    if (!style && stylesDotXml) {
        style = findAutoStyleStyle(styleName, family);
    }
    return style;

}

const KoXmlElement* KoOdfStylesReader::findStyleCustomStyle(const QString& styleName, const QString& family) const
{
    const KoXmlElement* style = d->customStyles.value(family).value(styleName);
    if (style && !family.isEmpty()) {
        const QString styleFamily = style->attributeNS(KoXmlNS::style, "family", QString());
        if (styleFamily != family) {
            warnOdf << "KoOdfStylesReader: was looking for style " << styleName
                    << " in family " << family << " but got " << styleFamily << '\n';
        }
    }
    return style;
}

const KoXmlElement* KoOdfStylesReader::findAutoStyleStyle(const QString& styleName, const QString& family) const
{
    const KoXmlElement* style = d->stylesAutoStyles.value(family).value(styleName);
    if (style) {
        const QString styleFamily = style->attributeNS(KoXmlNS::style, "family", QString());
        if (styleFamily != family) {
            warnOdf << "KoOdfStylesReader: was looking for style " << styleName
                    << " in family " << family << " but got " << styleFamily << '\n';
        }
    }
    return style;
}

const KoXmlElement* KoOdfStylesReader::findContentAutoStyle(const QString& styleName, const QString& family) const
{
    const KoXmlElement* style = d->contentAutoStyles.value(family).value(styleName);
    if (style) {
        const QString styleFamily = style->attributeNS(KoXmlNS::style, "family", QString());
        if (styleFamily != family) {
            warnOdf << "KoOdfStylesReader: was looking for style " << styleName
                    << " in family " << family << " but got " << styleFamily << '\n';
        }
    }
    return style;
}

QList<KoXmlElement*> KoOdfStylesReader::tableTemplates() const
{
    return d->tableTemplates;
}
