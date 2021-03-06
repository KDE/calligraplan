/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2007 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTCALENDAREDITOR_H
#define KPTCALENDAREDITOR_H

#include "planui_export.h"

#include "kptviewbase.h"
#include "kptitemmodelbase.h"
#include "kptcalendar.h" 
#include "kptcalendarmodel.h"

#include <QTableView>

class QPoint;
class KUndo2Command;

class KoDocument;

namespace KPlato
{

class View;
class Project;
class Calendar;
class CalendarDay;
class DateTableDataModel;
class KDatePicker;

class PLANUI_EXPORT CalendarTreeView : public TreeViewBase
{
    Q_OBJECT
public:
    explicit CalendarTreeView(QWidget *parent);

    CalendarItemModel *model() const { return static_cast<CalendarItemModel*>(TreeViewBase::model()); }

    Project *project() const { return model()->project(); }
    void setProject(Project *project) { model()->setProject(project); }

    Calendar *currentCalendar() const;
    Calendar *selectedCalendar() const;
    QList<Calendar*> selectedCalendars() const;

Q_SIGNALS:
    void currentIndexChanged(const QModelIndex&);
    void currentColumnChanged(const QModelIndex&, const QModelIndex&);
    void selectedIndexesChanged(const QModelIndexList&);

    void focusChanged();

protected Q_SLOTS:
    void slotHeaderContextMenuRequested(const QPoint &pos);
    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) override;
    void currentChanged (const QModelIndex & current, const QModelIndex & previous) override;

protected:
    void contextMenuEvent (QContextMenuEvent * event) override;
    void focusInEvent (QFocusEvent * event) override;
    void focusOutEvent (QFocusEvent * event) override;
    
    void dragMoveEvent(QDragMoveEvent *event) override;
};

class PLANUI_EXPORT CalendarDayView : public QTableView
{
    Q_OBJECT
public:
    explicit CalendarDayView(QWidget *parent);

    CalendarDayItemModel *model() const { return m_model; }

    Project *project() const { return model()->project(); }
    void setProject(Project *project) { model()->setProject(project); }

    CalendarDay *selectedDay() const;
    TimeInterval *selectedInterval() const;
    
    QSize sizeHint() const override;
    
    void setReadWrite(bool on) { m_readwrite = on; }
    bool isReadWrite() const { return m_readwrite; }

Q_SIGNALS:
    void currentIndexChanged(const QModelIndex&);
    void currentColumnChanged(const QModelIndex&, const QModelIndex&);
    void selectedIndexesChanged(const QModelIndexList&);

    void contextMenuRequested(const QModelIndex&, const QPoint&);
    void focusChanged();
    
    void executeCommand(KUndo2Command *cmd);
    
public Q_SLOTS:
    void setCurrentCalendar(KPlato::Calendar *calendar);
    
protected Q_SLOTS:
    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) override;
    void currentChanged (const QModelIndex & current, const QModelIndex & previous) override;

    void slotSetWork();
    void slotSetVacation();
    void slotSetUndefined();

    void slotIntervalEditDialogFinished(int result);

protected:
    void contextMenuEvent (QContextMenuEvent * event) override;
    void focusInEvent (QFocusEvent * event) override;
    void focusOutEvent (QFocusEvent * event) override;
    
private:
    CalendarDayItemModel *m_model;
    
    QAction *actionSetUndefined;
    QAction *actionSetVacation;
    QAction *actionSetWork;

    bool m_readwrite;
};

class PLANUI_EXPORT CalendarEditor : public ViewBase
{
    Q_OBJECT
public:
    CalendarEditor(KoPart *part, KoDocument *doc, QWidget *parent);
    
    void setupGui();
    Project *project() const override { return m_calendarview->project(); }
    void draw(Project &project) override;
    void draw() override;

    void updateReadWrite(bool readwrite) override;

    Calendar *currentCalendar() const override;

    /// Loads context info into this view.
    bool loadContext(const KoXmlElement &/*context*/) override;
    /// Save context info from this view.
    void saveContext(QDomElement &/*context*/) const override;

Q_SIGNALS:
    void addCalendar(KPlato::Calendar *calendar);
    void deleteCalendar(const QList<KPlato::Calendar*>&);
    
public Q_SLOTS:
    /// Activate/deactivate the gui
    void setGuiActive(bool activate) override;

protected:
    void updateActionsEnabled(bool on);
    void insertCalendar(Calendar *calendar, Calendar *parent, int pos = -1);
    
protected Q_SLOTS:
    void slotIntervalEditDialogFinished(int result);
    void slotOptions() override;

private Q_SLOTS:
    void slotContextMenuCalendar(const QModelIndex& index, const QPoint& pos);
    void slotContextMenuDay(const QModelIndex& index, const QPoint& pos);
    void slotContextMenuDate(QMenu*, const QList<QDate>&);
    
    void slotCalendarSelectionChanged(const QModelIndexList&);
    void slotCurrentCalendarChanged(const QModelIndex&);
    
    void slotDaySelectionChanged(const QModelIndexList&);
    void slotCurrentDayChanged(const QModelIndex&);
    
    void slotEnableActions();

    void slotAddCalendar();
    void slotAddSubCalendar();
    void slotDeleteCalendar();

    void slotAddDay();
    void slotAddInterval();
    void slotDeleteDaySelection();

    void slotSetWork();
    void slotSetVacation();
    void slotSetUndefined();

private:
    CalendarTreeView *m_calendarview;
    CalendarDayView *m_dayview;
    KDatePicker *m_datePicker;
    DateTableDataModel *m_model;
    
    QAction *actionAddCalendar;
    QAction *actionAddSubCalendar;
    QAction *actionDeleteSelection;

    QAction *actionAddDay;
    QAction *actionAddWorkInterval;
    QAction *actionDeleteDaySelection;

    QAction *actionSetUndefined;
    QAction *actionSetVacation;
    QAction *actionSetWork;
    
    QList<QDate> m_currentMenuDateList;

};


}  //KPlato namespace

#endif
