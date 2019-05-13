/* This file is part of the KDE project
   Copyright (C) 2004 - 2007, 2012 Dag Andersen <danders@get2net.dk>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

// clazy:excludeall=qstring-arg
#include "kpttaskprogresspanel.h"
#include "kptusedefforteditor.h"

#include <KoIcon.h>

#include <QDate>
#include <QDateTime>

#include <KLocalizedString>

#include "kpttask.h"
#include "kptcommand.h"
#include "kptcalendar.h"
#include "kptresource.h"
#include "kptdurationspinbox.h"
#include "kptschedule.h"
#include "kptproject.h"
#include "kptdebug.h"


using namespace KPlato;

int toEditMode(Completion::Entrymode m)
{
    const QList<Completion::Entrymode> modes = QList<Completion::Entrymode>() << Completion::EnterEffortPerResource << Completion::EnterEffortPerTask << Completion::EnterCompleted;
    return qBound(0, modes.indexOf(m), 1);
}
Completion::Entrymode fromEditMode(int m)
{
    const QList<Completion::Entrymode> modes = QList<Completion::Entrymode>() << Completion::EnterEffortPerResource << Completion::EnterEffortPerTask;
    return modes.value(m);
}

//-----------------
TaskProgressPanel::TaskProgressPanel( Task &task, ScheduleManager *sm, StandardWorktime *workTime, QWidget *parent )
    : TaskProgressPanelImpl( task, parent )
{
    Q_UNUSED(workTime);
    debugPlan;
    started->setChecked(m_completion.isStarted());
    finished->setChecked(m_completion.isFinished());
    startTime->setDateTime(m_completion.startTime());
    finishTime->setDateTime(m_completion.finishTime());
    finishTime->setMinimumDateTime( qMax( startTime->dateTime(), QDateTime(m_completion.entryDate(), QTime(), Qt::LocalTime) ) );

    scheduledEffort = task.estimate()->expectedValue();

    setYear( QDate::currentDate().year() );
    
    if ( m_completion.usedEffortMap().isEmpty() || m_task.requests().isEmpty() ) {
        foreach ( ResourceGroupRequest *g, m_task.requests().requests() ) {
            foreach ( ResourceRequest *r, g->resourceRequests() ) {
                m_completion.addUsedEffort( r->resource() );
            }
        }
    }
    enableWidgets();
    started->setFocus();

    connect( weekNumber, SIGNAL(currentIndexChanged(int)), SLOT(slotWeekNumberChanged(int)) );
    connect(ui_resourceCombo, SIGNAL(activated(QString)), resourceTable->model(), SLOT(addResource(QString)));
    connect( addEntryBtn, &QAbstractButton::clicked, entryTable, &CompletionEntryEditor::addEntry );
    connect( removeEntryBtn, &QAbstractButton::clicked, entryTable, &CompletionEntryEditor::removeEntry );

    entryTable->model()->setManager( sm );
    entryTable->model()->setTask( &task );
    entryTable->setCompletion( &m_completion );
    connect( entryTable, &CompletionEntryEditor::rowInserted, this, &TaskProgressPanel::slotEntryAdded );

    resourceTable->setProject( static_cast<Project*>( task.projectNode() ) );
    resourceTable->setCompletion( &m_completion );
    slotWeekNumberChanged( weekNumber->currentIndex() );
    updateResourceCombo();
    connect(resourceTable->model(), &UsedEffortItemModel::rowInserted, this, &TaskProgressPanelImpl::updateResourceCombo);

    //resourceTable->resizeColumnsToContents();

    connect(started, &QAbstractButton::toggled, this, &TaskProgressPanelImpl::slotStartedChanged);
    connect(started, &QAbstractButton::toggled, this, &TaskProgressPanelImpl::slotChanged);
    connect(finished, &QAbstractButton::toggled, this, &TaskProgressPanelImpl::slotFinishedChanged);
    connect(finished, &QAbstractButton::toggled, this, &TaskProgressPanelImpl::slotChanged);

    connect(startTime, &QDateTimeEdit::dateTimeChanged, this, &TaskProgressPanelImpl::slotChanged);
    connect(startTime, &QDateTimeEdit::dateTimeChanged, this, &TaskProgressPanelImpl::slotStartTimeChanged);
    connect(finishTime, &QDateTimeEdit::dateTimeChanged, this, &TaskProgressPanelImpl::slotChanged);
    connect(finishTime, &QDateTimeEdit::dateTimeChanged, this, &TaskProgressPanelImpl::slotFinishTimeChanged);
}

MacroCommand *TaskProgressPanel::buildCommand()
{
    Project *project = dynamic_cast<Project*>( m_task.projectNode() );
    if ( project == 0 ) {
        return 0;
    }
    return buildCommand( *project, m_original, m_completion );
}

MacroCommand *TaskProgressPanel::buildCommand( const Project &project, Completion &org, Completion &curr )
{
    MacroCommand *cmd = 0;
    KUndo2MagicString c = kundo2_i18n("Modify task completion");
    
    if ( org.entrymode() != curr.entrymode() ) {
        if ( cmd == 0 ) cmd = new MacroCommand( c );
        cmd->addCommand( new ModifyCompletionEntrymodeCmd(org, curr.entrymode() ) );
    }
    if ( org.startTime() != curr.startTime() ) {
        if ( cmd == 0 ) cmd = new MacroCommand( c );
        cmd->addCommand( new ModifyCompletionStartTimeCmd(org, curr.startTime() ) );
    }
    if ( org.finishTime() != curr.finishTime() ) {
        if ( cmd == 0 ) cmd = new MacroCommand( c );
        cmd->addCommand( new ModifyCompletionFinishTimeCmd(org, curr.finishTime() ) );
    }
    if ( org.isStarted() != curr.isStarted() ) {
        if ( cmd == 0 ) cmd = new MacroCommand( c );
        cmd->addCommand( new ModifyCompletionStartedCmd(org, curr.isStarted() ) );
    }
    if ( org.isFinished() != curr.isFinished() ) {
        if ( cmd == 0 ) cmd = new MacroCommand( c );
        cmd->addCommand( new ModifyCompletionFinishedCmd(org, curr.isFinished() ) );
    }
    QList<QDate> orgdates = org.entries().keys();
    QList<QDate> currdates = curr.entries().keys();
    foreach ( const QDate &d, orgdates ) {
        if ( currdates.contains( d ) ) {
            if ( curr.entry( d ) == org.entry( d ) ) {
                continue;
            }
            if ( cmd == 0 ) cmd = new MacroCommand( c );
            debugPlan<<"modify entry "<<d;
            Completion::Entry *e = new Completion::Entry( *( curr.entry( d ) ) );
            cmd->addCommand( new ModifyCompletionEntryCmd(org, d, e ) );
        } else {
            if ( cmd == 0 ) cmd = new MacroCommand( c );
            debugPlan<<"remove entry "<<d;
            cmd->addCommand( new RemoveCompletionEntryCmd(org, d ) );
        }
    }
    foreach ( const QDate &d, currdates ) {
        if ( ! orgdates.contains( d ) ) {
            if ( cmd == 0 ) cmd = new MacroCommand( c );
            Completion::Entry *e = new Completion::Entry( * ( curr.entry( d ) ) );
            debugPlan<<"add entry "<<d<<e;
            cmd->addCommand( new AddCompletionEntryCmd(org, d, e ) );
        }
    }
    const Completion::ResourceUsedEffortMap &map = curr.usedEffortMap();
    Completion::ResourceUsedEffortMap::const_iterator it;
    for (it = map.constBegin(); it != map.constEnd(); ++it) {
        Resource *r = project.findResource(it.key()->id());
        if ( r == 0 ) {
            warnPlan<<"Can't find resource:"<<it.key()->id()<<it.key()->name();
            continue;
        }
        Completion::UsedEffort *ue = map[ r ];
        if ( ue == 0 ) {
            continue;
        }
        if ( org.usedEffort( r ) == 0 || *ue != *(org.usedEffort( r )) ) {
            if ( cmd == 0 ) cmd = new MacroCommand( c );
            cmd->addCommand( new AddCompletionUsedEffortCmd( org, r, new Completion::UsedEffort( *ue ) ) );
        }
    }
    return cmd;
}

void TaskProgressPanel::slotWeekNumberChanged( int index )
{
    debugPlan<<index<<","<<m_weekOffset;
    QDate date = QDate( m_year, 1, 1 ).addDays( Qt::Monday - QDate( m_year, 1, 1 ).dayOfWeek() );
    date = date.addDays( index * 7 );
    resourceTable->setCurrentMonday( date );
}

void TaskProgressPanel::slotEntryAdded( const QDate &date )
{
    debugPlan<<date;
}

//-------------------------------------

TaskProgressPanelImpl::TaskProgressPanelImpl( Task &task, QWidget *parent )
    : QWidget(parent),
      m_task(task),
      m_original( task.completion() ),
      m_completion( m_original ),
      m_firstIsPrevYear( false ),
      m_lastIsNextYear( false )
{
    setupUi(this);

    connect(entryTable, &CompletionEntryEditor::selectedItemsChanged, this, &TaskProgressPanelImpl::slotSelectionChanged );
    removeEntryBtn->setEnabled( false );

    editmode->setCurrentIndex(toEditMode(m_original.entrymode()));
    connect( editmode, SIGNAL(currentIndexChanged(int)), SLOT(slotEditmodeChanged(int)) );
    connect( editmode, SIGNAL(activated(int)), SLOT(slotChanged()) );
    
    connect(resourceTable, &UsedEffortEditor::changed, this, &TaskProgressPanelImpl::slotChanged );
    connect(resourceTable, &UsedEffortEditor::resourceAdded, this, &TaskProgressPanelImpl::slotChanged );
    connect(resourceTable->model(), &UsedEffortItemModel::effortChanged, this, &TaskProgressPanelImpl::slotEffortChanged);

    connect(entryTable, &CompletionEntryEditor::changed, this, &TaskProgressPanelImpl::slotChanged );
    connect(entryTable, &CompletionEntryEditor::rowInserted, this, &TaskProgressPanelImpl::slotChanged );

    connect(entryTable, &CompletionEntryEditor::changed, this, &TaskProgressPanelImpl::slotEntryChanged );
    connect(entryTable, &CompletionEntryEditor::rowInserted, this, &TaskProgressPanelImpl::slotEntryChanged );
    connect(entryTable, &CompletionEntryEditor::rowRemoved, this, &TaskProgressPanelImpl::slotEntryChanged );

    connect( prevWeekBtn, &QAbstractButton::clicked, this, &TaskProgressPanelImpl::slotPrevWeekBtnClicked );
    connect( nextWeekBtn, &QAbstractButton::clicked, this, &TaskProgressPanelImpl::slotNextWeekBtnClicked );
    
    connect ( ui_year, SIGNAL(valueChanged(int)), SLOT(slotFillWeekNumbers(int)) );

    int y = 0;
    int wn = QDate::currentDate().weekNumber( &y );
    setYear( y );
    weekNumber->setCurrentIndex( wn - m_weekOffset );

}

void TaskProgressPanelImpl::slotEffortChanged(const QDate &date)
{
    if (date.isValid()) {
        entryTable->insertEntry(date);
    }
}

void TaskProgressPanelImpl::slotChanged() {
    emit changed();
}

void TaskProgressPanelImpl::slotEditmodeChanged( int idx )
{
    m_completion.setEntrymode(fromEditMode(idx));
    entryTable->model()->slotDataChanged();
    enableWidgets();
}

void TaskProgressPanelImpl::slotStartedChanged(bool state) {
    m_completion.setStarted( state );
    if (state) {
        QTime t = QTime::currentTime();
        t.setHMS( t.hour(), t.minute(), 0 );
        m_completion.setStartTime( QDateTime(QDate::currentDate(), t, Qt::LocalTime) );
        startTime->setDateTime( m_completion.startTime() );
        slotCalculateEffort();
    }
    enableWidgets();
}

void TaskProgressPanelImpl::setFinished() {
    QTime t = QTime::currentTime();
    t.setHMS( t.hour(), t.minute(), 0 );
    finishTime->setDateTime( QDateTime(QDate::currentDate(), t, Qt::LocalTime) );
    slotFinishTimeChanged( finishTime->dateTime() );
}

void TaskProgressPanelImpl::slotFinishedChanged(bool state) {
    debugPlan<<state;
    m_completion.setFinished( state );
    if (state) {
        debugPlan<<state;
        setFinished();
        debugPlan<<finishTime->dateTime();
        slotCalculateEffort();
    }   
    enableWidgets();
}

void TaskProgressPanelImpl::slotFinishTimeChanged( const QDateTime &dt )
{
    if ( ! m_completion.isFinished() ) {
        return;
    }
    m_completion.setFinishTime( dt );
    if ( m_completion.percentFinished() < 100 ) {
        m_completion.setPercentFinished( dt.date(), 100 );
    }
    entryTable->setCompletion( &m_completion ); // for refresh
}

void TaskProgressPanelImpl::slotStartTimeChanged( const QDateTime &dt )
{
    m_completion.setStartTime( dt );
    finishTime->setMinimumDateTime( qMax( startTime->dateTime(), QDateTime(m_completion.entryDate(), QTime(), Qt::LocalTime) ) );
    
}

void TaskProgressPanelImpl::slotEntryChanged()
{
    finishTime->setMinimumDateTime( qMax( startTime->dateTime(), QDateTime(m_completion.entryDate(), QTime(), Qt::LocalTime) ) );
}

void TaskProgressPanelImpl::updateResourceCombo()
{
    ui_resourceCombo->blockSignals(true);
    ui_resourceCombo->clear();
    ui_resourceCombo->addItems(resourceTable->model()->freeResources().keys());
    ui_resourceCombo->blockSignals(false);
}

void TaskProgressPanelImpl::enableWidgets() {
    editmode->setEnabled( !finished->isChecked() );

    started->setEnabled(!finished->isChecked());
    finished->setEnabled(started->isChecked());
    finishTime->setEnabled(finished->isChecked());
    startTime->setEnabled(started->isChecked() && !finished->isChecked());

    addEntryBtn->setEnabled( started->isChecked() && !finished->isChecked() );
    removeEntryBtn->setEnabled( ! entryTable->selectionModel()->selectedIndexes().isEmpty() && started->isChecked() && ! finished->isChecked() );

    ui_resourceFrame->setVisible(m_completion.entrymode() == Completion::EnterEffortPerResource);
    resourceTable->model()->setReadOnly( ( ! started->isChecked() ) || finished->isChecked() || editmode->currentIndex() != 0 );
}


void TaskProgressPanelImpl::slotPercentFinishedChanged( int ) {
    slotCalculateEffort();
}

void TaskProgressPanelImpl::slotCalculateEffort()
{
}

void TaskProgressPanelImpl::slotPrevWeekBtnClicked()
{
    debugPlan;
    int i = weekNumber->currentIndex();
    if ( i == 0 ) {
        debugPlan<<i;
        int decr = m_firstIsPrevYear ? 2 : 1;
        setYear( ui_year->value() - 1 );
        if ( m_lastIsNextYear ) {
            decr = 2;
        }
        weekNumber->setCurrentIndex( weekNumber->count() - decr );
    } else  {
        weekNumber->setCurrentIndex( i - 1 );
    }
}

void TaskProgressPanelImpl::slotNextWeekBtnClicked()
{
    int i = weekNumber->currentIndex();
    debugPlan<<i<<weekNumber->count();
    if ( i == weekNumber->count() - 1 ) {
        debugPlan<<i;
        int index = m_lastIsNextYear ? 1 : 0;
        setYear( ui_year->value() + 1 );
        if ( m_firstIsPrevYear ) {
            index = 1;
        }
        weekNumber->setCurrentIndex( index );
    } else {
        weekNumber->setCurrentIndex( i + 1 );
    }
}

void TaskProgressPanelImpl::setYear( int year )
{
    debugPlan;
    ui_year->setValue( year );
}

void TaskProgressPanelImpl::slotFillWeekNumbers( int year )
{
    debugPlan;
    weekNumber->clear();
    m_year = year;
    m_weekOffset = 1;
    int y = 0;
    QDate date( year, 1, 1 );
    int wn = date.weekNumber( &y );
    m_firstIsPrevYear = false;
    debugPlan<<date<<wn<<y<<year;
    if ( y < year ) {
        weekNumber->addItem( i18nc( "Week number (year)", "Week %1 (%2)", wn, y ) );
        m_weekOffset = 0;
        m_firstIsPrevYear = true;
        debugPlan<<"Added last week of prev year";
    }
    for ( int i=1; i <= 52; ++i ) {
        weekNumber->addItem( i18nc( "Week number", "Week %1", i ) );
    }
    date = QDate( year, 12, 31 );
    wn = date.weekNumber( &y );
    debugPlan<<date<<wn<<y<<year;
    m_lastIsNextYear = false;
    if ( wn == 53 ) {
        weekNumber->addItem( i18nc( "Week number", "Week %1", wn ) );
    } else if ( wn == 1 ) {
        weekNumber->addItem( i18nc( "Week number (year)", "Week %1 (%2)", wn, y ) );
        m_lastIsNextYear = true;
    }
}

void TaskProgressPanelImpl::slotSelectionChanged( const QItemSelection &sel )
{
    removeEntryBtn->setEnabled( ! sel.isEmpty() && started->isChecked() && ! finished->isChecked() );
}
