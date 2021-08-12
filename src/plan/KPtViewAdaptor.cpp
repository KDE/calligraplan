/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 2006 Stefan Nikolaus <stefan.nikolaus@kdemail.net>
    SPDX-FileCopyrightText: 2004 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "KPtViewAdaptor.h"
#include "kptview.h"


namespace KPlato
{

/************************************************
 *
 * ViewAdaptor
 *
 ************************************************/

ViewAdaptor::ViewAdaptor(View* t)
    : KoViewAdaptor(t)
{
//     setAutoRelaySignals(true);
    m_view = t;
}

ViewAdaptor::~ViewAdaptor()
{
}

// void ViewAdaptor::slotEditResource()
// {
//   m_view->slotEditResource();
// }
// 
// void ViewAdaptor::slotEditCut()
// {
//   m_view->slotEditCut();
// }
// 
// void ViewAdaptor::slotEditCopy()
// {
//   m_view->slotEditCopy();
// }
// 
// void ViewAdaptor::slotEditPaste()
// {
//   m_view->slotEditPaste();
// }
// 
// void ViewAdaptor::slotAddTask()
// {
//   m_view->slotAddTask();
// }
// 
// void ViewAdaptor::slotAddSubTask()
// {
//   m_view->slotAddSubTask();
// }
// 
// void ViewAdaptor::slotAddMilestone()
// {
//   m_view->slotAddMilestone();
// }
// 
// void ViewAdaptor::slotProjectEdit()
// {
//   m_view->slotProjectEdit();
// }
// 
// void ViewAdaptor::slotConfigure()
// {
//   m_view->slotConfigure();
// }

}  //KPlato namespace

