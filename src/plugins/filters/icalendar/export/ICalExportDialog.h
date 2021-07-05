/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef ICALEXPORTDIALOG_H
#define ICALEXPORTDIALOG_H

#include <KoDialog.h>

#include <ui_ICalExportDialog.h>

namespace KPlato
{

class Project;

class ICalExportDialog : public KoDialog
{
    Q_OBJECT
public:
    ICalExportDialog(Project &project, QWidget *parent=nullptr);

    bool includeProject() const;
    bool includeSummarytasks() const;
    long scheduleId() const;

private:
    Project &m_project;
    Ui::ICalExportDialog m_ui;
    
};

} //KPlato namespace

#endif
