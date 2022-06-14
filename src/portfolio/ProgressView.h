/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PLANPORTFOLIO_PROGRESSVIEW_H
#define PLANPORTFOLIO_PROGRESSVIEW_H

#include "planportfolio_export.h"

#include <KoView.h>
#include <KoPageLayout.h>

class KoDocument;
class KoPrintJob;
class QTreeView;
class QMenu;
class QItemSelection;

namespace KPlato {
    class TaskStatusView;
}

class PLANPORTFOLIO_EXPORT ProgressView : public KoView
{
    Q_OBJECT

public:
    explicit ProgressView(KoPart *part, KoDocument *doc, QWidget *parent = nullptr);
    ~ProgressView() override;

    QMenu *popupMenu(const QString& name);

    KoPrintJob* createPrintJob() override;

public Q_SLOTS:
    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

protected:
    void updateReadWrite(bool readwrite) override;

private:
    bool m_readWrite;
    QTreeView *m_view;
    KPlato::TaskStatusView *m_detailsView;
};

#endif
