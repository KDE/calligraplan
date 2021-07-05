/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2017 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef PROJECTSETTINGSVIEW_H
#define PROJECTSETTINGSVIEW_H

#include "planui_export.h"
#include "ui_ProjectSettingsView.h"
#include "kptviewbase.h"


class KoDocument;

class QUrl;
class QPoint;


namespace KPlato
{


class PLANUI_EXPORT ProjectSettingsView : public ViewBase
{
    Q_OBJECT
public:
    ProjectSettingsView(KoPart *part, KoDocument *doc, QWidget *parent);

    bool openHtml(const QUrl &url);

    void setupGui();

    virtual void updateReadWrite(bool readwrite);

    KoPrintJob *createPrintJob();


public Q_SLOTS:
    /// Activate/deactivate the gui
    virtual void setGuiActive(bool activate);

Q_SIGNALS:
    void connectResources(const QString &file);

protected:
    void updateActionsEnabled(bool on = true);

private Q_SLOTS:
    void slotContextMenuRequested(const QModelIndex &index, const QPoint& pos);
    
    void slotEnableActions(bool on);

    void slotOpenResourcesFile();
    void slotResourcesConnect();

private:
    Ui::ProjectSettingsView widget;
    
};

}  //KPlato namespace

#endif
