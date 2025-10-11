/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2011 Dag Andersen <dag.andersen@kdemail.net>
   SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
   
   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kptworkpackagemergedialog.h"
#include "kptpackage.h"
#include "kptproject.h"
#include "kpttask.h"
#include "kpttaskprogresspanel.h"
#include "kptcommand.h"
#include "kptduration.h"

#include <KMessageBox>
#include <KExtendableItemDelegate>
#include <KIO/CopyJob>

#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QList>
#include <QStyle>
#include <QStyleOption>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QCheckBox>

#include <KoIcon.h>

using namespace KPlato;


WorkPackageMergePanel::WorkPackageMergePanel(QWidget *parent)
    : QWidget(parent)
{
    setupUi(this);
}

WorkPackageMergeDialog::WorkPackageMergeDialog(Project *project, const QMap<QDateTime, Package*> &list, QWidget *parent)
    : KoDialog(parent)
    , m_project(project)
    , m_packages(list.values())
    , m_currentPackage(-1)
    , m_cmd(nullptr)
    , m_progressPanel(nullptr)
{
    setButtons(KoDialog::User1|KoDialog::User2);
    panel.ui_completionView->setRootIsDecorated(false);
    m_model = new QStandardItemModel(0, 4, panel.ui_completionView);
    m_model->setHeaderData(0, Qt::Horizontal, xi18nc("@title", "Date"));
    m_model->setHeaderData(1, Qt::Horizontal, xi18nc("@title", "Completion"));
    m_model->setHeaderData(2, Qt::Horizontal, xi18nc("@title", "Used Effort"));
    m_model->setHeaderData(3, Qt::Horizontal, xi18nc("@title", "Remaining Effort"));
    panel.ui_completionView->setModel(m_model);

    panel.ui_updateStarted->setToolTip(xi18nc("@info:tooltip", "Update started"));
    panel.ui_updateFinished->setToolTip(xi18nc("@info:tooltip", "Update finished"));

    setMainWidget(&panel);

    nextPackage();
}

WorkPackageMergeDialog::~WorkPackageMergeDialog()
{
    if (m_cmd) {
        m_cmd->undo();
        delete m_cmd;
    }
}

void WorkPackageMergeDialog::setPage(QWidget *page)
{
    disconnect(this, &KoDialog::user1Clicked, this, nullptr);
    disconnect(this, &KoDialog::user2Clicked, this, nullptr);
    panel.ui_stackedWidget->setCurrentWidget(page);
    if (page == panel.ui_packagePage) {
        setButtonText(KoDialog::User1, i18n("Merge"));
        setButtonIcon(KoDialog::User1, koIcon("go-next"));
        setButtonToolTip(KoDialog::User1, xi18nc("@info:tooltip", "Merge the changes and display the resulting progress information"));
        setButtonText(KoDialog::User2, i18n("Reject"));
        setButtonIcon(KoDialog::User2, koIcon("edit-delete"));
        setButtonToolTip(KoDialog::User2, xi18nc("@info:tooltip", "Reject this package and goto next package"));
        connect(this, &KoDialog::user1Clicked, this, &WorkPackageMergeDialog::slotMerge);
        connect(this, &KoDialog::user2Clicked, this, &WorkPackageMergeDialog::slotReject);
    } else if (page == panel.ui_progressPage) {
        setButtonText(KoDialog::User1, i18n("Next"));
        setButtonIcon(KoDialog::User1, koIcon("go-next"));
        setButtonToolTip(KoDialog::User1, xi18nc("@info:tooltip", "Commit the changes and go to next package"));
        setButtonText(KoDialog::User2, i18n("Back"));
        setButtonIcon(KoDialog::User2, koIcon("go-previous"));
        setButtonToolTip(KoDialog::User2, xi18nc("@info:tooltip", "Revert the changes and go back to the current package"));
        connect(this, &KoDialog::user1Clicked, this, &WorkPackageMergeDialog::slotApply);
        connect(this, &KoDialog::user2Clicked, this, &WorkPackageMergeDialog::slotBack);
    } else if (page == panel.ui_finishPage) {
        setButtons(KoDialog::Close);
    }
}

void WorkPackageMergeDialog::nextPackage()
{
    ++m_currentPackage;
    Package *package = m_packages.value(m_currentPackage);
    if (package) {
        panel.ui_taskName->setText(package->task->name());
        panel.ui_ownerName->setText(package->ownerName);
        panel.ui_packageTime->setText(QLocale().toString(package->timeTag, QLocale::ShortFormat));
        fillCompletionModel(package);
        setPage(panel.ui_packagePage);
    } else {
        gotoFinish();
    }
}

void WorkPackageMergeDialog::gotoFinish()
{
    setPage(panel.ui_finishPage);
}

void WorkPackageMergeDialog::gotoProgress()
{
    Package *package = m_packages.value(m_currentPackage);
    Q_ASSERT(package);
    if (package) {
        delete m_progressPanel;
        m_progressPanel = new TaskProgressPanel(*(package->toTask), nullptr, nullptr, panel.ui_stackedWidget);
        panel.ui_progressLayout->addWidget(m_progressPanel);
        setPage(panel.ui_progressPage);
    } else {
        gotoFinish();
    }
}

void WorkPackageMergeDialog::fillCompletionModel(Package *package)
{
    Q_ASSERT(package->project);
    Q_ASSERT(package->task);
    Q_ASSERT(package->toTask);
    QStandardItemModel *m = m_model;
    m->setRowCount(0);
    if (!package) {
        return;
    }
    const Completion &newCompletion = package->task->completion();
    const Resource *resource1 = nullptr;
    const QList<const Resource*> resources = newCompletion.resources();
    for(const Resource *r : resources) {
        if (r->id() == package->ownerId) {
            resource1 = r;
            break;
        }
    }
    Resource *resource2 = m_project->findResource(package->ownerId);
    Q_ASSERT(resource2);
    const Completion &currCompletion = package->toTask->completion();

    bool started = !currCompletion.isStarted() && newCompletion.isStarted();
    panel.ui_updateStarted->setChecked(started);
    if (newCompletion.isStarted()) {
        panel.ui_started->setText(QLocale().toString(newCompletion.startTime(), QLocale::ShortFormat));
    } else {
        panel.ui_started->setText(xi18nc("@info", "Task is not started"));
    }

    bool finished = !currCompletion.isFinished() && newCompletion.isFinished();
    panel.ui_updateFinished->setChecked(finished);
    if (newCompletion.isFinished()) {
        panel.ui_finished->setText(QLocale().toString(newCompletion.finishTime(), QLocale::ShortFormat));
    } else {
        panel.ui_finished->setText(xi18nc("@info", "Task is not finished"));
    }

    const Completion::EntryList &fromEntries = newCompletion.entries();
    Completion::EntryList::const_iterator it;
    for (it = fromEntries.constBegin(); it != fromEntries.constEnd(); ++it) {
        QList<QStandardItem*> lst;

        lst << new QStandardItem(QLocale().toString(it.key(), QLocale::ShortFormat));
        lst.last()->setData(it.key().toString(Qt::ISODate));
        lst.last()->setEditable(false);
        if (currCompletion.entries().contains(it.key())) {
            lst.last()->setToolTip(xi18nc("@info:tooltip", "Existing entry"));
        } else {
            lst.last()->setToolTip(xi18nc("@info:tooltip", "New entry"));
        }

        lst << new QStandardItem(QStringLiteral("%1% (%2%)").arg(it.value()->percentFinished).arg(currCompletion.percentFinished(it.key())));
        lst.last()->setData(it.value()->percentFinished);
        lst.last()->setToolTip(xi18nc("@info:tooltip", "New value: %1%<nl/>Current value: %2%", it.value()->percentFinished, currCompletion.percentFinished(it.key())));
        lst.last()->setCheckable(true);
        lst.last()->setEditable(false);
        if (package->settings.progress && it.value()->percentFinished != currCompletion.percentFinished(it.key())) {
            lst.last()->setCheckState(Qt::Checked);
        }

        Duration ne = newCompletion.actualEffort(resource1, it.key());
        Duration oe = currCompletion.actualEffort(resource2, it.key());
        QString nv = ne.toString(Duration::Format_Hour);
        QString ov = oe.toString(Duration::Format_Hour);
        lst << new QStandardItem(QStringLiteral("%1 (%2)").arg(nv, ov));
        lst.last()->setData(ne.toDouble());
        lst.last()->setToolTip(xi18nc("@info:tooltip", "New value: %1<nl/>Current value: %2", nv, ov));
        lst.last()->setCheckable(true);
        lst.last()->setEditable(false);
        if (package->settings.usedEffort && ne != oe) {
            lst.last()->setCheckState(Qt::Checked);
        }
        
        nv = it.value()->remainingEffort.toString(Duration::Format_Hour);
        ov = currCompletion.remainingEffort(it.key()).toString(Duration::Format_Hour);
        lst << new QStandardItem(QStringLiteral("%1 (%2)").arg(nv, ov));
        lst.last()->setData(it.value()->remainingEffort.toDouble());
        lst.last()->setToolTip(xi18nc("@info:tooltip", "New value: %1<nl/>Current value: %2", nv, ov));
        lst.last()->setCheckable(true);
        lst.last()->setEditable(false);
        if (package->settings.remainingEffort && it.value()->remainingEffort != currCompletion.remainingEffort(it.key())) {
            lst.last()->setCheckState(Qt::Checked);
        }

        m->appendRow(lst);
    }
    if (package->documents.isEmpty()) {
        panel.ui_documents->setText(i18n("No documents"));
        panel.ui_updateDocuments->setChecked(false);
    } else {
        panel.ui_documents->setText(i18n("Documents"));
        panel.ui_updateDocuments->setChecked(true);
        if (package->settings.documents) {
            panel.ui_updateDocuments->setChecked(true);
        }
    }
    for (int i = 0; i < m_model->columnCount(); ++i) {
        panel.ui_completionView->resizeColumnToContents(i);
    }
}

void WorkPackageMergeDialog::slotMerge()
{
    Package *package = m_packages.value(m_currentPackage);
    if (package) {
        acceptPackage(package);
    }
    gotoProgress();
}

void WorkPackageMergeDialog::slotReject()
{
    Package *package = m_packages.value(m_currentPackage);
    if (package) {
        // TODO: undo command
        WorkPackage *wp = new WorkPackage(package->task->workPackage());
        wp->setParentTask(package->toTask);
        wp->setTransmitionStatus(WorkPackage::TS_Rejected);
        package->toTask->addWorkPackage(wp);
        Q_EMIT terminateWorkPackage(package);
    }
    nextPackage();
}

void WorkPackageMergeDialog::slotApply()
{
    applyPackage();
    nextPackage();
}

void WorkPackageMergeDialog::slotBack()
{
    if (m_cmd) {
        m_cmd->undo();
        delete m_cmd;
        m_cmd = nullptr;
    }
    setPage(panel.ui_packagePage);
}

bool WorkPackageMergeDialog::updateStarted() const
{
    return panel.ui_updateStarted->isChecked();
}

bool WorkPackageMergeDialog::updateFinished() const
{
    return panel.ui_updateFinished->isChecked();
}

bool WorkPackageMergeDialog::updateEntry(int row) const
{
    return updateProgress(row) || updateUsedEffort(row) || updateRemainingEffort(row);
}

bool WorkPackageMergeDialog::updateProgress(int row) const
{
    return m_model->data(m_model->index(row, CompletionColumn), Qt::CheckStateRole).toBool();
}

bool WorkPackageMergeDialog::updateUsedEffort(int row) const
{
    return m_model->data(m_model->index(row, UsedEffortColumn), Qt::CheckStateRole).toBool();
}

bool WorkPackageMergeDialog::updateRemainingEffort(int row) const
{
    return m_model->data(m_model->index(row, RemainingEffortColumn), Qt::CheckStateRole).toBool();
}

bool WorkPackageMergeDialog::updateDocuments() const
{
    return panel.ui_updateDocuments->isChecked();
}

QDate WorkPackageMergeDialog::date(int row) const
{
    return m_model->data(m_model->index(row, DateColumn), Qt::UserRole+1).toDate();
}

int WorkPackageMergeDialog::completion(int row) const
{
    return m_model->data(m_model->index(row, CompletionColumn), Qt::UserRole+1).toInt();
}

Duration WorkPackageMergeDialog::usedEffort(int row) const
{
    return Duration(m_model->data(m_model->index(row, UsedEffortColumn), Qt::UserRole+1).toDouble());
}

Duration WorkPackageMergeDialog::remainingEffort(int row) const
{
    return Duration(m_model->data(m_model->index(row, RemainingEffortColumn), Qt::UserRole+1).toDouble());
}

void WorkPackageMergeDialog::acceptPackage(const Package *package)
{
    Task *to = package->toTask;
    const Task *from = package->task;

    Resource *resource = m_project->findResource(package->ownerId);
    if (resource == nullptr) {
        KMessageBox::error(nullptr, i18n("The package owner '%1' is not a resource in this project. You must handle this manually.", package->ownerName));
        return;
    }

    m_cmd = new MacroCommand(kundo2_noi18n(QStringLiteral("Merge workpackage")));
    Completion &org = to->completion();
    const Completion &curr = from->completion();

    if (updateStarted()) {
        if (org.isStarted() != curr.isStarted()) {
            m_cmd->addCommand(new ModifyCompletionStartedCmd(org, curr.isStarted()));
        }
        if (org.startTime() != curr.startTime()) {
            m_cmd->addCommand(new ModifyCompletionStartTimeCmd(org, curr.startTime()));
        }
    }
    if (updateFinished()) {
        if (org.isFinished() != curr.isFinished()) {
            m_cmd->addCommand(new ModifyCompletionFinishedCmd(org, curr.isFinished()));
        }
        if (org.finishTime() != curr.finishTime()) {
            m_cmd->addCommand(new ModifyCompletionFinishTimeCmd(org, curr.finishTime()));
        }
    }
    bool docsaved = false;
    if (updateDocuments()) {
        //TODO: handle remote files
        QMap<QString, QUrl>::const_iterator it = package->documents.constBegin();
        QMap<QString, QUrl>::const_iterator end = package->documents.constEnd();
        for (; it != end; ++it) {
            const QUrl src = QUrl::fromLocalFile(it.key());
            KIO::CopyJob *job = KIO::move(src, it.value(), KIO::Overwrite);
            if (job->exec()) {
                docsaved = true;
                //TODO: async
                debugPlan<<"Moved file:"<<src<<it.value();
            }
        }
    }
    for (int r = 0; r < m_model->rowCount(); ++r) {
        QDate d = date(r);
        Q_ASSERT(d.isValid());
        if (!d.isValid()) {
            continue;
        }
        if (updateEntry(r)) {
            Completion::Entry *e = nullptr;
            if (org.entries().contains(d)) {
                e = new Completion::Entry(*(org.entry(d)));
                m_cmd->addCommand(new RemoveCompletionEntryCmd(org, d));
            } else {
                e = new Completion::Entry();
            }
            if (updateProgress(r)) {
                e->percentFinished = completion(r);
            }
            if (updateRemainingEffort(r)) {
                e->remainingEffort = remainingEffort(r);
            }
            m_cmd->addCommand(new AddCompletionEntryCmd(org, d, e));
        }
        // actual effort per resource on date (but not entry)
        if (updateUsedEffort(r)) {
            m_cmd->addCommand(new AddCompletionActualEffortCmd(to, resource, d, Completion::UsedEffort::ActualEffort(usedEffort(r))));
        }
    }
    if (! docsaved && m_cmd->isEmpty()) {
        //KMessageBox::information(0, i18n("Nothing to save from this package"));
    }
    // add a copy to our tasks list of transmitted packages
    WorkPackage *wp = new WorkPackage(from->workPackage());
    wp->setParentTask(to);
    if (! wp->transmitionTime().isValid()) {
        wp->setTransmitionTime(package->timeTag);
    }
    wp->setTransmitionStatus(WorkPackage::TS_Receive);

    m_cmd->addCommand(new WorkPackageAddCmd(m_project, to, wp));
    m_cmd->redo();
}

void WorkPackageMergeDialog::applyPackage()
{
    MacroCommand *cmd = new MacroCommand(m_cmd->text());
    Q_EMIT executeCommand(cmd);
    cmd->addCommand(m_cmd);
    m_cmd = nullptr;
    // user might have made manual changes, must go after m_cmd
    MacroCommand *mc = m_progressPanel->buildCommand();
    if (mc) {
        mc->redo();
        cmd->addCommand(mc);
    }
    Q_EMIT terminateWorkPackage(m_packages.value(m_currentPackage));
}

