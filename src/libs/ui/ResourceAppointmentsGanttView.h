/* This file is part of the KDE project
 *  SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 *  SPDX-FileCopyrightText: 2005 Dag Andersen <dag.andersen@kdemail.net>
 *  SPDX-FileCopyrightText: 2006 Raphael Langerhorst <raphael.langerhorst@kdemail.net>
 * 
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef ResourceAppointmentsGanttView_H
#define ResourceAppointmentsGanttView_H

#include "planui_export.h"

#include "kptviewbase.h"
#include "kptitemviewsettup.h"
#include "NodeGanttViewBase.h"
#include "ui_ResourceAppointmentsGanttChartOptionsPanel.h"

#include <KGanttGlobal>
#include <KGanttView>
#include <KGanttDateTimeGrid>
#include <KGanttPrintingContext>

class KoDocument;

namespace KPlato
{

class ResourceAppointmentsGanttModel;

class ResourceAppointmentsGanttChartOptionsPanel : public QWidget
{
    Q_OBJECT
public:
    explicit ResourceAppointmentsGanttChartOptionsPanel(GanttViewBase *gantt, QWidget *parent = nullptr);

    void setValues();

public Q_SLOTS:
    void slotOk();
    void setDefault();

private:
    Ui::ResourceAppointmentsGanttChartOptionsPanel ui;
    GanttViewBase *m_gantt;
};

class ResourceAppointmentsGanttViewSettingsDialog : public ItemViewSettupDialog
{
    Q_OBJECT
public:
    explicit ResourceAppointmentsGanttViewSettingsDialog(GanttViewBase *gantt, ViewBase *view, bool selectPrint = false);

public Q_SLOTS:
    void slotOk() override;

private:
    void createPrintingOptions(bool setAsCurrent);

    GanttViewBase *m_gantt;
    ResourceAppointmentsGanttChartOptionsPanel *m_chartOptions;
    GanttPrintingOptionsWidget *m_printingOptions;
};

class PLANUI_EXPORT ResourceAppointmentsGanttView : public ViewBase
{
    Q_OBJECT
public:
    explicit ResourceAppointmentsGanttView(KoPart *part, KoDocument *doc, QWidget *parent, bool readWrite = true);
    ~ResourceAppointmentsGanttView() override;

    virtual void setZoom(double zoom);
    void setProject(Project *project) override;
    Project *project() const override;

    void setupGui();

    bool loadContext(const KoXmlElement &context) override;
    void saveContext(QDomElement &context) const override;

    void updateReadWrite(bool on) override;

    KoPrintJob *createPrintJob() override;

    GanttTreeView *treeView() const { return static_cast<GanttTreeView*>(m_gantt->leftView()); }

    Node *currentNode() const override;

Q_SIGNALS:
    void itemDoubleClicked();

public Q_SLOTS:
    void setScheduleManager(KPlato::ScheduleManager *sm) override;
    void slotEditCopy() override;

protected Q_SLOTS:
    void slotContextMenuRequested(const QModelIndex&, const QPoint &pos);
    void slotContextMenuRequestedFromGantt(const QModelIndex&, const QPoint &pos);
    void slotGanttHeaderContextMenuRequested(const QPoint &pt);
    void slotDateTimeGridChanged();
    void slotOptions() override;
    void ganttActions();

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
    GanttViewBase *m_gantt;
    Project *m_project;
    ResourceAppointmentsGanttModel *m_model;
    KGantt::TreeViewRowController *m_rowController;
    QDomDocument m_domdoc;
    QActionGroup *m_scalegroup;
};

}  //KPlato namespace

#endif
