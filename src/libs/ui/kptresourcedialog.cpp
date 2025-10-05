/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2003-2011, 2012 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kptresourcedialog.h"

#include "kptlocale.h"
#include "kptcommand.h"
#include "kptproject.h"
#include "kptresource.h"
#include "kptcalendar.h"
#include "ResourceItemSFModel.h"
#include "ResourceGroupItemModel.h"
#include "kptdebug.h"

#include <QList>
#include <QStringList>
#include <QSortFilterProxyModel>
#include <QStandardItem>

#ifdef PLAN_KDEPIMLIBS_FOUND
#include <akonadi/contact/emailaddressselectiondialog.h>
#include <akonadi/contact/emailaddressselectionwidget.h>
#include <akonadi/contact/emailaddressselection.h>
#endif


namespace KPlato
{

ResourceDialogImpl::ResourceDialogImpl(const Project &project, Resource &resource, bool baselined, QWidget *parent)
    : QWidget(parent),
    m_project(project),
    m_resource(resource)
{
    setupUi(this);

#ifndef PLAN_KDEPIMLIBS_FOUND
    chooseBtn->hide();
#endif

    // FIXME
    // [Bug 311940] New: Plan crashes when typing a text in the filter textbox before the textbook is fully loaded when selecting a contact from the addressbook
    chooseBtn->hide();

    QSortFilterProxyModel *pr = new QSortFilterProxyModel(ui_teamView);
    QStandardItemModel *m = new QStandardItemModel(ui_teamView);
    pr->setSourceModel(new QStandardItemModel(ui_teamView));
    ui_teamView->setModel(m);
    m->setHorizontalHeaderLabels(QStringList() << xi18nc("title:column", "Select team members") << xi18nc("title:column", "Groups"));
    const QList<Resource*> resources = m_project.resourceList();
    for (Resource *r : resources) {
        if (r->type() != Resource::Type_Work || r->id() == m_resource.id()) {
            continue;
        }
        QList<QStandardItem *> items;
        QStandardItem *item = new QStandardItem(r->name());
        item->setCheckable(true);
        item->setCheckState(m_resource.teamMemberIds().contains(r->id()) ? Qt::Checked : Qt::Unchecked);
        items << item;
        const auto groups = r->parentGroups();
        QStringList names;
        for (auto group : groups) {
            names << group->name();
        }
        item = new QStandardItem(names.join(QLatin1Char(',')));
        items << item;
        // Add id so we can find the resource
        item = new QStandardItem(r->id());
        items << item;
        m->appendRow(items);
    }
    if (baselined) {
        type->setEnabled(false);
        rateEdit->setEnabled(false);
        overtimeEdit->setEnabled(false);
        account->setEnabled(false);
    }
    // hide resource identity (last column)
    ui_teamView->setColumnHidden(m->columnCount() - 1, true);
    ui_teamView->resizeColumnToContents(0);
    ui_teamView->sortByColumn(0, Qt::AscendingOrder);
    slotTypeChanged(resource.type());
    connect(m, &QAbstractItemModel::dataChanged, this, &ResourceDialogImpl::slotTeamChanged);

    connect(type, SIGNAL(activated(int)), SLOT(slotTypeChanged(int)));
    connect(units, SIGNAL(valueChanged(int)), SLOT(slotChanged()));
    connect(nameEdit, &QLineEdit::textChanged, this, &ResourceDialogImpl::slotChanged);
    connect(initialsEdit, &QLineEdit::textChanged, this, &ResourceDialogImpl::slotChanged);
    connect(emailEdit, &QLineEdit::textChanged, this, &ResourceDialogImpl::slotChanged);

    connect(calendarList, SIGNAL(activated(int)), SLOT(slotChanged()));

    connect(rateEdit, &QLineEdit::textChanged, this, &ResourceDialogImpl::slotChanged);
    connect(overtimeEdit, &QLineEdit::textChanged, this, &ResourceDialogImpl::slotChanged);

    connect(chooseBtn, &QAbstractButton::clicked, this, &ResourceDialogImpl::slotChooseResource);

    connect(availableFrom, &QDateTimeEdit::dateTimeChanged, this, &ResourceDialogImpl::slotChanged);
    connect(availableUntil, &QDateTimeEdit::dateTimeChanged, this, &ResourceDialogImpl::slotChanged);
    connect(availableFrom, &QDateTimeEdit::dateTimeChanged, this, &ResourceDialogImpl::slotAvailableFromChanged);
    connect(availableUntil, &QDateTimeEdit::dateTimeChanged, this, &ResourceDialogImpl::slotAvailableUntilChanged);

    connect(ui_rbfrom, &QAbstractButton::toggled, this, &ResourceDialogImpl::slotChanged);
    connect(ui_rbuntil, &QAbstractButton::toggled, this, &ResourceDialogImpl::slotChanged);

    connect(ui_rbfrom, &QAbstractButton::toggled, availableFrom, &QWidget::setEnabled);
    connect(ui_rbuntil, &QAbstractButton::toggled, availableUntil, &QWidget::setEnabled);

    connect(useRequired, &QCheckBox::stateChanged, this, &ResourceDialogImpl::slotUseRequiredChanged);

    connect(account, SIGNAL(activated(int)), SLOT(slotChanged()));
}

void ResourceDialogImpl::slotTeamChanged(const QModelIndex &index) {
    if (! index.isValid()) {
        return;
    }
    bool checked = (bool)(index.data(Qt::CheckStateRole).toInt());
    int idCol = index.model()->columnCount() - 1;
    QString id = index.model()->index(index.row(), idCol).data().toString();
    if (checked) {
        if (! m_resource.teamMemberIds().contains(id)) {
            m_resource.addTeamMemberId(id);
        }
    } else {
        m_resource.removeTeamMemberId(id);
    }
    Q_EMIT changed();
}

void ResourceDialogImpl::slotTypeChanged(int index) {
    switch (index) {
        case Resource::Type_Work:
            ui_stackedWidget->setCurrentIndex(0);
            useRequired->setEnabled(true);
            slotUseRequiredChanged(useRequired->checkState());
            break;
        case Resource::Type_Material:
            ui_stackedWidget->setCurrentIndex(0);
            useRequired->setEnabled(false);
            slotUseRequiredChanged(false);
            break;
        case Resource::Type_Team:
            ui_stackedWidget->setCurrentIndex(1);
            break;
    }
    Q_EMIT changed();
}

void ResourceDialogImpl::slotChanged() {
    Q_EMIT changed();
}

void ResourceDialogImpl::setCurrentIndexes(const QModelIndexList &lst)
{
    m_currentIndexes.clear();
    for (const QModelIndex &idx : lst) {
        m_currentIndexes << QPersistentModelIndex(idx);
    }
    useRequired->setCheckState(m_currentIndexes.isEmpty() ? Qt::Unchecked : Qt::Checked);
    if (useRequired->isChecked()) {
        required->setCurrentIndexes(m_currentIndexes);
    }
    required->setEnabled(useRequired->isChecked());
}

void ResourceDialogImpl::slotUseRequiredChanged(int state)
{
    required->setEnabled(state);
    if (state) {
        required->setCurrentIndexes(m_currentIndexes);
    } else {
        m_currentIndexes = required->currentIndexes();
        required->setCurrentIndexes(QList<QPersistentModelIndex>());
    }
    slotChanged();
}

void ResourceDialogImpl::slotAvailableFromChanged(const QDateTime&) {
    if (availableUntil->dateTime() < availableFrom->dateTime()) {
        disconnect(availableUntil, &QDateTimeEdit::dateTimeChanged, this,  &ResourceDialogImpl::slotAvailableUntilChanged);
        //debugPlan<<"From:"<<availableFrom->dateTime().toString()<<" until="<<availableUntil->dateTime().toString();
        availableUntil->setDateTime(availableFrom->dateTime());
        connect(availableUntil, &QDateTimeEdit::dateTimeChanged, this, &ResourceDialogImpl::slotAvailableUntilChanged);
    }
}

void ResourceDialogImpl::slotAvailableUntilChanged(const QDateTime&) {
    if (availableFrom->dateTime() > availableUntil->dateTime()) {
        disconnect(availableFrom, &QDateTimeEdit::dateTimeChanged, this,  &ResourceDialogImpl::slotAvailableFromChanged);
        //debugPlan<<"Until:"<<availableUntil->dateTime().toString()<<" from="<<availableFrom->dateTime().toString();
        availableFrom->setDateTime(availableUntil->dateTime());
        connect(availableFrom, &QDateTimeEdit::dateTimeChanged, this, &ResourceDialogImpl::slotAvailableFromChanged);
    }
}

void ResourceDialogImpl::slotCalculationNeeded(const QString&) {
    Q_EMIT calculate();
    Q_EMIT changed();
}

void ResourceDialogImpl::slotChooseResource()
{
#ifdef PLAN_KDEPIMLIBS_FOUND
    QPointer<Akonadi::EmailAddressSelectionDialog> dlg = new Akonadi::EmailAddressSelectionDialog(this);
    if (dlg->exec() && dlg) {
        QStringList s;
        const Akonadi::EmailAddressSelection::List selections = dlg->selectedAddresses();
        if (! selections.isEmpty()) {
            const Akonadi::EmailAddressSelection s = selections.first();
            nameEdit->setText(s.name());
            emailEdit->setText(s.email());
            const QStringList l = s.name().split(' ');
            QString in;
            QStringList::ConstIterator it = l.begin();
            for (/*int i = 0*/; it != l.end(); ++it) {
                in += (*it)[0];
            }
            initialsEdit->setText(in);
        }
    }
#endif
}

//////////////////  ResourceDialog  ////////////////////////

ResourceDialog::ResourceDialog(Project &project, Resource *resource, QWidget *parent)
    : KoDialog(parent),
      m_project(project),
      m_original(resource),
      m_resource(resource),
      m_calculationNeeded(false)
{
    setCaption(i18n("Resource Settings"));
    setButtons(Ok|Cancel);
    setDefaultButton(Ok);
    showButtonSeparator(true);
    dia = new ResourceDialogImpl(project, m_resource, resource->isBaselined(), this);
    setMainWidget(dia);
    KoDialog::enableButtonOk(false);

    dia->nameEdit->setText(resource->name());
    dia->initialsEdit->setText(resource->initials());
    dia->emailEdit->setText(resource->email());
    dia->type->setCurrentIndex((int)resource->type()); // NOTE: must match enum
    dia->units->setValue(resource->units());
    DateTime dt = resource->availableFrom();
    if (dt.isValid()) {
        dia->ui_rbfrom->click();
    } else {
        dia->ui_rbfromunlimited->click();
    }
    dia->availableFrom->setDateTime(dt.isValid() ? dt : QDateTime(QDate::currentDate(), QTime(0, 0, 0), Qt::LocalTime));
    dia->availableFrom->setEnabled(dt.isValid());

    dt = resource->availableUntil();
    if (dt.isValid()) {
        dia->ui_rbuntil->click();
    } else {
        dia->ui_rbuntilunlimited->click();
    }
    dia->availableUntil->setDateTime(dt.isValid() ? dt : QDateTime(QDate::currentDate().addYears(2), QTime(0, 0, 0), Qt::LocalTime));
    dia->availableUntil->setEnabled(dt.isValid());
    dia->rateEdit->setText(project.locale()->formatMoney(resource->normalRate()));
    dia->overtimeEdit->setText(project.locale()->formatMoney(resource->overtimeRate()));

    int cal = 0;
    dia->calendarList->addItem(i18n("None"));
    m_calendars.insert(0, nullptr);
    const QList<Calendar*> list = project.allCalendars();
    int i=1;
    for (Calendar *c : list) {
        dia->calendarList->insertItem(i, c->name());
        m_calendars.insert(i, c);
        if (c == resource->calendar(true)) {
            cal = i;
         }
        ++i;
    }
    dia->calendarList->setCurrentIndex(cal);

    ResourceItemSFModel *m = new ResourceItemSFModel(this);
    m->setProject(&project);
    dia->required->setModel(m);
    dia->required->view()->expandAll();

    QItemSelectionModel *sm = dia->required->view()->selectionModel();
    const QList<Resource*> resources = resource->requiredResources();
    for (Resource *r : resources) {
        sm->select(m->index(r), QItemSelectionModel::Select | QItemSelectionModel::Rows);
    }
    dia->setCurrentIndexes(sm->selectedRows());

    QStringList lst;
    lst << i18n("None") << m_project.accounts().costElements();
    dia->account->addItems(lst);
    if (resource->account()) {
        dia->account->setCurrentIndex(lst.indexOf(resource->account()->name()));
    }
    connect(dia, SIGNAL(changed()), SLOT(enableButtonOk()));
    connect(dia, &ResourceDialogImpl::calculate, this, &ResourceDialog::slotCalculationNeeded);
    connect(dia->calendarList, SIGNAL(activated(int)), SLOT(slotCalendarChanged(int)));
    connect(dia->required, SIGNAL(changed()), SLOT(enableButtonOk()));
    connect(dia->account, &QComboBox::currentTextChanged, this, &ResourceDialog::slotAccountChanged);
    
    connect(&project, &Project::resourceToBeRemoved, this, &ResourceDialog::slotResourceRemoved);
}

void ResourceDialog::slotResourceRemoved(Project *project, int row, Resource *resource)
{
    Q_UNUSED(project)
    Q_UNUSED(row)
    if (m_original == resource) {
        reject();
    }
}

void ResourceDialog::enableButtonOk() {
		KoDialog::enableButtonOk(true);
}

void ResourceDialog::slotCalculationNeeded() {
    m_calculationNeeded = true;
}

void ResourceDialog::slotButtonClicked(int button) {
    if (button == KoDialog::Ok) {
        slotOk();
    } else {
        KoDialog::slotButtonClicked(button);
    }
}

void ResourceDialog::slotOk()
{
    m_resource.setName(dia->nameEdit->text());
    m_resource.setInitials(dia->initialsEdit->text());
    m_resource.setEmail(dia->emailEdit->text());
    m_resource.setType((Resource::Type)(dia->type->currentIndex()));
    m_resource.setUnits(dia->units->value());

    m_resource.setNormalRate(m_project.locale()->readMoney(dia->rateEdit->text()));
    m_resource.setOvertimeRate(m_project.locale()->readMoney(dia->overtimeEdit->text()));
    m_resource.setCalendar(m_calendars[dia->calendarList->currentIndex()]);
    m_resource.setAvailableFrom(dia->ui_rbfrom->isChecked() ? dia->availableFrom->dateTime() : QDateTime());
    m_resource.setAvailableUntil(dia->ui_rbuntil->isChecked() ? dia->availableUntil->dateTime() : QDateTime());
    ResourceItemSFModel *m = static_cast<ResourceItemSFModel*>(dia->required->model());
    QStringList lst;
    const auto indexes = dia->required->currentIndexes();
    for (const QPersistentModelIndex &i : indexes) {
        Resource *r = m->resource(i);
        if (r) lst << r->id();
    }
    m_resource.setRequiredIds(lst);
    accept();
}

void ResourceDialog::slotCalendarChanged(int /*cal*/) {

}

void ResourceDialog::slotAccountChanged(const QString &name)
{
    m_resource.setAccount(m_project.accounts().findAccount(name));
}

MacroCommand *ResourceDialog::buildCommand() {
    return buildCommand(m_original, m_resource);
}

// static
MacroCommand *ResourceDialog::buildCommand(Resource *original, Resource &resource) {
    MacroCommand *m=nullptr;
    KUndo2MagicString n = kundo2_i18n("Modify Resource");
    if (resource.name() != original->name()) {
        if (!m) m = new MacroCommand(n);
        m->addCommand(new ModifyResourceNameCmd(original, resource.name()));
    }
    if (resource.initials() != original->initials()) {
        if (!m) m = new MacroCommand(n);
        m->addCommand(new ModifyResourceInitialsCmd(original, resource.initials()));
    }
    if (resource.email() != original->email()) {
        if (!m) m = new MacroCommand(n);
        m->addCommand(new ModifyResourceEmailCmd(original, resource.email()));
    }
    if (resource.type() != original->type()) {
        if (!m) m = new MacroCommand(n);
        m->addCommand(new ModifyResourceTypeCmd(original, resource.type()));
    }
    if (resource.units() != original->units()) {
        if (!m) m = new MacroCommand(n);
        m->addCommand(new ModifyResourceUnitsCmd(original, resource.units()));
    }
    if (resource.availableFrom() != original->availableFrom()) {
        if (!m) m = new MacroCommand(n);
        m->addCommand(new ModifyResourceAvailableFromCmd(original, resource.availableFrom()));
    }
    if (resource.availableUntil() != original->availableUntil()) {
        if (!m) m = new MacroCommand(n);
        m->addCommand(new ModifyResourceAvailableUntilCmd(original, resource.availableUntil()));
    }
    if (resource.normalRate() != original->normalRate()) {
        if (!m) m = new MacroCommand(n);
        m->addCommand(new ModifyResourceNormalRateCmd(original, resource.normalRate()));
    }
    if (resource.overtimeRate() != original->overtimeRate()) {
        if (!m) m = new MacroCommand(n);
        m->addCommand(new ModifyResourceOvertimeRateCmd(original, resource.overtimeRate()));
    }
    if (resource.calendar(true) != original->calendar(true)) {
        if (!m) m = new MacroCommand(n);
        m->addCommand(new ModifyResourceCalendarCmd(original, resource.calendar(true)));
    }
    if (resource.requiredIds() != original->requiredIds()) {
        if (!m) m = new MacroCommand(n);
        m->addCommand(new ModifyRequiredResourcesCmd(original, resource.requiredIds()));
    }
    if (resource.account() != original->account()) {
        if (!m) m = new MacroCommand(n);
        m->addCommand(new ResourceModifyAccountCmd(*original, original->account(), resource.account()));
    }
    if (resource.type() == Resource::Type_Team) {
        //debugPlan<<original->teamMembers()<<resource.teamMembers();
        const QStringList ids = resource.teamMemberIds();
        for (const QString &id : ids) {
            if (! original->teamMemberIds().contains(id)) {
                if (!m) m = new MacroCommand(n);
                m->addCommand(new AddResourceTeamCmd(original, id));
            }
        }
        const QStringList ids2 = original->teamMemberIds();
        for (const QString &id : ids2) {
            if (! resource.teamMemberIds().contains(id)) {
                if (!m) m = new MacroCommand(n);
                m->addCommand(new RemoveResourceTeamCmd(original, id));
            }
        }
    }
    return m;
}

}  //KPlato namespace
