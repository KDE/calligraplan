/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 2006 Stefan Nikolaus <stefan.nikolaus@kdemail.net>
    SPDX-FileCopyrightText: 2004 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPT_VIEW_ADAPTOR_H
#define KPT_VIEW_ADAPTOR_H

#include <KoViewAdaptor.h>

namespace KPlato
{

class View;

class ViewAdaptor : public KoViewAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.calligra.projectmanaging.view")

public:
    explicit ViewAdaptor(View*);
    virtual ~ViewAdaptor();

public Q_SLOTS:
//     void slotEditResource();
//     void slotEditCut();
//     void slotEditCopy();
//     void slotEditPaste();
//     void slotAddTask();
//     void slotAddSubTask();
//     void slotAddMilestone();
//     void slotProjectEdit();
//     void slotConfigure();

private:
    View* m_view;
};

}  //KPlato namespace

#endif
