/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2017 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "ProjectSettingsView.h"

#include "kptdebug.h"

#include <KoDocument.h>

#include <QFileDialog>

namespace KPlato
{

//-----------------------------------
ProjectSettingsView::ProjectSettingsView(KoPart *part, KoDocument *doc, QWidget *parent)
    : ViewBase(part, doc, parent)
{
    widget.setupUi(this);
    setupGui();

    connect(widget.resourcesBrowseBtn, SIGNAL(clicked()), this, SLOT(slotOpenResourcesFile()));
    connect(widget.resourcesConnectBtn, SIGNAL(clicked()), this, SLOT(slotResourcesConnect()));
}

void ProjectSettingsView::updateReadWrite(bool /*readwrite */)
{
}

void ProjectSettingsView::setGuiActive(bool activate)
{
    debugPlan<<activate;
}

void ProjectSettingsView::slotContextMenuRequested(const QModelIndex &/*index*/, const QPoint& /*pos */)
{
    //debugPlan<<index.row()<<","<<index.column()<<":"<<pos;
}

void ProjectSettingsView::slotEnableActions(bool on)
{
    updateActionsEnabled(on);
}

void ProjectSettingsView::updateActionsEnabled(bool /*on */)
{
}

void ProjectSettingsView::setupGui()
{
    // Add the context menu actions for the view options
}

KoPrintJob *ProjectSettingsView::createPrintJob()
{
    return 0;//m_view->createPrintJob(this);
}

void ProjectSettingsView::slotOpenResourcesFile()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Resources"), "", tr("Resources files (*.plan)"));
    widget.resourcesFile->setText(fileName);
}

void ProjectSettingsView::slotResourcesConnect()
{
    QString fn = widget.resourcesFile->text();
    if (fn.startsWith('/')) {
        fn.prepend("file:/");
    }
    Q_EMIT connectResources(fn);
}

} // namespace KPlato
