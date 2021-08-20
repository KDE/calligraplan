/* This file is part of the KDE project
 *  SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
 * 
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef BASICGANTTVIEWSETTINGSDIALOG_H
#define BASICGANTTVIEWSETTINGSDIALOG_H

#include "planui_export.h"

#include "kptitemviewsettup.h"

class KoPageLayoutWidget;

namespace KPlato
{

class ViewBase;
class GanttViewBase;
class GanttPrintingOptionsWidget;

class PLANUI_EXPORT BasicGanttViewSettingsDialog : public ItemViewSettupDialog
{
    Q_OBJECT
public:
    explicit BasicGanttViewSettingsDialog(GanttViewBase *gantt, ViewBase *view, bool selectPrint = false);

protected Q_SLOTS:
    void slotOk();

private:
    GanttViewBase *m_gantt;
    GanttPrintingOptionsWidget *m_printingoptions;
};

}  //KPlato namespace

#endif
