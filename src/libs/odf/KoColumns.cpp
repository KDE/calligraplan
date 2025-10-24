/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2012 Friedrich W. H. Kossebau <kossebau@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "KoColumns.h"

#include "KoXmlWriter.h"
#include "KoGenStyle.h"
#include "KoXmlReader.h"
#include "KoXmlNS.h"
#include "KoUnit.h"
#include "OdfDebug.h"

static const int defaultColumnCount = 1;
static const KoColumns::SeparatorStyle defaultSeparatorStyle = KoColumns::None;
static const int defaultSeparatorHeight = 100;
static const  Qt::GlobalColor defaultSeparatorColor = Qt::black;
static const KoColumns::SeparatorVerticalAlignment defaultSeparatorVerticalAlignment = KoColumns::AlignTop;
static const int defaultColumnGapWidth = 17; // in pt, ~ 6mm



const char * KoColumns::separatorStyleString(KoColumns::SeparatorStyle separatorStyle)
{
    const char * result;

    //  skip KoColumns::None, is default
    if (separatorStyle == Solid) {
        result = "solid";
    } else if (separatorStyle == Dotted) {
        result = "dotted";
    } else if (separatorStyle == Dashed) {
        result = "dashed";
    } else if (separatorStyle == DotDashed) {
        result = "dot-dashed";
    } else {
        result = "none";
    }

    return result;
}

const char * KoColumns::separatorVerticalAlignmentString(KoColumns::SeparatorVerticalAlignment separatorVerticalAlignment)
{
    const char * result;

    //  skip KoColumns::AlignTop, is default
    if (separatorVerticalAlignment == AlignVCenter) {
        result = "middle";
    } else if (separatorVerticalAlignment == AlignBottom) {
        result = "bottom";
    } else {
        result = "top";
    }

    return result;
}

KoColumns::SeparatorVerticalAlignment KoColumns::parseSeparatorVerticalAlignment(const QString &value)
{
    // default to AlignTop
    SeparatorVerticalAlignment result = defaultSeparatorVerticalAlignment;

    if (! value.isEmpty()) {
        // skip "top", is default
        if (value == QStringLiteral("middle")) {
            result = AlignVCenter;
        } else if (value == QStringLiteral("bottom")) {
            result = AlignBottom;
        }
    }

    return result;
}

QColor KoColumns::parseSeparatorColor(const QString &value)
{
    QColor result(value);

    if (! result.isValid())
        // default is black, cmp. ODF 1.2 §19.467
        result = QColor(defaultSeparatorColor);

    return result;
}

// TODO: see if there is another parse method somewhere which can be used here
int KoColumns::parseSeparatorHeight(const QString &value)
{
    int result = defaultSeparatorHeight;

    // only try to convert if it ends with a %, so is also not empty
    if (value.endsWith(QLatin1Char('%'))) {
        bool ok = false;
        // try to convert
        result = value.left(value.length()-1).toInt(&ok);
        // reset to 100% if conversion failed (which sets result to 0)
        if (! ok) {
            result = defaultSeparatorHeight;
        }
    }

    return result;
}

KoColumns::SeparatorStyle KoColumns::parseSeparatorStyle(const QString &value)
{
    SeparatorStyle result = None;
    if (! value.isEmpty()) {
        //  skip "none", is default
        if (value == QStringLiteral("solid")) {
            result = Solid;
        } else if (value == QStringLiteral("dotted")) {
            result = Dotted;
        } else if (value == QStringLiteral("dashed")) {
            result = Dashed;
        } else if (value == QStringLiteral("dot-dashed")) {
            result = DotDashed;
        }
    }

    return result;
}

int KoColumns::parseRelativeWidth(const QString &value)
{
    int result = 0;

    // only try to convert if it ends with a *, so is also not empty
    if (value.endsWith(QLatin1Char('*'))) {
        bool ok = false;
        // try to convert
        result = value.left(value.length()-1).toInt(&ok);
        if (! ok) {
            result = 0;
        }
    }

    return result;
}

KoColumns::KoColumns()
  : count(defaultColumnCount)
  , gapWidth(defaultColumnGapWidth)
  , separatorStyle(defaultSeparatorStyle)
  , separatorColor(defaultSeparatorColor)
  , separatorVerticalAlignment(defaultSeparatorVerticalAlignment)
  , separatorHeight(defaultSeparatorHeight)
{
}

void KoColumns::reset()
{
    count = defaultColumnCount;
    gapWidth = defaultColumnGapWidth;
    separatorStyle = defaultSeparatorStyle;
    separatorColor = QColor(defaultSeparatorColor);
    separatorVerticalAlignment = defaultSeparatorVerticalAlignment;
    separatorHeight = defaultSeparatorHeight;
}

bool KoColumns::operator==(const KoColumns& rhs) const {
    return
        count == rhs.count &&
        (columnData.isEmpty() && rhs.columnData.isEmpty() ?
             (qAbs(gapWidth - rhs.gapWidth) <= 1E-10) :
             (columnData == rhs.columnData));
}

bool KoColumns::operator!=(const KoColumns& rhs) const {
    return
        count != rhs.count ||
        (columnData.isEmpty() && rhs.columnData.isEmpty() ?
             qAbs(gapWidth - rhs.gapWidth) > 1E-10 :
             ! (columnData == rhs.columnData));
}

void KoColumns::loadOdf(const KoXmlElement &style)
{
    KoXmlElement columnsElement = KoXml::namedItemNS(style, KoXmlNS::style, "columns");
    if (!columnsElement.isNull()) {
        count = columnsElement.attributeNS(KoXmlNS::fo, "column-count").toInt();
        if (count < 1)
            count = 1;
        gapWidth = KoUnit::parseValue(columnsElement.attributeNS(KoXmlNS::fo, "column-gap"));

        KoXmlElement columnSep = KoXml::namedItemNS(columnsElement, KoXmlNS::style, "column-sep");
        if (! columnSep.isNull()) {
            separatorStyle = parseSeparatorStyle(columnSep.attributeNS(KoXmlNS::style, "style"));
            separatorWidth = KoUnit::parseValue(columnSep.attributeNS(KoXmlNS::style, "width"));
            separatorHeight = parseSeparatorHeight(columnSep.attributeNS(KoXmlNS::style, "height"));
            separatorColor = parseSeparatorColor(columnSep.attributeNS(KoXmlNS::style, "color"));
            separatorVerticalAlignment =
                parseSeparatorVerticalAlignment(columnSep.attributeNS(KoXmlNS::style, "vertical-align"));
        }

        KoXmlElement columnElement;
        forEachElement(columnElement, columnsElement) {
            if(columnElement.localName() != QStringLiteral("column") ||
               columnElement.namespaceURI() != KoXmlNS::style)
                continue;

            ColumnDatum datum;
            datum.leftMargin = KoUnit::parseValue(columnElement.attributeNS(KoXmlNS::fo, "start-indent"), 0.0);
            datum.rightMargin = KoUnit::parseValue(columnElement.attributeNS(KoXmlNS::fo, "end-indent"), 0.0);
            datum.topMargin = KoUnit::parseValue(columnElement.attributeNS(KoXmlNS::fo, "space-before"), 0.0);
            datum.bottomMargin = KoUnit::parseValue(columnElement.attributeNS(KoXmlNS::fo, "space-after"), 0.0);
            datum.relativeWidth = parseRelativeWidth(columnElement.attributeNS(KoXmlNS::style, "rel-width"));
            // on a bad relativeWidth just drop all data
            if (datum.relativeWidth <= 0) {
                columnData.clear();
                break;
            }

            columnData.append(datum);
        }

        if (! columnData.isEmpty() && count != columnData.count()) {
            warnOdf << "Found not as many <style:column> elements as attribute fo:column-count has set:"<< count;
            columnData.clear();
        }
    } else {
        reset();
    }
}

void KoColumns::saveOdf(KoGenStyle &style) const
{
    if (count > 1) {
        QBuffer buffer;
        buffer.open(QIODevice::WriteOnly);
        KoXmlWriter writer(&buffer);

        writer.startElement("style:columns");
        writer.addAttribute("fo:column-count", count);
        if (columnData.isEmpty()) {
            writer.addAttributePt("fo:column-gap", gapWidth);
        }

        if (separatorStyle != KoColumns::None) {
            writer.startElement("style:column-sep");
            writer.addAttribute("style:style", separatorStyleString(separatorStyle));
            writer.addAttributePt("style:width", separatorWidth);
            writer.addAttribute("style:height", QString::number(separatorHeight)+QLatin1Char('%'));
            writer.addAttribute("style:color", separatorColor.name());
            writer.addAttribute("style:vertical-align", separatorVerticalAlignmentString(separatorVerticalAlignment));
            writer.endElement(); // style:column-sep
        }

        for(const ColumnDatum &cd : std::as_const(columnData)) {
            writer.startElement("style:column");
            writer.addAttributePt("fo:start-indent", cd.leftMargin);
            writer.addAttributePt("fo:end-indent", cd.rightMargin);
            writer.addAttributePt("fo:space-before", cd.topMargin);
            writer.addAttributePt("fo:space-after", cd.bottomMargin);
            writer.addAttribute("style:rel-width", QString::number(cd.relativeWidth)+QLatin1Char('*'));
            writer.endElement(); // style:column
        }

        writer.endElement(); // style:columns

        QString contentElement = QString::fromUtf8(buffer.buffer().constData(), buffer.buffer().size());
        style.addChildElement("style:columns", contentElement);
    }
}
