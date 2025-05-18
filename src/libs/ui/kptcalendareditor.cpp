/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2007, 2012 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2017 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "kptcalendareditor.h"

#include "kcalendar/kdatepicker.h"
#include "kcalendar/kdatetable.h"
//#include "kptcalendarpanel.h"
#include "kptcommand.h"
#include "kptcalendarmodel.h"
#include "kptcalendar.h"
#include "kptduration.h"
#include "kptnode.h"
#include "kptproject.h"
#include "kpttask.h"
#include "kptdatetime.h"
#include "kptintervaledit.h"
#include "kptitemviewsettup.h"
#include "kptdebug.h"

#include <KoIcon.h>

#include <QDragMoveEvent>
#include <QList>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QAction>
#include <QMenu>

#include <KLocalizedString>
#include <KActionCollection>

#include <KoDocument.h>


namespace KPlato
{


//--------------------
CalendarTreeView::CalendarTreeView(QWidget *parent)
    : TreeViewBase(parent)
{
    header()->setContextMenuPolicy(Qt::CustomContextMenu);
    setModel(new CalendarItemModel(this));

    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSelectionModel(new QItemSelectionModel(model(), this));

    setItemDelegateForColumn(CalendarItemModel::Origin, new EnumDelegate(this));
    setItemDelegateForColumn(CalendarItemModel::TimeZone, new EnumDelegate(this)); // timezone
#ifdef HAVE_KHOLIDAYS
    setItemDelegateForColumn(CalendarItemModel::HolidayRegion, new EnumDelegate(this));
#endif
    connect(header(), SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(slotHeaderContextMenuRequested(QPoint)));
}

void CalendarTreeView::slotHeaderContextMenuRequested(const QPoint &pos)
{
    Q_EMIT contextMenuRequested(QModelIndex(), mapToGlobal(pos), QModelIndexList());
}

void CalendarTreeView::contextMenuEvent (QContextMenuEvent *event)
{
    Q_EMIT contextMenuRequested(indexAt(event->pos()), event->globalPos(), QModelIndexList());
}

void CalendarTreeView::focusInEvent (QFocusEvent *event)
{
    //debugPlan;
    TreeViewBase::focusInEvent(event);
    Q_EMIT focusChanged();
}

void CalendarTreeView::focusOutEvent (QFocusEvent * event)
{
    //debugPlan;
    TreeViewBase::focusInEvent(event);
    Q_EMIT focusChanged();
}

void CalendarTreeView::selectionChanged(const QItemSelection &sel, const QItemSelection &desel)
{
    //debugPlan<<sel.indexes().count();
    //for (const QModelIndex &i : std::as_const(selectionModel()->selectedIndexes())) { debugPlan<<i.row()<<","<<i.column(); }
    TreeViewBase::selectionChanged(sel, desel);
    Q_EMIT selectedIndexesChanged(selectionModel()->selectedIndexes());
}

void CalendarTreeView::currentChanged(const QModelIndex & current, const QModelIndex & previous)
{
    //debugPlan;
    TreeViewBase::currentChanged(current, previous);
    // possible bug in qt: in QAbstractItemView::SingleSelection you can select multiple items/rows
    selectionModel()->select(current, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    Q_EMIT currentIndexChanged(current);
}

Calendar *CalendarTreeView::currentCalendar() const
{
    return model()->calendar(currentIndex());
}

Calendar *CalendarTreeView::selectedCalendar() const
{
    QModelIndexList lst = selectionModel()->selectedRows();
    if (lst.count() == 1) {
        return model()->calendar(lst.first());
    }
    return nullptr;
}

QList<Calendar*> CalendarTreeView::selectedCalendars() const
{
    QList<Calendar *> lst;
    const QModelIndexList indexes = selectionModel()->selectedRows();
    for (const QModelIndex &i : indexes) {
        Calendar *a = model()->calendar(i);
        if (a) {
            lst << a;
        }
    }
    return lst;
}

void CalendarTreeView::dragMoveEvent(QDragMoveEvent *event)
{
    if (dragDropMode() == InternalMove && (event->source() != this || !(event->possibleActions() & Qt::MoveAction))) {
        return;
    }
    TreeViewBase::dragMoveEvent(event);
    if (! event->isAccepted()) {
        return;
    }
    // QTreeView thinks it's ok to drop, but it might not be...
    event->ignore();
    QModelIndex index = indexAt(event->position().toPoint());
    if (! index.isValid()) {
        if (model()->dropAllowed(nullptr, event->mimeData())) {
            event->accept();
        }
        return;
    }
    Calendar *c = model()->calendar(index);
    if (c == nullptr) {
        errorPlan<<"no calendar to drop on!";
        return; // hmmm
    }
    switch (dropIndicatorPosition()) {
        case AboveItem:
        case BelowItem:
            // c == sibling
            // if siblings parent is me or child of me: illegal
            if (model()->dropAllowed(c->parentCal(), event->mimeData())) {
                event->accept();
            }
            break;
        case OnItem:
            // c == new parent
            if (model()->dropAllowed(c, event->mimeData())) {
                event->accept();
            }
            break;
        default:
            break;
    }
}

//--------------------
CalendarDayView::CalendarDayView(QWidget *parent)
    : QTableView(parent),
    m_readwrite(false)
{
    setTabKeyNavigation(false);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_model = new CalendarDayItemModel(this);
    setModel(m_model);
    verticalHeader()->hide();

    actionSetWork = new QAction(i18n("Work..."), this);
    connect(actionSetWork, &QAction::triggered, this, &CalendarDayView::slotSetWork);
    actionSetVacation = new QAction(i18n("Non-working"), this);
    connect(actionSetVacation, &QAction::triggered, this, &CalendarDayView::slotSetVacation);
    actionSetUndefined = new QAction(i18n("Undefined"), this);
    connect(actionSetUndefined, &QAction::triggered, this, &CalendarDayView::slotSetUndefined);
}

QSize CalendarDayView::sizeHint() const
{
    QSize s = QTableView::sizeHint();
    s.setHeight(horizontalHeader()->height() + rowHeight(0) + frameWidth() * 2);
    return s;
}

void CalendarDayView::slotSetWork()
{
    debugPlan;
    if (receivers(SIGNAL(executeCommand(KUndo2Command*))) == 0) {
        return;
    }
    Calendar *cal = model()->calendar();
    if (cal == nullptr) {
        return;
    }
    QModelIndexList lst = selectionModel()->selectedIndexes();
    if (lst.isEmpty()) {
        lst << currentIndex();
    }
    if (lst.isEmpty()) {
        return;
    }
    QList<CalendarDay*> days;
    for (const QModelIndex &i : std::as_const(lst)) {
        CalendarDay *day = model()->day(i);
        if (day == nullptr) {
            continue;
        }
        days << day;
    }
    IntervalEditDialog *dlg = new IntervalEditDialog(cal, days, this);
    connect(dlg, SIGNAL(finished(int)), SLOT(slotIntervalEditDialogFinished(int)));
    dlg->open();
}

void CalendarDayView::slotIntervalEditDialogFinished(int result)
{
    IntervalEditDialog *dlg = qobject_cast<IntervalEditDialog*>(sender());
    if (dlg == nullptr) {
        return;
    }
    if (result == QDialog::Accepted) {
        MacroCommand *cmd = dlg->buildCommand();
        if (cmd) {
            Q_EMIT executeCommand(cmd);
        }
    }
    dlg->deleteLater();
}

void CalendarDayView::slotSetVacation()
{
    debugPlan;
    if (receivers(SIGNAL(executeCommand(KUndo2Command*))) == 0) {
        return;
    }
    QModelIndexList lst = selectionModel()->selectedIndexes();
    if (lst.isEmpty()) {
        lst << currentIndex();
    }
    if (lst.isEmpty()) {
        return;
    }
    bool mod = false;
    MacroCommand *m = new MacroCommand(kundo2_i18n("Modify Weekday State"));
    for (const QModelIndex &i : std::as_const(lst)) {
        CalendarDay *day = model()->day(i);
        if (day == nullptr || day->state() == CalendarDay::NonWorking) {
            continue;
        }
        mod = true;
        m->addCommand(new CalendarModifyStateCmd(model()->calendar(), day, CalendarDay::NonWorking));
    }
    if (mod) {
        Q_EMIT executeCommand(m);
    } else {
        delete m;
    }
}

void CalendarDayView::slotSetUndefined()
{
    debugPlan;
    if (receivers(SIGNAL(executeCommand(KUndo2Command*))) == 0) {
        return;
    }
    QModelIndexList lst = selectionModel()->selectedIndexes();
    if (lst.isEmpty()) {
        lst << currentIndex();
    }
    if (lst.isEmpty()) {
        return;
    }
    bool mod = false;
    MacroCommand *m = new MacroCommand(kundo2_i18n("Modify Weekday State"));
    for (const QModelIndex &i : std::as_const(lst)) {
        CalendarDay *day = model()->day(i);
        if (day == nullptr || day->state() == CalendarDay::Undefined) {
            continue;
        }
        mod = true;
        m->addCommand(new CalendarModifyStateCmd(model()->calendar(), day, CalendarDay::Undefined));
    }
    if (mod) {
        Q_EMIT executeCommand(m);
    } else {
        delete m;
    }
}

void CalendarDayView::setCurrentCalendar(Calendar *calendar)
{
    model()->setCalendar(calendar);
}

void CalendarDayView::contextMenuEvent (QContextMenuEvent *event)
{
    //debugPlan;
    if (!model()->calendar() || model()->calendar()->isShared()) {
        return;
    }
    QMenu menu;
    menu.addAction(actionSetWork);
    menu.addAction(actionSetVacation);
    menu.addAction(actionSetUndefined);

    menu.exec(event->globalPos(), actionSetWork);

    //Q_EMIT contextMenuRequested(indexAt(event->pos()), event->globalPos());
}

void CalendarDayView::focusInEvent (QFocusEvent *event)
{
    //debugPlan;
    QTableView::focusInEvent(event);
    Q_EMIT focusChanged();
}

void CalendarDayView::focusOutEvent (QFocusEvent * event)
{
    //debugPlan;
    QTableView::focusInEvent(event);
    Q_EMIT focusChanged();
}

void CalendarDayView::selectionChanged(const QItemSelection &sel, const QItemSelection &desel)
{
    //debugPlan<<sel.indexes().count();
    QTableView::selectionChanged(sel, desel);
    Q_EMIT selectedIndexesChanged(selectionModel()->selectedIndexes());
}

void CalendarDayView::currentChanged(const QModelIndex & current, const QModelIndex & previous)
{
    //debugPlan;
    QTableView::currentChanged(current, previous);
//    selectionModel()->select(current, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    Q_EMIT currentIndexChanged(current);
}

CalendarDay *CalendarDayView::selectedDay() const
{
    QModelIndexList lst = selectionModel()->selectedIndexes();
    if (lst.count() == 1) {
        return model()->day(lst.first());
    }
    return nullptr;
}

//-----------------------------------
CalendarEditor::CalendarEditor(KoPart *part, KoDocument *doc, QWidget *parent)
    : ViewBase(part, doc, parent),
    m_model(new DateTableDataModel(this))
{
    setXMLFile(QStringLiteral("CalendarEditorUi.rc"));

    setWhatsThis(
              xi18nc("@info:whatsthis",
                      "<title>Work & Vacation Editor</title>"
                      "<para>"
                      "A calendar defines availability for resources or tasks of type <emphasis>Duration</emphasis>. "
                      "A calendar can be specific to a resource or task, or shared by multiple resources or tasks. "
                      "A day can be of type <emphasis>Undefined</emphasis>, <emphasis>Non-working day</emphasis> or <emphasis>Working day</emphasis>. "
                      "A working day has one or more work intervals defined. "
                      "</para><para>"
                      "A calendar can have sub calendars. If a day is undefined in a calendar, the parent calendar is checked. "
                      "An <emphasis>Undefined</emphasis> day defaults to <emphasis>Non-working</emphasis> if used by a resource, or <emphasis>available all day</emphasis> if used by a task."
                      "</para><para>"
                      "A calendar can be defined as the <emphasis>Default calendar</emphasis>. "
                      "The default calendar is used by a working resource, when the resources calendar is not explicitly set."
                      "<nl/><link url='%1'>More...</link>"
                      "</para>", QStringLiteral("plan:work-and-vacation-editor")));

    setupGui();

    QVBoxLayout *l = new QVBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);
    QSplitter *sp = new QSplitter(this);
    l->addWidget(sp);

    m_calendarview = new CalendarTreeView(sp);
    connect(this, &ViewBase::expandAll, m_calendarview, &TreeViewBase::slotExpand);
    connect(this, &ViewBase::collapseAll, m_calendarview, &TreeViewBase::slotCollapse);

    QFrame *f = new QFrame(sp);
    l = new QVBoxLayout(f);
    l->setContentsMargins(0, 0, 0, 0);

    m_dayview = new CalendarDayView(f);
    l->addWidget(m_dayview);

    sp = new QSplitter(f);
    l->addWidget(sp);
    m_datePicker = new KDatePicker(sp);
    m_datePicker->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    m_datePicker->dateTable()->setWeekNumbersEnabled(true);
    m_datePicker->dateTable()->setGridEnabled(true);
    m_datePicker->dateTable()->setSelectionMode(KDateTable::ExtendedSelection);
    m_datePicker->dateTable()->setDateDelegate(new DateTableDateDelegate(m_datePicker->dateTable()));
    m_datePicker->dateTable()->setModel(m_model);
    m_datePicker->dateTable()->setPopupMenuEnabled(true);

    m_calendarview->setDragDropMode(QAbstractItemView::InternalMove);
    m_calendarview->setDropIndicatorShown(true);
    m_calendarview->setDragEnabled (true);
    m_calendarview->setAcceptDrops(true);
    m_calendarview->setAcceptDropsOnView(true);

    connect(m_datePicker->dateTable(), SIGNAL(aboutToShowContextMenu(QMenu*,QList<QDate>)), SLOT(slotContextMenuDate(QMenu*,QList<QDate>)));

/*    const QDate date(2007,7,19);
    const QColor fgColor(Qt::darkGray);
    KDateTable::BackgroundMode bgMode = KDateTable::CircleMode;
    const QColor bgColor(Qt::lightGray);
    m_datePicker->dateTable()->setCustomDatePainting(date, fgColor, bgMode, bgColor);*/


    m_calendarview->setEditTriggers(m_calendarview->editTriggers() | QAbstractItemView::EditKeyPressed);

    m_dayview->setEditTriggers(m_dayview->editTriggers() | QAbstractItemView::EditKeyPressed);

    m_calendarview->setDragDropMode(QAbstractItemView::InternalMove);
    m_calendarview->setDropIndicatorShown (true);
    m_calendarview->setDragEnabled (true);
    m_calendarview->setAcceptDrops(true);

    connect(m_calendarview->model(), &ItemModelBase::executeCommand, doc, &KoDocument::addCommand);
    connect(m_dayview->model(), &ItemModelBase::executeCommand, doc, &KoDocument::addCommand);
    connect(m_dayview, &CalendarDayView::executeCommand, doc, &KoDocument::addCommand);

    connect(m_calendarview, &CalendarTreeView::currentIndexChanged, this, &CalendarEditor::slotCurrentCalendarChanged);
    connect(m_calendarview, &CalendarTreeView::selectedIndexesChanged, this, &CalendarEditor::slotCalendarSelectionChanged);
    connect(m_calendarview, &CalendarTreeView::contextMenuRequested, this, &CalendarEditor::slotContextMenuCalendar);

    connect(m_dayview, &CalendarDayView::currentIndexChanged, this, &CalendarEditor::slotCurrentDayChanged);
    connect(m_dayview, &CalendarDayView::selectedIndexesChanged, this, &CalendarEditor::slotDaySelectionChanged);
    connect(m_dayview, &CalendarDayView::contextMenuRequested, this, &CalendarEditor::slotContextMenuDay);

    connect(m_dayview->model(), &QAbstractItemModel::dataChanged, this, &CalendarEditor::slotEnableActions);

    connect(m_calendarview, &CalendarTreeView::focusChanged, this, &CalendarEditor::slotEnableActions);
    connect(m_dayview, &CalendarDayView::focusChanged, this, &CalendarEditor::slotEnableActions);

}

void CalendarEditor::draw(Project &project)
{
    m_calendarview->setProject(&project);
    m_dayview->setProject(&project);
}

void CalendarEditor::draw()
{
}

void CalendarEditor::setGuiActive(bool activate)
{
    //debugPlan<<activate;
    updateActionsEnabled(true);
    ViewBase::setGuiActive(activate);
    if (activate) {
        if (!m_calendarview->currentIndex().isValid()) {
            m_calendarview->selectionModel()->setCurrentIndex(m_calendarview->model()->index(0, 0), QItemSelectionModel::NoUpdate);
        }
        //slotSelectionChanged(m_calendarview->selectionModel()->selectedRows());
    }
}

void CalendarEditor::slotContextMenuDate(QMenu *menu, const QList<QDate> &dates)
{
    if (!currentCalendar() || currentCalendar()->isShared()) {
        return;
    }
    if (dates.isEmpty()) {
        m_currentMenuDateList << m_datePicker->date();
    } else {
        m_currentMenuDateList = dates;
    }
    menu->addAction(actionSetWork);
    menu->addAction(actionSetVacation);
    menu->addAction(actionSetUndefined);
}

void CalendarEditor::slotContextMenuCalendar(const QModelIndex &index, const QPoint& pos)
{
    Q_UNUSED(index)
    Q_UNUSED(pos)
    slotHeaderContextMenuRequested(pos);
}

void CalendarEditor::slotContextMenuDay(const QModelIndex &index, const QPoint& pos)
{
    debugPlan<<index.row()<<","<<index.column()<<":"<<pos;
/*    QString name;
    if (index.isValid()) {
        if (m_dayview->model()->day(index)) {
            name = "calendareditor_day_popup";
        }
    }
    debugPlan<<name;
    if (name.isEmpty()) {
        return;
    }
    Q_EMIT requestPopupMenu(name, pos);*/
}

bool CalendarEditor::loadContext(const KoXmlElement &context)
{
    return m_calendarview->loadContext(m_calendarview->model()->columnMap(), context);
}

void CalendarEditor::saveContext(QDomElement &context) const
{
    m_calendarview->saveContext(m_calendarview->model()->columnMap(), context);
}

Calendar *CalendarEditor::currentCalendar() const
{
    return m_calendarview->currentCalendar();
}

void CalendarEditor::slotCurrentCalendarChanged(const QModelIndex &)
{
    //debugPlan<<curr.row()<<","<<curr.column();
    m_dayview->setCurrentCalendar(currentCalendar());
    if (m_model) {
        m_model->setCalendar(currentCalendar());
    }
}

void CalendarEditor::slotCalendarSelectionChanged(const QModelIndexList& /*list */)
{
    //debugPlan<<list.count();
    updateActionsEnabled(true);
}

void CalendarEditor::slotCurrentDayChanged(const QModelIndex &)
{
    //debugPlan<<curr.row()<<","<<curr.column();
}

void CalendarEditor::slotDaySelectionChanged(const QModelIndexList&)
{
    //debugPlan<<list.count();
    updateActionsEnabled(true);
}

void CalendarEditor::slotEnableActions()
{
    updateActionsEnabled(true);
}

void CalendarEditor::updateActionsEnabled(bool on)
{
    QList<Calendar *> lst = m_calendarview->selectedCalendars();
    bool one = lst.count() == 1;
    bool more = lst.count() > 1;
    actionAddCalendar ->setEnabled(on && !more);
    actionAddSubCalendar ->setEnabled(on && one);
    actionDeleteSelection->setEnabled(on && (one || more));
}

void CalendarEditor::setupGui()
{
    KActionCollection *coll = actionCollection();

    actionAddCalendar = new QAction(koIcon("resource-calendar-insert"), i18n("Add Calendar"), this);
    coll->addAction(QStringLiteral("add_calendar"), actionAddCalendar);
    coll->setDefaultShortcut(actionAddCalendar, Qt::CTRL | Qt::Key_I);
    connect(actionAddCalendar , &QAction::triggered, this, &CalendarEditor::slotAddCalendar);

    actionAddSubCalendar = new QAction(koIcon("resource-calendar-child-insert"), i18n("Add Subcalendar"), this);
    coll->addAction(QStringLiteral("add_subcalendar"), actionAddSubCalendar);
    coll->setDefaultShortcut(actionAddSubCalendar, Qt::SHIFT | Qt::CTRL | Qt::Key_I);
    connect(actionAddSubCalendar , &QAction::triggered, this, &CalendarEditor::slotAddSubCalendar);

    actionDeleteSelection = new QAction(koIcon("edit-delete"), xi18nc("@action", "Delete"), this);
    coll->addAction(QStringLiteral("delete_selection"), actionDeleteSelection);
    coll->setDefaultShortcut(actionDeleteSelection, Qt::Key_Delete);
    connect(actionDeleteSelection, &QAction::triggered, this, &CalendarEditor::slotDeleteCalendar);

    actionSetWork = new QAction(i18n("Work..."), this);
    connect(actionSetWork, &QAction::triggered, this, &CalendarEditor::slotSetWork);
    actionSetVacation = new QAction(i18n("Non-working"), this);
    connect(actionSetVacation, &QAction::triggered, this, &CalendarEditor::slotSetVacation);
    actionSetUndefined = new QAction(i18n("Undefined"), this);
    connect(actionSetUndefined, &QAction::triggered, this, &CalendarEditor::slotSetUndefined);

    createOptionActions(ViewBase::OptionExpand | ViewBase::OptionCollapse | ViewBase::OptionViewConfig);
}

void CalendarEditor::slotOptions()
{
    ItemViewSettupDialog *dlg = new ItemViewSettupDialog(this, m_calendarview, false, this);
    connect(dlg, SIGNAL(finished(int)), SLOT(slotOptionsFinished(int)));
    dlg->open();
}

void CalendarEditor::updateReadWrite(bool readwrite)
{
    m_calendarview->setReadWrite(readwrite);
    m_dayview->setReadWrite(readwrite);
    ViewBase::updateReadWrite(readwrite);
}

void CalendarEditor::slotAddCalendar ()
{
    //debugPlan;
    // get parent through sibling
    Calendar *cal = m_calendarview->selectedCalendar();
    Calendar *parent = cal ? cal->parentCal() : nullptr;
    int pos = parent ? parent->indexOf(cal) : project()->indexOf(cal);
    if (pos >= 0) {
        ++pos; // after selected calendar
    }
    insertCalendar (new Calendar(), parent, pos);
}

void CalendarEditor::slotAddSubCalendar ()
{
    //debugPlan;
    insertCalendar (new Calendar (), m_calendarview->selectedCalendar ());
}

void CalendarEditor::insertCalendar (Calendar *calendar, Calendar *parent, int pos)
{
    QModelIndex i = m_calendarview->model()->insertCalendar (calendar, pos, parent);
    if (i.isValid()) {
        QModelIndex p = m_calendarview->model()->parent(i);
        //if (parent) debugPlan<<" parent="<<parent->name()<<":"<<p.row()<<","<<p.column();
        //debugPlan<<i.row()<<","<<i.column();
        m_calendarview->setExpanded(p, true);
        m_calendarview->setCurrentIndex(i);
        m_calendarview->edit(i);
    }
}

void CalendarEditor::slotDeleteCalendar()
{
    //debugPlan;
    m_calendarview->model()->removeCalendar(m_calendarview->selectedCalendar());
}

void CalendarEditor::slotAddInterval ()
{
    //debugPlan;
/*    CalendarDay *parent = m_dayview->selectedDay ();
    if (parent == 0) {
        TimeInterval *ti = m_dayview->selectedInterval();
        if (ti == 0) {
            return;
        }
        parent = m_dayview->model()->parentDay(ti);
        if (parent == 0) {
            return;
        }
    }
    QModelIndex i = m_dayview->model()->insertInterval(new TimeInterval(), parent);
    if (i.isValid()) {
        QModelIndex p = m_dayview->model()->index(parent);
        m_dayview->setExpanded(p, true);
        m_dayview->setCurrentIndex(i);
        m_dayview->edit(i);
    }*/
}

void CalendarEditor::slotDeleteDaySelection()
{
    //debugPlan;
/*    TimeInterval *ti = m_dayview->selectedInterval();
    if (ti != 0) {
        m_dayview->model()->removeInterval(ti);
        return;
    }
    CalendarDay *day = m_dayview->selectedDay();
    if (day != 0) {
        m_dayview->model()->removeDay(day);
    }*/
}

void CalendarEditor::slotAddDay ()
{
    //debugPlan;
/*    Calendar *c = currentCalendar();
    if (c == 0) {
        return;
    }
    QDate date = QDate::currentDate();
    while (c->day(date)) {
        date = date.addDays(1);
    }
    QModelIndex i = m_dayview->model()->insertDay(new CalendarDay(date,  CalendarDay::NonWorking));
    if (i.isValid()) {
        QModelIndex p = m_dayview->model()->parent(i);
        m_dayview->setExpanded(p, true);
        m_dayview->setCurrentIndex(i);
        m_dayview->edit(i);
    }*/
}

void CalendarEditor::slotSetWork()
{
    debugPlan<<currentCalendar()<<m_currentMenuDateList;
    if (currentCalendar() == nullptr || m_currentMenuDateList.isEmpty()) {
        return;
    }
    IntervalEditDialog *dlg = new IntervalEditDialog(currentCalendar(), m_currentMenuDateList, this);
    connect(dlg, SIGNAL(finished(int)), SLOT(slotIntervalEditDialogFinished(int)));
    dlg->open();
    m_currentMenuDateList.clear();
}

void CalendarEditor::slotIntervalEditDialogFinished(int result)
{
    IntervalEditDialog *dlg = qobject_cast<IntervalEditDialog*>(sender());
    if (dlg == nullptr) {
        return;
    }
    if (result == QDialog::Accepted) {
        MacroCommand *cmd = dlg->buildCommand();
        if (cmd) {
            part()->addCommand(cmd);
        }
    }
    dlg->deleteLater();
}

void CalendarEditor::slotSetVacation()
{
    debugPlan<<m_currentMenuDateList;
    if (m_currentMenuDateList.isEmpty() || currentCalendar() == nullptr) {
        return;
    }
    bool mod = false;
    MacroCommand *m = new MacroCommand(kundo2_i18n("Modify Calendar"));
    for (const QDate &date : std::as_const(m_currentMenuDateList)) {
        debugPlan<<"handle:"<<date;
        CalendarDay *day = currentCalendar()->findDay(date);
        if (day == nullptr) {
            mod = true;
            day = new CalendarDay(date, CalendarDay::NonWorking);
            m->addCommand(new CalendarAddDayCmd(currentCalendar(), day));
            if (m_currentMenuDateList.count() == 1) {
                m->setText(kundo2_i18n("%1: Set to Non-Working", date.toString()));
            }
        } else if (day->state() != CalendarDay::NonWorking) {
            mod = true;
            m->addCommand(new CalendarModifyStateCmd(currentCalendar(), day, CalendarDay::NonWorking));
            if (m_currentMenuDateList.count() == 1) {
                m->setText(kundo2_i18n("%1: Set to Non-Working", date.toString()));
            }
        }
    }
    if (mod) {
        part()->addCommand(m);
    } else {
        delete m;
    }
    m_currentMenuDateList.clear();
}

void CalendarEditor::slotSetUndefined()
{
    debugPlan;
    if (m_currentMenuDateList.isEmpty() || currentCalendar() == nullptr) {
        return;
    }
    bool mod = false;
    MacroCommand *m = new MacroCommand(kundo2_i18n("Modify Calendar"));
    for (const QDate &date : std::as_const(m_currentMenuDateList)) {
        CalendarDay *day = currentCalendar()->findDay(date);
        if (day && day->state() != CalendarDay::Undefined) {
            mod = true;
            m->addCommand(new CalendarRemoveDayCmd(currentCalendar(), day));
            if (m_currentMenuDateList.count() == 1) {
                m->setText(kundo2_i18n("Set %1 to Undefined", date.toString()));
            }
        }
    }
    if (mod) {
        part()->addCommand(m);
    } else {
        delete m;
    }
    m_currentMenuDateList.clear();
}


} // namespace KPlato
