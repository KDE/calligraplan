/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2005-2006, 2012 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kptaccountsview.h"

#include "kptaccountsviewconfigdialog.h"
#include "kptdatetime.h"
#include "kptproject.h"
#include "kpteffortcostmap.h"
#include "kptaccountsmodel.h"
#include "kptdebug.h"

#include <KoDocument.h>
#include <KoXmlReader.h>
#include <KoIcon.h>

#include <QVBoxLayout>
#include <QPrinter>
#include <QPrintDialog>
#include <QHeaderView>
#include <QMenu>


namespace KPlato
{

AccountsTreeView::AccountsTreeView(QWidget *parent)
    : DoubleTreeViewBase(parent)
{
    debugPlan<<"---------------"<<this<<"------------------";
    setDragPixmap(koIcon("account").pixmap(32));

    setSelectionMode(QAbstractItemView::ExtendedSelection);

    CostBreakdownItemModel *m = new CostBreakdownItemModel(this);
    setModel(m);
    
    QHeaderView *v = m_leftview->header();
    v->setStretchLastSection(false);
    v->setSectionResizeMode(CostBreakdownItemModel::Description, QHeaderView::Stretch);
    v->setSectionResizeMode (CostBreakdownItemModel::Total, QHeaderView::ResizeToContents);
    
    v = m_rightview->header();
    v->setSectionResizeMode (QHeaderView::ResizeToContents);
    v->setStretchLastSection(false);

    m_leftHidden = QList<int>() << CostBreakdownItemModel::Planned << CostBreakdownItemModel::Actual << -1;
    slotModelReset();
    
    connect(m, &QAbstractItemModel::modelReset, this, &AccountsTreeView::slotModelReset);

    setWhatsThis(
              xi18nc("@info:whatsthis",
                     "<title>Cost Breakdown View</title>"
                     "<para>"
                     "Displays aggregated total cost as well as cost distribution over time."
                     "</para><para>"
                     "This view supports configuration and printing using the context menu."
                     "<nl/><link url='%1'>More...</link>"
                     "</para>", QStringLiteral("plan:cost-breakdown-view")));
}

void AccountsTreeView::slotModelReset()
{
    hideColumns(m_leftHidden);
    QHeaderView *v = m_leftview->header();
    debugPlan<<v->sectionSize(2)<<v->sectionSizeHint(2)<<v->defaultSectionSize()<<v->minimumSectionSize();

    CostBreakdownItemModel *m = static_cast<CostBreakdownItemModel*>(model());
    QList<int> cols;
    for (int c = 0; c < m->propertyCount(); ++c) {
        cols << c;
    }
    hideColumns(m_rightview,  cols);
}

CostBreakdownItemModel *AccountsTreeView::model() const
{
    return static_cast<CostBreakdownItemModel*>(DoubleTreeViewBase::model());
}

bool AccountsTreeView::cumulative() const
{
    return model()->cumulative();
}

void AccountsTreeView::setCumulative(bool on)
{
    model()->setCumulative(on);
}

int AccountsTreeView::periodType() const
{
    return model()->periodType();
}
    
void AccountsTreeView::setPeriodType(int period)
{
    model()->setPeriodType(period);
}

int AccountsTreeView::startMode() const
{
    return model()->startMode();
}

void AccountsTreeView::setStartMode(int mode)
{
    model()->setStartMode(mode);
}

int AccountsTreeView::endMode() const
{
    return model()->endMode();
}

void AccountsTreeView::setEndMode(int mode)
{
    model()->setEndMode(mode);
}

QDate AccountsTreeView::startDate() const
{
    return model()->startDate();
}

void AccountsTreeView::setStartDate(const QDate &date)
{
    model()->setStartDate(date);
}

QDate AccountsTreeView::endDate() const
{
    return model()->endDate();
}

void AccountsTreeView::setEndDate(const QDate &date)
{
    model()->setEndDate(date);
}

int AccountsTreeView::showMode() const
{
    return model()->showMode();
}
void AccountsTreeView::setShowMode(int show)
{
    model()->setShowMode(show);
}

//------------------------
AccountsView::AccountsView(KoPart *part, Project *project, KoDocument *doc, QWidget *parent)
    : ViewBase(part, doc, parent),
        m_project(project),
        m_manager(nullptr)
{
    setXMLFile(QStringLiteral("AccountsViewUi.rc"));
    init();

    setupGui();

    m_view->setDragDropMode(QAbstractItemView::DragOnly);
    m_view->setDropIndicatorShown(false);
    m_view->setDragEnabled (true);
    m_view->setAcceptDrops(false);
    m_view->setAcceptDropsOnView(false);

    connect(this, &ViewBase::expandAll, m_view, &DoubleTreeViewBase::slotExpand);
    connect(this, &ViewBase::collapseAll, m_view, &DoubleTreeViewBase::slotCollapse);

    connect(m_view, &DoubleTreeViewBase::contextMenuRequested, this, &AccountsView::slotContextMenuRequested);
    
    connect(m_view, SIGNAL(headerContextMenuRequested(QPoint)), SLOT(slotHeaderContextMenuRequested(QPoint)));
}

void AccountsView::setZoom(double zoom)
{
    Q_UNUSED(zoom);
}

void AccountsView::init()
{
    QVBoxLayout *l = new QVBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);
    m_view = new AccountsTreeView(this);
    l->addWidget(m_view);
    setProject(m_project);
}

void AccountsView::setupGui()
{
    createOptionActions(ViewBase::OptionAll);
}

void AccountsView::slotContextMenuRequested(const QModelIndex &index, const QPoint &pos)
{
    debugPlan;
    m_view->setContextMenuIndex(index);
    slotHeaderContextMenuRequested(pos);
    m_view->setContextMenuIndex(QModelIndex());
}

void AccountsView::slotHeaderContextMenuRequested(const QPoint &pos)
{
    debugPlan;
    QList<QAction*> lst = contextActionList();
    if (! lst.isEmpty()) {
        QMenu::exec(lst, pos,  lst.first());
    }
}

void AccountsView::slotEditCopy()
{
    debugPlan;
    m_view->editCopy();
}


void AccountsView::slotOptions()
{
    debugPlan;
    AccountsviewConfigDialog *dlg = new AccountsviewConfigDialog(this, m_view, this, sender()->objectName() == QStringLiteral("print_options"));
    connect(dlg, SIGNAL(finished(int)), SLOT(slotOptionsFinished(int)));
    dlg->open();
}

void AccountsView::setProject(Project *project)
{
    model()->setProject(project);
    m_project = project;
}

void AccountsView::setScheduleManager(ScheduleManager *sm)
{
    if (!sm && m_manager) {
        // we should only get here if the only schedule manager is scheduled,
        // or when last schedule manager is deleted
        m_domdoc.clear();
        QDomElement element = m_domdoc.createElement(QStringLiteral("expanded"));
        m_domdoc.appendChild(element);
        m_view->masterView()->saveExpanded(element);
    }
    bool tryexpand = sm && !m_manager;
    bool expand = sm && m_manager && sm != m_manager;
    QDomDocument doc;
    if (expand) {
        QDomElement element = doc.createElement(QStringLiteral("expanded"));
        doc.appendChild(element);
        m_view->masterView()->saveExpanded(element);
    }
    m_manager = sm;
    model()->setScheduleManager(sm);

    if (expand) {
        m_view->masterView()->doExpand(doc);
    } else if (tryexpand) {
        m_view->masterView()->doExpand(m_domdoc);
    }
}

CostBreakdownItemModel *AccountsView::model() const
{
    return static_cast<CostBreakdownItemModel*>(m_view->model());
}


#if 0
void AccountsView::print(QPrinter &printer, QPrintDialog &printDialog)
{
    //debugPlan;
    uint top, left, bottom, right;
    printer.margins(&top, &left, &bottom, &right);
    //debugPlan<<m.width()<<"x"<<m.height()<<" :"<<top<<","<<left<<","<<bottom<<","<<right<<" :"<<size();
    QPainter p;
    p.begin(&printer);
    p.setViewport(left, top, printer.width() - left - right, printer.height() - top - bottom);
    p.setClipRect(left, top, printer.width() - left - right, printer.height() - top - bottom);
    QRect preg = p.clipRegion().boundingRect();
    //debugPlan<<"p="<<preg;
    //p.drawRect(preg.x(), preg.y(), preg.width()-1, preg.height()-1);
    double scale = qMin((double) preg.width() / (double) size().width(), (double) preg.height() / (double) (size().height()));
    //debugPlan<<"scale="<<scale;
    if (scale < 1.0) {
        p.scale(scale, scale);
}
    QPixmap labelPixmap = QPixmap::grabWidget(m_label);
    p.drawPixmap(m_label->pos(), labelPixmap);
    p.translate(0, m_label->size().height());
    m_dlv->paintContents(&p);
    p.end();
}
#endif

bool AccountsView::loadContext(const KoXmlElement &context)
{
    //debugPlan;
    ViewBase::loadContext(context);

    m_view->setShowMode(context.attribute("show-mode").toInt());
    m_view->setCumulative((bool)(context.attribute("cumulative").toInt()));
    m_view->setPeriodType(context.attribute("period-type", QString::number(0)).toInt());
    m_view->setStartDate(QDate::fromString(context.attribute("start-date", QString()), Qt::ISODate));
    m_view->setStartMode(context.attribute("start-mode", QString::number(0)).toInt());
    m_view->setEndDate(QDate::fromString(context.attribute("end-date", QString()), Qt::ISODate));
    m_view->setEndMode(context.attribute("end-mode", QString::number(0)).toInt());
    
    //debugPlan<<m_view->startMode()<<m_view->startDate()<<m_view->endMode()<<m_view->endDate();
    // Skip context loading, it doea not work for this type of view
    // m_view->masterView()->setObjectName("AccountsView");
    // m_view->loadContext(model()->columnMap(), context);
    return true;
}

void AccountsView::saveContext(QDomElement &context) const
{
    //debugPlan;
    ViewBase::saveContext(context);

    context.setAttribute(QStringLiteral("show-mode"), QString::number(m_view->showMode()));
    context.setAttribute(QStringLiteral("cumulative"), QString::number(m_view->cumulative()));
    context.setAttribute(QStringLiteral("period-type"), QString::number(m_view->periodType()));
    context.setAttribute(QStringLiteral("start-mode"), QString::number(m_view->startMode()));
    context.setAttribute(QStringLiteral("start-date"), m_view->startDate().toString(Qt::ISODate));
    context.setAttribute(QStringLiteral("end-mode"), QString::number(m_view->endMode()));
    context.setAttribute(QStringLiteral("end-date"), m_view->endDate().toString(Qt::ISODate));

    m_view->saveContext(model()->columnMap(), context);
}

KoPrintJob *AccountsView::createPrintJob()
{
    return m_view->createPrintJob(this);
}


}  //KPlato namespace
