/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2009 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTLOCALECONFIGMONEYDIALOG_H
#define KPTLOCALECONFIGMONEYDIALOG_H

#include "planui_export.h"

#include <KoDialog.h>

class KUndo2Command;

namespace KPlato
{

class Locale;
class LocaleConfigMoney;
class Project;

class PLANUI_EXPORT LocaleConfigMoneyDialog : public KoDialog {
    Q_OBJECT
public:
    explicit LocaleConfigMoneyDialog(Locale *locale, QWidget *parent=nullptr);

    KUndo2Command *buildCommand(Project &project);

protected Q_SLOTS:
    void slotChanged();

private:
    LocaleConfigMoney *m_panel;

};

} //KPlato namespace

#endif // KPLATO_LOCALECONFIGMONEYDIALOG_H
