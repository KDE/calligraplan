/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2007-2010 Dag Andersen <dag.andersen@kdemail.net>
   SPDX-FileCopyrightText: 2012 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kptviewlistdialog.h"
#include "kptviewlist.h"
#include "kptview.h"

#ifdef PLAN_USE_KREPORT
#include "reports/reportview.h"
#endif

#include <kptdebug.h>

#include <KLocalizedString>

#include <QTextEdit>

namespace KPlato
{

ViewListDialog::ViewListDialog(View *view, ViewListWidget &viewlist, QWidget *parent)
    : KoDialog(parent)
{
    setCaption(xi18nc("@title:window", "Add View"));
    setButtons(KoDialog::Ok | KoDialog::Cancel);
    setDefaultButton(Ok);

    m_panel = new AddViewPanel(view, viewlist, this);

    setMainWidget(m_panel);

    enableButtonOk(false);

    connect(this,&KoDialog::okClicked,this,&ViewListDialog::slotOk);
    connect(m_panel, &AddViewPanel::enableButtonOk, this, &KoDialog::enableButtonOk);
    connect(m_panel, &AddViewPanel::viewCreated, this, &ViewListDialog::viewCreated);

    connect(&viewlist, &ViewListWidget::viewListItemRemoved, this, &ViewListDialog::slotViewListItemRemoved);
}

void ViewListDialog::slotViewListItemRemoved(ViewListItem *)
{
    reject();
}

void ViewListDialog::slotOk() {
    if (m_panel->ok()) {
        accept();
    }
}

//------------------------
AddViewPanel::AddViewPanel(View *view, ViewListWidget &viewlist, QWidget *parent)
    : QWidget(parent),
      m_view(view),
      m_viewlist(viewlist),
      m_viewnameChanged(false),
      m_viewtipChanged(false)
{
    widget.setupUi(this);

    // NOTE: these lists must match switch in ok() FIXME: refactor
    m_viewtypes
            << QStringLiteral("ResourceGroupEditor")
            << QStringLiteral("ResourceEditor")
            << QStringLiteral("TaskEditor")
            << QStringLiteral("CalendarEditor")
            << QStringLiteral("AccountsEditor")
            << QStringLiteral("DependencyEditor")
            << QStringLiteral("PertEditor")
            << QStringLiteral("ScheduleHandlerView")
            << QStringLiteral("TaskStatusView")
            << QStringLiteral("TaskView")
            << QStringLiteral("TaskWorkPackageView")
            << QStringLiteral("GanttView")
            << QStringLiteral("MilestoneGanttView")
            << QStringLiteral("ResourceAppointmentsView")
            << QStringLiteral("ResourceAppointmentsGanttView")
            << QStringLiteral("ResourceCoverageView")
            << QStringLiteral("AccountsView")
            << QStringLiteral("ProjectStatusView")
            << QStringLiteral("PerformanceStatusView")
            << QStringLiteral("ReportsGeneratorView");
#ifdef PLAN_USE_KREPORT
    m_viewtypes << "ReportView";
#endif
    QStringList lst;
    lst << i18n("Resource Group Editor")
            << i18n("Resource Editor")
            << i18n("Task Editor")
            << i18n("Work & Vacation Editor")
            << i18n("Accounts Editor")
            << i18n("Dependency Editor (Graphic)")
            << i18n("Dependency Editor (List)")
            << i18n("Schedule Handler")
            << i18n("Task Status")
            << i18n("Task View")
            << i18n("Work Package View")
            << i18n("Gantt View")
            << i18n("Milestone Gantt View")
            << i18n("Resource Assignments")
            << i18n("Resource Assignments (Gantt)")
            << i18n("Resource Coverage")
            << i18n("Cost Breakdown")
            << i18n("Project Performance Chart")
            << i18n("Tasks Performance Chart")
            << i18n("Reports generator");
#ifdef PLAN_USE_KREPORT
      lst << i18n("Report");
#endif
    widget.viewtype->addItems(lst);

    const QList<ViewListItem*> items = m_viewlist.categories();
    for (ViewListItem *item : items) {
        m_categories.insert(item->text(0), item);
    }
    widget.category->addItems(m_categories.keys());
    ViewListItem *curr = m_viewlist.currentCategory();
    if (curr) {
        const QList<ViewListItem*> &items = m_categories.values();
        widget.category->setCurrentIndex(items.indexOf(curr));
    }
    fillAfter(m_categories.value(widget.category->currentText()));

    viewtypeChanged(widget.viewtype->currentIndex());

    connect(widget.viewname, &QLineEdit::textChanged, this, &AddViewPanel::changed);
    connect(widget.tooltip, &QLineEdit::textChanged, this, &AddViewPanel::changed);
    connect(widget.viewname, &QLineEdit::textChanged, this, &AddViewPanel::viewnameChanged);
    connect(widget.tooltip, &QLineEdit::textChanged, this, &AddViewPanel::viewtipChanged);
    connect(widget.insertAfter, SIGNAL(currentIndexChanged(int)), SLOT(changed()));
    connect(widget.viewtype, SIGNAL(currentIndexChanged(int)), SLOT(viewtypeChanged(int)));
    connect(widget.category, &QComboBox::editTextChanged, this, &AddViewPanel::categoryChanged);

    QString categoryWhatsThis = xi18nc("@info:whatsthis",
                                       "<title>The category of the view</title><nl/>"
                                       "The view is placed under this category in the view selector.<nl/>"
                                       "You can edit the category name to create a new category.");
    widget.categoryLabel->setWhatsThis(categoryWhatsThis);
    widget.category->setWhatsThis(categoryWhatsThis);

    QTimer::singleShot(0, this, &AddViewPanel::changed);
}

void AddViewPanel::viewnameChanged(const QString &text)
{
    m_viewnameChanged = ! text.isEmpty();
}

void AddViewPanel::viewtipChanged(const QString &text)
{
    m_viewtipChanged = ! text.isEmpty();
}

void AddViewPanel::viewtypeChanged(int idx)
{
    ViewInfo vi = m_view->defaultViewInfo(m_viewtypes.value(idx));
    if (widget.viewname->text().isEmpty()) {
        m_viewnameChanged = false;
    }
    if (! m_viewnameChanged) {
        widget.viewname->setText(vi.name);
        m_viewnameChanged = false;
    }
    if (widget.tooltip->text().isEmpty()) {
        m_viewtipChanged = false;
    }
    if (! m_viewtipChanged) {
        QTextEdit e;
        e.setText(vi.tip);
        widget.tooltip->setText(e.toPlainText());
        m_viewtipChanged = false;
    }
}

void AddViewPanel::categoryChanged()
{
    debugPlan<<widget.category->currentText();
    fillAfter(m_categories.value(widget.category->currentText()));
    changed();
}

void AddViewPanel::fillAfter(ViewListItem *cat)
{
    debugPlan<<cat;
    widget.insertAfter->clear();
    if (cat) {
        widget.insertAfter->addItem(i18n("Top"));
//        int idx = 0;
        for (int i = 0; i < cat->childCount(); ++i) {
            ViewListItem *itm = static_cast<ViewListItem*>(cat->child(i));
            widget.insertAfter->addItem(itm->text(0), QVariant::fromValue((void*)itm));
        }
        if (cat == m_viewlist.currentCategory()) {
            ViewListItem *v = m_viewlist.currentItem();
            if (v && v->type() != ViewListItem::ItemType_Category) {
                widget.insertAfter->setCurrentIndex(cat->indexOfChild(v) + 1);
            }
        }
    }
}

bool AddViewPanel::ok()
{
    QString n = widget.category->currentText();
    ViewListItem *curr = m_categories.value(n);
    QString c = curr == nullptr ? n : curr->tag();

    ViewListItem *cat = m_viewlist.addCategory(c, n);
    if (cat == nullptr) {
        return false;
    }
    ViewBase *v = nullptr;
    int index = widget.insertAfter->currentIndex();
    int viewtype = widget.viewtype->currentIndex();
    switch (viewtype) {
        case 0: { // Resource group editor
            v = m_view->createResourceGroupEditor(cat, m_viewtypes.value(viewtype), widget.viewname->text(), widget.tooltip->text(), index);
            break; }
        case 1: { // Resource editor
            v = m_view->createResourceEditor(cat, m_viewtypes.value(viewtype), widget.viewname->text(), widget.tooltip->text(), index);
            break; }
        case 2: { // Task editor
            v = m_view->createTaskEditor(cat, m_viewtypes.value(viewtype), widget.viewname->text(), widget.tooltip->text(), index);
            break; }
        case 3: { // Work & Vacation Editor
            v = m_view->createCalendarEditor(cat, m_viewtypes.value(viewtype), widget.viewname->text(), widget.tooltip->text(), index);
            break; }
        case 4: { // Accounts Editor
            v = m_view->createAccountsEditor(cat, m_viewtypes.value(viewtype), widget.viewname->text(), widget.tooltip->text(), index);
            break; }
        case 5: { // Dependency Editor (Graphic)
            v = m_view->createDependencyEditor(cat, m_viewtypes.value(viewtype), widget.viewname->text(), widget.tooltip->text(), index);
            break; }
        case 6: { // Dependency Editor (List)
            v = m_view->createPertEditor(cat, m_viewtypes.value(viewtype), widget.viewname->text(), widget.tooltip->text(), index);
            break; }
        case 7: { // Schedules Handler
            v = m_view->createScheduleHandler(cat, m_viewtypes.value(viewtype), widget.viewname->text(), widget.tooltip->text(), index);
            break; }
        case 8: { // Task status
            v = m_view->createTaskStatusView(cat, m_viewtypes.value(viewtype), widget.viewname->text(), widget.tooltip->text(), index);
            break; }
        case 9: { // Task status
            v = m_view->createTaskView(cat, m_viewtypes.value(viewtype), widget.viewname->text(), widget.tooltip->text(), index);
            break; }
        case 10: { // Task work package
            v = m_view->createTaskWorkPackageView(cat, m_viewtypes.value(viewtype), widget.viewname->text(), widget.tooltip->text(), index);
            break; }
        case 11: { // Gantt View
            v = m_view->createGanttView(cat, m_viewtypes.value(viewtype), widget.viewname->text(), widget.tooltip->text(), index);
            break; }
        case 12: { // Milestone Gantt View
            v = m_view->createMilestoneGanttView(cat, m_viewtypes.value(viewtype), widget.viewname->text(), widget.tooltip->text(), index);
            break; }
        case 13: { // Resource Assignments
            v = m_view->createResourceAppointmentsView(cat, m_viewtypes.value(viewtype), widget.viewname->text(), widget.tooltip->text(), index);
            break; }
        case 14: { // Resource Assignments (Gantt)
            v = m_view->createResourceAppointmentsGanttView(cat, m_viewtypes.value(viewtype), widget.viewname->text(), widget.tooltip->text(), index);
            break; }
        case 15: { // Resource Coverage
            v = m_view->createResourceCoverageView(cat, m_viewtypes.value(viewtype), widget.viewname->text(), widget.tooltip->text(), index);
            break; }
        case 16: { // Cost Breakdown
            v = m_view->createAccountsView(cat, m_viewtypes.value(viewtype), widget.viewname->text(), widget.tooltip->text(), index);
            break; }
        case 17: { // Project Performance Chart
            v = m_view->createProjectStatusView(cat, m_viewtypes.value(viewtype), widget.viewname->text(), widget.tooltip->text(), index);
            break; }
        case 18: { // Task Performance Chart
            v = m_view->createPerformanceStatusView(cat, m_viewtypes.value(viewtype), widget.viewname->text(), widget.tooltip->text(), index);
            break; }
        case 19: { // Reports generator
            v = m_view->createReportsGeneratorView(cat, m_viewtypes.value(viewtype), widget.viewname->text(), widget.tooltip->text(), index);
            break; }
#ifdef PLAN_USE_KREPORT
        case 20: { // Report view
            v = m_view->createReportView(cat, m_viewtypes.value(viewtype), widget.viewname->text(), widget.tooltip->text(), index);
            break; }
#endif
        default:
            errorPlan<<"Unknown view type!";
            break;
    }
    Q_EMIT viewCreated(v);
    return true;
}

void AddViewPanel::changed()
{
    bool disable = widget.viewname->text().isEmpty() | widget.viewtype->currentText().isEmpty() | widget.category->currentText().isEmpty();
    Q_EMIT enableButtonOk(! disable);
}

//------------------------
ViewListEditViewDialog::ViewListEditViewDialog(ViewListWidget &viewlist, ViewListItem *item, QWidget *parent)
    : KoDialog(parent)
{
    setCaption(xi18nc("@title:window", "Configure View"));
    setButtons(KoDialog::Ok | KoDialog::Cancel);
    setDefaultButton(Ok);

    m_panel = new EditViewPanel(viewlist, item, this);

    setMainWidget(m_panel);

    enableButtonOk(false);

    connect(this,&KoDialog::okClicked,this,&ViewListEditViewDialog::slotOk);
    connect(m_panel, &EditViewPanel::enableButtonOk, this, &KoDialog::enableButtonOk);

    connect(&viewlist, &ViewListWidget::viewListItemRemoved, this, &ViewListEditViewDialog::slotViewListItemRemoved);
}

void ViewListEditViewDialog::slotViewListItemRemoved(ViewListItem *)
{
    reject();
}

void ViewListEditViewDialog::slotOk() {
    if (m_panel->ok()) {
        accept();
    }
}

EditViewPanel::EditViewPanel(ViewListWidget &viewlist, ViewListItem *item, QWidget *parent)
    : QWidget(parent),
      m_item(item),
      m_viewlist(viewlist)
{
    widget.setupUi(this);

    widget.viewname->setText(item->text(0));
    QTextEdit e;
    e.setText(item->toolTip(0));
    widget.tooltip->setText(e.toPlainText());

    const QList<ViewListItem*> items = m_viewlist.categories();
    for (ViewListItem *item : items) {
        m_categories.insert(item->text(0), item);
    }
    widget.category->addItems(m_categories.keys());
    ViewListItem *curr = m_viewlist.currentCategory();
    if (curr) {
        const QList<ViewListItem*> &items = m_categories.values();
        widget.category->setCurrentIndex(items.indexOf(curr));
    }
    categoryChanged();

    connect(widget.viewname, &QLineEdit::textChanged, this, &EditViewPanel::changed);
    connect(widget.tooltip, &QLineEdit::textChanged, this, &EditViewPanel::changed);
    connect(widget.insertAfter, SIGNAL(currentIndexChanged(int)), SLOT(changed()));
    connect(widget.category, &QComboBox::editTextChanged, this, &EditViewPanel::categoryChanged);

    QString categoryWhatsThis = xi18nc("@info:whatsthis",
                                       "<title>The category of the view</title><nl/>"
                                       "The view is placed under this category in the view selector.<nl/>"
                                       "Selecting a different category will move the view to the new category.<nl/>"
                                       "You can edit the category name to create a new category.");
    widget.categoryLabel->setWhatsThis(categoryWhatsThis);
    widget.category->setWhatsThis(categoryWhatsThis);
}

bool EditViewPanel::ok()
{
    QString n = widget.category->currentText();
    ViewListItem *curr = m_categories.value(n);
    QString c = curr == nullptr ? n : curr->tag();

    ViewListItem *cat = m_viewlist.addCategory(c, n);
    if (cat == nullptr) {
        warnPlan<<"No category";
        return false;
    }
    if (widget.viewname->text() != m_item->text(0)) {
        m_item->setText(0, widget.viewname->text());
    }
    if (widget.tooltip->text() != m_item->toolTip(0)) {
        m_item->setToolTip(0, widget.tooltip->text());
    }
    m_viewlist.removeViewListItem(m_item);
    int index = qMin(widget.insertAfter->currentIndex(), cat->childCount());
    m_viewlist.addViewListItem(m_item, cat, index);

    return true;
}

void EditViewPanel::changed()
{
    bool disable = widget.viewname->text().isEmpty() | widget.category->currentText().isEmpty();
    Q_EMIT enableButtonOk(! disable);
}

void EditViewPanel::categoryChanged()
{
    debugPlan<<widget.category->currentText();
    fillAfter(m_categories.value(widget.category->currentText()));
    changed();
}

void EditViewPanel::fillAfter(ViewListItem *cat)
{
    debugPlan<<cat;
    widget.insertAfter->clear();
    if (cat) {
        widget.insertAfter->addItem(i18n("Top"));
//        int idx = 0;
        for (int i = 0; i < cat->childCount(); ++i) {
            ViewListItem *itm = static_cast<ViewListItem*>(cat->child(i));
            widget.insertAfter->addItem(itm->text(0), QVariant::fromValue((void*)itm));
        }
        if (cat == m_viewlist.currentCategory()) {
            ViewListItem *v = m_viewlist.currentItem();
            if (v && v->type() != ViewListItem::ItemType_Category) {
                widget.insertAfter->setCurrentIndex(cat->indexOfChild(v) + 1);
            }
        }
    }
}

//------------------------
ViewListEditCategoryDialog::ViewListEditCategoryDialog(ViewListWidget &viewlist, ViewListItem *item, QWidget *parent)
    : KoDialog(parent)
{
    setCaption(xi18nc("@title:window", "Configure Category"));
    setButtons(KoDialog::Ok | KoDialog::Cancel);
    setDefaultButton(Ok);

    m_panel = new EditCategoryPanel(viewlist, item, this);

    setMainWidget(m_panel);

    enableButtonOk(false);

    connect(this,&KoDialog::okClicked,this,&ViewListEditCategoryDialog::slotOk);
    connect(m_panel, &EditCategoryPanel::enableButtonOk, this, &KoDialog::enableButtonOk);

    connect(&viewlist, &ViewListWidget::viewListItemRemoved, this, &ViewListEditCategoryDialog::slotViewListItemRemoved);
}

void ViewListEditCategoryDialog::slotViewListItemRemoved(ViewListItem *)
{
    reject();
}

void ViewListEditCategoryDialog::slotOk() {
    if (m_panel->ok()) {
        accept();
    }
}

EditCategoryPanel::EditCategoryPanel(ViewListWidget &viewlist, ViewListItem *item, QWidget *parent)
    : QWidget(parent),
      m_item(item),
      m_viewlist(viewlist)
{
    widget.setupUi(this);

    widget.viewname->setText(item->text(0));
    QTextEdit e;
    e.setText(item->toolTip(0));
    widget.tooltip->setText(e.toPlainText());

    fillAfter();

    connect(widget.viewname, &QLineEdit::textChanged, this, &EditCategoryPanel::changed);
    connect(widget.tooltip, &QLineEdit::textChanged, this, &EditCategoryPanel::changed);
    connect(widget.insertAfter, SIGNAL(currentIndexChanged(int)), SLOT(changed()));
}

bool EditCategoryPanel::ok()
{
    if (widget.viewname->text() != m_item->text(0)) {
        m_item->setText(0, widget.viewname->text());
    }
    if (widget.tooltip->text() != m_item->toolTip(0)) {
        m_item->setToolTip(0, widget.tooltip->text());
    }
    bool ex = m_item->isExpanded();
    m_viewlist.removeViewListItem(m_item);
    int index = widget.insertAfter->currentIndex();
    m_viewlist.addViewListItem(m_item, nullptr, index);
    m_item->setExpanded(ex);
    return true;
}

void EditCategoryPanel::changed()
{
    bool disable = widget.viewname->text().isEmpty();
    Q_EMIT enableButtonOk(! disable);
}

void EditCategoryPanel::fillAfter()
{
    debugPlan;
    widget.insertAfter->clear();
    widget.insertAfter->addItem(i18n("Top"));
    int idx = 0;
    QList<ViewListItem*> lst = m_viewlist.categories();
    for (int i = 0; i < lst.count(); ++i) {
        ViewListItem *itm = lst[ i ];
        if (m_item == itm) {
            idx = i;
        } else {
            widget.insertAfter->addItem(itm->text(0), QVariant::fromValue((void*)itm));
        }
    }
    widget.insertAfter->setCurrentIndex(idx);
}

#ifdef PLAN_USE_KREPORT
//------ Reports
ViewListReportsDialog::ViewListReportsDialog(View *view, ViewListWidget &viewlist, const QDomDocument &doc, QWidget *parent)
    : KoDialog(parent)
{
    setCaption(xi18nc("@title:window", "Add Report"));
    setButtons(KoDialog::Ok | KoDialog::Cancel);
    setDefaultButton(Ok);

    m_panel = new AddReportsViewPanel(view, viewlist, doc, this);

    setMainWidget(m_panel);

    enableButtonOk(true);

    connect(this,SIGNAL(okClicked()),this,SLOT(slotOk()));
    connect(m_panel, SIGNAL(enableButtonOk(bool)), SLOT(enableButtonOk(bool)));
    connect(m_panel, SIGNAL(viewCreated(KPlato::ViewBase*)), SIGNAL(viewCreated(KPlato::ViewBase*)));

    connect(&viewlist, SIGNAL(viewListItemRemoved(KPlato::ViewListItem*)), SLOT(slotViewListItemRemoved(KPlato::ViewListItem*)));
}

void ViewListReportsDialog::slotViewListItemRemoved(ViewListItem *)
{
    reject();
}

void ViewListReportsDialog::slotOk() {
    if (m_panel->ok()) {
        accept();
    }
}

//------------------------
AddReportsViewPanel::AddReportsViewPanel(View *view, ViewListWidget &viewlist, const QDomDocument &doc, QWidget *parent)
    : QWidget(parent),
      m_view(view),
      m_viewlist(viewlist),
      m_viewnameChanged(false),
      m_viewtipChanged(false),
      m_data(doc)
{
    widget.setupUi(this);

    // NOTE: these lists must match switch in ok() FIXME: refactor
    m_viewtypes << "ReportView";
    QStringList lst;
    lst << xi18n("Report");
    widget.viewtype->addItems(lst);

    const QList<ViewListItem*> items = m_viewlist.categories();
    for (ViewListItem *item : items) {
        m_categories.insert(item->text(0), item);
    }
    widget.category->addItems(m_categories.keys());
    ViewListItem *curr = m_viewlist.currentCategory();
    if (curr) {
        widget.category->setCurrentIndex(m_categories.values().indexOf(curr));
    }
    fillAfter(m_categories.value(widget.category->currentText()));

    viewtypeChanged(widget.viewtype->currentIndex());

    connect(widget.viewname, SIGNAL(textChanged(QString)), SLOT(changed()));
    connect(widget.tooltip, SIGNAL(textChanged(QString)), SLOT(changed()));
    connect(widget.viewname, SIGNAL(textChanged(QString)), SLOT(viewnameChanged(QString)));
    connect(widget.tooltip, SIGNAL(textChanged(QString)), SLOT(viewtipChanged(QString)));
    connect(widget.insertAfter, SIGNAL(currentIndexChanged(int)), SLOT(changed()));
    connect(widget.viewtype, SIGNAL(currentIndexChanged(int)), SLOT(viewtypeChanged(int)));
    connect(widget.category, SIGNAL(editTextChanged(QString)), SLOT(categoryChanged()));
}

void AddReportsViewPanel::viewnameChanged(const QString &text)
{
    m_viewnameChanged = ! text.isEmpty();
}

void AddReportsViewPanel::viewtipChanged(const QString &text)
{
    m_viewtipChanged = ! text.isEmpty();
}

void AddReportsViewPanel::viewtypeChanged(int idx)
{
    ViewInfo vi = m_view->defaultViewInfo(m_viewtypes.value(idx));
    if (widget.viewname->text().isEmpty()) {
        m_viewnameChanged = false;
    }
    if (! m_viewnameChanged) {
        widget.viewname->setText(vi.name);
        m_viewnameChanged = false;
    }
    if (widget.tooltip->text().isEmpty()) {
        m_viewtipChanged = false;
    }
    if (! m_viewtipChanged) {
        QTextEdit e;
        e.setText(vi.tip);
        widget.tooltip->setText(e.toPlainText());
        m_viewtipChanged = false;
    }
}

void AddReportsViewPanel::categoryChanged()
{
    debugPlan<<widget.category->currentText();
    fillAfter(m_categories.value(widget.category->currentText()));
    changed();
}

void AddReportsViewPanel::fillAfter(ViewListItem *cat)
{
    debugPlan<<cat;
    widget.insertAfter->clear();
    if (cat) {
        widget.insertAfter->addItem(i18n("Top"));
//        int idx = 0;
        for (int i = 0; i < cat->childCount(); ++i) {
            ViewListItem *itm = static_cast<ViewListItem*>(cat->child(i));
            widget.insertAfter->addItem(itm->text(0), QVariant::fromValue((void*)itm));
        }
        if (cat == m_viewlist.currentCategory()) {
            ViewListItem *v = m_viewlist.currentItem();
            if (v && v->type() != ViewListItem::ItemType_Category) {
                widget.insertAfter->setCurrentIndex(cat->indexOfChild(v) + 1);
            }
        }
    }
}

bool AddReportsViewPanel::ok()
{
    QString n = widget.category->currentText();
    ViewListItem *curr = m_categories.value(n);
    QString c = curr == 0 ? n : curr->tag();

    ViewListItem *cat = m_viewlist.addCategory(c, n);
    if (cat == 0) {
        return false;
    }
    ViewBase *v = 0;
    int index = widget.insertAfter->currentIndex();
    int viewtype = widget.viewtype->currentIndex();
    switch (viewtype) {
        case 0: { // Report view
            v = m_view->createReportView(cat, m_viewtypes.value(viewtype), widget.viewname->text(), widget.tooltip->text(), index);
            static_cast<ReportView*>(v)->loadXML(m_data);
            break; }
        default:
            errorPlan<<"Unknown view type!";
            break;
    }
    Q_EMIT viewCreated(v);
    return true;
}

void AddReportsViewPanel::changed()
{
    bool disable = widget.viewname->text().isEmpty() | widget.viewtype->currentText().isEmpty() | widget.category->currentText().isEmpty();
    Q_EMIT enableButtonOk(! disable);
}
#endif

}  //KPlato namespace
