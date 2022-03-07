/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2007 Fredy Yanardi <fyanardi@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "kptviewlistdocker.h"

#include "kptviewlist.h"

#include "kptview.h"
#include "kptdebug.h"

#include <KLocalizedString>


namespace KPlato
{

ViewListDocker::ViewListDocker(View *view)
{
    updateWindowTitle(false);
    setView(view);
}

ViewListDocker::~ViewListDocker()
{
}

View *ViewListDocker::view()
{
    return m_view;
}

void ViewListDocker::setView(View *view)
{
    m_view = view;
    QWidget *wdg = widget();
    if (wdg)
        delete wdg;
    m_viewlist = new ViewListWidget(view->getPart(), this);
    setWhatsThis(m_viewlist->whatsThis());
    setWidget(m_viewlist);
    m_viewlist->setProject(&(view->getProject()));
    connect(m_viewlist, &ViewListWidget::selectionChanged, view, &View::slotSelectionChanged);
    connect(view, &View::currentScheduleManagerChanged, m_viewlist, &ViewListWidget::setSelectedSchedule);
    connect(m_viewlist, &ViewListWidget::updateViewInfo, view, &View::slotUpdateViewInfo);

}

void ViewListDocker::slotModified()
{
    setWindowTitle(xi18nc("@title:window", "View Selector [modified]"));
}

void ViewListDocker::updateWindowTitle(bool modified)
{
    if (modified) {
        setWindowTitle(xi18nc("@title:window", "View Selector [modified]"));
    } else {
        setWindowTitle(xi18nc("@title:window", "View Selector"));
    }
}

//----------
ViewListDockerFactory::ViewListDockerFactory(View *view)
{
    m_view = view;
}

QString ViewListDockerFactory::id() const
{
    return QStringLiteral("KPlatoViewList");
}

QDockWidget* ViewListDockerFactory::createDockWidget()
{
    ViewListDocker *widget = new ViewListDocker(m_view);
    widget->setObjectName(id());

    return widget;
}

} //namespace KPlato
