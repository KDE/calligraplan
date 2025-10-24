/* This file is part of the KDE project
 *
 * SPDX-FileCopyrightText: 2011 Pierre Ducroquet <pinaraf@pinaraf.info>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "KoShadowStyle.h"

#include <KoUnit.h>


// KoShadowStyle private class
class KoShadowStylePrivate: public QSharedData
{
public:
    KoShadowStylePrivate();
    ~KoShadowStylePrivate();

    QVector<KoShadowStyle::ShadowData> shadows;
};

KoShadowStylePrivate::KoShadowStylePrivate()
{
}

KoShadowStylePrivate::~KoShadowStylePrivate()
{
}

// KoShadowStyle::ShadowData structure
KoShadowStyle::ShadowData::ShadowData()
    : color(), offset(0, 0), radius(0.0)
{
}

bool KoShadowStyle::ShadowData::operator==(const KoShadowStyle::ShadowData &other) const
{
    return (color == other.color) && (offset == other.offset) && (radius == other.radius);
}

// KoShadowStyle class
KoShadowStyle::KoShadowStyle()
    : d(new KoShadowStylePrivate)
{
}

KoShadowStyle::KoShadowStyle(const KoShadowStyle &other)
    : d(other.d)
{
}

KoShadowStyle::~KoShadowStyle()
{
}

bool KoShadowStyle::operator==(const KoShadowStyle &other) const
{
    if (d.data() == other.d.data())
        return true;

    if (shadowCount() != other.shadowCount())
        return false;

    for (const ShadowData &data : std::as_const(d->shadows))
    {
        if (!other.d->shadows.contains(data))
            return false;
    }
    return true;
}

bool KoShadowStyle::operator!=(const KoShadowStyle &other) const
{
    return !operator==(other);
}

// load value string as specified by CSS2 §7.16.5 "text-shadow"
bool KoShadowStyle::loadOdf (const QString &data)
{
    if (data == QStringLiteral("none"))
        return true;

    const QStringList sub_shadows = data.split(QLatin1Char(','));
    for (const QString &shadow : sub_shadows) {
        QStringList words = shadow.split(QLatin1Char(' '), Qt::SkipEmptyParts);
        if (words.isEmpty())
            return false;

        KoShadowStyle::ShadowData currentData;

        // look for color at begin
        QColor shadowColor(words.first());
        if (shadowColor.isValid()) {
            currentData.color = shadowColor;
            words.removeFirst();
        } else if (words.length() > 2) {
            // look for color at end, if there could be one
            shadowColor = QColor(words.last());
            if (shadowColor.isValid()) {
                currentData.color = shadowColor;
                words.removeLast();
            }
        }
        // We keep an invalid color.if none was found

        // "Each shadow effect must specify a shadow offset and may optionally
        // specify a blur radius and a shadow color.", from CSS2 §7.16.5 "text-shadow"
        // But for some reason also no offset has been accepted before. TODO: which?
        if (! words.isEmpty()) {
            if ((words.length() < 2) || (words.length() > 3))
                return false;

            // Parse offset
            currentData.offset.setX(KoUnit::parseValue(words.at(0), 0.0));
            currentData.offset.setY(KoUnit::parseValue(words.at(1), 0.0));
            // Parse blur radius if present
            if (words.length() == 3)
                currentData.radius = KoUnit::parseValue(words.at(2), 0.0);
        }
        d->shadows << currentData;
    }
    return true;
}

int KoShadowStyle::shadowCount() const
{
    return d->shadows.size();
}

QString KoShadowStyle::saveOdf() const
{
    if (d->shadows.isEmpty())
        return QStringLiteral("none");

    QStringList parts;
    const QString pt = QStringLiteral("%1pt");
    for (const ShadowData &data : std::as_const(d->shadows)) {
        QStringList elements;
        if (data.color.isValid()) {
            elements << data.color.name();
        }
        elements << pt.arg(data.offset.x()) << pt.arg(data.offset.y());
        if (data.radius != 0)
            elements << pt.arg(data.radius);

        parts << elements.join(QStringLiteral(" "));
    }
    return parts.join(QStringLiteral(","));
}

