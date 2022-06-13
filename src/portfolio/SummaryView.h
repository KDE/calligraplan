/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PLANPORTFOLIO_SUMMARYVIEW_H
#define PLANPORTFOLIO_SUMMARYVIEW_H

#include "planportfolio_export.h"

#include <kptviewbase.h>

class KoDocument;
class KoPrintJob;
class QTreeView;
class QMenu;

class PLANPORTFOLIO_EXPORT SummaryView : public KPlato::ViewBase
{
    Q_OBJECT

public:
    explicit SummaryView(KoPart *part, KoDocument *doc, QWidget *parent = nullptr);
    ~SummaryView() override;

    QMenu *popupMenu(const QString& name);

    KoPrintJob *createPrintJob() override;

protected:
    void updateReadWrite(bool readwrite) override;

private:
    void setupGui();

private Q_SLOTS:
    void itemDoubleClicked(const QPersistentModelIndex &idx);
    void slotContextMenuRequested(const QPoint &pos);
    void slotDescription();

private:
    bool m_readWrite;
    QTreeView *m_view;
};

#endif
