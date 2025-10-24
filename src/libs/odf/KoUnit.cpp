/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2001 David Faure <faure@kde.org>
   SPDX-FileCopyrightText: 2004 Nicolas GOUTTE <goutte@kde.org>
   SPDX-FileCopyrightText: 2012 Friedrich W. H. Kossebau <kossebau@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "KoUnit.h"

#include <cmath>

#include <QTransform>

#include <KLocalizedString>
#include <OdfDebug.h>
#include <QtGlobal>
#include <QLocale>

// ensure the same order as in KoUnit::Unit
static const char* const unitNameList[KoUnit::TypeCount] =
{
    "mm",
    "pt",
    "in",
    "cm",
    "dm",
    "pi",
    "cc",
    "px"
};

QString KoUnit::unitDescription(KoUnit::Type type)
{
    switch (type) {
    case KoUnit::Millimeter:
        return i18n("Millimeters (mm)");
    case KoUnit::Centimeter:
        return i18n("Centimeters (cm)");
    case KoUnit::Decimeter:
        return i18n("Decimeters (dm)");
    case KoUnit::Inch:
        return i18n("Inches (in)");
    case KoUnit::Pica:
        return i18n("Pica (pi)");
    case KoUnit::Cicero:
        return i18n("Cicero (cc)");
    case KoUnit::Point:
        return i18n("Points (pt)");
    case KoUnit::Pixel:
        return i18n("Pixels (px)");
    default:
        return i18n("Unsupported unit");
    }
}

// grouped by units which are similar
static const KoUnit::Type typesInUi[KoUnit::TypeCount] =
{
    KoUnit::Millimeter,
    KoUnit::Centimeter,
    KoUnit::Decimeter,
    KoUnit::Inch,
    KoUnit::Pica,
    KoUnit::Cicero,
    KoUnit::Point,
    KoUnit::Pixel,
};

QStringList KoUnit::listOfUnitNameForUi(ListOptions listOptions)
{
    QStringList lst;
    for (int i = 0; i < KoUnit::TypeCount; ++i) {
        const Type type = typesInUi[i];
        if ((type != Pixel) || ((listOptions & HideMask) == ListAll))
            lst.append(unitDescription(type));
    }
    return lst;
}

KoUnit KoUnit::fromListForUi(int index, ListOptions listOptions, qreal factor)
{
    KoUnit::Type type = KoUnit::Point;

    if ((0 <= index) && (index < KoUnit::TypeCount)) {
        // iterate through all enums and skip the Pixel enum if needed
        for (int i = 0; i < KoUnit::TypeCount; ++i) {
            if ((listOptions&HidePixel) && (typesInUi[i] == Pixel)) {
                ++index;
                continue;
            }
            if (i == index) {
                type = typesInUi[i];
                break;
            }
        }
    }

    return KoUnit(type, factor);
}

int KoUnit::indexInListForUi(ListOptions listOptions) const
{
    if ((listOptions&HidePixel) && (m_type == Pixel)) {
        return -1;
    }

    int result = -1;

    int skipped = 0;
    for (int i = 0; i < KoUnit::TypeCount; ++i) {
        if ((listOptions&HidePixel) && (typesInUi[i] == Pixel)) {
            ++skipped;
            continue;
        }
        if (typesInUi[i] == m_type) {
            result = i - skipped;
            break;
        }
    }

    return result;
}

qreal KoUnit::toUserValue(qreal ptValue) const
{
    switch (m_type) {
    case Millimeter:
        return toMillimeter(ptValue);
    case Centimeter:
        return toCentimeter(ptValue);
    case Decimeter:
        return toDecimeter(ptValue);
    case Inch:
        return toInch(ptValue);
    case Pica:
        return toPica(ptValue);
    case Cicero:
        return toCicero(ptValue);
    case Pixel:
        return ptValue * m_pixelConversion;
    case Point:
    default:
        return toPoint(ptValue);
    }
}

qreal KoUnit::ptToUnit(const qreal ptValue, const KoUnit &unit)
{
    switch (unit.m_type) {
    case Millimeter:
        return POINT_TO_MM(ptValue);
    case Centimeter:
        return POINT_TO_CM(ptValue);
    case Decimeter:
        return POINT_TO_DM(ptValue);
    case Inch:
        return POINT_TO_INCH(ptValue);
    case Pica:
        return POINT_TO_PI(ptValue);
    case Cicero:
        return POINT_TO_CC(ptValue);
    case Pixel:
        return ptValue * unit.m_pixelConversion;
    case Point:
    default:
        return ptValue;
    }
}

QString KoUnit::toUserStringValue(qreal ptValue) const
{
    return QLocale().toString(toUserValue(ptValue));
}

qreal KoUnit::fromUserValue(qreal value) const
{
    switch (m_type) {
    case Millimeter:
        return MM_TO_POINT(value);
    case Centimeter:
        return CM_TO_POINT(value);
    case Decimeter:
        return DM_TO_POINT(value);
    case Inch:
        return INCH_TO_POINT(value);
    case Pica:
        return PI_TO_POINT(value);
    case Cicero:
        return CC_TO_POINT(value);
    case Pixel:
        return value / m_pixelConversion;
    case Point:
    default:
        return value;
    }
}

qreal KoUnit::fromUserValue(const QString &value, bool *ok) const
{
    return fromUserValue(QLocale().toDouble(value, ok));
}

qreal KoUnit::parseValue(const QString& _value, qreal defaultVal)
{
    if (_value.isEmpty())
        return defaultVal;

    QString value(_value.simplified());
    value.remove(QLatin1Char(' '));

    int firstLetter = -1;
    for (int i = 0; i < value.length(); ++i) {
        if (value.at(i).isLetter()) {
            if (value.at(i) == QLatin1Char('e'))
                continue;
            firstLetter = i;
            break;
        }
    }

    if (firstLetter == -1)
        return value.toDouble();

    const QString symbol = value.mid(firstLetter);
    value.truncate(firstLetter);
    const qreal val = value.toDouble();

    if (symbol == QStringLiteral("pt"))
        return val;

    bool ok;
    KoUnit u = KoUnit::fromSymbol(symbol, &ok);
    if (ok)
        return u.fromUserValue(val);

    if (symbol == QStringLiteral("m"))
        return DM_TO_POINT(val * 10.0);
    else if (symbol == QStringLiteral("km"))
        return DM_TO_POINT(val * 10000.0);
    warnOdf << "KoUnit::parseValue: Unit " << symbol << " is not supported, please report.";

    // TODO : add support for mi/ft ?
    return defaultVal;
}

KoUnit KoUnit::fromSymbol(const QString &symbol, bool *ok)
{
    Type result = Point;

    if (symbol == QStringLiteral("inch") /*compat*/) {
        result = Inch;
        if (ok)
            *ok = true;
    } else {
        if (ok)
            *ok = false;

        for (int i = 0; i < TypeCount; ++i) {
            if (symbol == QLatin1String(unitNameList[i])) {
                result = static_cast<Type>(i);
                if (ok)
                    *ok = true;
            }
        }
    }

    return KoUnit(result);
}

qreal KoUnit::convertFromUnitToUnit(const qreal value, const KoUnit &fromUnit, const KoUnit &toUnit, qreal factor)
{
    qreal pt;
    switch (fromUnit.type()) {
    case Millimeter:
        pt = MM_TO_POINT(value);
        break;
    case Centimeter:
        pt = CM_TO_POINT(value);
        break;
    case Decimeter:
        pt = DM_TO_POINT(value);
        break;
    case Inch:
        pt = INCH_TO_POINT(value);
        break;
    case Pica:
        pt = PI_TO_POINT(value);
        break;
    case Cicero:
        pt = CC_TO_POINT(value);
        break;
    case Pixel:
        pt = value / factor;
        break;
    case Point:
    default:
        pt = value;
    }

    switch (toUnit.type()) {
    case Millimeter:
        return POINT_TO_MM(pt);
    case Centimeter:
        return POINT_TO_CM(pt);
    case Decimeter:
        return POINT_TO_DM(pt);
    case Inch:
        return POINT_TO_INCH(pt);
    case Pica:
        return POINT_TO_PI(pt);
    case Cicero:
        return POINT_TO_CC(pt);
    case Pixel:
        return pt * factor;
    case Point:
    default:
        return pt;
    }

}

QString KoUnit::symbol() const
{
    return QLatin1String(unitNameList[m_type]);
}

qreal KoUnit::parseAngle(const QString& _value, qreal defaultVal)
{
    if (_value.isEmpty())
        return defaultVal;

    QString value(_value.simplified());
    value.remove(QLatin1Char(' '));

    int firstLetter = -1;
    for (int i = 0; i < value.length(); ++i) {
        if (value.at(i).isLetter()) {
            if (value.at(i) == QLatin1Char('e'))
                continue;
            firstLetter = i;
            break;
        }
    }

    if (firstLetter == -1)
        return value.toDouble();

    const QString type = value.mid(firstLetter);
    value.truncate(firstLetter);
    const qreal val = value.toDouble();

    if (type == QStringLiteral("deg"))
        return val;
    else if (type == QStringLiteral("rad"))
        return val * 180 / M_PI;
    else if (type == QStringLiteral("grad"))
        return val * 0.9;

    return defaultVal;
}

qreal KoUnit::approxTransformScale(const QTransform &t)
{
    return std::sqrt(qAbs(t.determinant()));
}

void KoUnit::adjustByPixelTransform(const QTransform &t)
{
    m_pixelConversion *= approxTransformScale(t);
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug, const KoUnit &unit)
{
#ifndef NDEBUG
    debug.nospace() << unit.symbol();
#else
    Q_UNUSED(unit);
#endif
    return debug.space();

}
#endif
