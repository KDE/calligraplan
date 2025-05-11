/*
 * localemon.cpp
 *
 * SPDX-FileCopyrightText: 1999-2003 Hans Petter Bieker <bieker@kde.org>
 * SPDX-FileCopyrightText: 2009, 2012 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2016 Dag Andersen <dag.andersen@kdemail.net>
 *
 * Requires the Qt widget libraries, available at no cost at
 * https://qt.io/
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
//#include "toplevel.h"
#include "localemon.h"

#include "kptlocale.h"
#include "kptcommand.h"
#include "kptdebug.h"

#include <QCheckBox>
#include <QComboBox>
#include <QStandardPaths>

#include <KConfig>
#include <KConfigGroup>
#include <KSharedConfig>

namespace KPlato {

LocaleConfigMoney::LocaleConfigMoney(Locale *locale,
                                       QWidget *parent)
  : QWidget(parent),
    m_locale(locale)
{
  setupUi(this);

  // Money
  m_labMonCurSym->setObjectName(QStringLiteral("Currency symbol:"));
  m_labMonFraDig->setObjectName(QStringLiteral("Fract digits:"));

  connect(m_edMonCurSym,&QLineEdit::textChanged,this, &LocaleConfigMoney::slotMonCurSymChanged);

  connect(m_inMonFraDig,SIGNAL(valueChanged(int)),SLOT(slotMonFraDigChanged(int)));

  m_inMonFraDig->setRange(0, 10);
  m_inMonFraDig->setSingleStep(1);

  slotTranslate();
  slotLocaleChanged();
}

LocaleConfigMoney::~LocaleConfigMoney()
{
}

void LocaleConfigMoney::slotLocaleChanged()
{
  m_edMonCurSym->setText(m_locale->currencySymbolExplicit());
  m_inMonFraDig->setValue(m_locale->monetaryDecimalPlaces());
}

void LocaleConfigMoney::slotMonCurSymChanged(const QString &/*t*/)
{
  Q_EMIT localeChanged();
}

void LocaleConfigMoney::slotMonFraDigChanged(int /*value*/)
{
  Q_EMIT localeChanged();
}

void LocaleConfigMoney::slotMonPosPreCurSymChanged()
{
  Q_EMIT localeChanged();
}

void LocaleConfigMoney::slotMonNegPreCurSymChanged()
{
  Q_EMIT localeChanged();
}

void LocaleConfigMoney::slotMonPosMonSignPosChanged(int /*i*/)
{
  Q_EMIT localeChanged();
}

void LocaleConfigMoney::slotMonNegMonSignPosChanged(int /*i*/)
{
  Q_EMIT localeChanged();
}


void LocaleConfigMoney::slotTranslate()
{
  QString str;

  str = i18n("Here you can enter your usual currency "
               "symbol, e.g. $ or €.");
  m_labMonCurSym->setWhatsThis(str);
  m_edMonCurSym->setWhatsThis(str);
}

MacroCommand *LocaleConfigMoney::buildCommand()
{
    MacroCommand *m = new MacroCommand();
    if (m_locale->currencySymbolExplicit() != m_edMonCurSym->text()) {
        m->addCommand(new ModifyCurrencySymolCmd(m_locale, m_edMonCurSym->text()));
    }
    if (m_locale->monetaryDecimalPlaces() != m_inMonFraDig->value()) {
        m->addCommand(new ModifyCurrencyFractionalDigitsCmd(m_locale, m_inMonFraDig->value()));
    }
    debugPlan<<"buildCommand:"<<m->isEmpty();
    if (m->isEmpty()) {
        delete m;
        return nullptr;
    }
    return m;
}

} // namespace KPlato
