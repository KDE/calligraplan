/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PLANPORTFOLIO_PORTFOLIOVIEW_H
#define PLANPORTFOLIO_PORTFOLIOVIEW_H

#include "planportfolio_export.h"
#include "ui_PortfolioView.h"

#include <KoView.h>

class RecentFilesModel;
class KoDocument;
class KoPrintJob;
class KoXmlElement;

class QItemSelection;

namespace KIO {
    class UDSEntry;
}

class PLANPORTFOLIO_EXPORT PortfolioView : public KoView
{
    Q_OBJECT

public:
    explicit PortfolioView(KoPart *part, KoDocument *doc, QWidget *parent = nullptr);
    ~PortfolioView() override;

    QMenu *popupMenu(const QString& name);

    KoPrintJob *createPrintJob() override;

    void loadProject(const QUrl &url);

    bool hasWriteAccess(KIO::UDSEntry &entry) const;

    Q_INVOKABLE void saveSettings(QDomElement &settings) const;
    Q_INVOKABLE void loadSettings(KoXmlElement &settings);

Q_SIGNALS:
    void openUrl(const QUrl &url);

protected:
    void updateReadWrite(bool readwrite) override;
    void updateActionsEnabled();

private:
    void setupGui();

private Q_SLOTS:
    void slotRecentFileActivated(const QModelIndex &idx);
    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void slotAddProject();
    void slotRemoveSelected();

    void slotLoadCompleted();
    void slotLoadCanceled();

    void slotUpdateView();
private:
    bool m_readWrite;
    Ui::PortfolioView ui;
    RecentFilesModel *m_recentProjects;
};

#endif
