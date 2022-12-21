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

    /// Set if use shared resources was false and has been set true
    bool updateSharedResources() const;

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

Q_SIGNALS:
    void obligatedFieldsFilled(bool);
    void changed();
    void loadResourceAssignments(QUrl url);

private:
    Project &project;
    DocumentsPanel *m_documents;
    TaskDescriptionPanel *m_description;
    bool m_updateSharedResources = false;
};


}  //KPlato namespace

#endif // MAINPROJECTPANEL_H
