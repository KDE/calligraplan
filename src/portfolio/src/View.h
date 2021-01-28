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

#ifndef PLANPORTFOLIO_VIEW_H
#define PLANPORTFOLIO_VIEW_H

#include "planportfolio_export.h"

#include <KoView.h>
#include <KoPageLayout.h>

class KoDocument;
class KoPrintJob;
class KPageWidget;
class KPageWidgetItem;

class QMenu;

class PLANPORTFOLIO_EXPORT View : public KoView
{
    Q_OBJECT

public:
    explicit View(KoPart *part, KoDocument *doc, QWidget *parent = nullptr);
    ~View() override;

    QMenu *popupMenu(const QString& name);

    KoPageLayout pageLayout() const override;
    void setPageLayout(const KoPageLayout &pageLayout) override;

    QPrintDialog* createPrintDialog(KoPrintJob*, QWidget*) override;

public Q_SLOTS:
    void slotOpenDocument(KoDocument *doc);

protected:
    void guiActivateEvent(bool activate) override;
    void setupActions();

protected:
    void updateReadWrite(bool readwrite) override;

private Q_SLOTS:
    void slotCurrentPageChanged(KPageWidgetItem *current, KPageWidgetItem *before);

private:
    bool m_readWrite;
    KPageWidget *m_views;
};

#endif
