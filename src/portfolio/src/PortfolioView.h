/* This file is part of the KDE project
 * Copyright (C) 2021 Dag Andersen <dag.andersen@kdemail.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
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
