/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
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

    KoPrintJob *createPrintJob() override;

    KPageWidgetItem *openDocument(KoDocument *doc);

    void guiActivateEvent(bool activate) override;

public Q_SLOTS:
    void slotOpenDocument(KoDocument *doc);

protected:
    void setupActions();

protected:
    void updateReadWrite(bool readwrite) override;

private Q_SLOTS:
    void slotCurrentPageChanged(KPageWidgetItem *current, KPageWidgetItem *before);

private:
    bool m_readWrite;
    KPageWidget *m_views;
    KPageWidgetItem *m_ganttSummary;
    QHash<QString, KPageWidgetItem*> m_ganttViews;
};

#endif
