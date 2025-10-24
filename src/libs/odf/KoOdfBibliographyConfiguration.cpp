/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2011 Smit Patel <smitpatel24@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */
// clazy:excludeall=qstring-arg
#include "KoOdfBibliographyConfiguration.h"

#include <OdfDebug.h>
#include "KoXmlNS.h"
#include "KoXmlWriter.h"

#include <QList>

const QList<QString> KoOdfBibliographyConfiguration::bibTypes = QList<QString>() << QStringLiteral("article") << QStringLiteral("book") << QStringLiteral("booklet") << QStringLiteral("conference")
                                                                     << QStringLiteral("email") << QStringLiteral("inbook") << QStringLiteral("incollection")
                                                                     << QStringLiteral("inproceedings") << QStringLiteral("journal") << QStringLiteral("manual")
                                                                     << QStringLiteral("mastersthesis") << QStringLiteral("misc") << QStringLiteral("phdthesis")
                                                                     << QStringLiteral("proceedings") << QStringLiteral("techreport") << QStringLiteral("unpublished")
                                                                     << QStringLiteral("www") << QStringLiteral("custom1") << QStringLiteral("custom2")
                                                                     << QStringLiteral("custom3") << QStringLiteral("custom4") << QStringLiteral("custom5");

const QList<QString> KoOdfBibliographyConfiguration::bibDataFields = QList<QString>() << QStringLiteral("address") << QStringLiteral("annote") << QStringLiteral("author")
                                                                          << QStringLiteral("bibliography-type") << QStringLiteral("booktitle")
                                                                          << QStringLiteral("chapter") << QStringLiteral("custom1") << QStringLiteral("custom2")
                                                                          << QStringLiteral("custom3") << QStringLiteral("custom4") << QStringLiteral("custom5")
                                                                          << QStringLiteral("edition") << QStringLiteral("editor") << QStringLiteral("howpublished")
                                                                          << QStringLiteral("identifier") << QStringLiteral("institution") << QStringLiteral("isbn")
                                                                          << QStringLiteral("issn") << QStringLiteral("journal") << QStringLiteral("month") << QStringLiteral("note")
                                                                          << QStringLiteral("number") << QStringLiteral("organizations") << QStringLiteral("pages")
                                                                          << QStringLiteral("publisher") << QStringLiteral("report-type") << QStringLiteral("school")
                                                                          << QStringLiteral("series") << QStringLiteral("title") << QStringLiteral("url") << QStringLiteral("volume")
                                                                          << QStringLiteral("year");

class Q_DECL_HIDDEN KoOdfBibliographyConfiguration::Private
{
public:
    QString prefix;
    QString suffix;
    bool numberedEntries;
    bool sortByPosition;
    QString sortAlgorithm;
    QVector<SortKeyPair> sortKeys;
};

KoOdfBibliographyConfiguration::KoOdfBibliographyConfiguration()
    : d(new Private())
{
    d->prefix = QStringLiteral("[");
    d->suffix = QStringLiteral("]");
    d->numberedEntries = false;
    d->sortByPosition = true;
}

KoOdfBibliographyConfiguration::~KoOdfBibliographyConfiguration()
{
    delete d;
}

KoOdfBibliographyConfiguration::KoOdfBibliographyConfiguration(const KoOdfBibliographyConfiguration &other)
    : QObject(), d(new Private())
{
    *this = other;
}

KoOdfBibliographyConfiguration &KoOdfBibliographyConfiguration::operator=(const KoOdfBibliographyConfiguration &other)
{
    d->prefix = other.d->prefix;
    d->suffix = other.d->suffix;
    d->numberedEntries = other.d->numberedEntries;
    d->sortAlgorithm = other.d->sortAlgorithm;
    d->sortByPosition = other.d->sortByPosition;
    d->sortKeys = other.d->sortKeys;

    return *this;
}


void KoOdfBibliographyConfiguration::loadOdf(const KoXmlElement &element)
{
    d->prefix = element.attributeNS(KoXmlNS::text, "prefix", QString());
    d->suffix = element.attributeNS(KoXmlNS::text, "suffix", QString());
    d->numberedEntries = (element.attributeNS(KoXmlNS::text, "numbered-entries", QStringLiteral("false")) == QStringLiteral("true"))
                         ? true : false;
    d->sortByPosition = (element.attributeNS(KoXmlNS::text, "sort-by-position", QStringLiteral("true")) == QStringLiteral("true"))
                        ? true : false;
    d->sortAlgorithm = element.attributeNS(KoXmlNS::text, "sort-algorithm", QString());

    for (KoXmlNode node = element.firstChild(); !node.isNull(); node = node.nextSibling())
    {
        KoXmlElement child = node.toElement();

        if (child.namespaceURI() == KoXmlNS::text && child.localName() == QStringLiteral("sort-key")) {
            QString key = child.attributeNS(KoXmlNS::text, "key", QString());
            Qt::SortOrder order = (child.attributeNS(KoXmlNS::text, "sort-ascending", "true") == QStringLiteral("true"))
                    ? (Qt::AscendingOrder): (Qt::DescendingOrder);
            if(!key.isNull() && KoOdfBibliographyConfiguration::bibDataFields.contains(key)) {
                d->sortKeys << QPair<QString, Qt::SortOrder>(key,order);
            }
        }
    }
}

void KoOdfBibliographyConfiguration::saveOdf(KoXmlWriter *writer) const
{
    writer->startElement("text:bibliography-configuration");

    if (!d->prefix.isNull()) {
        writer->addAttribute("text:prefix", d->prefix);
    }

    if (!d->suffix.isNull()) {
        writer->addAttribute("text:suffix", d->suffix);
    }

    if (!d->sortAlgorithm.isNull()) {
        writer->addAttribute("text:sort-algorithm", d->sortAlgorithm);
    }

    writer->addAttribute("text:numbered-entries", d->numberedEntries ? "true" : "false");
    writer->addAttribute("text:sort-by-position", d->sortByPosition ? "true" : "false");

    for (const SortKeyPair &key : std::as_const(d->sortKeys)) {
            writer->startElement("text:sort-key");
            writer->addAttribute("text:key", key.first);
            writer->addAttribute("text:sort-ascending",key.second);
            writer->endElement();
    }
    writer->endElement();
}

QString KoOdfBibliographyConfiguration::prefix() const
{
    return d->prefix;
}

QString KoOdfBibliographyConfiguration::suffix() const
{
    return d->suffix;
}

QString KoOdfBibliographyConfiguration::sortAlgorithm() const
{
    return d->sortAlgorithm;
}

bool KoOdfBibliographyConfiguration::sortByPosition() const
{
    return d->sortByPosition;
}

QVector<SortKeyPair> KoOdfBibliographyConfiguration::sortKeys() const
{
    return d->sortKeys;
}

bool KoOdfBibliographyConfiguration::numberedEntries() const
{
    return d->numberedEntries;
}

void KoOdfBibliographyConfiguration::setNumberedEntries(bool enable)
{
    d->numberedEntries = enable;
}

void KoOdfBibliographyConfiguration::setPrefix(const QString &prefixValue)
{
    d->prefix = prefixValue;
}

void KoOdfBibliographyConfiguration::setSuffix(const QString &suffixValue)
{
    d->suffix = suffixValue;
}

void KoOdfBibliographyConfiguration::setSortAlgorithm(const QString &algorithm)
{
    d->sortAlgorithm = algorithm;
}

void KoOdfBibliographyConfiguration::setSortByPosition(bool enable)
{
    d->sortByPosition = enable;
}

void KoOdfBibliographyConfiguration::setSortKeys(const QVector<SortKeyPair> &sortKeys)
{
    d->sortKeys = sortKeys;
}
