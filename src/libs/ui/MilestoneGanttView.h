/* This file is part of the KDE project
 *  SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 *  SPDX-FileCopyrightText: 2005 Dag Andersen <dag.andersen@kdemail.net>
 *  SPDX-FileCopyrightText: 2006 Raphael Langerhorst <raphael.langerhorst@kdemail.net>
 * 
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef MilestoneGanttView_H
#define MilestoneGanttView_H

#include "planui_export.h"

#include "kptviewbase.h"
#include "NodeGanttViewBase.h"

#include "kptitemviewsettup.h"
#include "kptnodeitemmodel.h"
#include "ui_MilestoneGanttChartOptionsPanel.h"

class KoDocument;

namespace KPlato
{

class MilestoneGanttChartOptionsPanel : public QWidget
{
    Q_OBJECT
public:
    MilestoneGanttChartOptionsPanel(NodeGanttViewBase *gantt, QWidget *parent = nullptr);

    void setValues(const GanttItemDelegate &del);

public Q_SLOTS:
    void slotOk();
    void setDefault();

private:
    Ui::MilestoneGanttChartOptionsPanel ui;
    NodeGanttViewBase *m_gantt;
};

class MilestoneGanttViewSettingsDialog : public ItemViewSettupDialog
{
    Q_OBJECT
public:
    MilestoneGanttViewSettingsDialog(NodeGanttViewBase *gantt, ViewBase *view, bool selectPrint = false);

protected Q_SLOTS:
    void slotOk() override;

private:
    GanttViewBase *m_gantt;
    MilestoneGanttChartOptionsPanel *m_chartOptions;
};


class MilestoneKGanttView : public NodeGanttViewBase
{
    Q_OBJECT
public:
    explicit MilestoneKGanttView(QWidget *parent);

    MilestoneItemModel *model() const;
    void setProject(Project *project) override;
    void setScheduleManager(ScheduleManager *sm);

public Q_SLOTS:
    void slotProjectCalculated(KPlato::ScheduleManager *sm);

protected:
    ScheduleManager *m_manager;
};

class PLANUI_EXPORT MilestoneGanttView : public ViewBase
{
    Q_OBJECT
public:
    MilestoneGanttView(KoPart *part, KoDocument *doc, QWidget *parent, bool readWrite = true);

    virtual void setZoom(double zoom);
    void show();
    void setProject(Project *project) override;
    Project *project() const override { return m_gantt->project(); }
    using ViewBase::draw;
    void draw(Project &project) override;
    void drawChanges(Project &project) override;

    void setupGui();

    Node *currentNode() const override;

    void clear();

    bool loadContext(const KoXmlElement &context) override;
    void saveContext(QDomElement &context) const override;

    void updateReadWrite(bool on) override;

    bool showNoInformation() const { return m_showNoInformation; }

    KoPrintJob *createPrintJob() override;

public Q_SLOTS:
    void setScheduleManager(KPlato::ScheduleManager *sm) override;

    void setShowTaskName(bool on) { m_showTaskName = on; }
    void setShowProgress(bool on) { m_showProgress = on; }
    void setShowPositiveFloat(bool on) { m_showPositiveFloat = on; }
    void setShowCriticalTasks(bool on) { m_showCriticalTasks = on; }
    void setShowNoInformation(bool on) { m_showNoInformation = on; }

    void slotEditCopy() override;

protected Q_SLOTS:
    void slotContextMenuRequested(const QModelIndex&, const QPoint &pos);
    void slotGanttHeaderContextMenuRequested(const QPoint &pt);
    void slotDateTimeGridChanged();
    void slotOptions() override;
    void ganttActions();
    void itemDoubleClicked(const QPersistentModelIndex &idx);

private Q_SLOTS:
    void slotOpenCurrentNode();
    void slotOpenNode(KPlato::Node *node);
    void slotTaskProgress();
    void slotOpenProjectDescription();
    void slotTaskDescription();
    void slotOpenTaskDescription(bool);
    void slotDocuments();

    void slotTaskEditFinished(int result);
    void slotSummaryTaskEditFinished(int result);
    void slotTaskProgressFinished(int result);
    void slotMilestoneProgressFinished(int result);
    void slotTaskDescriptionFinished(int result);
    void slotDocumentsFinished(int result);

    void slotSelectionChanged();

private:
    void updateActionsEnabled(bool on);

private:
    bool m_readWrite;
    int m_defaultFontSize;
    QSplitter *m_splitter;
    MilestoneKGanttView *m_gantt;
    bool m_showTaskName;
    bool m_showProgress;
    bool m_showPositiveFloat;
    bool m_showCriticalTasks;
    bool m_showNoInformation;
    Project *m_project;
    QActionGroup *m_scalegroup;
};

}  //KPlato namespace

#endif
