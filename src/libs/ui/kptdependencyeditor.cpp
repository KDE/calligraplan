/* This file is part of the KDE project
 SPDX-FileCopyrightText: 2007 Dag Andersen <dag.andersen@kdemail.net>
 SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>

 SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "kptdependencyeditor.h"

#include "PlanMacros.h"

#include "kptglobal.h"
#include "kptcommonstrings.h"
#include "kptitemmodelbase.h"
#include "kptcommand.h"
#include "kptproject.h"
#include "kptrelation.h"
#include "kptschedule.h"
#include "kptdebug.h"
#include "config.h"
#include "kptrelationdialog.h"
#include "RelationEditorDialog.h"
#include "kpttaskdialog.h"
#include "kptsummarytaskdialog.h"
#include "kpttaskprogressdialog.h"
#include "kptmilestoneprogressdialog.h"
#include "kptdocumentsdialog.h"
#include "kpttaskdescriptiondialog.h"

#include "KoPageLayoutWidget.h"
#include <KoIcon.h>
#include <KoDocument.h>

#include <QGraphicsSceneMouseEvent>
#include <QItemSelectionModel>
#include <QModelIndex>
#include <QPainterPath>
#include <QPalette>
#include <QStyleOptionViewItem>
#include <QVBoxLayout>
#include <QWidget>
#include <QKeyEvent>
#include <QAction>
#include <QMenu>

#include <KLocalizedString>
#include <KActionMenu>
#include <KActionCollection>


#define ConnectCursor Qt::DragLinkCursor

namespace KPlato
{

void plan_paintFocusSelectedItem(QPainter *painter, const QStyleOptionGraphicsItem *option)
{
    if (option->state & (QStyle::State_Selected | QStyle::State_HasFocus)) {
        painter->save();
        if (option->state & QStyle::State_Selected) {
            debugPlanDepEditor<<"selected";
            QPalette::ColorGroup cg = option->state & QStyle::State_Enabled
                    ? QPalette::Normal : QPalette::Disabled;
            if (cg == QPalette::Normal && !(option->state & QStyle::State_Active))
                cg = QPalette::Inactive;

            QLinearGradient g(0.0, option->rect.top(), 0.0, option->rect.bottom());
            QColor col = QApplication::palette().brush(cg, QPalette::Highlight).color();
            g.setColorAt(0.0, col.lighter(125));
            g.setColorAt(1.0, col.lighter(60));

            painter->setPen(Qt::NoPen);
            painter->setBrush(QBrush(g));
            painter->drawRect(option->exposedRect);
        }
        if (option->state & QStyle::State_HasFocus) {
            debugPlanDepEditor<<"has focus";
            QPalette::ColorGroup cg = option->state & QStyle::State_Enabled
                    ? QPalette::Active : QPalette::Disabled;
            if (cg == QPalette::Active && !(option->state & QStyle::State_Active))
                cg = QPalette::Inactive;

            QPen p(Qt::DotLine);
            p.setWidthF(2.);
            if (option->state & QStyle::State_Selected) {
                p.setColor(QApplication::palette().color(cg, QPalette::Shadow));
                debugPlanDepEditor<<"focus: selected"<<p.color();
            } else {
                p.setColor(QApplication::palette().color(cg, QPalette::Highlight));
                debugPlanDepEditor<<"focus: not selected"<<p.color();
            }
            painter->setPen(p);
            painter->setBrush(Qt::NoBrush);
            painter->drawRect(option->exposedRect);
        }
        painter->restore();
    }
}

//----------------------
DependecyViewPrintingDialog::DependecyViewPrintingDialog(ViewBase *parent, DependencyView *view)
    : PrintingDialog(parent),
    m_depview(view)
{
    debugPlanDepEditor<<this;
}

int DependecyViewPrintingDialog::documentLastPage() const
{
    //TODO
    return documentFirstPage();
}

QList<QWidget*> DependecyViewPrintingDialog::createOptionWidgets() const
{
    QList<QWidget*> lst;
    lst << createPageLayoutWidget();
    lst += PrintingDialog::createOptionWidgets();
    return  lst;
}

void DependecyViewPrintingDialog::printPage(int page, QPainter &painter)
{
    painter.save();

    QRect hRect = headerRect();
    QRect fRect = footerRect();
    QRect pageRect = printer().pageLayout().paintRectPixels(printer().resolution());
    pageRect.moveTo(0, 0);
    debugPlanDepEditor<<pageRect<<hRect<<fRect;

    painter.translate(pageRect.topLeft());

    painter.setClipping(true);

    paintHeaderFooter(painter, printingOptions(), page, *(m_depview->project()));

    int gap = 8;
    int pageHeight = pageRect.height();
    if (hRect.isValid()) {
        pageHeight -= (hRect.height() + gap);
    }
    if (fRect.isValid()) {
        pageHeight -= (fRect.height() + gap);
    }
    painter.translate(0, hRect.height() + gap);

    QRectF r(0, 0, pageRect.width(), pageHeight);
    const auto sr = m_depview->itemScene()->sceneRect();
    // do not scale if scene fits inside the page
    if (sr.width() < r.width()) {
        r.setWidth(sr.width());
    }
    if (sr.height() < r.height()) {
        r.setHeight(sr.height());
    }
    m_depview->itemScene()->render(&painter, r);

    painter.restore();
}


DependencyLinkItemBase::DependencyLinkItemBase(QGraphicsItem *parent)
    : QGraphicsPathItem(parent),
    m_editable(false),
    predItem(nullptr),
    succItem(nullptr),
    relation(nullptr),
    m_arrow(new QGraphicsPathItem(this))
{
}

DependencyLinkItemBase::DependencyLinkItemBase(DependencyNodeItem *predecessor, DependencyNodeItem *successor, Relation *rel, QGraphicsItem *parent)
    : QGraphicsPathItem(parent),
    m_editable(false),
    predItem(predecessor),
    succItem(successor),
    relation(rel),
    m_arrow(new QGraphicsPathItem(this))
{
}

DependencyLinkItemBase::~DependencyLinkItemBase()
{
}

DependencyScene *DependencyLinkItemBase::itemScene() const
{
    return static_cast<DependencyScene*>(scene());
}

void DependencyLinkItemBase::createPath(const QPointF &sp, int starttype, const QPointF &ep, int endtype)
{
    //if (predItem && succItem) debugPlanDepEditor<<predItem->text()<<" ->"<<succItem->text()<<" visible="<<isVisible();
    if (! isVisible()) {
        return;
    }
    qreal hgap = itemScene()->horizontalGap();

    bool up = sp.y() > ep.y();
    bool right = sp.x() < ep.x();
    bool same = sp.x() == ep.x();

    QPainterPath link(sp);
    qreal x = sp.x();
    qreal y = sp.y();
    if (right && starttype == DependencyNodeItem::Finish) {
        x = ep.x();
        x += endtype == DependencyNodeItem::Start ? - hgap/2 - 6 : hgap/2 - 6;
        link.lineTo(x, y);
        x += 6;
        QPointF cp(x, y);
        y += up ? -6 : +6;
        link.quadTo(cp, QPointF(x, y));
        y = up ? ep.y() + 6 : ep.y() - 6;
        link.lineTo(x, y);
        y = ep.y();
        cp = QPointF(x, y);
        x += endtype == DependencyNodeItem::Start ? 6 : -6;
        link.quadTo(cp, QPointF(x, y));
    } else if (right && starttype == DependencyNodeItem::Start) {
        x = sp.x() - hgap/2 + 6;
        link.lineTo(x, y);
        x -= 6;
        QPointF cp(x, y);
        y += up ? -6 : +6;
        link.quadTo(cp, QPointF(x, y));
        y = up ? ep.y() + 6 : ep.y() - 6;
        link.lineTo(x, y);
        y = ep.y();
        cp = QPointF(x, y);
        x += endtype == DependencyNodeItem::Start ? 6 : -6;
        link.quadTo(cp, QPointF(x, y));
    } else if (same) {
        x = ep.x();
        x += endtype == DependencyNodeItem::Start ? - hgap/2 + 6 : hgap/2 - 6;
        link.lineTo(x, y);
        x += endtype == DependencyNodeItem::Start ? -6 : +6;
        QPointF cp(x, y);
        y += up ? -6 : 6;
        link.quadTo(cp, QPointF(x, y));
        y = up ? ep.y() + 6 : ep.y() - 6;
        link.lineTo(x, y);
        y = ep.y();
        cp = QPointF(x, y);
        if (endtype == DependencyNodeItem::Start) {
            x += 6;
        } else {
            x -= 6;
        }
        link.quadTo(cp, QPointF(x, y));
    } else {
        x = ep.x();
        x += endtype == DependencyNodeItem::Start ? - hgap/2 + 6 : hgap/2 + 6;
        link.lineTo(x, y);
        x -= 6;
        QPointF cp(x, y);
        y += up ? -6 : 6;
        link.quadTo(cp, QPointF(x, y));
        y = up ? ep.y() + 6 : ep.y() - 6;
        link.lineTo(x, y);
        y = ep.y();
        cp = QPointF(x, y);
        x += endtype == DependencyNodeItem::Start ? 6 : -6;
        link.quadTo(cp, QPointF(x, y));
    }
    link.lineTo(ep);

    setPath(link);

    QPainterPath arrow;
    x = endtype == DependencyNodeItem::Start ? -6 : 6;
    arrow.moveTo(ep);
    arrow.lineTo(ep.x() + x, ep.y() - 3);
    arrow.lineTo(ep.x() + x, ep.y() + 3);
    arrow.lineTo(ep);
    m_arrow->setPath(arrow);
    m_arrow->show();
}

//--------------------------------
DependencyLinkItem::DependencyLinkItem(DependencyNodeItem *predecessor, DependencyNodeItem *successor, Relation *rel, QGraphicsItem *parent)
    : DependencyLinkItemBase(predecessor, successor, rel, parent)
{
    setZValue(100.0);
    setAcceptHoverEvents(true);
    //debugPlanDepEditor<<predecessor->text()<<"("<<predecessor->column()<<") -"<<successor->text();
    predItem->addChildRelation(this);
    succItem->addParentRelation(this);
    succItem->setColumn();

}

DependencyLinkItem::~DependencyLinkItem()
{
    if (predItem) {
        predItem->takeChildRelation(this);
    }
    if (succItem) {
        succItem->takeParentRelation(this);
    }
}

int DependencyLinkItem::newChildColumn() const
{
    int col = predItem->column();
    if (relation->type() == Relation::FinishStart) {
        ++col;
    }
    //debugPlanDepEditor<<"new col="<<col;
    return col;
}

void DependencyLinkItem::setItemVisible(bool show)
{
    setVisible(show && predItem->isVisible() && succItem->isVisible());
}

void DependencyLinkItem::createPath()
{
    setVisible(predItem->isVisible() && succItem->isVisible());
    if (! isVisible()) {
        //debugPlanDepEditor<<"Visible="<<isVisible()<<":"<<predItem->node()->name()<<" -"<<succItem->node()->name();
        return;
    }
    QPointF sp = startPoint();
    QPointF ep = endPoint();
    int stype = 0, etype = 0;
    switch (relation->type()) {
        case Relation::StartStart:
            stype = DependencyNodeItem::Start; etype = DependencyNodeItem::Start;
            break;
        case Relation::FinishStart:
            stype = DependencyNodeItem::Finish; etype = DependencyNodeItem::Start;
            break;
        case Relation::FinishFinish:
            stype = DependencyNodeItem::Finish; etype = DependencyNodeItem::Finish;
            break;
        default:
            break;
    }
    DependencyLinkItemBase::createPath(sp, stype, ep, etype);
}

QPointF DependencyLinkItem::startPoint() const
{
    if (relation->type() == Relation::StartStart) {
        return predItem->connectorPoint(DependencyNodeItem::Start);
    }
    return predItem->connectorPoint(DependencyNodeItem::Finish);
}

QPointF DependencyLinkItem::endPoint() const
{
    if (relation->type() == Relation::FinishFinish) {
        return succItem->connectorPoint(DependencyNodeItem::Finish);
    }
    return succItem->connectorPoint(DependencyNodeItem::Start);
}

void DependencyLinkItem::hoverEnterEvent(QGraphicsSceneHoverEvent * /*event*/)
{
    setZValue(zValue() + 1);
    QPen p = pen();
    p.setWidth(2);
    setPen(p);
    m_arrow->setPen(p);
    update();
}

void DependencyLinkItem::hoverLeaveEvent(QGraphicsSceneHoverEvent * /*event*/)
{
    resetHooverIndication();
}

void DependencyLinkItem::resetHooverIndication()
{
    setZValue(zValue() - 1);
    QPen p = pen();
    p.setWidth(1);
    setPen(p);
    m_arrow->setPen(p);
    update();
}

void DependencyLinkItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    //debugPlanDepEditor;
    QGraphicsItem::GraphicsItemFlags f = flags();
    if (isEditable() && itemScene()->connectionMode()) {
        itemScene()->clearConnection();
        setFlags(f & ~QGraphicsItem::ItemIsSelectable);
    }
    QGraphicsPathItem::mousePressEvent(event);
    if (f != flags()) {
        setFlags(f);
    }
}

void DependencyLinkItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    QPen p = pen();
    p.setColor(QApplication::palette().color(QPalette::WindowText));
    setPen(p);
    m_arrow->setPen(p);
    m_arrow->setBrush(p.color());
    DependencyLinkItemBase::paint(painter, option, widget);
}

//--------------------
DependencyCreatorItem::DependencyCreatorItem(QGraphicsItem *parent)
    : DependencyLinkItemBase(parent),
    predConnector(nullptr),
    succConnector(nullptr),
    m_editable(false)
{
    setZValue(1000.0);
    clear();
    setPen(QPen(Qt::blue, 2));
    m_arrow->setBrush(Qt::blue);
    m_arrow->setPen(QPen(Qt::blue, 2));
}

void DependencyCreatorItem::clear()
{
    hide();
    if (predConnector && predConnector->parentItem()) {
        static_cast<DependencyNodeItem*>(predConnector->parentItem())->setConnectorHoverMode(true);
    } else if (succConnector && succConnector->parentItem()) {
        static_cast<DependencyNodeItem*>(succConnector->parentItem())->setConnectorHoverMode(true);
    }
    predConnector = nullptr;
    succConnector = nullptr;
    setPath(QPainterPath());
    m_arrow->setPath(QPainterPath());
}

void DependencyCreatorItem::setPredConnector(DependencyConnectorItem *item)
{
    predConnector = item;
    //static_cast<DependencyNodeItem*>(item->parentItem())->setConnectorHoverMode(false);
}

void DependencyCreatorItem::setSuccConnector(DependencyConnectorItem *item)
{
    succConnector = item;
}

void DependencyCreatorItem::createPath()
{
    if (predConnector == nullptr) {
        return;
    }
    if (succConnector == nullptr) {
        return;
    }
    QPointF sp = predConnector->connectorPoint();
    QPointF ep = succConnector->connectorPoint();
    DependencyLinkItemBase::createPath(sp, predConnector->ctype(), ep, succConnector->ctype());
}

void DependencyCreatorItem::createPath(const QPointF &ep)
{
    m_arrow->hide();
    if (succConnector) {
        return createPath();
    }
    if (predConnector == nullptr) {
        return;
    }
    QPointF sp = predConnector->connectorPoint();

    QPainterPath link(sp);
    link.lineTo(ep);
    setPath(link);

}

QPointF DependencyCreatorItem::startPoint() const
{
    return predConnector == nullptr ? QPointF() : predConnector->connectorPoint();
}

QPointF DependencyCreatorItem::endPoint() const
{
    return succConnector == nullptr ? QPointF() : succConnector->connectorPoint();
}

//--------------------
DependencyConnectorItem::DependencyConnectorItem(DependencyNodeItem::ConnectorType type, DependencyNodeItem *parent)
    : QGraphicsRectItem(parent),
    m_ctype(type),
    m_editable(false)
{
    setCursor(ConnectCursor);
    setAcceptHoverEvents(true);
    setZValue(500.0);

    setFlag(QGraphicsItem::ItemIsFocusable);
}

DependencyScene *DependencyConnectorItem::itemScene() const
{
    return static_cast<DependencyScene*>(scene());
}

DependencyNodeItem *DependencyConnectorItem::nodeItem() const
{
    return static_cast<DependencyNodeItem*>(parentItem());
}

Node *DependencyConnectorItem::node() const
{
    return static_cast<DependencyNodeItem*>(parentItem())->node();
}

QPointF DependencyConnectorItem::connectorPoint() const
{
    QRectF r = rect();
    return QPointF(r.x()+r.width(), r.y() + r.height()/2);
}

void DependencyConnectorItem::hoverEnterEvent(QGraphicsSceneHoverEvent * /*event*/)
{
    itemScene()->connectorEntered(this, true);
}

void DependencyConnectorItem::hoverLeaveEvent(QGraphicsSceneHoverEvent * /*event*/)
{
    itemScene()->connectorEntered(this, false);
}

void DependencyConnectorItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (! isEditable()) {
        event->ignore();
        return;
    }
    if (event->button() == Qt::LeftButton) {
        m_mousePressPos = event->pos();
    } else {
        event->ignore();
    }
}

void DependencyConnectorItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    m_mousePressPos = QPointF();
    if (event->button() != Qt::LeftButton) {
        event->ignore();
        return;
    }
    if (rect().contains(event->scenePos())) {
        // user clicked on this item
        bool multiSelect = (event->modifiers() & Qt::ControlModifier) != 0;
        if (multiSelect) {
            itemScene()->multiConnectorClicked(this);
        } else {
            itemScene()->singleConnectorClicked(this);
        }
        return;
    }
    QGraphicsItem *item = nullptr;
    const QList<QGraphicsItem*> items = itemScene()->items(event->scenePos());
    for (QGraphicsItem *i : items) {
        if (i->type() == DependencyConnectorItem::Type) {
            item = i;
            break;
        }
    }
    if (item == nullptr || item == itemScene()->fromItem()) {
        itemScene()->setFromItem(nullptr);
        return;
    }
    itemScene()->singleConnectorClicked(static_cast<DependencyConnectorItem*>(item));
}

void DependencyConnectorItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->buttons() == Qt::LeftButton) {
        if (! m_mousePressPos.isNull()) {
            itemScene()->setFromItem(this);
            m_mousePressPos = QPointF();
        }
        QGraphicsItem *item = nullptr;
        const QList<QGraphicsItem*> items = itemScene()->items(event->scenePos());
        for (QGraphicsItem *i : items) {
            if (i->type() == DependencyConnectorItem::Type) {
                item = i;
                break;
            }
        }
        if (item != this) {
            itemScene()->connectorEntered(this, false);
        }
        if (item != nullptr) {
            itemScene()->connectorEntered(static_cast<DependencyConnectorItem*>(item), true);
        }
    } else {
        event->ignore();
    }
}

void DependencyConnectorItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(widget)
    //debugPlanDepEditor;
    QStyleOptionGraphicsItem opt(*option);
    opt.exposedRect = rect();
    if (itemScene()->fromItem() == this) {
        opt.state |= QStyle::State_Selected;
    }
    if (itemScene()->focusItem() == this) {
        opt.state |= QStyle::State_HasFocus;
    }
    plan_paintFocusSelectedItem(painter, &opt);

    QRectF r = rect();
    if (ctype() == DependencyNodeItem::Start) {
        r.setRect(r.right() - (r.width()/2.0) + 1.0, r.y() + (r.height() * 0.33), r.width() / 2.0, r.height() * 0.33);
    } else {
        r.setRect(r.right() - (r.width()/2.0) - 1.0, r.y() + (r.height() * 0.33), r.width() / 2.0, r.height() * 0.33);
    }
    painter->fillRect(r, QApplication::palette().brush(QPalette::WindowText));
}

QList<DependencyLinkItem*> DependencyConnectorItem::predecessorItems() const
{
    return nodeItem()->predecessorItems(m_ctype);
}

QList<DependencyLinkItem*> DependencyConnectorItem::successorItems() const
{
    return nodeItem()->successorItems(m_ctype);
}

//--------------------
DependencyExpandItem::DependencyExpandItem(QGraphicsItem *parent)
    : QGraphicsRectItem(parent)
    , expanded(true)
{
    setRect(-6.0, 0.0, 12.0, -12.0);

    setPen(QPen(Qt::darkGray));
    setBrush(Qt::darkGray);

}

bool DependencyExpandItem::isExpanded() const
{
    return expanded;
}

void DependencyExpandItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    event->ignore();
    if (event->button() == Qt::LeftButton) {
        static_cast<DependencyNodeItem*>(parentItem())->setExpanded(!expanded);
        event->accept();
    }
}

void DependencyExpandItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    const auto r = rect();
    if (!expanded) {
        painter->translate(r.width() * 0.4, r.height() * 0.5);
        painter->rotate(-90.0);
    }
    QPainterPath p;
    p.moveTo(r.left(), -7.0);
    p.lineTo(r.center().x(), -1.0);
    p.lineTo(r.right(), -7.0);
    p.closeSubpath();
    painter->setPen(pen());
    painter->setBrush(brush());
    painter->drawPath(p);
}

DependencyNodeItem::DependencyNodeItem(Node *node, DependencyNodeItem *parent)
    : QGraphicsRectItem(parent),
    m_node(node),
    m_parent(nullptr),
    m_editable(false),
    m_expandItem(nullptr)
{
    setAcceptHoverEvents(true);
    setZValue(400.0);
    setParentItem(parent);
    m_start = new DependencyConnectorItem(DependencyNodeItem::Start, this);
    m_finish = new DependencyConnectorItem(DependencyNodeItem::Finish, this);

    m_text = new QGraphicsTextItem(this);
    m_textFont = m_text->font();
    m_textFont.setPointSize(10);
    m_text->setFont(m_textFont);
    setText();

    setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsFocusable);

    // do not attach this item to the scene as it gives continuous paint events when a node item is selected
    m_symbol = new DependencyNodeSymbolItem(this);
    m_symbol->setZValue(zValue() + 10.0);
    setSymbol();

    m_treeIndicator = new QGraphicsPathItem(this);
    m_treeIndicator->setPen(QPen(Qt::gray));

    if (node->numChildren()) {
        m_expandItem = new DependencyExpandItem(this);
        m_expandItem->setZValue(zValue() + 20.0);
    }
}

DependencyNodeItem::~DependencyNodeItem()
{
    auto children = m_children;
    qDeleteAll(children);
    m_children.clear();

    qDeleteAll(m_childrelations);
    qDeleteAll(m_parentrelations);
    if (itemScene()) {
        itemScene()->itemToBeRemoved(this, m_parent);
    }
    setParentItem(nullptr);
}

bool DependencyNodeItem::isSummaryTask() const
{
    return !m_children.isEmpty();
}

void DependencyNodeItem::setText()
{
    m_text->setPlainText(m_node == nullptr ? QString() : QStringLiteral("%1  %2").arg(m_node->wbsCode()).arg(m_node->name()));
}

DependencyScene *DependencyNodeItem::itemScene() const
{
    return static_cast<DependencyScene*>(scene());
}

void DependencyNodeItem::setSymbol()
{
    m_symbol->setSymbol(m_node->type(), itemScene()->symbolRect());
}

QPointF DependencyNodeItem::connectorPoint(DependencyNodeItem::ConnectorType type) const
{
    if (type == Start) {
        return m_start->connectorPoint();
    }
    return m_finish->connectorPoint();
}

void DependencyNodeItem::setConnectorHoverMode(bool mode)
{
    m_start->setAcceptHoverEvents(mode);
    m_finish->setAcceptHoverEvents(mode);
}

void DependencyNodeItem::setParentItem(DependencyNodeItem *parent)
{
    if (m_parent) {
        m_parent->takeChild(this);
    }
    m_parent = parent;
    if (m_parent) {
        m_parent->addChild(this);
    }
}

void DependencyNodeItem::setChildrenVisible(bool visible)
{
    for (DependencyNodeItem *ch : std::as_const(m_children)) {
        itemScene()->setItemVisible(ch, visible);
    }
}

void DependencyNodeItem::updateExpandItem()
{
    if (m_node->numChildren() == 0) {
        if (m_expandItem) {
            m_expandItem->setVisible(false);
        }
    } else {
        m_expandItem->setVisible(true);
    }
}

bool DependencyNodeItem::isExpanded() const
{
    return m_expandItem ? m_expandItem->isExpanded() : false;
}

void DependencyNodeItem::setExpanded(bool mode)
{
    if (m_expandItem) {
        m_expandItem->expanded = mode;
        setChildrenVisible(mode);
        update();
    }
}

void DependencyNodeItem::setItemVisible(bool show)
{
    setVisible(show);
    //debugPlanDepEditor<<isVisible()<<","<<node()->name();
    for (DependencyLinkItem *i : std::as_const(m_parentrelations)) {
        i->setItemVisible(show);
    }
    for (DependencyLinkItem *i : std::as_const(m_childrelations)) {
        i->setItemVisible(show);
    }
    m_treeIndicator->setVisible(show);
}

void DependencyNodeItem::addChild(DependencyNodeItem *ch)
{
    m_children.append(ch);
    if (!m_expandItem) {
        m_expandItem = new DependencyExpandItem(this);
        m_expandItem->setZValue(zValue() + 20.0);
        update();
    }
}

DependencyNodeItem *DependencyNodeItem::takeChild(DependencyNodeItem *ch)
{
    int i = m_children.indexOf(ch);
    if (i == -1) {
        return nullptr;
    }
    auto item = m_children.takeAt(i);
    Q_ASSERT(item == ch);
    if (m_children.isEmpty()) {
        delete m_expandItem;
        m_expandItem = nullptr;
        update();
    }
    return item;
}

void DependencyNodeItem::setRectangle(const QRectF &rect)
{
    //debugPlanDepEditor<<text()<<":"<<rect;
    setRect(rect);

    qreal connection = static_cast<DependencyScene*>(scene())->connectorWidth();
    m_start->setRect(rect.x() + connection, rect.y(), -connection, rect.height());
    m_finish->setRect(rect.right() - connection, rect.y(), connection, rect.height());

    m_text->setPos(m_finish->rect().right() + 2.0, itemScene()->gridY(row()));

    m_symbol->setPos(rect.topLeft() + QPointF(connection, 0) + QPointF(2.0, 2.0));
    if (m_expandItem) {
        m_expandItem->setPos(treeIndicatorX(), rect.bottom());
    }
}

void DependencyNodeItem::moveToY(qreal y)
{
    QRectF r = rect();
    r. moveTop(y);
    setRectangle(r);
    //debugPlanDepEditor<<text()<<" move to="<<y<<" new pos:"<<rect();
    for (DependencyLinkItem *i : std::as_const(m_parentrelations)) {
        i->createPath();
    }
    for (DependencyLinkItem *i : std::as_const(m_childrelations)) {
        i->createPath();
    }
    DependencyNodeItem *par = this;
    while (par->parentItem()) {
        par = par->parentItem();
    }
    par->setTreeIndicator(true);
}

void DependencyNodeItem::setRow(int row)
{
    moveToY(itemScene()->itemY(row));
}

int DependencyNodeItem::row() const
{
    return itemScene()->row(rect().y());
}

void DependencyNodeItem::moveToX(qreal x)
{
    QRectF r = rect();
    r. moveLeft(x);
    setRectangle(r);
    //debugPlanDepEditor<<m_text->toPlainText()<<" to="<<x<<" new pos:"<<rect();
    for (DependencyLinkItem *i : std::as_const(m_parentrelations)) {
        i->createPath();
    }
    for (DependencyLinkItem *i : std::as_const(m_childrelations)) {
        i->createPath();
    }
    DependencyNodeItem *par = this;
    while (par->parentItem()) {
        par = par->parentItem();
    }
    par->setTreeIndicator(true);
}

void DependencyNodeItem::setColumn()
{
    int col = m_parent == nullptr ? 0 : m_parent->column() + 1;
    //debugPlanDepEditor<<this<<text();
    for (DependencyLinkItem *i : std::as_const(m_parentrelations)) {
        col = qMax(col, i->newChildColumn());
    }
    if (col != column()) {
        setColumn(col);
        for (DependencyLinkItem *i : std::as_const(m_childrelations)) {
            i->succItem->setColumn();
        }
        //debugPlanDepEditor<<m_children.count()<<"Column="<<column()<<","<<text();
        for (DependencyNodeItem *i : std::as_const(m_children)) {
            i->setColumn();
        }
    }
}

void DependencyNodeItem::setColumn(int col)
{
    moveToX(itemScene()->itemX(col));
}

int DependencyNodeItem::column() const
{
    return itemScene()->column(rect().x());
}

DependencyLinkItem *DependencyNodeItem::takeParentRelation(DependencyLinkItem *r)
{
    int i = m_parentrelations.indexOf(r);
    if (i == -1) {
        return nullptr;
    }
    DependencyLinkItem *dep = m_parentrelations.takeAt(i);
    setColumn();
    return dep;
}

DependencyLinkItem *DependencyNodeItem::takeChildRelation(DependencyLinkItem *r)
{
    int i = m_childrelations.indexOf(r);
    if (i == -1) {
        return nullptr;
    }
    return m_childrelations.takeAt(i);
}

void DependencyNodeItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsItem::GraphicsItemFlags f = flags();
    if (itemScene()->connectionMode()) {
        itemScene()->clearConnection();
        setFlags(f & ~QGraphicsItem::ItemIsSelectable);
    } else {
        if (event->button() == Qt::LeftButton && m_expandItem) {
            m_expandItem->expanded = !m_expandItem->expanded;
        }
    }
    QGraphicsRectItem::mousePressEvent(event);
    if (f != flags()) {
        setFlags(f);
    }
}

void DependencyNodeItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsRectItem::mouseReleaseEvent(event);
}

void DependencyNodeItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *)
{
    //debugPlanDepEditor;
    QLinearGradient g(0.0, rect().top(), 0.0, rect().bottom());
    g.setColorAt(0.0, QApplication::palette().color(QPalette::Light));
    g.setColorAt(1.0, QApplication::palette().color(QPalette::Mid));
    QBrush b(g);
    painter->setBrush(b);
    painter->setPen(QPen(Qt::NoPen));
    painter->drawRect(rect());

    QStyleOptionGraphicsItem opt(*option);
    opt.exposedRect = rect().adjusted(-m_start->rect().width(), 0.0, -m_finish->rect().width(), 0.0);
    if (this == itemScene()->focusItem()) {
        opt.state |= QStyle::State_HasFocus;
    }
    plan_paintFocusSelectedItem(painter, &opt);

    // paint the symbol
    m_symbol->paint(itemScene()->project(), painter, &opt);
}

DependencyConnectorItem *DependencyNodeItem::connectorItem(ConnectorType ctype) const
{
    switch (ctype) {
        case Start: return m_start;
        case Finish: return m_finish;
        default: break;
    }
    return nullptr;
}

QList<DependencyLinkItem*> DependencyNodeItem::predecessorItems(ConnectorType ctype) const
{
    QList<DependencyLinkItem*> lst;
    for (DependencyLinkItem *i : std::as_const(m_parentrelations)) {
        if (ctype == Start && (i->relation->type() == Relation::StartStart || i->relation->type() == Relation::FinishStart)) {
            lst << i;
        }
        if (ctype == Finish && i->relation->type() == Relation::FinishFinish) {
            lst << i;
        }
    }
    return lst;
}

QList<DependencyLinkItem*> DependencyNodeItem::successorItems(ConnectorType ctype) const
{
    QList<DependencyLinkItem*> lst;
    for (DependencyLinkItem *i : std::as_const(m_childrelations)) {
        if (ctype == Start && i->relation->type() == Relation::StartStart) {
            lst << i;
        }
        if (ctype == Finish && (i->relation->type() == Relation::FinishFinish || i->relation->type() == Relation::FinishStart)) {
            lst << i;
        }
    }
    return lst;
}

qreal DependencyNodeItem::treeIndicatorX() const
{
    const auto r = rect();
    return r.x() + (r.width() * 0.65);
}

void DependencyNodeItem::setTreeIndicator(bool on)
{
    paintTreeIndicator(on);
    for (DependencyNodeItem *i : std::as_const(m_children)) {
        if (i->isVisible()) {
            i->setTreeIndicator(on);
        }
    }
}

void DependencyNodeItem::paintTreeIndicator(bool on)
{
    if (! on) {
        m_treeIndicator->hide();
        return;
    }
    QPainterPath p;
    qreal y1 = itemScene()->gridY(row());
    qreal y2 = itemScene()->gridY(row() + 1);
    for (DependencyNodeItem *par = m_parent; par; par = par->parentItem()) {
        qreal x = par->treeIndicatorX();
        p.moveTo(x, y1);
        if (par == m_parent) {
            p.lineTo(x, (y1 + y2) / 2.0);
            p.lineTo(x + 6, (y1 + y2) / 2.0);
            if (m_node->siblingAfter()) {
                p.moveTo(x, (y1 + y2) / 2.0);
                p.lineTo(x, y2);
            }
        } else {
            const QList<DependencyNodeItem*> &children = par->children();
            if (children.last()->rect().y() > rect().y()) {
                p.lineTo(x, (y1 + y2) / 2.0);
                p.lineTo(x, y2);
            }
        }
    }
    if (m_node->numChildren()) {
        qreal x = treeIndicatorX();
        qreal y = rect().bottom();
        p.moveTo(x, y);
        p.lineTo(x, itemScene()->gridY(row() + 1));
    }
    if (p.isEmpty()) {
        m_treeIndicator->hide();
    } else {
        m_treeIndicator->setPath(p);
        m_treeIndicator->show();
    }
    //debugPlanDepEditor<<text()<<rect()<<p;
}

//--------------------
void DependencyNodeSymbolItem::setSymbol(int type, const QRectF &rect)
{
    m_nodetype = type;
    m_itemtype = KGantt::TypeNone;
    QPainterPath p;
    switch (type) {
        case Node::Type_Summarytask:
            m_itemtype = KGantt::TypeSummary;
            p.moveTo(rect.topLeft());
            p.lineTo(rect.topRight());
            p.lineTo(rect.left() + rect.width() / 2.0, rect.bottom());
            p.closeSubpath();
            break;
        case Node::Type_Task:
            m_itemtype = KGantt::TypeTask;
            p.moveTo(rect.topLeft());
            p.lineTo(rect.topRight());
            p.lineTo(rect.bottomRight());
            p.lineTo(rect.bottomLeft());
            p.closeSubpath();
            break;
        case Node::Type_Milestone:
            m_itemtype = KGantt::TypeEvent;
            p.moveTo(rect.left() + (rect.width() / 2.0), rect.top());
            p.lineTo(rect.right(), rect.top() + (rect.height() / 2.0));
            p.lineTo(rect.left() + (rect.width() / 2.0), rect.bottom());
            p.lineTo(rect.left(), rect.top() + (rect.height() / 2.0));
            p.closeSubpath();
            break;
        default:
            break;
    }
    setPath(p);
}

void DependencyNodeSymbolItem::paint(Project *project, QPainter *painter, const QStyleOptionGraphicsItem *option)
{
    painter->setPen(Qt::NoPen);
    painter->translate(option->exposedRect.x() + 2.0, option->exposedRect.y() + 2.0);
    auto p = path();
    if (project) {
        switch (m_nodetype) {
            case Node::Type_Summarytask: {
                painter->setBrush(project->config().summaryTaskDefaultColor());
                break;
            }
            case Node::Type_Task:
                painter->setBrush(project->config().taskNormalColor());
                break;
            case Node::Type_Milestone:
                painter->setBrush(project->config().milestoneNormalColor());
                break;
            default:
                painter->setBrush(m_delegate.defaultBrush(m_itemtype));
                break;
        }
    } else {
        painter->setBrush(m_delegate.defaultBrush(m_itemtype));
    }
    painter->drawPath(p);

}

//--------------------
DependencyScene::DependencyScene(QWidget *parent)
    : QGraphicsScene(parent),
    m_model(nullptr),
    m_readwrite(false)
{
    setSceneRect(QRectF());
    m_connectionitem = new DependencyCreatorItem();
    addItem(m_connectionitem);
    //debugPlanDepEditor;
    m_connectionitem->hide();
    connect(qApp, &QApplication::paletteChanged, this, &DependencyScene::update);
}

DependencyScene::~DependencyScene()
{
    //debugPlanDepEditor<<" DELETED";
    clearScene();
}

void DependencyScene::setFromItem(DependencyConnectorItem *item)
{
    DependencyConnectorItem *old = fromItem();
    m_connectionitem->clear();
    if (old && old->parentItem()) {
        old->parentItem()->update();
    }
    if (item) {
        const QList<QGraphicsItem*> items = this->items();
        for (QGraphicsItem *i : items) {
            if (i != m_connectionitem && i->type() != DependencyConnectorItem::Type) {
                i->setAcceptHoverEvents(false);
                if (i->type() == DependencyLinkItem::Type) {
                    static_cast<DependencyLinkItem*>(i)->resetHooverIndication();
                }
            }
        }
        item->setCursor(ConnectCursor);
        m_connectionitem->setPredConnector(item);
        m_connectionitem->show();
    } else {
        const QList<QGraphicsItem*> items = this->items();
        for (QGraphicsItem *i : items) {
            if (i != m_connectionitem && i->type() != DependencyConnectorItem::Type)
                i->setAcceptHoverEvents(true);
        }
    }
    if (item && item->parentItem()) {
        item->parentItem()->update();
    }
}

bool DependencyScene::connectionIsValid(DependencyConnectorItem *pred, DependencyConnectorItem *succ)
{
    if (pred->ctype() == DependencyNodeItem::Start && succ->ctype() == DependencyNodeItem::Finish) {
        return false;
    }
    Node *par = static_cast<DependencyNodeItem*>(pred->parentItem())->node();
    Node *ch = static_cast<DependencyNodeItem*>(succ->parentItem())->node();
    return m_project->linkExists(par, ch) || m_project->legalToLink(par, ch);
}

void DependencyScene::connectorEntered(DependencyConnectorItem *item, bool entered)
{
    //debugPlanDepEditor<<entered;
    item->setCursor(ConnectCursor);
    if (! entered) {
        // when we leave a connector we don't have a successor
        m_connectionitem->setSuccConnector(nullptr);
        return;
    }
    if (m_connectionitem->predConnector == item) {
        // when inside the predecessor, clicking is allowed (deselects connector)
        item->setCursor(ConnectCursor);
        return;
    }
    if (! m_connectionitem->isVisible()) {
        // we are not in connection mode
        return;
    }
    if (m_connectionitem->predConnector == nullptr) {
        // nothing we can do if we don't have a predecessor (shouldn't happen)
        return;
    }
    if (item->parentItem() == m_connectionitem->predConnector->parentItem()) {
        // not allowed to connect to the same node
        item->setCursor(Qt::ForbiddenCursor);
        return;
    }
    if (! (connectionIsValid(m_connectionitem->predConnector, item))) {
        // invalid connection (circular dependency, connecting to parent node, etc)
        item->setCursor(Qt::ForbiddenCursor);
        return;
    }
    m_connectionitem->setSuccConnector(item);
    m_connectionitem->createPath();
}

void DependencyScene::drawBackground (QPainter *painter, const QRectF &rect)
{
    QGraphicsScene::drawBackground(painter, rect);
    QBrush br(QApplication::palette().brush(QPalette::AlternateBase));
    int first = row(rect.y());
    int last = row(rect.bottom());
    for (int r = first; r <= last; ++r) {
        if (r % 2 == 1) {
            qreal oy = gridY(r);
            QRectF rct(rect.x(), oy, rect.width(), gridHeight());
            painter->fillRect(rct, br);
            //debugPlanDepEditor<<r<<": oy="<<oy<<""<<rct;
        }
    }
}

QList<QGraphicsItem*> DependencyScene::itemList(int type) const
{
    QList<QGraphicsItem*> lst;
    const QList<QGraphicsItem*> items = this->items();
    for (QGraphicsItem *i : items) {
        if (i->type() == type) {
            lst << i;
        }
    }
    return lst;
}

void DependencyScene::clearScene()
{
    m_connectionitem->clear();
    QList<QGraphicsItem*> its, deps;
    const QList<QGraphicsItem*> items = this->items();
    for (QGraphicsItem *i : items) {
        if (i->type() == DependencyNodeItem::Type && i->parentItem() == nullptr) {
            its << i;
        } else if (i->type() == DependencyLinkItem::Type) {
            deps << i;
        }
    }
    qDeleteAll(deps);
    qDeleteAll(its);
    removeItem(m_connectionitem);
    qDeleteAll(this->items());
    setSceneRect(QRectF());
    addItem(m_connectionitem);
    //debugPlanDepEditor;
}

QList<DependencyNodeItem*> DependencyScene::removeChildItems(DependencyNodeItem *item)
{
    QList<DependencyNodeItem*> lst;
    const QList<DependencyNodeItem*> items = item->children();
    for (DependencyNodeItem *i : items) {
        m_allItems.removeAt(m_allItems.indexOf(i));
        lst << i;
        lst += removeChildItems(i);
    }
    return lst;
}

void DependencyScene::moveItem(DependencyNodeItem *item, const QList<Node*> &lst)
{
    //debugPlanDepEditor<<item->text();
    int idx = m_allItems.indexOf(item);
    int ndx = lst.indexOf(item->node());
    Q_ASSERT(idx != -1 && ndx != -1);
    Node *oldParent = item->parentItem() == nullptr ? nullptr : item->parentItem()->node();
    Node *newParent = item->node()->parentNode();
    if (newParent == m_project) {
        newParent = nullptr;
    } else debugPlanDepEditor<<newParent->name()<<newParent->level();
    if (idx != ndx || oldParent != newParent) {
        // If I have children, these must be moved too.
        const QList<DependencyNodeItem*> items = removeChildItems(item);

        m_allItems.removeAt(idx);
        m_allItems.insert(ndx, item);
        item->setParentItem(m_allItems.value(lst.indexOf(newParent)));
        item->setColumn();
        //debugPlanDepEditor<<item->text()<<":"<<idx<<"->"<<ndx<<", "<<item->column()<<r;
        if (! items.isEmpty()) {
            for (DependencyNodeItem *i : items) {
                m_allItems.insert(++ndx, i);
                i->setColumn();
                //debugPlanDepEditor<<i->text()<<": ->"<<ndx<<", "<<i->column()<<r;
            }
        }
    }
}

void DependencyScene::setItemVisible(DependencyNodeItem *item, bool show)
{
    //debugPlanDepEditor<<"Visible count="<<m_visibleItems.count()<<" total="<<m_allItems.count();
    item->setItemVisible(show);
    int row = m_allItems.indexOf(item);
    if (row == -1) {
        debugPlanDepEditor<<"Unknown item!!";
        return;
    }
    if (show && CONTAINS(m_hiddenItems, item)) {
        moveItem(item, m_project->flatNodeList()); // might have been moved
    }
    m_hiddenItems.clear();
    m_visibleItems.clear();
    int viewrow = 0;
    for (int i = 0; i < m_allItems.count(); ++i) {
        DependencyNodeItem *itm = m_allItems[ i ];
        if (itm->isVisible()) {
            m_visibleItems.insert(i, itm);
            //debugPlanDepEditor<<itm->text()<<":"<<i<<viewrow;
            itm->setRow(viewrow);
            ++viewrow;
        } else {
            m_hiddenItems.insert(i, itm);
        }
    }
}

DependencyNodeItem *DependencyScene::findPrevItem(Node *node)  const
{
    if (node->numChildren() == 0) {
        return findItem(node);
    }
    return findPrevItem(node->childNodeIterator().last());
}

DependencyNodeItem *DependencyScene::itemBefore(DependencyNodeItem *parent, Node *node)  const
{
    Node *sib = node->siblingBefore();
    DependencyNodeItem *bef = parent;
    if (sib) {
        bef = findPrevItem(sib);
    }
    return bef;
}

DependencyNodeItem *DependencyScene::createItem(Node *node)
{
    DependencyNodeItem *parent = findItem(node->parentNode());
    if (parent && !parent->isExpanded()) {
        parent->setExpanded(true);
    }
    DependencyNodeItem *after = itemBefore(parent, node);
    int i = m_allItems.count()-1;
    if (after) {
        i = m_allItems.indexOf(after);
        //debugPlanDepEditor<<"after="<<after->node()->name()<<" pos="<<i;
    }
    DependencyNodeItem *item = new DependencyNodeItem(node, parent);
    if (item->scene() != this) {
        addItem(item);
    }
    item->setEditable(m_readwrite);
    item->startConnector()->setEditable(m_readwrite);
    item->finishConnector()->setEditable(m_readwrite);
    //debugPlanDepEditor<<item->text()<<item;
    int col = 0;
    if (parent) {
        col += parent->column() + 1;
    }
    item->setRectangle(QRectF(itemX(col), itemY(), itemWidth(), itemHeight()));
    m_allItems.insert(i+1, item);
    setItemVisible(item, true);
    return item;
}

DependencyLinkItem *DependencyScene::findItem(const Relation* rel) const
{
    const QList<QGraphicsItem*> items = itemList(DependencyLinkItem::Type);
    for (QGraphicsItem *i : items) {
        if (static_cast<DependencyLinkItem*>(i)->relation == rel) {
            return static_cast<DependencyLinkItem*>(i);
        }
    }
    return nullptr;
}

DependencyLinkItem *DependencyScene::findItem(const DependencyConnectorItem *c1, const DependencyConnectorItem *c2, bool exact) const
{
    DependencyNodeItem *n1 = c1->nodeItem();
    DependencyNodeItem *n2 = c2->nodeItem();
    const QList<QGraphicsItem*> items = itemList(DependencyLinkItem::Type);
    for (QGraphicsItem *i : items) {
        DependencyLinkItem *link = static_cast<DependencyLinkItem*>(i);
        if (link->predItem == n1 && link->succItem == n2) {
            switch (link->relation->type()) {
                case Relation::StartStart:
                    if (c1->ctype() == DependencyNodeItem::Start && c2->ctype() == DependencyNodeItem::Start) {
                        return link;
                    }
                    break;
                case Relation::FinishStart:
                    if (c1->ctype() == DependencyNodeItem::Finish && c2->ctype() == DependencyNodeItem::Start) {
                        return link;
                    }
                    break;
                case Relation::FinishFinish:
                    if (c1->ctype() == DependencyNodeItem::Finish && c2->ctype() == DependencyNodeItem::Finish) {
                        return link;
                    }
                    break;
                default:
                    break;
            }
            return nullptr;
        }
        if (link->predItem == n2 && link->succItem == n1) {
            if (exact) {
                return nullptr;
            }
            switch (link->relation->type()) {
                case Relation::StartStart:
                    if (c2->ctype() == DependencyNodeItem::Start && c1->ctype() == DependencyNodeItem::Start) {
                        return link;
                    }
                    break;
                case Relation::FinishStart:
                    if (c2->ctype() == DependencyNodeItem::Finish && c1->ctype() == DependencyNodeItem::Start) {
                        return link;
                    }
                    break;
                case Relation::FinishFinish:
                    if (c2->ctype() == DependencyNodeItem::Finish && c1->ctype() == DependencyNodeItem::Finish) {
                        return link;
                    }
                    break;
                default:
                    break;
            }
            return nullptr;
        }
    }
    return nullptr;
}

DependencyNodeItem *DependencyScene::findItem(const Node *node) const
{
    const QList<QGraphicsItem*> items = itemList(DependencyNodeItem::Type);
    for (QGraphicsItem *i : items) {
        if (static_cast<DependencyNodeItem*>(i)->node() == node) {
            return static_cast<DependencyNodeItem*>(i);
        }
    }
    return nullptr;
}

void DependencyScene::itemToBeRemoved(DependencyNodeItem *item, DependencyNodeItem *parentItem)
{
    if (item) {
        m_allItems.removeAll(item);
        m_hiddenItems.remove(m_hiddenItems.key(item));
        m_visibleItems.remove(m_visibleItems.key(item));
        for (DependencyNodeItem *i : std::as_const(m_visibleItems)) {
            if (i->row() > item->row()) {
                i->setRow(i->row() - 1);
            } else if (i->row() == item->row() - 1) {
                i->setRow(i->row());
            }
        }
        if (parentItem) {
            parentItem->updateExpandItem();
        }
    }
    removeItem(item);
}

void DependencyScene::createLinks()
{
    for (DependencyNodeItem *i : std::as_const(m_allItems)) {
        createLinks(i);
    }
}
void DependencyScene::createLinks(DependencyNodeItem *item)
{
    const QList<Relation*> relations = item->node()->dependChildNodes();
    for (Relation *rel : relations) {
        createLink(item, rel);
    }
}
void DependencyScene::createLink(DependencyNodeItem *parent, Relation *rel)
{
    DependencyNodeItem *child = findItem(rel->child());
    if (parent == nullptr || child == nullptr) {
        return;
    }
    DependencyLinkItem *dep = new DependencyLinkItem(parent, child, rel);
    dep->setEditable(m_readwrite);
    addItem(dep);
    //debugPlanDepEditor;
    dep->createPath();
}

void DependencyScene::mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    if (m_connectionitem->isVisible()) {
        int x = qMin(qMax(sceneRect().left() + 2, mouseEvent->scenePos().x()), sceneRect().right() - 4);
        int y = qMin(qMax(sceneRect().top() + 2, mouseEvent->scenePos().y()), sceneRect().bottom() - 4);
        m_connectionitem->createPath(QPoint(x, y));
    }
    QGraphicsScene::mouseMoveEvent(mouseEvent);
    //debugPlanDepEditor<<mouseEvent->scenePos()<<","<<mouseEvent->isAccepted();

}

void DependencyScene::keyPressEvent(QKeyEvent *keyEvent)
{
    //debugPlanDepEditor<<focusItem();
    if (m_visibleItems.isEmpty()) {
        return QGraphicsScene::keyPressEvent(keyEvent);
    }
    QGraphicsItem *fitem = focusItem();
    if (fitem == nullptr) {
        setFocusItem(m_visibleItems.first());
        if (focusItem()) {
            focusItem()->update();
        }
        Q_EMIT focusItemChanged(focusItem(), nullptr, Qt::OtherFocusReason); // we do not use olditem and focus reason
        return;
    }
    switch (keyEvent->key()) {
        case Qt::Key_Left: {
            if (fitem->type() == DependencyNodeItem::Type) {
                DependencyConnectorItem *item = static_cast<DependencyNodeItem*>(fitem)->startConnector();
                if (item) {
                    setFocusItem(item);
                }
            } else if (fitem->type() == DependencyConnectorItem::Type) {
                DependencyConnectorItem *citem = static_cast<DependencyConnectorItem*>(fitem);
                if (citem->ctype() == DependencyNodeItem::Start) {
                    //Goto prev nodes finishConnector
                    DependencyNodeItem *nitem = static_cast<DependencyNodeItem*>(citem->parentItem());
                    DependencyNodeItem *item = nodeItem(nitem->row() - 1);
                    if (item) {
                        setFocusItem(item->finishConnector());
                    }
                } else {
                    // Goto node item (parent)
                    setFocusItem(citem->parentItem());
                }
            }
            break;
        }
        case Qt::Key_Right: {
            if (fitem->type() == DependencyNodeItem::Type) {
                DependencyConnectorItem *item = static_cast<DependencyNodeItem*>(fitem)->finishConnector();
                if (item) {
                    setFocusItem(item);
                }
            } else if (fitem->type() == DependencyConnectorItem::Type) {
                DependencyConnectorItem *citem = static_cast<DependencyConnectorItem*>(fitem);
                if (citem->ctype() == DependencyNodeItem::Finish) {
                    //Goto prev nodes startConnector
                    DependencyNodeItem *nitem = static_cast<DependencyNodeItem*>(citem->parentItem());
                    DependencyNodeItem *item = nodeItem(nitem->row() + 1);
                    if (item) {
                        setFocusItem(item->startConnector());
                    }
                } else {
                    // Goto node item (parent)
                    setFocusItem(citem->parentItem());
                }
            }
            break;
        }
        case Qt::Key_Up: {
            if (fitem->type() == DependencyNodeItem::Type) {
                DependencyNodeItem *item = nodeItem(static_cast<DependencyNodeItem*>(fitem)->row() - 1);
                if (item) {
                    setFocusItem(item);
                }
            } else if (fitem->type() == DependencyConnectorItem::Type) {
                DependencyConnectorItem *citem = static_cast<DependencyConnectorItem*>(fitem);
                DependencyNodeItem *nitem = static_cast<DependencyNodeItem*>(citem->parentItem());
                if (citem->ctype() == DependencyNodeItem::Finish) {
                    DependencyNodeItem *item = nodeItem(nitem->row() - 1);
                    if (item) {
                        setFocusItem(item->finishConnector());
                    }
                } else {
                    DependencyNodeItem *item = nodeItem(static_cast<DependencyNodeItem*>(fitem)->row() - 1);
                    if (item) {
                        setFocusItem(item->startConnector());
                    }
                }
            }
            break;
        }
        case Qt::Key_Down: {
            if (fitem->type() == DependencyNodeItem::Type) {
                DependencyNodeItem *item = nodeItem(static_cast<DependencyNodeItem*>(fitem)->row() + 1);
                if (item) {
                    setFocusItem(item);
                }
            } else if (fitem->type() == DependencyConnectorItem::Type) {
                DependencyConnectorItem *citem = static_cast<DependencyConnectorItem*>(fitem);
                DependencyNodeItem *nitem = static_cast<DependencyNodeItem*>(citem->parentItem());
                if (citem->ctype() == DependencyNodeItem::Finish) {
                    DependencyNodeItem *item = nodeItem(nitem->row() + 1);
                    if (item) {
                        setFocusItem(item->finishConnector());
                    }
                } else {
                    DependencyNodeItem *item = nodeItem(static_cast<DependencyNodeItem*>(fitem)->row() + 1);
                    if (item) {
                        setFocusItem(item->startConnector());
                    }
                }
            }
            break;
        }
        case Qt::Key_Space:
        case Qt::Key_Select: {
            if (fitem->type() == DependencyConnectorItem::Type) {
                singleConnectorClicked(static_cast<DependencyConnectorItem*>(fitem));
            } else if (fitem->type() == DependencyNodeItem::Type) {
                singleConnectorClicked(nullptr);
                const QList<QGraphicsItem*> items = selectedItems();
                for (QGraphicsItem *i : items) {
                    i->setSelected(false);
                }
                fitem->setSelected(true);
            }
            return;
        }
        case Qt::Key_Plus:
            if (fitem->type() == DependencyNodeItem::Type) {
                auto item = static_cast<DependencyNodeItem*>(fitem);
                if (item->isSummaryTask()) {
                    if (!item->isExpanded()) {
                        item->setExpanded(true);
                    }
                }
            }
            break;
        case Qt::Key_Minus:
            if (fitem->type() == DependencyNodeItem::Type) {
                auto item = static_cast<DependencyNodeItem*>(fitem);
                if (item->isSummaryTask()) {
                    if (item->isExpanded()) {
                        item->setExpanded(false);
                    }
                }
            }
            break;
        default:
            QGraphicsScene::keyPressEvent(keyEvent);
    }
    if (fitem) {
        fitem->parentItem() ? fitem->parentItem()->update() : fitem->update();
    }
    if (focusItem()) {
        focusItem()->parentItem() ? focusItem()->parentItem()->update() : focusItem()->update();
    }
    if (fitem != focusItem()) {
        Q_EMIT focusItemChanged(focusItem(), nullptr, Qt::OtherFocusReason); // we do not use olditem and focus reason
    }
}

DependencyNodeItem *DependencyScene::nodeItem(int row) const
{
    if (row < 0 || m_visibleItems.isEmpty()) {
        return nullptr;
    }
    for (DependencyNodeItem *i : std::as_const(m_visibleItems)) {
        if (i->row() == row) {
            return i;
        }
    }
    return nullptr;
}

void DependencyScene::singleConnectorClicked(DependencyConnectorItem *item)
{
    //debugPlanDepEditor;
    clearSelection();
    QList<DependencyConnectorItem*> lst;
    if (item == nullptr || item == fromItem()) {
        setFromItem(nullptr);
        m_clickedItems = lst;
    } else if (fromItem() == nullptr) {
        setFromItem(item);
    } else if (connectionIsValid(fromItem(), item)) {
        Q_EMIT connectItems(fromItem(), item);
        setFromItem(nullptr);
    } else {
        setFromItem(nullptr);
    }
    Q_EMIT connectorClicked(item);
}

void DependencyScene::multiConnectorClicked(DependencyConnectorItem *item)
{
    //debugPlanDepEditor;
    singleConnectorClicked(item);
}

void DependencyScene::clearConnection()
{
    setFromItem(nullptr);
    m_clickedItems.clear();
}

void DependencyScene::mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    //debugPlanDepEditor;
    QGraphicsScene::mousePressEvent(mouseEvent);
    if (! mouseEvent->isAccepted()) {
        clearConnection();
    }
}

void DependencyScene::mouseDoubleClickEvent (QGraphicsSceneMouseEvent *event)
{
    //debugPlanDepEditor<<event->pos()<<event->scenePos()<<event->screenPos();
    Q_EMIT itemDoubleClicked(itemAt(event->scenePos(), QTransform()));
}

void DependencyScene::contextMenuEvent (QGraphicsSceneContextMenuEvent *event)
{
    if (event->reason() == QGraphicsSceneContextMenuEvent::Mouse) {
        debugPlanDepEditor<<"Mouse:"<<itemAt(event->scenePos(), QTransform())<<event->pos()<<event->scenePos()<<event->screenPos();
        Q_EMIT contextMenuRequested(itemAt(event->scenePos(), QTransform()), event->screenPos());
        return;
    }
    if (focusItem()) {
        if (focusItem()->type() == DependencyConnectorItem::Type) {
            DependencyConnectorItem *to = static_cast<DependencyConnectorItem*>(focusItem());
            DependencyConnectorItem *from = fromItem();
            debugPlanDepEditor<<"DependencyConnectorItem:"<<from<<to;
            if (from) {
                DependencyLinkItem *link = findItem(from, to);
                if (link) {
                    Q_EMIT dependencyContextMenuRequested(link, to);
                    setFromItem(nullptr); // avoid showing spurious DependencyCreatorItem
                    return;
                } else debugPlanDepEditor<<"No link";
            }
        } else debugPlanDepEditor<<"Not connector type"<<focusItem();
    } else debugPlanDepEditor<<"No focusItem";
    Q_EMIT contextMenuRequested(focusItem(), QPoint());
}

void DependencyScene::setReadWrite(bool on)
{
    m_readwrite = on;
    const QList<QGraphicsItem*> items = this->items();
    for (QGraphicsItem *i : items) {
        if (i->type() == DependencyConnectorItem::Type) {
            static_cast<DependencyConnectorItem*>(i)->setEditable(on);
        } else if (i->type() == DependencyLinkItem::Type) {
            static_cast<DependencyLinkItem*>(i)->setEditable(on);
        }
    }
}

void DependencyScene::update()
{
    Q_EMIT sceneRectChanged(sceneRect());
}

//--------------------

DependencyView::DependencyView(QWidget *parent)
    : QGraphicsView(parent),
    m_project(nullptr),
    m_dirty(false),
    m_active(false)
{
    setItemScene(new DependencyScene(this));
    setAlignment(Qt::AlignLeft | Qt::AlignTop);

    connect(scene(), &QGraphicsScene::selectionChanged, this, &DependencyView::slotSelectionChanged);
    connect(itemScene(), &DependencyScene::connectItems, this, &DependencyView::makeConnection);

    connect(itemScene(), &DependencyScene::dependencyContextMenuRequested, this, &DependencyView::slotDependencyContextMenuRequested);

    connect(itemScene(), &DependencyScene::contextMenuRequested, this, &DependencyView::slotContextMenuRequested); // clazy:exclude=old-style-connect

    connect(itemScene(), &DependencyScene::focusItemChanged, this, &DependencyView::slotFocusItemChanged);

    m_autoScrollTimer.start(100);
    connect(&m_autoScrollTimer, &QTimer::timeout, this, &DependencyView::slotAutoScroll);
}

DependencyView::~DependencyView()
{
    disconnect(this, nullptr, nullptr, nullptr);
}

void DependencyView::slotContextMenuRequested(QGraphicsItem *item, const QPoint &pos)
{
    QPoint position = pos;
    if (item && position.isNull()) {
        position = mapToGlobal(item->mapToScene(item->boundingRect().topRight()).toPoint());
    } else if (!item && pos.isNull()) {
        position = mapToGlobal(geometry().topLeft() + geometry().bottomRight() / 6);
    }
    debugPlanDepEditor<<item<<pos<<':'<<position;
    Q_EMIT contextMenuRequested(item, position);
}

void DependencyView::slotDependencyContextMenuRequested(DependencyLinkItem *item, DependencyConnectorItem *connector)
{
    Q_UNUSED(connector)
    if (item) {
        debugPlanDepEditor<<item<<item->boundingRect()<<(item->mapToScene(item->pos()).toPoint())<<(mapToGlobal(item->mapToParent(item->pos()).toPoint()));
        Q_EMIT contextMenuRequested(item, mapToGlobal(item->mapToScene(item->boundingRect().topRight()).toPoint()));
    }
}

void DependencyView::slotConnectorClicked(DependencyConnectorItem *item)
{
    if (itemScene()->fromItem() == nullptr) {
        itemScene()->setFromItem(item);
    } else {
        //debugPlanDepEditor<<"Q_EMIT makeConnection:"<<static_cast<DependencyNodeItem*>(item->parentItem())->text();
        Q_EMIT makeConnection(itemScene()->fromItem(), item);
    }
}

void DependencyView::slotSelectionChanged()
{
    //HACK because of tt bug 160653
    QTimer::singleShot(0, this, &DependencyView::slotSelectedItems);
}

void DependencyView::slotSelectedItems()
{
    Q_EMIT selectionChanged(itemScene()->selectedItems());
}

void DependencyView::slotFocusItemChanged(QGraphicsItem *item)
{
    debugPlanDepEditor<<item;
    if (item) {
        ensureVisible(item, 10, 10);
    }
    Q_EMIT focusItemChanged(item);
}

void DependencyView::setItemScene(DependencyScene *scene)
{
    setScene(scene);
    scene->setProject(m_project);
    //slotResizeScene(m_treeview->viewport()->size());
    if (m_project) {
        createItems();
    }
}

void DependencyView::setActive(bool activate)
{
    m_active = activate;
    if (m_active && m_dirty) {
        createItems();
    }
}
void DependencyView::setProject(Project *project)
{
    if (m_project) {
        disconnect(m_project, &Project::relationAdded, this, &DependencyView::slotRelationAdded);
        disconnect(m_project, &Project::relationRemoved, this, &DependencyView::slotRelationRemoved);
        disconnect(m_project, &Project::relationModified, this, &DependencyView::slotRelationModified);

        disconnect(m_project, &Project::nodeAdded, this, &DependencyView::slotNodeAdded);
        disconnect(m_project, &Project::nodeRemoved, this, &DependencyView::slotNodeRemoved);
        disconnect(m_project, &Project::nodeChanged, this, &DependencyView::slotNodeChanged);
        disconnect(m_project, &Project::nodeMoved, this, &DependencyView::slotNodeMoved);

        if (itemScene()) {
            itemScene()->clearScene();
        }
    }
    m_project = project;
    if (project) {
        connect(m_project, &Project::relationAdded, this, &DependencyView::slotRelationAdded);
        connect(m_project, &Project::relationRemoved, this, &DependencyView::slotRelationRemoved);
        connect(m_project, &Project::relationModified, this, &DependencyView::slotRelationModified);

        connect(m_project, &Project::nodeAdded, this, &DependencyView::slotNodeAdded);
        connect(m_project, &Project::nodeRemoved, this, &DependencyView::slotNodeRemoved);
        connect(m_project, &Project::nodeChanged, this, &DependencyView::slotNodeChanged);
        connect(m_project, &Project::nodeMoved, this, &DependencyView::slotNodeMoved);

        connect(m_project, &Project::wbsDefinitionChanged, this, &DependencyView::slotWbsCodeChanged);

        if (itemScene()) {
            itemScene()->setProject(project);
            if (m_active) {
                createItems();
            } else {
                m_dirty = true;
            }
        }
    }
}

DependencyLinkItem *DependencyView::findItem(const Relation* rel) const
{
    return itemScene()->findItem(rel);
}

DependencyNodeItem *DependencyView::findItem(const Node *node) const
{
    return itemScene()->findItem(node);
}

void DependencyView::slotRelationAdded(Relation* rel)
{
    if (m_dirty) {
        return;
    }
    DependencyLinkItem *item = findItem(rel);
    if (item == nullptr) {
        DependencyNodeItem *p = findItem(rel->parent());
        DependencyNodeItem *c = findItem(rel->child());
        DependencyLinkItem *r = new DependencyLinkItem(p, c, rel);
        scene()->addItem(r);
        //debugPlanDepEditor;
        r->createPath();
        r->setVisible(c->isVisible() && p->isVisible());
    } else debugPlanDepEditor<<"Relation already exists!";
}

void DependencyView::slotRelationRemoved(Relation* rel)
{
    if (m_dirty) {
        return;
    }
    DependencyLinkItem *item = findItem(rel);
    if (item) {
        scene()->removeItem(item);
        delete item;
    } else debugPlanDepEditor<<"Relation does not exist!";
}

void DependencyView::slotRelationModified(Relation* rel)
{
    //debugPlanDepEditor;
    if (m_dirty) {
        return;
    }
    slotRelationRemoved(rel);
    slotRelationAdded(rel);
}

void DependencyView::slotNodeAdded(Node *node)
{
    //debugPlanDepEditor;
    if (m_dirty) {
        return;
    }
    DependencyNodeItem *item = findItem(node);
    if (item == nullptr) {
        item = createItem(node);
    } else {
        //debugPlanDepEditor<<node->name();
        itemScene()->setItemVisible(item, true);
    }
    ensureVisible(item);
    slotWbsCodeChanged();
}

void DependencyView::slotNodeRemoved(Node *node)
{
    if (m_dirty) {
        return;
    }
    DependencyNodeItem *item = findItem(node);
    if (item) {
        //debugPlanDepEditor<<node->name();
        delete item;
    } else debugPlanDepEditor<<"Node does not exist!";
    slotWbsCodeChanged();
}

void DependencyView::slotNodeChanged(Node *node)
{
    if (m_dirty) {
        return;
    }
    DependencyNodeItem *item = findItem(node);
    if (item && item->isVisible()) {
        item->setText();
        item->setSymbol();
    } else debugPlanDepEditor<<"Node does not exist!";
}

void DependencyView::slotWbsCodeChanged()
{
    if (m_dirty) {
        return;
    }
    const QList<DependencyNodeItem*> items = itemScene()->nodeItems();
    for (DependencyNodeItem *i : items) {
        if (i->isVisible()) {
            i->setText();
        }
    }
}

void DependencyView::slotNodeMoved(Node *node)
{
    if (m_dirty) {
        return;
    }
    slotNodeRemoved(node);
    slotNodeAdded(node);
}

void DependencyView::setItemExpanded(int , bool)
{
}

void DependencyView::createItems()
{
    itemScene()->clearScene();
    m_dirty = false;
    if (m_project == nullptr) {
        return;
    }
    scene()->addLine(0.0, 0.0, 1.0, 0.0);
    createItems(m_project);

    createLinks();
}

DependencyNodeItem *DependencyView::createItem(Node *node)
{
    return itemScene()->createItem(node);
}

void DependencyView::createItems(Node *node)
{
    if (node != m_project) {
        //debugPlanDepEditor<<node->name()<<" ("<<node->numChildren()<<")";
        DependencyNodeItem *i = createItem(node);
        if (i == nullptr) {
            return;
        }
    }
    const QList<Node*> nodes = node->childNodeIterator();
    for (Node *n : nodes) {
        createItems(n);
    }
}

void DependencyView::createLinks()
{
    //debugPlanDepEditor;
    itemScene()->createLinks();
}

void DependencyView::keyPressEvent(QKeyEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        switch (event->key()) {
            case Qt::Key_Plus:
                return scale(1.1, 1.1);
            case Qt::Key_Minus:
                return scale(0.9, 0.9);
            default: break;
        }
    }
    QGraphicsView::keyPressEvent(event);
}

void DependencyView::mouseMoveEvent(QMouseEvent *mouseEvent)
{
    m_cursorPos = mouseEvent->pos();
    if (itemScene()->connectionMode() && itemScene()->mouseGrabberItem()) {
        QPointF spos = mapToScene(m_cursorPos);
        Qt::CursorShape c = Qt::ArrowCursor;
        const QList<QGraphicsItem*> items = itemScene()->items(spos);
        for (QGraphicsItem *i : items) {
            if (i->type() == DependencyConnectorItem::Type) {
                if (i == itemScene()->fromItem()) {
                    c = ConnectCursor;
                } else {
                    if (itemScene()->connectionIsValid(itemScene()->fromItem(), static_cast<DependencyConnectorItem*>(i))) {
                        c = ConnectCursor;
                    } else {
                        c = Qt::ForbiddenCursor;
                    }
                }
            }
        }
        if (viewport()->cursor().shape() != c) {
            viewport()->setCursor(c);
        }
    }
    QGraphicsView::mouseMoveEvent(mouseEvent);
    //debugPlanDepEditor<<mouseEvent->scenePos()<<","<<mouseEvent->isAccepted();

}

void DependencyView::slotAutoScroll()
{
    if (itemScene()->connectionMode()) {
        ensureVisible(QRectF(mapToScene(m_cursorPos), QSizeF(1, 1)), 2, 2);
    }
}

//-----------------------------------
DependencyeditorConfigDialog::DependencyeditorConfigDialog(ViewBase *view, QWidget *p, bool selectPrint)
    : KPageDialog(p),
    m_view(view)
{
    setWindowTitle(i18n("Settings"));
    setFaceType(KPageDialog::Plain); // only one page, KPageDialog will use margins

    QTabWidget *tab = new QTabWidget();

    QWidget *w = ViewBase::createPageLayoutWidget(view);
    tab->addTab(w, w->windowTitle());
    m_pagelayout = w->findChild<KoPageLayoutWidget*>();
    Q_ASSERT(m_pagelayout);

    m_headerfooter = ViewBase::createHeaderFooterWidget(view);
    m_headerfooter->setOptions(view->printingOptions());
    tab->addTab(m_headerfooter, m_headerfooter->windowTitle());

    KPageWidgetItem *page = addPage(tab, i18n("Printing"));
    page->setHeader(i18n("Printing Options"));
    if (selectPrint) {
        setCurrentPage(page);
    }
    connect(this, &QDialog::accepted, this, &DependencyeditorConfigDialog::slotOk);
}

void DependencyeditorConfigDialog::slotOk()
{
    debugPlan;
    m_view->setPageLayout(m_pagelayout->pageLayout());
    m_view->setPrintingOptions(m_headerfooter->options());
}

//--------------------
DependencyEditor::DependencyEditor(KoPart *part, KoDocument *doc, QWidget *parent)
    : ViewBase(part, doc, parent),
    m_currentnode(nullptr),
    m_currentrelation(nullptr),
    m_manager(nullptr)
{
    setXMLFile(QStringLiteral("DependencyEditorUi.rc"));

    setupGui();

    QVBoxLayout * l = new QVBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);
    m_view = new DependencyView(this);
    l->addWidget(m_view);

    connect(m_view, &DependencyView::makeConnection, this, &DependencyEditor::slotCreateRelation);
    connect(m_view, &DependencyView::selectionChanged, this, &DependencyEditor::slotSelectionChanged);
    connect(m_view, &DependencyView::focusItemChanged, this, &DependencyEditor::slotFocusItemChanged);
    connect(m_view->itemScene(), &DependencyScene::itemDoubleClicked, this, &DependencyEditor::slotItemDoubleClicked);
    connect(m_view, &DependencyView::contextMenuRequested, this, &DependencyEditor::slotContextMenuRequested);

    setWhatsThis(
              xi18nc("@info:whatsthis",
                     "<title>Task Dependency Editor</title>"
                     "<para>"
                     "Edit dependencies between tasks."
                     "Dependencies can be added by dragging a connection area (start or finish)"
                     " from one task to a connection area of a different task."
                     " You can edit or delete a dependency using the context menu."
                     "</para><para>"
                     "This view supports printing using the context menu."
                     "<nl/><link url='%1'>More...</link>"
                     "</para>", QStringLiteral("plan:task-dependency-editor-graphical")));
}

void DependencyEditor::updateReadWrite(bool on)
{
    m_view->itemScene()->setReadWrite(on);
    ViewBase::updateReadWrite(on);
}

void DependencyEditor::slotItemDoubleClicked(QGraphicsItem *item)
{
    //debugPlanDepEditor;
    if (! isReadWrite()) {
        return;
    }
    if (item && item->type() == DependencyLinkItem::Type) {
        auto a = actionCollection()->action(QStringLiteral("edit_dependency"));
        if (a) {
            m_currentrelation = static_cast<DependencyLinkItem*>(item)->relation;
            a->trigger();
        }
        m_currentrelation = nullptr;
        return;
    }
    if (item && item->type() == DependencyNodeItem::Type) {
        auto a = actionCollection()->action(QStringLiteral("node_properties"));
        m_currentnode = static_cast<DependencyNodeItem*>(item)->node();
        if (a) {
            a->trigger();
        }
        return;
    }
}

void DependencyEditor::slotCreateRelation(DependencyConnectorItem *pred, DependencyConnectorItem *succ)
{
    //debugPlanDepEditor;
    if (! isReadWrite()) {
        return;
    }
    Node *par = pred->node();
    Node *ch = succ->node();
    Relation::Type type = Relation::FinishStart;
    if (pred->ctype() == DependencyNodeItem::Start) {
        if (succ->ctype() == DependencyNodeItem::Start) {
            type = Relation::StartStart;
        }
    } else {
        if (succ->ctype() == DependencyNodeItem::Start) {
            type = Relation::FinishStart;
        } else {
            type = Relation::FinishFinish;
        }
    }
    Relation *rel = ch->findRelation(par);
    if (rel == nullptr) {
        //debugPlanDepEditor<<"New:"<<par->name()<<" ->"<<ch->name()<<","<<type;
        slotAddRelation(par, ch, type);
    } else if (rel->type() != type) {
        //debugPlanDepEditor<<"Mod:"<<par->name()<<" ->"<<ch->name()<<","<<type;
        slotModifyRelation(rel, type);
    }
}

void DependencyEditor::draw(Project &project)
{
    m_view->setProject(&project);
}

void DependencyEditor::draw()
{
}

void DependencyEditor::setGuiActive(bool activate)
{
    //debugPlanDepEditor<<activate;
    updateActionsEnabled(true);
    ViewBase::setGuiActive(activate);
    m_view->setActive(activate);
/*    if (activate && !m_view->selectionModel()->currentIndex().isValid()) {
        m_view->selectionModel()->setCurrentIndex(m_view->model()->index(0, 0), QItemSelectionModel::NoUpdate);
    }*/
}

void DependencyEditor::slotCurrentChanged(const QModelIndex &, const QModelIndex &)
{
    //debugPlanDepEditor<<curr.row()<<","<<curr.column();
    slotEnableActions();
}

void DependencyEditor::slotSelectionChanged(const QList<QGraphicsItem*>&)
{
    //debugPlanDepEditor<<lst.count();
}

void DependencyEditor::slotFocusItemChanged(QGraphicsItem *item)
{
    m_currentnode = nullptr;
    auto itm = qgraphicsitem_cast<DependencyNodeItem*>(item);
    if (itm) {
        m_currentnode = itm->node();
    }
    slotEnableActions();
}

int DependencyEditor::selectedNodeCount() const
{
    return selectedNodes().count();
}

QList<Node*> DependencyEditor::selectedNodes() const {
    QList<Node*> lst;
    const QList<QGraphicsItem*> items = m_view->itemScene()->selectedItems();
    for (QGraphicsItem *i : items) {
        if (i->type() == DependencyNodeItem::Type) {
            lst << static_cast<DependencyNodeItem*>(i)->node();
        }
    }
    return lst;
}

Node *DependencyEditor::selectedNode() const
{
    QList<Node*> lst = selectedNodes();
    if (lst.count() != 1) {
        return nullptr;
    }
    return lst.first();
}

Node *DependencyEditor::currentNode() const {
    return m_currentnode;
/*    Node * n = 0;
    QGraphicsItem *i = m_view->itemScene()->focusItem();
    if (i && i->type() == DependencyNodeItem::Type) {
        n = static_cast<DependencyNodeItem*>(i)->node();
    }
    if (n == 0 || n->type() == Node::Type_Project) {
        return 0;
    }
    return n;*/
}

Relation *DependencyEditor::currentRelation() const {
    return m_currentrelation;
}

void DependencyEditor::setScheduleManager(ScheduleManager *sm)
{
    m_manager = sm;
}

void DependencyEditor::slotContextMenuRequested(QGraphicsItem *item, const QPoint& pos)
{
    //debugPlanDepEditor<<item<<","<<pos;
    if (! isReadWrite()) {
        return;
    }
    QString name;
    if (item && item->type() == DependencyNodeSymbolItem::Type) {
        item = item->parentItem();
    }
    if (item) {
        if (item->type() == DependencyNodeItem::Type) {
            m_currentnode = static_cast<DependencyNodeItem*>(item)->node();
            if (m_currentnode == nullptr) {
                //debugPlanDepEditor<<"No node";
                return;
            }
            bool scheduled = m_manager != nullptr && m_currentnode->isScheduled(m_manager->scheduleId());
            switch (m_currentnode->type()) {
                case Node::Type_Task:
                    name = scheduled ? QStringLiteral("task_popup") : QStringLiteral("task_edit_popup");
                    break;
                case Node::Type_Milestone:
                    name = scheduled ? QStringLiteral("taskeditor_milestone_popup") : QStringLiteral("task_edit_popup");
                    break;
                case Node::Type_Summarytask:
                    name = QStringLiteral("summarytask_popup");
                    break;
                default:
                    break;
            }
            //debugPlanDepEditor<<m_currentnode->name()<<" :"<<pos;
        } else if (item->type() == DependencyLinkItem::Type) {
            m_currentrelation = static_cast<DependencyLinkItem*>(item)->relation;
            if (m_currentrelation) {
                name = QStringLiteral("relation_popup");
            }
        } else if (item->type() == DependencyConnectorItem::Type) {
            DependencyConnectorItem *c = static_cast<DependencyConnectorItem*>(item);
            QList<DependencyLinkItem*> items;
            QList<QAction*> actions;
            QMenu menu;
            const QList<DependencyLinkItem*> preds = c->predecessorItems();
            for (DependencyLinkItem *i : preds) {
                items << i;
                actions << menu.addAction(koIcon("document-properties"), i->predItem->text());
            }
            menu.addSeparator();
            const QList<DependencyLinkItem*> succs = c->successorItems();
            for (DependencyLinkItem *i : succs) {
                items << i;
                actions << menu.addAction(koIcon("document-properties"), i->succItem->text());
            }
            if (! actions.isEmpty()) {
                menu.setActiveAction(actions.first());
                QAction *action = menu.exec(pos);
                if (action && actions.contains(action)) {
                    editRelation(items[ actions.indexOf(action) ]->relation);
                }
            }
            return;
        }
    }
    //debugPlanDepEditor<<name;
    if (! name.isEmpty()) {
        openContextMenu(name, pos);
    } else {
        QList<QAction*> lst = contextActionList();
        if (! lst.isEmpty()) {
            QMenu::exec(lst, pos,  lst.first());
        }
    }
    m_currentnode = nullptr;
    m_currentrelation = nullptr;
}

void DependencyEditor::slotEnableActions()
{
    updateActionsEnabled(true);
}

void DependencyEditor::updateActionsEnabled(bool on)
{
    if (! on || ! isReadWrite()) { //FIXME: read-write is not set properly
        menuAddTask->setEnabled(false);
        actionAddTask->setEnabled(false);
        actionAddMilestone->setEnabled(false);
        menuAddSubTask->setEnabled(false);
        actionAddSubtask->setEnabled(false);
        actionAddSubMilestone->setEnabled(false);
        actionDeleteTask->setEnabled(false);
        actionLinkTask->setEnabled(false);
        return;
    }
    if (!m_currentnode) {
        // allow adding to project
        menuAddTask->setEnabled(true);
        actionAddTask->setEnabled(true);
        actionAddMilestone->setEnabled(true);
        menuAddSubTask->setEnabled(false);
        actionAddSubtask->setEnabled(false);
        actionAddSubMilestone->setEnabled(false);
        actionDeleteTask->setEnabled(false);
        actionLinkTask->setEnabled(false);
        return;
    }
    const auto nodes = selectedNodes();
    if (nodes.count() > 1) {
        bool baselined = false;
        Project *p = m_view->project();
        if (p && p->isBaselined()) {
            for (const auto n : nodes) {
                if (n->isBaselined()) {
                    baselined = true;
                    break;
                }
            }
        }
        // only allow delete selected if not baselined
        menuAddTask->setEnabled(false);
        actionAddTask->setEnabled(false);
        actionAddMilestone->setEnabled(false);
        menuAddSubTask->setEnabled(false);
        actionAddSubtask->setEnabled(false);
        actionAddSubMilestone->setEnabled(false);
        actionDeleteTask->setEnabled(!baselined);
        actionLinkTask->setEnabled(false);
        return;
    }
    const Node *n = m_currentnode;
    if (n->type() != Node::Type_Task && n->type() != Node::Type_Milestone && n->type() != Node::Type_Summarytask) {
        // should not happen, disable everything
        menuAddTask->setEnabled(false);
        actionAddTask->setEnabled(false);
        actionAddMilestone->setEnabled(false);
        menuAddSubTask->setEnabled(false);
        actionAddSubtask->setEnabled(false);
        actionAddSubMilestone->setEnabled(false);
        actionDeleteTask->setEnabled(false);
        actionLinkTask->setEnabled(false);
        return;
    }
    const bool baselined = n->isBaselined();
    menuAddTask->setEnabled(true);
    actionAddTask->setEnabled(true);
    actionAddMilestone->setEnabled(true);
    menuAddSubTask->setEnabled(!baselined);
    actionAddSubtask->setEnabled(!baselined);
    actionAddSubMilestone->setEnabled(!baselined);
    actionDeleteTask->setEnabled(!baselined);
    actionLinkTask->setEnabled(true);
}

void DependencyEditor::setupGui()
{
    KActionCollection *coll = actionCollection();

    auto actionEditRelation  = new QAction(koIcon("document-edit"), i18n("Edit Dependency..."), this);
    actionCollection()->addAction(QStringLiteral("edit_dependency"), actionEditRelation);
    connect(actionEditRelation, &QAction::triggered, this, &DependencyEditor::slotModifyCurrentRelation);

    auto actionDeleteRelation  = new QAction(koIcon("edit-delete"), i18n("Delete Dependency"), this);
    actionCollection()->addAction(QStringLiteral("delete_dependency"), actionDeleteRelation);
    connect(actionDeleteRelation, &QAction::triggered, this, &DependencyEditor::slotDeleteRelation);

    actionLinkTask  = new QAction(koIcon("link"), xi18nc("@action", "Link"), this);
    actionCollection()->setDefaultShortcut(actionLinkTask, Qt::CTRL | Qt::Key_L);
    actionCollection()->addAction(QStringLiteral("link_task"), actionLinkTask);
    connect(actionLinkTask, &QAction::triggered, this, &DependencyEditor::slotLinkTask);

    menuAddTask = new KActionMenu(koIcon("view-task-add"), i18n("Add Task"), this);
    coll->addAction(QStringLiteral("add_task"), menuAddTask);
    connect(menuAddTask, &QAction::triggered, this, &DependencyEditor::slotAddTask);

    actionAddTask  = new QAction(i18n("Add Task..."), this);
    actionAddTask->setShortcut(Qt::CTRL | Qt::Key_I);
    connect(actionAddTask, &QAction::triggered, this, &DependencyEditor::slotAddTask);
    menuAddTask->addAction(actionAddTask);

    actionAddMilestone  = new QAction(i18n("Add Milestone..."), this);
    actionAddMilestone->setShortcut(Qt::CTRL | Qt::ALT | Qt::Key_I);
    connect(actionAddMilestone, &QAction::triggered, this, &DependencyEditor::slotAddMilestone);
    menuAddTask->addAction(actionAddMilestone);


    menuAddSubTask = new KActionMenu(koIcon("view-task-child-add"), i18n("Add Sub-Task"), this);
    coll->addAction(QStringLiteral("add_subtask"), menuAddSubTask);
    connect(menuAddSubTask, &QAction::triggered, this, &DependencyEditor::slotAddSubtask);

    actionAddSubtask  = new QAction(i18n("Add Sub-Task..."), this);
    actionAddSubtask->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_I);
    connect(actionAddSubtask, &QAction::triggered, this, &DependencyEditor::slotAddSubtask);
    menuAddSubTask->addAction(actionAddSubtask);

    actionAddSubMilestone = new QAction(i18n("Add Sub-Milestone..."), this);
    actionAddSubMilestone->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::ALT | Qt::Key_I);
    connect(actionAddSubMilestone, &QAction::triggered, this, &DependencyEditor::slotAddSubMilestone);
    menuAddSubTask->addAction(actionAddSubMilestone);

    actionDeleteTask  = new QAction(koIcon("edit-delete"), xi18nc("@action", "Delete"), this);
    coll->addAction(QStringLiteral("delete_task"), actionDeleteTask);
    coll->setDefaultShortcut(actionDeleteTask, Qt::Key_Delete);
    connect(actionDeleteTask, &QAction::triggered, this, &DependencyEditor::slotDeleteTask);

    auto actionOpenNode  = new QAction(koIcon("document-edit"), i18n("Edit..."), this);
    actionCollection()->addAction(QStringLiteral("node_properties"), actionOpenNode);
    connect(actionOpenNode, &QAction::triggered, this, &DependencyEditor::slotOpenCurrentNode);

    auto actionTaskProgress  = new QAction(koIcon("document-edit"), i18n("Progress..."), this);
    actionCollection()->addAction(QStringLiteral("task_progress"), actionTaskProgress);
    connect(actionTaskProgress, &QAction::triggered, this, &DependencyEditor::slotTaskProgress);

    auto actionTaskDescription  = new QAction(koIcon("document-edit"), i18n("Description..."), this);
    actionCollection()->addAction(QStringLiteral("task_description"), actionTaskDescription);
    connect(actionTaskDescription, &QAction::triggered, this, &DependencyEditor::slotTaskDescription);

    auto actionDocuments  = new QAction(koIcon("document-edit"), i18n("Documents..."), this);
    actionCollection()->addAction(QStringLiteral("task_documents"), actionDocuments);
    connect(actionDocuments, &QAction::triggered, this, &DependencyEditor::slotDocuments);

    createOptionActions(ViewBase::OptionPrint | ViewBase::OptionPrintPreview | ViewBase::OptionPrintPdf | ViewBase::OptionPrintConfig);
}

void DependencyEditor::slotOptions()
{
    debugPlan;
    DependencyeditorConfigDialog *dlg = new DependencyeditorConfigDialog(this, this, sender()->objectName() == QStringLiteral("print_options"));
    connect(dlg, &QDialog::finished, this, &DependencyEditor::slotOptionsFinished);
    dlg->open();
}

void DependencyEditor::openRelationDialog(Node *par, Node *child)
{
    //debugPlan;
    Relation * rel = new Relation(par, child);
    AddRelationDialog *dia = new AddRelationDialog(*project(), rel, m_view);
    connect(dia, &QDialog::finished, this, &DependencyEditor::slotAddRelationFinished);
    dia->open();
}

void DependencyEditor::slotAddRelation(KPlato::Node *par, KPlato::Node *child, int linkType)
{
    //debugPlan;
    if (linkType == Relation::FinishStart ||
            linkType == Relation::StartStart ||
            linkType == Relation::FinishFinish) {
        Relation * rel = new Relation(par, child, static_cast<Relation::Type>(linkType));
        koDocument()->addCommand(new AddRelationCmd(*project(), rel, kundo2_i18n("Add task dependency")));
    } else {
        openRelationDialog(par, child);
    }
}

void DependencyEditor::slotAddRelationFinished(int result)
{
    AddRelationDialog *dia = qobject_cast<AddRelationDialog*>(sender());
    if (dia == nullptr) {
        return;
    }
    if (result == QDialog::Accepted) {
        KUndo2Command * m = dia->buildCommand();
        if (m) {
            koDocument()->addCommand(m);
        }
    }
    dia->deleteLater();
}

void DependencyEditor::editRelation(Relation *rel)
{
    //debugPlan;
    ModifyRelationDialog *dia = new ModifyRelationDialog(*project(), rel, this);
    connect(dia, &QDialog::finished, this, &DependencyEditor::slotModifyRelationFinished);
    dia->open();
}

void DependencyEditor::slotModifyRelation(KPlato::Relation *rel, int linkType)
{
    //debugPlan;
    if (linkType == Relation::FinishStart ||
            linkType == Relation::StartStart ||
            linkType == Relation::FinishFinish) {
        koDocument()->addCommand(new ModifyRelationTypeCmd(rel, static_cast<Relation::Type>(linkType)));
    } else {
        editRelation(rel);
    }
}

void DependencyEditor::slotModifyCurrentRelation()
{
    Relation *rel = currentRelation();
    if (rel) {
        editRelation(rel);
    }
}

void DependencyEditor::slotModifyRelationFinished(int result)
{
    ModifyRelationDialog *dia = qobject_cast<ModifyRelationDialog*>(sender());
    if (dia == nullptr) {
        return ;
    }
    if (result == QDialog::Accepted) {
        KUndo2Command *cmd = dia->buildCommand();
        if (cmd) {
            koDocument()->addCommand(cmd);
        }
    }
    dia->deleteLater();
}


void DependencyEditor::slotDeleteRelation()
{
    Relation *rel = currentRelation();
    if (rel) {
        koDocument()->addCommand(new DeleteRelationCmd(*project(), rel, kundo2_i18n("Delete task dependency")));
    }
}

void DependencyEditor::slotAddTask()
{
    Task * node = project()->createTask(project()->config().taskDefaults());
    TaskAddDialog *dia = new TaskAddDialog(*project(), *node, currentNode(), project()->accounts(), this);
    connect(dia, &QDialog::finished, this, &DependencyEditor::slotAddTaskFinished);
    dia->open();
}

void DependencyEditor::slotAddTaskFinished(int result)
{
    TaskAddDialog *dia = qobject_cast<TaskAddDialog*>(sender());
    if (dia == nullptr) {
        return;
    }
    if (result == QDialog::Accepted) {
        KUndo2Command *m = dia->buildCommand();
        koDocument()->addCommand(m); // add task to project
    }
    dia->deleteLater();
}

void DependencyEditor::slotAddMilestone()
{
    Task * node = project()->createTask();
    node->estimate() ->clear();

    TaskAddDialog *dia = new TaskAddDialog(*project(), *node, currentNode(), project()->accounts(), this);
    connect(dia, &QDialog::finished, this, &DependencyEditor::slotAddMilestoneFinished);
    dia->open();
}

void DependencyEditor::slotAddMilestoneFinished(int result)
{
    TaskAddDialog *dia = qobject_cast<TaskAddDialog*>(sender());
    if (dia == nullptr) {
        return;
    }
    if (result == QDialog::Accepted) {
        MacroCommand *c = new MacroCommand(kundo2_i18n("Add milestone"));
        c->addCommand(dia->buildCommand());
        koDocument()->addCommand(c); // add task to project
    }
    dia->deleteLater();
}

void DependencyEditor::slotAddSubtask()
{
    Task * node = project()->createTask(project()->config().taskDefaults());
    SubTaskAddDialog *dia = new SubTaskAddDialog(*project(), *node, currentNode(), project()->accounts(), this);
    connect(dia, &QDialog::finished, this, &DependencyEditor::slotAddSubTaskFinished);
    dia->open();
}

void DependencyEditor::slotAddSubTaskFinished(int result)
{
    SubTaskAddDialog *dia = qobject_cast<SubTaskAddDialog*>(sender());
    if (dia == nullptr) {
        return;
    }
    if (result  == QDialog::Accepted) {
        KUndo2Command *m = dia->buildCommand();
        koDocument()->addCommand(m); // add task to project
    }
    dia->deleteLater();
}

void DependencyEditor::slotAddSubMilestone()
{
    Task * node = project()->createTask();
    node->estimate() ->clear();

    SubTaskAddDialog *dia = new SubTaskAddDialog(*project(), *node, currentNode(), project()->accounts(), this);
    connect(dia, &QDialog::finished, this, &DependencyEditor::slotAddSubMilestoneFinished);
    dia->open();
}

void DependencyEditor::slotAddSubMilestoneFinished(int result)
{
    SubTaskAddDialog *dia = qobject_cast<SubTaskAddDialog*>(sender());
    if (dia == nullptr) {
        return;
    }
    if (result == QDialog::Accepted) {
        MacroCommand *c = new MacroCommand(kundo2_i18n("Add sub-milestone"));
        c->addCommand(dia->buildCommand());
        koDocument()->addCommand(c); // add task to project
    }
    dia->deleteLater();
}

void DependencyEditor::edit(const QModelIndex &i)
{
    if (i.isValid()) {
/*        QModelIndex p = m_view->itemModel()->parent(i);
        m_view->treeView()->setExpanded(p, true);
        m_view->treeView()->selectionModel()->setCurrentIndex(i, QItemSelectionModel::NoUpdate);
        m_view->treeView()->edit(i);*/
    }
}

void DependencyEditor::slotDeleteTask()
{
    //debugPlanDepEditor;
    QList<Node*> lst = selectedNodes();
    if (lst.count() == 1) {
        koDocument()->addCommand(new NodeDeleteCmd(lst.first(), kundo2_i18nc("Delete one task", "Delete task")));
    }
}

void DependencyEditor::slotOpenCurrentNode()
{
    //debugPlan;
    Node * node = currentNode();
    slotOpenNode(node);
}

void DependencyEditor::slotOpenNode(KPlato::Node *node)
{
    //debugPlan;
    if (!node) {
        return ;
    }
    switch (node->type()) {
        case Node::Type_Task: {
                Task *task = static_cast<Task *>(node);
                TaskDialog *dia = new TaskDialog(*project(), *task, project()->accounts(), m_view);
                connect(dia, &QDialog::finished, this, &DependencyEditor::slotTaskEditFinished);
                dia->open();
                break;
            }
        case Node::Type_Milestone: {
                // Use the normal task dialog for now.
                // Maybe milestone should have it's own dialog, but we need to be able to
                // enter a duration in case we accidentally set a tasks duration to zero
                // and hence, create a milestone
                Task *task = static_cast<Task *>(node);
                TaskDialog *dia = new TaskDialog(*project(), *task, project()->accounts(), m_view);
                connect(dia, &QDialog::finished, this, &DependencyEditor::slotTaskEditFinished);
                dia->open();
                break;
            }
        case Node::Type_Summarytask: {
                Task *task = dynamic_cast<Task *>(node);
                Q_ASSERT(task);
                SummaryTaskDialog *dia = new SummaryTaskDialog(*task, m_view);
                connect(dia, &QDialog::finished, this, &DependencyEditor::slotSummaryTaskEditFinished);
                dia->open();
                break;
            }
        default:
            break; // avoid warnings
    }
}

void DependencyEditor::slotTaskEditFinished(int result)
{
    TaskDialog *dia = qobject_cast<TaskDialog*>(sender());
    if (dia == nullptr) {
        return;
    }
    if (result == QDialog::Accepted) {
        KUndo2Command * cmd = dia->buildCommand();
        if (cmd) {
            koDocument()->addCommand(cmd);
        }
    }
    dia->deleteLater();
}

void DependencyEditor::slotSummaryTaskEditFinished(int result)
{
    SummaryTaskDialog *dia = qobject_cast<SummaryTaskDialog*>(sender());
    if (dia == nullptr) {
        return;
    }
    if (result == QDialog::Accepted) {
        KUndo2Command * cmd = dia->buildCommand();
        if (cmd) {
            koDocument()->addCommand(cmd);
        }
    }
    dia->deleteLater();
}

void DependencyEditor::slotTaskProgress()
{
    //debugPlan;
    Node * node = currentNode();
    if (!node)
        return ;

    switch (node->type()) {
        case Node::Type_Project: {
                break;
            }
        case Node::Type_Subproject:
            //TODO
            break;
        case Node::Type_Task: {
                Task *task = dynamic_cast<Task *>(node);
                Q_ASSERT(task);
                TaskProgressDialog *dia = new TaskProgressDialog(*task, scheduleManager(),  project()->standardWorktime(), m_view);
                connect(dia, &QDialog::finished, this, &DependencyEditor::slotTaskProgressFinished);
                dia->open();
                break;
            }
        case Node::Type_Milestone: {
                Task *task = dynamic_cast<Task *>(node);
                Q_ASSERT(task);
                MilestoneProgressDialog *dia = new MilestoneProgressDialog(*task, m_view);
                connect(dia, &QDialog::finished, this, &DependencyEditor::slotMilestoneProgressFinished);
                dia->open();
                break;
            }
        case Node::Type_Summarytask: {
                // TODO
                break;
            }
        default:
            break; // avoid warnings
    }
}

void DependencyEditor::slotTaskProgressFinished(int result)
{
    TaskProgressDialog *dia = qobject_cast<TaskProgressDialog*>(sender());
    if (dia == nullptr) {
        return;
    }
    if (result == QDialog::Accepted) {
        KUndo2Command * m = dia->buildCommand();
        if (m) {
            koDocument()->addCommand(m);
        }
    }
    dia->deleteLater();
}

void DependencyEditor::slotMilestoneProgressFinished(int result)
{
    MilestoneProgressDialog *dia = qobject_cast<MilestoneProgressDialog*>(sender());
    if (dia == nullptr) {
        return;
    }
    if (result == QDialog::Accepted) {
        KUndo2Command * m = dia->buildCommand();
        if (m) {
            koDocument()->addCommand(m);
        }
    }
    dia->deleteLater();
}

void DependencyEditor::slotOpenProjectDescription()
{
    debugPlan<<koDocument()->isReadWrite();
    TaskDescriptionDialog *dia = new TaskDescriptionDialog(*project(), m_view, !isReadWrite());
    connect(dia, &QDialog::finished, this, &DependencyEditor::slotTaskDescriptionFinished);
    dia->open();
}

void DependencyEditor::slotTaskDescription()
{
    slotOpenTaskDescription(!isReadWrite());
}

void DependencyEditor::slotOpenTaskDescription(bool ro)
{
    //debugPlan;
    Node * node = currentNode();
    if (!node)
        return ;

    switch (node->type()) {
        case Node::Type_Subproject:
            //TODO
            break;
        case Node::Type_Project:
        case Node::Type_Task:
        case Node::Type_Milestone:
        case Node::Type_Summarytask: {
                TaskDescriptionDialog *dia = new TaskDescriptionDialog(*node, m_view, ro);
                connect(dia, &QDialog::finished, this, &DependencyEditor::slotTaskDescriptionFinished);
                dia->open();
                break;
            }
        default:
            break; // avoid warnings
    }
}

void DependencyEditor::slotTaskDescriptionFinished(int result)
{
    TaskDescriptionDialog *dia = qobject_cast<TaskDescriptionDialog*>(sender());
    if (dia == nullptr) {
        return;
    }
    if (result == QDialog::Accepted) {
        KUndo2Command * m = dia->buildCommand();
        if (m) {
            koDocument()->addCommand(m);
        }
    }
    dia->deleteLater();
}

void DependencyEditor::slotDocuments()
{
    //debugPlan;
    Node * node = currentNode();
    if (!node) {
        return ;
    }
    switch (node->type()) {
        case Node::Type_Subproject:
            //TODO
            break;
        case Node::Type_Project:
        case Node::Type_Summarytask:
        case Node::Type_Task:
        case Node::Type_Milestone: {
            DocumentsDialog *dia = new DocumentsDialog(*node, m_view);
            connect(dia, &QDialog::finished, this, &DependencyEditor::slotDocumentsFinished);
            dia->open();
            break;
        }
        default:
            break; // avoid warnings
    }
}

void DependencyEditor::slotDocumentsFinished(int result)
{
    DocumentsDialog *dia = qobject_cast<DocumentsDialog*>(sender());
    if (dia == nullptr) {
        return;
    }
    if (result == QDialog::Accepted) {
        KUndo2Command * m = dia->buildCommand();
        if (m) {
            koDocument()->addCommand(m);
        }
    }
    dia->deleteLater();
}

void DependencyEditor::slotLinkTask()
{
    //debugPlan;
    Node *n = selectedNode();
    if (n && project()) {
        RelationEditorDialog dlg(project(), n);
        if (dlg.exec()) {
            KUndo2Command *cmd = dlg.buildCommand();
            if (cmd) {
                koDocument()->addCommand(cmd);
            }
        }
    }
}

KoPrintJob *DependencyEditor::createPrintJob()
{
    DependecyViewPrintingDialog *dia = new DependecyViewPrintingDialog(this, m_view);
    dia->printer().setCreator(QStringLiteral("Plan %1").arg(QStringLiteral(PLAN_VERSION_STRING)));
//    dia->printer().setFullPage(true); // ignore printer margins
    return dia;
}

} // namespace KPlato
