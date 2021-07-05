/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2004-2007 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTMAINPROJECTPANEL_H
#define KPTMAINPROJECTPANEL_H

#include "planui_export.h"

#include "ui_kptmainprojectpanelbase.h"

#include <QWidget>

class QDateTime;

namespace KPlato
{

class Project;
class MacroCommand;
class TaskDescriptionPanel;
class DocumentsPanel;

class MainProjectPanel : public QWidget, public Ui_MainProjectPanelBase {
    Q_OBJECT
public:
    explicit MainProjectPanel(Project &project, QWidget *parent=nullptr);

    virtual QDateTime startDateTime();
    virtual QDateTime endDateTime();

    MacroCommand *buildCommand();

    bool ok();

    bool loadSharedResources() const;

    void initTaskModules();
    MacroCommand *buildTaskModulesCommand();

public Q_SLOTS:
    virtual void slotCheckAllFieldsFilled();
    virtual void slotChooseLeader();
    virtual void slotStartDateClicked();
    virtual void slotEndDateClicked();
    virtual void enableDateTime();

    void insertTaskModuleClicked();
    void removeTaskModuleClicked();
    void taskModulesSelectionChanged();

private Q_SLOTS:
    void openResourcesFile();
    void openProjectsPlace();
    void loadProjects();
    void clearProjects();

Q_SIGNALS:
    void obligatedFieldsFilled(bool);
    void changed();
    void loadResourceAssignments(QUrl url);
    void clearResourceAssignments();

private:
    Project &project;
    DocumentsPanel *m_documents;
    TaskDescriptionPanel *m_description;
};


}  //KPlato namespace

#endif // MAINPROJECTPANEL_H
