/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2015 Friedrich W. H. Kossebau <kossebau@kde.org>
  SPDX-FileCopyrightText: 2016 Dag Andersen <dag.andersen@kdemail.net>
  
  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTLOCALE_H
#define KPTLOCALE_H

#include "plankernel_export.h"

#include <QString>

#include <QLocale>

namespace KPlato
{

/**
 * Temporary wrapper for KLocale's currency methods.
 */
class PLANKERNEL_EXPORT Locale
{
public:
    Locale();
    ~Locale();

    /**
     * Sets the locale used to get the currency symbol.
     */
    void setCurrencyLocale(QLocale::Language language, QLocale::Territory territory);
    /// Return the language used when getting the currency symbol
    QLocale::Language currencyLanguage() const;
    /// Return the country used when getting the currency symbol
    QLocale::Territory currencyTerritory() const;


    /**
     * Changes the current currency symbol.
     *
     * This symbol should be consistent with the selected Currency Code
     *
     * @param symbol The new currency symbol
     * @see currencyCode, KCurrency::currencySymbols
     */
    void setCurrencySymbol(const QString &symbol);

    /**
     * Returns what the symbol denoting currency in the current locale
     * is as defined by user settings should look like.
     *
     * @return The default currency symbol used by locale.
     */
    QString currencySymbol() const;

     /**
     * @since 4.4
     *
     * Changes the number of decimal places used when formatting money.
     *
     * @param digits The default number of digits to use.
     */
    void setMonetaryDecimalPlaces(int digits);

    /**
     * @since 4.4
     *
     * The number of decimal places to include in monetary values (usually 2).
     *
     * @return Default number of monetary decimal places used by locale.
     */
    int monetaryDecimalPlaces() const;

    /**
     * Given a double, converts that to a numeric string containing
     * the localized monetary equivalent.
     *
     * e.g. given 123456, return "$ 123,456.00".
     *
     * If precision isn't specified or is < 0, then the default monetaryDecimalPlaces() is used.
     *
     * @param num The number we want to format
     * @param currency The currency symbol you want.
     * @param precision Number of decimal places displayed
     *
     * @return The number of money as a localized string
     * @see monetaryDecimalPlaces()
     */
    QString formatMoney(double num, const QString &currency = QString(), int precision = -1) const;

    /**
     * Converts a localized monetary string to a double.
     *
     * @param numStr the string we want to convert.
     * @param ok the boolean that is set to false if it's not a number.
     *           If @p ok is 0, it will be ignored
     *
     * @return The string converted to a double
     */
    double readMoney(const QString &numStr, bool *ok = nullptr) const;

    /**
     * Returns the explicitly set currency symbol.
     * Will be empty if not set by the user.
     */
    QString currencySymbolExplicit() const;

private:
    QString m_currencySymbol;
    int m_decimalPlaces;
    // to keep track of currency
    QLocale::Language m_language;
    QLocale::Territory m_territory;
};

}  //KPlato namespace

#endif
