/* This file is part of the KDE project
 *  Copyright (C) 2019 Dag Andersen <dag.andersen@kdemail.net>
 *  Copyright (C) 2005 Dag Andersen <dag.andersen@kdemail.net>
 *  Copyright (C) 2006 Raphael Langerhorst <raphael.langerhorst@kdemail.net>
 * 
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 * 
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 * 
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#ifndef KPTGANTTVIEW_H
#define KPTGANTTVIEW_H

#include "planui_export.h"

#include "kptviewbase.h"
#include "kptitemviewsettup.h"
#include "kptnodeitemmodel.h"
#include "kptganttitemdelegate.h"

#include "ui_kptganttprintingoptions.h"
#include "ui_kptganttchartdisplayoptions.h"

#include <KGanttGlobal>
#include <KGanttView>
#include <KGanttDateTimeGrid>

class KoDocument;

class QPoint;
class QSplitter;
class QActionGroup;

class KoPrintJob;

namespace KGantt
{
    class TreeViewRowController;
}

namespace KPlato
{

class Node;
class MilestoneItemModel;
class GanttItemModel;
class ResourceAppointmentsGanttModel;
class Task;
class Project;
class Relation;
class ScheduleManager;
class MyKGanttView;
class GanttPrintingOptions;
class GanttViewBase;
class NodeGanttViewBase;
class GanttPrintingOptionsWidget;
class DateTimeTimeLine;

//---------------------------------------
class GanttChartDisplayOptionsPanel : public QWidget, public Ui::GanttChartDisplayOptions
{
    Q_OBJECT
public:
    explicit GanttChartDisplayOptionsPanel(GanttViewBase *gantt, GanttItemDelegate *delegate, QWidget *parent = 0);

    void setValues(const GanttItemDelegate &del);

public Q_SLOTS:
    void slotOk();
    void setDefault();

Q_SIGNALS:
    void changed();

private:
    GanttItemDelegate *m_delegate;
    GanttViewBase *m_gantt;
};

class GanttViewSettingsDialog : public ItemViewSettupDialog
{
    Q_OBJECT
public:
    explicit GanttViewSettingsDialog(GanttViewBase *gantt, GanttItemDelegate *delegate, ViewBase *view, bool selectPrint = false);

protected Q_SLOTS:
    void slotOk() override;

private:
    GanttViewBase *m_gantt;
    GanttPrintingOptionsWidget *m_printingoptions;
};

//--------------------
class GanttPrintingOptions
{
public:
    GanttPrintingOptions();

    bool loadContext(const KoXmlElement &settings);
    void saveContext(QDomElement &settings) const;

    bool printRowLabels;
    bool singlePage;
};

class PLANUI_EXPORT GanttPrintingOptionsWidget : public QWidget, public Ui::GanttPrintingOptionsWidget
{
    Q_OBJECT
public:
    explicit GanttPrintingOptionsWidget(QWidget *parent = 0);

    GanttPrintingOptions options() const;

    void setPrintRowLabels(bool value) { ui_printRowLabels->setChecked(value); }
    bool printRowLabels() const { return ui_printRowLabels->isChecked(); }

    void setSinglePage(bool value)  { value ? ui_singlePage->setChecked(false) : ui_multiplePages->setChecked(true); }
    bool singlePage() const { return ui_singlePage->isChecked(); }

public Q_SLOTS:
    void setOptions(const KPlato::GanttPrintingOptions &opt);
};

class GanttPrintingDialog : public PrintingDialog
{
    Q_OBJECT
public:
    GanttPrintingDialog(ViewBase *view, GanttViewBase *gantt);

    void startPrinting(RemovePolicy removePolicy) override;
    QList<QWidget*> createOptionWidgets() const override;
    void printPage(int page, QPainter &painter) override;

    int documentLastPage() const override;

protected Q_SLOTS:
    void slotPrintRowLabelsToogled(bool on);
    void slotSinglePageToogled(bool on);

protected:
    GanttViewBase *m_gantt;
    QRectF m_sceneRect;
    int m_horPages;
    int m_vertPages;
    double m_headerHeight;
    GanttPrintingOptionsWidget *m_options;
    QImage m_image;
};

class PLANUI_EXPORT GanttTreeView : public TreeViewBase
{
    Q_OBJECT
public:
    explicit GanttTreeView(QWidget *parent);

};

class GanttZoomWidget : public QSlider {
    Q_OBJECT
public:
    explicit GanttZoomWidget(QWidget *parent);

    void setGrid(KGantt::DateTimeGrid *grid);

    void setEnableHideOnLeave(bool hide);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private Q_SLOTS:
    void sliderValueChanged(int value);

private:
    bool m_hide;
    KGantt::DateTimeGrid *m_grid;
};

class PLANUI_EXPORT GanttViewBase : public KGantt::View
{
    Q_OBJECT
public:
    explicit GanttViewBase(QWidget *parent);
    ~GanttViewBase() override;

    GanttTreeView *treeView() const;
    GanttPrintingOptions printingOptions() const { return m_printOptions; }

    virtual bool loadContext(const KoXmlElement &settings);
    virtual void saveContext(QDomElement &settings) const;

    DateTimeTimeLine *timeLine() const;

    void editCopy();

    void handleContextMenuEvent(const QModelIndex &idx, const QPoint &pos);

public Q_SLOTS:
    void setPrintingOptions(const KPlato::GanttPrintingOptions &opt) { m_printOptions = opt; }

Q_SIGNALS:
    void contextMenuRequested(const QModelIndex &idx, const QPoint &pos);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

    friend class GanttPrintingDialog;
    GanttPrintingOptions m_printOptions;

private:
    QPoint m_dragStartPosition;
    Qt::MouseButton m_mouseButton;
    GanttZoomWidget *m_zoomwidget;
};

class NodeGanttViewBase : public GanttViewBase
{
    Q_OBJECT
public:
    explicit NodeGanttViewBase(QWidget *parent);
    ~NodeGanttViewBase() override;

    NodeSortFilterProxyModel *sfModel() const;
    void setItemModel(ItemModelBase *model);
    ItemModelBase *model() const;
    void setProject(Project *project);
    Project *project() const { return m_project; }

    GanttItemDelegate *delegate() const { return m_ganttdelegate; }

    bool loadContext(const KoXmlElement &settings) override;
    void saveContext(QDomElement &settings) const override;

public Q_SLOTS:
    void setShowUnscheduledTasks(bool show);

protected:
    Project *m_project;
    GanttItemDelegate *m_ganttdelegate;
    NodeItemModel m_defaultModel;
    KGantt::TreeViewRowController *m_rowController;
};

class PLANUI_EXPORT MyKGanttView : public NodeGanttViewBase
{
    Q_OBJECT
public:
    explicit MyKGanttView(QWidget *parent);

    GanttItemModel *model() const;
    void setProject(Project *project);
    void setScheduleManager(ScheduleManager *sm);

public Q_SLOTS:
    void clearDependencies();
    void createDependencies();
    void addDependency(KPlato::Relation *rel);
    void removeDependency(KPlato::Relation *rel);
    void slotProjectCalculated(KPlato::ScheduleManager *sm);

    void slotNodeInserted(KPlato::Node *node);

protected:
    ScheduleManager *m_manager;
};

class PLANUI_EXPORT GanttView : public ViewBase
{
    Q_OBJECT
public:
    GanttView(KoPart *part, KoDocument *doc, QWidget *parent, bool readWrite = true);

    //~GanttView();

    virtual void setZoom(double zoom);
    void setupGui();
    Project *project() const override { return m_gantt->project(); }
    void setProject(Project *project) override;

    using ViewBase::draw;
    void draw(Project &project) override;
    void drawChanges(Project &project) override;

    Node *currentNode() const override;

    void clear();

    bool loadContext(const KoXmlElement &context) override;
    void saveContext(QDomElement &context) const override;

    void updateReadWrite(bool on) override;

    KoPrintJob *createPrintJob() override;

    void setShowSpecialInfo(bool on) { m_gantt->model()->setShowSpecial(on); }
    bool showSpecialInfo() const { return m_gantt->model()->showSpecial(); }

Q_SIGNALS:
    void modifyRelation(KPlato::Relation *rel) ;
    void addRelation(KPlato::Node *par, KPlato::Node *child);
    void modifyRelation(KPlato::Relation *rel, int linkType) ;
    void addRelation(KPlato::Node *par, KPlato::Node *child, int linkType);

public Q_SLOTS:
    void setScheduleManager(KPlato::ScheduleManager *sm) override;
    void setShowResources(bool on);
    void setShowTaskName(bool on);
    void setShowTaskLinks(bool on);
    void setShowProgress(bool on);
    void setShowPositiveFloat(bool on);
    void setShowCriticalTasks(bool on);
    void setShowCriticalPath(bool on);
    void setShowNoInformation(bool on);
    void setShowAppointments(bool on);

    void slotEditCopy() override;

protected Q_SLOTS:
    void slotContextMenuRequested(const QModelIndex&, const QPoint &pos);
    void slotGanttHeaderContextMenuRequested(const QPoint &pt);
    void slotDateTimeGridChanged();
    void slotOptions() override;
    void slotOptionsFinished(int result) override;
    void ganttActions();
    void itemDoubleClicked(const QPersistentModelIndex &idx);

private:
    bool m_readWrite;
    int m_defaultFontSize;
    QSplitter *m_splitter;
    MyKGanttView *m_gantt;
    Project *m_project;

    QAction *actionShowProject;
    QAction *actionShowUnscheduled;
    QDomDocument m_domdoc;

    QActionGroup *m_scalegroup;
};

class MilestoneGanttViewSettingsDialog : public ItemViewSettupDialog
{
    Q_OBJECT
public:
    MilestoneGanttViewSettingsDialog(GanttViewBase *gantt, ViewBase *view, bool selectPrint = false);

protected Q_SLOTS:
    void slotOk() override;

private:
    GanttViewBase *m_gantt;
    GanttPrintingOptionsWidget *m_printingoptions;
};


class PLANUI_EXPORT MilestoneKGanttView : public NodeGanttViewBase
{
    Q_OBJECT
public:
    explicit MilestoneKGanttView(QWidget *parent);

    MilestoneItemModel *model() const;
    void setProject(Project *project);
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

class ResourceAppointmentsGanttViewSettingsDialog : public ItemViewSettupDialog
{
    Q_OBJECT
public:
    ResourceAppointmentsGanttViewSettingsDialog(GanttViewBase *gantt, ViewBase *view, bool selectPrint = false);

public Q_SLOTS:
    void slotOk() override;

private:
    GanttViewBase *m_gantt;
    GanttPrintingOptionsWidget *m_printingoptions;

};

class PLANUI_EXPORT ResourceAppointmentsGanttView : public ViewBase
{
    Q_OBJECT
public:
    ResourceAppointmentsGanttView(KoPart *part, KoDocument *doc, QWidget *parent, bool readWrite = true);
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
