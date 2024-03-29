/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2003-2007 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTMAINPROJECTDIALOG_H
#define KPTMAINPROJECTDIALOG_H

#include "planui_export.h"

#include <KoDialog.h>


namespace KPlato
{

class Project;
class MainProjectPanel;
class MacroCommand;


class PLANUI_EXPORT MainProjectDialog : public KoDialog {
    Q_OBJECT
public:
    explicit MainProjectDialog(Project &project, QWidget *parent=nullptr, bool edit=true);

    MacroCommand *buildCommand();

    /// Set if use shared resources was false and has been set true
    bool updateSharedResources() const;

Q_SIGNALS:
    void dialogFinished(int);

protected Q_SLOTS:
    void slotRejected();
    void slotOk();

private:
    Project &project;
    MainProjectPanel *panel;
};

}  //KPlato namespace

#endif // MAINPROJECTDIALOG_H
