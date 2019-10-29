/* This file is part of the KDE project
   Copyright (C) 2011 Dag Andersen <danders@get2net.dk>
   Copyright (C) 2019 Dag Andersen <danders@get2net.dk>
   
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
   Boston, MA 02110-1301, USA.
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
    explicit WorkPackageMergePanel(QWidget *parent = 0);
};

class PLANUI_EXPORT WorkPackageMergeDialog : public KoDialog
{
    Q_OBJECT
public:
    enum Columns { DateColumn = 0, CompletionColumn, UsedEffortColumn, RemainingEffortColumn };

    WorkPackageMergeDialog(Project *project, const QMap<QDateTime, Package*> &list, QWidget *parent = 0);
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
