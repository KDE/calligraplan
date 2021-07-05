/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PLANPORTFOLIO_PORTFOLIOVIEW_H
#define PLANPORTFOLIO_PORTFOLIOVIEW_H

#include "planportfolio_export.h"

#include <KoView.h>

class KoDocument;
class KoPrintJob;
class QTreeView;
class QMenu;
class QItemSelection;

class PLANPORTFOLIO_EXPORT PortfolioView : public KoView
{
    Q_OBJECT

public:
    explicit PortfolioView(KoPart *part, KoDocument *doc, QWidget *parent = nullptr);
    ~PortfolioView() override;

    QMenu *popupMenu(const QString& name);

    QPrintDialog* createPrintDialog(KoPrintJob*, QWidget*) override;

    void loadProject(const QUrl &url);

protected:
    void updateReadWrite(bool readwrite) override;
    void updateActionsEnabled();

private:
    void setupGui();

private Q_SLOTS:
    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void slotAddProject();
    void slotRemoveSelected();

    void slotLoadCompleted();
    void slotLoadCanceled();

private:
    bool m_readWrite;
    QTreeView *m_view;
};

#endif
