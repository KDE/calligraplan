/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2005 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTWBSDEFINITIONDIALOG_H
#define KPTWBSDEFINITIONDIALOG_H

#include "planui_export.h"

#include <KoDialog.h>

class KUndo2Command;

namespace KPlato
{

class WBSDefinitionPanel;
class WBSDefinition;
class Project;

class PLANUI_EXPORT WBSDefinitionDialog : public KoDialog {
    Q_OBJECT
public:
    explicit WBSDefinitionDialog(Project &project, WBSDefinition &def, QWidget *parent=nullptr);

    KUndo2Command *buildCommand();

protected Q_SLOTS:
    void slotOk();

private:
    WBSDefinitionPanel *m_panel;
};

} //KPlato namespace

#endif // WBSDEFINITIONDIALOG_H
