/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2007 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTWORKPACKAGESENDDIALOG_H
#define KPTWORKPACKAGESENDDIALOG_H

#include "planui_export.h"

#include <KoDialog.h>


namespace KPlato
{

class WorkPackageSendPanel;
class Node;
class ScheduleManager;

class PLANUI_EXPORT WorkPackageSendDialog : public KoDialog
{
    Q_OBJECT
public:
    explicit WorkPackageSendDialog(const QList<Node*> &tasks, ScheduleManager *sm, QWidget *parent=nullptr);

    WorkPackageSendPanel *panel() const { return m_wp; }

    QSize sizeHint() const override;

private:
    WorkPackageSendPanel *m_wp;
};

} //KPlato namespace

#endif // KPTWORKPACKAGESENDDIALOG_H
