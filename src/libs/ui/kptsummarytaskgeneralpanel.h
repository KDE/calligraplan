/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2004-2007 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTSUMMARYTASKGENERALPANEL_H
#define KPTSUMMARYTASKGENERALPANEL_H

#include "planui_export.h"

#include "ui_kptsummarytaskgeneralpanelbase.h"

namespace KPlato
{

class SummaryTaskGeneralPanel;
class TaskDescriptionPanel;
class Task;
class MacroCommand;

class SummaryTaskGeneralPanel : public QWidget, public Ui_SummaryTaskGeneralPanelBase {
    Q_OBJECT
public:
    explicit SummaryTaskGeneralPanel(Task &task, QWidget *parent=nullptr);

    MacroCommand *buildCommand();

    bool ok();

    void setStartValues(Task &task);

Q_SIGNALS:
    void obligatedFieldsFilled(bool);

public Q_SLOTS:
    void slotObligatedFieldsFilled();
    void slotChooseResponsible();
    
private:
    Task &m_task;
    TaskDescriptionPanel *m_description;
};

} //KPlato namespace

#endif // SUMMARYTASKGENERALPANEL_H
