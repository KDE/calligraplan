/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 *  
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KPTLOADSHAREDPROJECTSDIALOG_H
#define KPTLOADSHAREDPROJECTSDIALOG_H

#include <KoDialog.h>

class QUrl;
class QTreeView;

namespace KPlato
{

class Project;

class LoadSharedProjectsDialog : public KoDialog
{
    Q_OBJECT
public:
    LoadSharedProjectsDialog(Project &project, const QUrl &own, QWidget *parent=nullptr);

    QList<QUrl> urls() const;

private:
    QTreeView *m_view;
};


} //KPlato namespace

#endif
