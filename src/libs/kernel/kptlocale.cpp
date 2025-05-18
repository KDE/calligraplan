/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2015 Friedrich W. H. Kossebau <kossebau@kde.org>
  SPDX-FileCopyrightText: 2016 Dag Andersen <dag.andersen@kdemail.net>
  
  SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kptlocale.h"

#include "kptdebug.h"

#include <QLocale>


namespace KPlato
{

Locale::Locale()
{
    QLocale locale;
    m_language = locale.language();
    m_territory = locale.territory();
    m_decimalPlaces = 2;
}

Locale::~Locale()
{
}

void Locale::setCurrencyLocale(QLocale::Language language, QLocale::Territory territory)
{
    m_language = language;
    m_territory = territory;
}

void Locale::setCurrencySymbol(const QString &symbol)
{
    m_currencySymbol = symbol;
}

QString Locale::currencySymbol() const
{
    QString s = m_currencySymbol;
    if (s.isEmpty()) {
        QLocale locale(m_language, m_territory);
        s = locale.currencySymbol(QLocale::CurrencySymbol);
    }
    return s;
}

void Locale::setMonetaryDecimalPlaces(int digits)
{
    m_decimalPlaces = digits;
}

int Locale::monetaryDecimalPlaces() const
{
    return m_decimalPlaces;
}

QString Locale::formatMoney(double num, const QString &currency, int precision) const
{
    QString c = currency;
    if (c.isEmpty()) {
        c = currencySymbol();
    }
    int p = precision;
    if (p < 0) {
        p = m_decimalPlaces;
    }
    QLocale l;
    QString s = l.toCurrencyString(num, c, p);
    return s;
}

double Locale::readMoney(const QString &numStr, bool *ok) const
{
    QLocale l;
    QString s = numStr;
    bool okk = false;
    s.remove(currencySymbol());
    double v = l.toDouble(s, &okk);
    if (ok) {
        *ok = okk;
    }
    if (!okk) {
        errorPlan<<"Failed to read money:"<<numStr;
    }
    return v;
}

QString Locale::currencySymbolExplicit() const
{
    return m_currencySymbol;
}

QLocale::Language Locale::currencyLanguage() const
{
    return m_language;
}

QLocale::Country Locale::currencyTerritory() const
{
    return m_territory;
}

}  //KPlato namespace
