/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2007-2010 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PROJECTSTATUSVIEW_H
#define PROJECTSTATUSVIEW_H

#include "planui_export.h"

#include "PerformanceStatusBase.h"

#include "kptitemmodelbase.h"

#include "kptviewbase.h"
#include "kptitemviewsettup.h"
#include "kptnodechartmodel.h"

#include <QSplitter>

#include <KChartBarDiagram>


class QItemSelection;

class KoDocument;
class KoPageLayoutWidget;
class PrintingHeaderFooter;

namespace KChart
{
    class CartesianCoordinatePlane;
    class CartesianAxis;
    class Legend;
}

namespace KPlato
{

class Project;
class Node;
class ScheduleManager;
class TaskStatusItemModel;
class NodeItemModel;
class PerformanceStatusBase;

class PLANUI_EXPORT ProjectStatusView : public ViewBase
{
    Q_OBJECT
public:
    ProjectStatusView(KoPart *part, KoDocument *doc, QWidget *parent);

    void setupGui();
    Project *project() const override { return m_project; }

    /// Loads context info into this view. Reimplement.
    bool loadContext(const KoXmlElement &/*context*/) override;
    /// Save context info from this view. Reimplement.
    void saveContext(QDomElement &/*context*/) const override;

    KoPrintJob *createPrintJob() override;

public Q_SLOTS:
    /// Activate/deactivate the gui
    void setGuiActive(bool activate) override;

    void setProject(KPlato::Project *project) override;
    void setScheduleManager(KPlato::ScheduleManager *sm) override;
    void slotEditCopy() override;

protected:
    void updateActionsEnabled(bool on);
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

protected Q_SLOTS:
    void slotOptions() override;

private:
    Project *m_project;
    PerformanceStatusBase *m_view;
    QPoint m_dragStartPosition;
};

//--------------------------------------
class ProjectStatusViewSettingsDialog : public KPageDialog
{
    Q_OBJECT
public:
    explicit ProjectStatusViewSettingsDialog(ViewBase *base, PerformanceStatusBase *view, QWidget *parent = nullptr, bool selectPrint = false);

protected Q_SLOTS:
    void slotOk();

private:
    ViewBase *m_base;
    KoPageLayoutWidget *m_pagelayout;
    PrintingHeaderFooter *m_headerfooter;
};



} //namespace KPlato


#endif
