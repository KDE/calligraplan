/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
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
