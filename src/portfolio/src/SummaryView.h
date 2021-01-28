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

#ifndef PLANPORTFOLIO_SUMMARYVIEW_H
#define PLANPORTFOLIO_SUMMARYVIEW_H

#include "planportfolio_export.h"

#include <KoView.h>

class KoDocument;
class KoPrintJob;
class QTreeView;
class QMenu;

class PLANPORTFOLIO_EXPORT SummaryView : public KoView
{
    Q_OBJECT

public:
    explicit SummaryView(KoPart *part, KoDocument *doc, QWidget *parent = nullptr);
    ~SummaryView() override;

    QMenu *popupMenu(const QString& name);

    QPrintDialog* createPrintDialog(KoPrintJob*, QWidget*) override;

protected:
    void updateReadWrite(bool readwrite) override;

private:
    void setupGui();

private:
    bool m_readWrite;
    QTreeView *m_view;
};

#endif
