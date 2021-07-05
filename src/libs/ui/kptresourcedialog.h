/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2003-2011 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTRESOURCEDIALOG_H
#define KPTRESOURCEDIALOG_H

#include "planui_export.h"

#include "ui_resourcedialogbase.h"
#include "kptresource.h"

#include <KoDialog.h>

#include <QMap>

namespace KPlato
{

class Project;
class Resource;
class Calendar;
class MacroCommand;

class ResourceDialogImpl : public QWidget, public Ui_ResourceDialogBase {
    Q_OBJECT
public:
    explicit ResourceDialogImpl(const Project &m_project, Resource &resource, bool baselined, QWidget *parent);

public Q_SLOTS:
    void slotChanged();
    void slotTypeChanged(int);
    void slotCalculationNeeded(const QString&);
    void slotChooseResource();
    
    void setCurrentIndexes(const QModelIndexList &lst);

Q_SIGNALS:
    void changed();
    void calculate();

protected Q_SLOTS:
    void slotAvailableFromChanged(const QDateTime& dt);
    void slotAvailableUntilChanged(const QDateTime& dt);
    
    void slotUseRequiredChanged(int state);
    void slotTeamChanged(const QModelIndex &index);

private:
    const Project &m_project;
    Resource &m_resource;
    QList<QPersistentModelIndex> m_currentIndexes;
};

class PLANUI_EXPORT ResourceDialog : public KoDialog {
    Q_OBJECT
public:
    ResourceDialog(Project &project, Resource *resource, QWidget *parent=nullptr, const char *name=nullptr);

    bool calculationNeeded() {  return m_calculationNeeded; }

    Calendar *calendar() { return m_calendars[dia->calendarList->currentIndex()]; }
    MacroCommand *buildCommand();
    
    static MacroCommand *buildCommand(Resource *original, Resource &resource);
    
protected Q_SLOTS:
    void enableButtonOk();
    void slotCalculationNeeded();
    void slotOk();
    void slotCalendarChanged(int);
    void slotButtonClicked(int button) override;
    void slotAccountChanged(const QString &name);

    void slotResourceRemoved(KPlato::Project *project, int row, KPlato::Resource *resource);

private:
    Project &m_project;
    Resource *m_original;
    Resource m_resource;
    ResourceDialogImpl *dia;
    bool m_calculationNeeded;
    
    QMap<int, Calendar*> m_calendars;
    QMap<QString, ResourceGroup*> m_groups;
};

} //KPlato namespace

#endif
