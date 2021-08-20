/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PLANPORTFOLIO_GANTTVIEW_H
#define PLANPORTFOLIO_GANTTVIEW_H

#include "planportfolio_export.h"

#include <kptviewbase.h>

class KoDocument;
class KoPrintJob;
class QTreeView;
class QMenu;

namespace KPlato {
    class GanttViewBase;
}

class PLANPORTFOLIO_EXPORT GanttView : public KPlato::ViewBase
{
    Q_OBJECT

public:
    explicit GanttView(KoPart *part, KoDocument *doc, QWidget *parent = nullptr);
    ~GanttView() override;

    QMenu *popupMenu(const QString& name);

    KoPrintJob *createPrintJob() override;

Q_SIGNALS:
    void openDocument(KoDocument *doc);

protected:
    void updateReadWrite(bool readwrite) override;

protected Q_SLOTS:
    void openProject();
    void slotCustomContextMenuRequested(const QPoint &pos);
    void slotOptions() override;

private:
    void setupGui();

private:
    bool m_readWrite;
    KPlato::GanttViewBase *m_view;
};

#endif
