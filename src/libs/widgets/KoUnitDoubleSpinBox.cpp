/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2002 Rob Buis (buis@kde.org)
   SPDX-FileCopyrightText: 2004 Nicolas GOUTTE <goutte@kde.org>
   SPDX-FileCopyrightText: 2007 Thomas Zander <zander@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "KoUnitDoubleSpinBox.h"

#include <KoUnit.h>

#include <WidgetsDebug.h>

#include <KLocalizedString>

// #define DEBUG_VALIDATOR
// #define DEBUG_VALUEFROMTEXT

class Q_DECL_HIDDEN KoUnitDoubleSpinBox::Private
{
public:
    Private(double low, double up, double step)
        : lowerInPoints(low),
        upperInPoints(up),
        stepInPoints(step),
        unit(KoUnit(KoUnit::Point))
    {
    }

    double lowerInPoints; ///< lowest value in points
    double upperInPoints; ///< highest value in points
    double stepInPoints;  ///< step in points
    KoUnit unit;
};

KoUnitDoubleSpinBox::KoUnitDoubleSpinBox(QWidget *parent)
    : QDoubleSpinBox(parent),
    d(new Private(-9999, 9999, 1))
{
    QDoubleSpinBox::setDecimals(2);
    //setAcceptLocalizedNumbers(true);
    KoUnitDoubleSpinBox::setUnit(KoUnit(KoUnit::Point));
    setAlignment(Qt::AlignRight);

    connect(this, SIGNAL(valueChanged(double)), SLOT(privateValueChanged()));
}

KoUnitDoubleSpinBox::~KoUnitDoubleSpinBox()
{
    delete d;
}

QValidator::State KoUnitDoubleSpinBox::validate(QString &input, int &pos) const
{
#ifdef DEBUG_VALIDATOR
    debugWidgets <<"KoUnitDoubleSpinBox::validate :" << input <<" at" << pos;
#else
    Q_UNUSED(pos);
#endif

    QRegularExpression regexp(QStringLiteral("([ a-zA-Z]+)$")); // Letters or spaces at end
    auto match = regexp.match(input);
    const int res = match.hasMatch() ? match.capturedStart(1) : -1;

    if (res == -1)
    {
        // Nothing like an unit? The user is probably editing the unit
#ifdef DEBUG_VALIDATOR
        debugWidgets <<"Intermediate (no unit)";
#endif
        return QValidator::Intermediate;
    }

    // ### TODO: are all the QString::trimmed really necessary?
    const QString number (input.left(res).trimmed());
    const QString unitName (match.captured(1).trimmed().toLower());

#ifdef DEBUG_VALIDATOR
    debugWidgets <<"Split:" << number <<":" << unitName <<":";
#endif

    const double value = valueFromText(number);
    if (!qIsNaN(value))
    {
        bool ok;
        KoUnit::fromSymbol(unitName, &ok);
        if (!ok)
        {
            // Probably the user is trying to edit the unit
#ifdef DEBUG_VALIDATOR
            debugWidgets <<"Intermediate (unknown unit)";
#endif
            return QValidator::Intermediate;
        }
    }
    else
    {
        warnWidgets << "Not a number: " << number;
        return QValidator::Invalid;
    }
    //newVal = KoUnit::ptToUnit(newVal, d->unit);
    //input = textFromValue(newVal); // FIXME: don't overwrite for now; the effect is not exactly what I expect...

    return QValidator::Acceptable;
}

void KoUnitDoubleSpinBox::changeValue(double val)
{
    QDoubleSpinBox::setValue(d->unit.toUserValue(val));
    // TODO: Q_EMIT valueChanged ONLY if the value was out-of-bounds
    // This will allow the 'user' dialog to set a dirty bool and ensure
    // a proper value is getting saved.
}

void KoUnitDoubleSpinBox::privateValueChanged() {
    Q_EMIT valueChangedPt(value ());
}

void KoUnitDoubleSpinBox::setUnit(const KoUnit &unit)
{
    if (unit == d->unit) return;

    double oldvalue = d->unit.fromUserValue(QDoubleSpinBox::value());
    QDoubleSpinBox::setMinimum(unit.toUserValue(d->lowerInPoints));
    QDoubleSpinBox::setMaximum(unit.toUserValue(d->upperInPoints));

    qreal step = unit.toUserValue(d->stepInPoints);

    if (unit.type() == KoUnit::Pixel) {
        // limit the pixel step by 1.0
        step = qMax(qreal(1.0), step);
    }

    QDoubleSpinBox::setSingleStep(step);
    d->unit = unit;
    QDoubleSpinBox::setValue(KoUnit::ptToUnit(oldvalue, unit));
    setSuffix(unit.symbol().prepend(QLatin1Char(' ')));
}

double KoUnitDoubleSpinBox::value() const
{
    return d->unit.fromUserValue(QDoubleSpinBox::value());
}

void KoUnitDoubleSpinBox::setMinimum(double min)
{
  d->lowerInPoints = min;
  QDoubleSpinBox::setMinimum(d->unit.toUserValue(min));
}

void KoUnitDoubleSpinBox::setMaximum(double max)
{
  d->upperInPoints = max;
  QDoubleSpinBox::setMaximum(d->unit.toUserValue(max));
}

void KoUnitDoubleSpinBox::setLineStep(double step)
{
  d->stepInPoints = KoUnit(KoUnit::Point).toUserValue(step);
  QDoubleSpinBox::setSingleStep(step);
}

void KoUnitDoubleSpinBox::setLineStepPt(double step)
{
  d->stepInPoints = step;
  QDoubleSpinBox::setSingleStep(d->unit.toUserValue(step));
}

void KoUnitDoubleSpinBox::setMinMaxStep(double min, double max, double step)
{
  setMinimum(min);
  setMaximum(max);
  setLineStepPt(step);
}

QString KoUnitDoubleSpinBox::textFromValue(double value) const
{
    //debugWidgets <<"textFromValue:" << QString::number(value, 'f', 12) <<" =>" << num;
    //const QString num(QString("%1%2").arg(QLocale().toString(value, d->precision), m_unit.symbol()));
    //const QString num (QString("%1").arg(QLocale().toString(value, d->precision)));
    return QLocale().toString(value, 'f', decimals());
}

double KoUnitDoubleSpinBox::valueFromText(const QString& str) const
{
    QString str2(str);
    str2.remove(d->unit.symbol());
    return QLocale().toDouble(str2);
//    QString str2(str);
//    /* KLocale::readNumber wants the thousand separator exactly at 1000.
//       But when editing, it might be anywhere. So we need to remove it. */
//    const QString sep(KGlobal::locale()->thousandsSeparator());
//    if (!sep.isEmpty())
//        str2.remove(sep);
//    str2.remove(d->unit.symbol());
//    bool ok;
//    const double dbl = KGlobal::locale()->readNumber(str2, &ok);
//#ifdef DEBUG_VALUEFROMTEXT
//    if (ok)
//      debugWidgets <<"valueFromText:" << str <<": => :" << str2 <<": =>" << QString::number(dbl, 'f', 12);
//    else
//        warnWidgets << "valueFromText error:" << str << ": => :" << str2 << ":";
//#endif
//    return dbl;
}
