/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef WELCOMEVIEW_H
#define WELCOMEVIEW_H

#include "komain_export.h"

#include "ui_WelcomeView.h"

#include <KoFileDialog.h>

#include <QWidget>
#include <QPointer>

class KoDocument;
class KoMainWindow;
class KoPart;
class KoFileDialog;

class QUrl;
class QItemSelecteion;

class RecentProjectsModel;
class RecentPortfoliosModel;

class KOMAIN_EXPORT WelcomeView : public QWidget
{
    Q_OBJECT
public:
    WelcomeView(KoMainWindow *parent);
    ~WelcomeView() override;

    void setupGui();

    KoMainWindow *mainWindow() const;

    KoPart *part(const QString &appName, const QString &mimeType) const;

Q_SIGNALS:
    void newProject();
    void openProject();
    void recentProject(const QUrl &file, KoPart *part);
    void selectDefaultView();
    void loadSharedResources(const QUrl &url, const QUrl &projects);
    void openExistingFile(const QUrl &url);

    void projectCreated();
    void finished();

    void openTemplate(QUrl);

private Q_SLOTS:
    void slotContextMenuRequested(const QModelIndex &index, const QPoint& pos);
    void slotRecentFileSelected(const QModelIndex &idx);

    void slotEnableActions(bool on);

    void slotNewProject();
    void slotOpenProject();
    void slotLoadSharedResources(const QString &file, const QUrl &projects, bool loadProjectsAtStartup);

    void slotOpenProjectTemplate(const QModelIndex &idx);

    void slotCreateResourceFile();

    void slotNewPortfolio();
    void slotOpenPortfolio();
    void slotRecentPortfolioSelected(const QModelIndex &idx);

private:
    void setProjectTemplatesModel();

private:
    Ui::WelcomeView ui;
    RecentProjectsModel *m_recentProjects;
    RecentPortfoliosModel *m_recentPortfolios;
    QPointer<KoFileDialog> m_filedialog;
};

#endif
