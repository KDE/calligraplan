/*
 * localemon.h
 *
 * SPDX-FileCopyrightText: 1999-2003 Hans Petter Bieker <bieker@kde.org>
 * SPDX-FileCopyrightText: 2007 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2016 Dag Andersen <dag.andersen@kdemail.net>
 *
 * Requires the Qt widget libraries, available at no cost at
 * https://qt.io/
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KPLATO_LOCALEMON_H
#define KPLATO_LOCALEMON_H

#include "planui_export.h"

#include "ui_localemon.h"

namespace KPlato
{

class MacroCommand;
class Locale;

class PLANUI_EXPORT LocaleConfigMoney : public QWidget, Ui::LocaleConfigMoney
{
  Q_OBJECT

public:
  LocaleConfigMoney(Locale *locale, QWidget *parent);
  ~LocaleConfigMoney() override;

  /// Save all changes
  MacroCommand *buildCommand();

public Q_SLOTS:
  /**
   * Loads all settings from the current locale into the current widget.
   */
  void slotLocaleChanged();
  /**
   * Retranslate all objects owned by this object using the current locale.
   */
  void slotTranslate();

Q_SIGNALS:
  void localeChanged();

private Q_SLOTS:
  // Money
  void slotMonCurSymChanged(const QString &t);
/*  void slotMonDecSymChanged(const QString &t);
  void slotMonThoSepChanged(const QString &t);*/
  void slotMonFraDigChanged(int value);
  void slotMonPosPreCurSymChanged();
  void slotMonNegPreCurSymChanged();
  void slotMonPosMonSignPosChanged(int i);
  void slotMonNegMonSignPosChanged(int i);
//  void slotMonDigSetChanged(int i);

private:
  Locale *m_locale;
};

} // namespace KPlato

#endif // LOCALEMON_H
