/* This file is part of the KDE project
  Copyright (C) 2017 Dag Andersen <danders@get2net.dk>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#ifndef WELCOMEVIEW_H
#define WELCOMEVIEW_H

#include "planui_export.h"
#include "kptviewbase.h"
#include "ui_WelcomeView.h"
#include "kptmainprojectdialog.h"

#include <KoFileDialog.h>

class KoDocument;

class QUrl;
class QItemSelecteion;


namespace KPlato
{

class RecentFilesModel;

class PLANUI_EXPORT WelcomeView : public ViewBase
{
    Q_OBJECT
public:
    WelcomeView(KoPart *part, KoDocument *doc, QWidget *parent);
    ~WelcomeView() override;

    void setRecentFiles(const QList<QAction*> &actions);

    void setupGui();

    void updateReadWrite(bool readwrite) override;

    KoPrintJob *createPrintJob() override;


public Q_SLOTS:
    /// Activate/deactivate the gui
    void setGuiActive(bool activate) override;

Q_SIGNALS:
    void newProject();
    void openProject();
    void recentProject(const QUrl &file, KoPart *part);
    void showIntroduction();
    void selectDefaultView();
    void loadSharedResources(const QUrl &url, const QUrl &projects);
    void openExistingFile(const QUrl &url);

    void projectCreated();
    void finished();

    void openTemplate(QUrl);

protected:
    void updateActionsEnabled( bool on = true);

private Q_SLOTS:
    void slotContextMenuRequested(const QModelIndex &index, const QPoint& pos);
    void slotRecentFileSelected(const QModelIndex &idx);
    
    void slotEnableActions(bool on);

    void slotNewProject();
    void slotOpenProject();
    void slotLoadSharedResources(const QString &file, const QUrl &projects, bool loadProjectsAtStartup);

    void slotProjectEditFinished(int result);
    void slotOpenFileFinished(int result);

    void slotCreateResourceFile();

private:
    Ui::WelcomeView widget;
    RecentFilesModel *m_model;
    QPointer<MainProjectDialog> m_projectdialog;
    QPointer<KoFileDialog> m_filedialog;
};

}  //KPlato namespace

#endif
