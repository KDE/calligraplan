/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2004-2007, 2011, 2012 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kptmainprojectpanel.h"
#include <commands/SetTaskModulesCommand.h>
#include <commands/SetFreedaysCalendarCmd.h>

#include "kptdebug.h"

#include <KLocalizedString>

#ifdef PLAN_KDEPIMLIBS_FOUND
#include <akonadi/contact/emailaddressselectiondialog.h>
#include <akonadi/contact/emailaddressselectionwidget.h>
#include <akonadi/contact/emailaddressselection.h>
#endif

#include "kptproject.h"
#include "kptcommand.h"
#include <ProjectModifyTimeZoneCmd.h>
#include "kptschedule.h"
#include "kpttaskdescriptiondialog.h"
#include "kptdocumentspanel.h"

#include <QFileDialog>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QItemSelectionModel>
#include <QFileDialog>
#include <QTimer>

namespace KPlato
{

MainProjectPanel::MainProjectPanel(Project &p, QWidget *parent)
    : QWidget(parent),
      project(p)
{
    setupUi(this);

#ifndef PLAN_KDEPIMLIBS_FOUND
    chooseLeader->hide();
#endif

    // FIXME
    // [Bug 311940] New: Plan crashes when typing a text in the filter textbox before the textbook is fully loaded when selecting a contact from the addressbook
    chooseLeader->hide();

    QString s = xi18nc("@info:whatsthis", "The Work Breakdown Structure introduces numbering for all tasks in the project, according to the task structure."
                       "<nl/>The WBS code is auto-generated."
                       "<nl/>You can define the WBS code pattern using the <interface>Project->Define WBS Pattern</interface> menu entry.");
    wbslabel->setWhatsThis(s);
    wbs->setWhatsThis(s);

    namefield->setText(project.name());
    leaderfield->setText(project.leader());
 //   useSharedResources->setEnabled(!project.isSharedResourcesLoaded());
    useSharedResources->setChecked(project.useSharedResources());
    resourcesFile->setText(project.sharedResourcesFile());
    const auto timezones = QTimeZone::availableTimeZoneIds();
    for (const auto &id : timezones) {
        ui_timezone->addItem(QString::fromUtf8(id));
    }
    ui_timezone->setCurrentText(QString::fromUtf8(p.timeZone().id()));

    const Project::WorkPackageInfo wpi = p.workPackageInfo();
    ui_CheckForWorkPackages->setChecked(wpi.checkForWorkPackages);
    ui_RetrieveUrl->setUrl(wpi.retrieveUrl);
    ui_DeleteFile->setChecked(wpi.deleteAfterRetrieval);
    ui_ArchiveFile->setChecked(wpi.archiveAfterRetrieval);
    ui_ArchiveUrl->setUrl(wpi.archiveUrl);
    ui_PublishUrl->setUrl(wpi.publishUrl);

    ui_RetrieveUrl->setMode(KFile::Directory);
    ui_ArchiveUrl->setMode(KFile::Directory);
    ui_PublishUrl->setMode(KFile::Directory);

    // Disable publish for now
    // FIXME: Enable when fully implemented
    ui_publishGroup->hide();

    m_documents = new DocumentsPanel(p, ui_documents);
    ui_documents->layout()->addWidget(m_documents);

    m_description = new TaskDescriptionPanel(p, ui_description);
    m_description->namefield->hide();
    m_description->namelabel->hide();
    ui_description->layout()->addWidget(m_description);

    wbs->setText(project.wbsCode());
    if (wbs->text().isEmpty()) {
        wbslabel->hide();
        wbs->hide();
    }

    DateTime st = project.constraintStartTime();
    DateTime et = project.constraintEndTime();
    startDate->setDate(st.date());
    startTime->setTime(QTime(st.time().hour(), st.time().minute(), 0));
    endDate->setDate(et.date());
    endTime->setTime(QTime(et.time().hour(), et.time().minute(), 0));
    enableDateTime();
    namefield->setFocus();

    useSharedResources->setToolTip(xi18nc("@info:tooltip", "Enables sharing resources with other projects"));
    useSharedResources->setWhatsThis(xi18nc("@info:whatsthis",
                                           "<title>Shared resources</title>"
                                           "Resources can be shared between projects"
                                           " to avoid overbooking resources across projects."
                                           " Shared resources must be defined in a separate file,"
                                           " and you must have at least read access to it."
                                           " The projects that share the resources must also be"
                                           " accessible by you."
                                           ));
    s = xi18nc("@info:tooltip", "File where shared resources are defined");
    resourcesFile->setToolTip(s);

    initTaskModules();

    freedays->disconnect();
    freedays->clear();
    freedays->addItem(i18nc("@item:inlistbox", "No freedays"));
    auto current = project.freedaysCalendar();
    int currentIndex = 0;
    const auto calendars = project.calendars();
    for (auto *c : calendars) {
        freedays->addItem(c->name(), c->id());
        if (c == current) {
            currentIndex = freedays->count() - 1;
        }
    }
    freedays->setCurrentIndex(currentIndex);
    connect(freedays, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
        auto box = qobject_cast<QComboBox*>(sender());
        project.setFreedaysCalendar(project.findCalendar(box->currentData().toString()));
    });

    // signals and slots connections
    connect(m_documents, &DocumentsPanel::changed, this, &MainProjectPanel::slotCheckAllFieldsFilled);
    connect(m_description, &TaskDescriptionPanelImpl::textChanged, this, &MainProjectPanel::slotCheckAllFieldsFilled);
    connect(endDate, &QDateTimeEdit::dateChanged, this, &MainProjectPanel::slotCheckAllFieldsFilled);
    connect(endTime, &QDateTimeEdit::timeChanged, this, &MainProjectPanel::slotCheckAllFieldsFilled);
    connect(startDate, &QDateTimeEdit::dateChanged, this, &MainProjectPanel::slotCheckAllFieldsFilled);
    connect(startTime, &QDateTimeEdit::timeChanged, this, &MainProjectPanel::slotCheckAllFieldsFilled);
    connect(namefield, &QLineEdit::textChanged, this, &MainProjectPanel::slotCheckAllFieldsFilled);
    connect(leaderfield, &QLineEdit::textChanged, this, &MainProjectPanel::slotCheckAllFieldsFilled);
    connect(useSharedResources, &QGroupBox::toggled, this, &MainProjectPanel::slotCheckAllFieldsFilled);
    connect(resourcesFile, &QLineEdit::textChanged, this, &MainProjectPanel::slotCheckAllFieldsFilled);
    connect(chooseLeader, &QAbstractButton::clicked, this, &MainProjectPanel::slotChooseLeader);

    connect(resourcesBrowseBtn, &QAbstractButton::clicked, this, &MainProjectPanel::openResourcesFile);

    connect(ui_CheckForWorkPackages, &QCheckBox::stateChanged, this, &MainProjectPanel::slotCheckAllFieldsFilled);
    connect(ui_RetrieveUrl, &KUrlRequester::textEdited, this, &MainProjectPanel::slotCheckAllFieldsFilled);
    connect(ui_RetrieveUrl, &KUrlRequester::urlSelected, this, &MainProjectPanel::slotCheckAllFieldsFilled);
    connect(ui_DeleteFile, &QRadioButton::toggled, this, &MainProjectPanel::slotCheckAllFieldsFilled);
    connect(ui_ArchiveFile, &QRadioButton::toggled, this, &MainProjectPanel::slotCheckAllFieldsFilled);
    connect(ui_ArchiveUrl, &KUrlRequester::textEdited, this, &MainProjectPanel::slotCheckAllFieldsFilled);
    connect(ui_ArchiveUrl, &KUrlRequester::urlSelected, this, &MainProjectPanel::slotCheckAllFieldsFilled);
    connect(ui_PublishUrl, &KUrlRequester::textEdited, this, &MainProjectPanel::slotCheckAllFieldsFilled);
    connect(ui_PublishUrl, &KUrlRequester::urlSelected, this, &MainProjectPanel::slotCheckAllFieldsFilled);

    // initiate ok button
    QTimer::singleShot(0, this, &MainProjectPanel::slotCheckAllFieldsFilled);
}

void MainProjectPanel::initTaskModules()
{
    QStandardItemModel *m = new QStandardItemModel(0, 1, ui_taskModulesView);
    const QList<QUrl> lst = project.taskModules(false);
    for (const QUrl &url : lst) {
        QStandardItem *item = new QStandardItem(url.toString());
        m->appendRow(item);
    }
    ui_taskModulesView->setModel(m);
    ui_useLocalModules->setChecked(project.useLocalTaskModules());
    connect(ui_insertModule, &QToolButton::clicked, this, &MainProjectPanel::insertTaskModuleClicked);
    connect(ui_removeModule, &QToolButton::clicked, this, &MainProjectPanel::removeTaskModuleClicked);
    connect(ui_taskModulesView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainProjectPanel::taskModulesSelectionChanged);
    connect(ui_useLocalModules, &QCheckBox::toggled, this, &MainProjectPanel::slotCheckAllFieldsFilled);
}

bool MainProjectPanel::ok() {
    if (useSharedResources->isChecked() && resourcesFile->text().isEmpty()) {
        return false;
    }
    return true;
}

MacroCommand *MainProjectPanel::buildCommand() {
    MacroCommand *m = nullptr;
    KUndo2MagicString c = kundo2_i18n("Modify main project");
    if (project.name() != namefield->text()) {
        if (!m) m = new MacroCommand(c);
        m->addCommand(new NodeModifyNameCmd(project, namefield->text()));
    }
    if (project.leader() != leaderfield->text()) {
        if (!m) m = new MacroCommand(c);
        m->addCommand(new NodeModifyLeaderCmd(project, leaderfield->text()));
    }
    if (startDateTime() != project.constraintStartTime()) {
        if (!m) m = new MacroCommand(c);
        m->addCommand(new ProjectModifyStartTimeCmd(project, startDateTime()));
    }
    if (endDateTime() != project.constraintEndTime()) {
        if (!m) m = new MacroCommand(c);
        m->addCommand(new ProjectModifyEndTimeCmd(project, endDateTime()));
    }
    if (QLatin1String(project.timeZone().id()) != ui_timezone->currentText()) {
        if (!m) m = new MacroCommand(c);
        m->addCommand(new ProjectModifyTimeZoneCmd(project, QTimeZone(ui_timezone->currentText().toLatin1())));
    }
    if (project.useSharedResources() != useSharedResources->isChecked()) {
        if (!m) m = new MacroCommand(c);
        m->addCommand(new UseSharedResourcesCmd(&project, useSharedResources->isChecked()));
    }
    if (project.sharedResourcesFile() != resourcesFile->text()) {
        if (!m) m = new MacroCommand(c);
        m->addCommand(new SharedResourcesFileCmd(&project, resourcesFile->text()));
    }
    auto calendar = project.findCalendar(freedays->currentData().toString());
    if (project.freedaysCalendar() != calendar) {
        if (!m) m = new MacroCommand(c);
        m->addCommand(new SetFreedaysCalendarCmd(&project, calendar));
    }
    MacroCommand *cmd = m_description->buildCommand();
    if (cmd) {
        if (!m) m = new MacroCommand(c);
        m->addCommand(cmd);
    }
    cmd = m_documents->buildCommand();
    if (cmd) {
        if (!m) m = new MacroCommand(c);
        m->addCommand(cmd);
    }
    cmd = buildTaskModulesCommand();
    if (cmd) {
        if (!m) m = new MacroCommand(c);
        m->addCommand(cmd);
    }

    Project::WorkPackageInfo wpi;
    wpi.checkForWorkPackages = ui_CheckForWorkPackages->isChecked();
    wpi.retrieveUrl = ui_RetrieveUrl->url();
    wpi.deleteAfterRetrieval = ui_DeleteFile->isChecked();
    wpi.archiveAfterRetrieval = ui_ArchiveFile->isChecked();
    wpi.archiveUrl = ui_ArchiveUrl->url();
    wpi.publishUrl = ui_PublishUrl->url();
    if (wpi != project.workPackageInfo()) {
        ProjectModifyWorkPackageInfoCmd *cmd = new ProjectModifyWorkPackageInfoCmd(project, wpi);
        if (!m) m = new MacroCommand(c);
        m->addCommand(cmd);
    }

    return m;
}

MacroCommand *MainProjectPanel::buildTaskModulesCommand()
{
    MacroCommand *cmd = new MacroCommand();
    QAbstractItemModel *m = ui_taskModulesView->model();
    QList<QUrl> lst;
    for (QModelIndex idx = m->index(0,0); idx.isValid(); idx = idx.sibling(idx.row()+1, 0)) {
        QUrl url = QUrl::fromUserInput(idx.data().toString());
        if (url.isValid() && !lst.contains(url)) {
            lst << url;
        }
    }
    cmd->addCommand(new SetTaskModulesCommand(&project, lst, ui_useLocalModules->isChecked()));
    return cmd;
}

void MainProjectPanel::slotCheckAllFieldsFilled()
{
    Q_EMIT changed();
    bool state = !namefield->text().isEmpty();
    if (state && useSharedResources->isChecked()) {
        state = !resourcesFile->text().isEmpty();
    }
    Q_EMIT obligatedFieldsFilled(state);
}


void MainProjectPanel::slotChooseLeader()
{
#ifdef PLAN_KDEPIMLIBS_FOUND
    QPointer<Akonadi::EmailAddressSelectionDialog> dlg = new Akonadi::EmailAddressSelectionDialog(this);
    if (dlg->exec() && dlg) {
        QStringList names;
        const Akonadi::EmailAddressSelection::List selections = dlg->selectedAddresses();
        for (const Akonadi::EmailAddressSelection &selection : selections) {
            QString s = selection.name();
            if (! selection.email().isEmpty()) {
                if (! selection.name().isEmpty()) {
                    s += " <";
                }
                s += selection.email();
                if (! selection.name().isEmpty()) {
                    s += '>';
                }
                if (! s.isEmpty()) {
                    names << s;
                }
            }
        }
        if (! names.isEmpty()) {
            leaderfield->setText(names.join(", "));
        }
    }
#endif
}


void MainProjectPanel::slotStartDateClicked()
{
    enableDateTime();
}


void MainProjectPanel::slotEndDateClicked()
{
    enableDateTime();
}



void MainProjectPanel::enableDateTime()
{
    debugPlan;
    startTime->setEnabled(true);
    startDate->setEnabled(true);
    endTime->setEnabled(true);
    endDate->setEnabled(true);
}


QDateTime MainProjectPanel::startDateTime()
{
    return QDateTime(startDate->date(), startTime->time(), Qt::LocalTime);
}


QDateTime MainProjectPanel::endDateTime()
{
    return QDateTime(endDate->date(), endTime->time(), Qt::LocalTime);
}

void MainProjectPanel::openResourcesFile()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Resources"), QStringLiteral(""), tr("Resources file (*.plan)"));
    resourcesFile->setText(fileName);
}

bool MainProjectPanel::loadSharedResources() const
{
    return useSharedResources->isChecked();
}

void MainProjectPanel::insertTaskModuleClicked()
{
    QString dirName = QFileDialog::getExistingDirectory(this, i18n("Task Modules Path"));
    if (!dirName.isEmpty()) {
        dirName = QUrl::fromUserInput(dirName).toString();
        QStandardItemModel *m = static_cast<QStandardItemModel*>(ui_taskModulesView->model());
        if (m->rowCount() == 0) {
            QStandardItem *item = new QStandardItem(dirName);
            m->appendRow(item);
            slotCheckAllFieldsFilled();
        } else {
            for (int r = 0; r < m->rowCount(); ++r) {
                QUrl url1(dirName);
                QUrl url2 = QUrl::fromUserInput(m->index(r, 0).data().toString());
                if (url1.matches(url2, QUrl::StripTrailingSlash|QUrl::NormalizePathSegments)) {
                    break;
                }
                QStandardItem *item = new QStandardItem(dirName);
                m->appendRow(item);
                slotCheckAllFieldsFilled();
            }
        }
    }
}

void MainProjectPanel::removeTaskModuleClicked()
{
    const QList<QModelIndex> lst = ui_taskModulesView->selectionModel()->selectedRows();
    for (const QModelIndex &idx : lst) {
        ui_taskModulesView->model()->removeRow(idx.row(), idx.parent());
    }
    if (!lst.isEmpty()) {
        slotCheckAllFieldsFilled();
    }
}

void MainProjectPanel::taskModulesSelectionChanged()
{

}

}  //KPlato namespace
