/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2011 Dag Andersen <dag.andersen@kdemail.net>
   SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
   
   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef WORKPACKAGEMERGEDIALOG_H
#define WORKPACKAGEMERGEDIALOG_H

#include "planui_export.h"

#include "ui_kptworkpackagemergepanel.h"

#include <KoDialog.h>

#include <QDateTime>
#include <QMap>

class KExtendableItemDelegate;

class QStandardItemModel;

class KUndo2Command;

namespace KPlato
{

class Package;
class TaskProgressPanel;
class Project;
class Duration;
class MacroCommand;

class WorkPackageMergePanel : public QWidget, public Ui::WorkPackageMergePanel
{
    Q_OBJECT
public:
    explicit WorkPackageMergePanel(QWidget *parent = nullptr);
};

class PLANUI_EXPORT WorkPackageMergeDialog : public KoDialog
{
    Q_OBJECT
public:
    enum Columns { DateColumn = 0, CompletionColumn, UsedEffortColumn, RemainingEffortColumn };

    WorkPackageMergeDialog(Project *project, const QMap<QDateTime, Package*> &list, QWidget *parent = nullptr);
    ~WorkPackageMergeDialog() override;

    QList<int> checkedList() const;

Q_SIGNALS:
    void executeCommand(KUndo2Command *cmd);
    void terminateWorkPackage(const KPlato::Package *package);

protected Q_SLOTS:
    void slotMerge();
    void slotReject();
    void slotApply();
    void slotBack();

protected:
    void nextPackage();
    void gotoProgress();
    void gotoFinish();
    void fillCompletionModel(Package *package);
    void acceptPackage(const Package *package);
    void applyPackage();

    bool updateStarted() const;
    bool updateFinished() const;
    bool updateEntry(int row) const;
    bool updateProgress(int row) const;
    bool updateUsedEffort(int row) const;
    bool updateRemainingEffort(int row) const;
    bool updateDocuments() const;

    QDate date(int row) const;
    int completion(int row) const;
    Duration usedEffort(int row) const;
    Duration remainingEffort(int row) const;

    void setPage(QWidget *page);

private:
    Project *m_project;
    WorkPackageMergePanel panel;
    KExtendableItemDelegate *m_delegate;
    QList<Package*> m_packages;
    int m_currentPackage;
    QStandardItemModel *m_model;
    MacroCommand *m_cmd;
    TaskProgressPanel *m_progressPanel;
};

} // namespace KPlato

#endif
