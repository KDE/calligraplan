/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2007-2010 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kptviewlist.h"

#include <QString>
#include <QStringList>
#include <QItemDelegate>
#include <QStyle>
#include <QBrush>
#include <QContextMenuEvent>
#include <QHeaderView>
#include <QMenu>

#include <KMessageBox>
#include <KComboBox>

#include <KoIcon.h>
#include "KoDocument.h"

#include "kptviewbase.h"
#include "kptmaindocument.h"
#include "kptviewlistdialog.h"
#include "kptviewlistdocker.h"
#include "kptschedulemodel.h"
#include "Help.h"
#include <kptdebug.h>

#include <assert.h>


namespace KPlato
{

// <Code mostly nicked from qt designer ;)>
class ViewCategoryDelegate : public QItemDelegate
{
    public:
        ViewCategoryDelegate(QObject *parent, QTreeView *view)
        : QItemDelegate(parent),
        m_view(view)
        {}

        QSize sizeHint (const QStyleOptionViewItem & option, const QModelIndex & index) const override;
        void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const override;

    private:
        QTreeView *m_view;
};

QSize ViewCategoryDelegate::sizeHint (const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    const QAbstractItemModel * model = index.model();
    Q_ASSERT(model);
    if (model->parent(index).isValid()) {
        return QItemDelegate::sizeHint(option, index);
    }
    return QItemDelegate::sizeHint(option, index).expandedTo(QSize(0, 16));
}

void ViewCategoryDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    const QAbstractItemModel * model = index.model();
    Q_ASSERT(model);

    if (!model->parent(index).isValid()) {
        // this is a top-level item.
        QStyleOptionButton buttonOption;
        buttonOption.state = option.state;

        buttonOption.rect = option.rect;
        buttonOption.palette = option.palette;
        buttonOption.features = QStyleOptionButton::None;
        m_view->style() ->drawControl(QStyle::CE_PushButton, &buttonOption, painter, m_view);

        QStyleOption branchOption;
        static const int i = 9; // ### hardcoded in qcommonstyle.cpp
        QRect r = option.rect;
        branchOption.rect = QRect(r.left() + i / 2, r.top() + (r.height() - i) / 2, i, i);
        branchOption.palette = option.palette;
        branchOption.state = QStyle::State_Children;

        if (m_view->isExpanded(index))
            branchOption.state |= QStyle::State_Open;

        m_view->style() ->drawPrimitive(QStyle::PE_IndicatorBranch, &branchOption, painter, m_view);

        // draw text
        QRect textrect = QRect(r.left() + i * 2, r.top(), r.width() - ((5 * i) / 2), r.height());
        QString text = option.fontMetrics.elidedText(model->data(index, Qt::DisplayRole).toString(), Qt::ElideMiddle, textrect.width());
        m_view->style() ->drawItemText(painter, textrect, Qt::AlignLeft|Qt::AlignVCenter,
        option.palette, m_view->isEnabled(), text);

    } else {
        QItemDelegate::paint(painter, option, index);
    }

}

ViewListItem::ViewListItem(const QString &tag, const QStringList &strings, int type)
    : QTreeWidgetItem(strings, type),
    m_tag(tag)
{
}

ViewListItem::ViewListItem(QTreeWidget *parent, const QString &tag, const QStringList &strings, int type)
    : QTreeWidgetItem(parent, strings, type),
    m_tag(tag)
{
}

ViewListItem::ViewListItem(QTreeWidgetItem *parent, const QString &tag, const QStringList &strings, int type)
    : QTreeWidgetItem(parent, strings, type),
    m_tag(tag)
{
}

void ViewListItem::setReadWrite(bool rw)
{
    if (type() == ItemType_SubView) {
        static_cast<ViewBase*>(view())->updateReadWrite(rw);
    }
}

void ViewListItem::setView(ViewBase *view)
{
    setData(0, ViewListItem::DataRole_View,  QVariant::fromValue<QObject*>(static_cast<QObject*>(view)));
}

ViewBase *ViewListItem::view() const
{
    if (data(0, ViewListItem::DataRole_View).isValid()) {
        return static_cast<ViewBase*>(data(0, ViewListItem::DataRole_View).value<QObject*>());
    }
    return nullptr;
}

void ViewListItem::setDocument(KoDocument *doc)
{
    setData(0, ViewListItem::DataRole_Document,  QVariant::fromValue<QObject*>(static_cast<QObject*>(doc)));
}

KoDocument *ViewListItem::document() const
{
    if (data(0, ViewListItem::DataRole_Document).isValid()) {
        return static_cast<KoDocument*>(data(0, ViewListItem::DataRole_Document).value<QObject*>());
    }
    return nullptr;
}

QString ViewListItem::viewType() const
{
    if (type() != ItemType_SubView) {
        return QString();
    }
    QString name = QLatin1String(view()->metaObject()->className());
    if (name.contains(QLatin1Char(':'))) {
        name = name.remove(0, name.lastIndexOf(QLatin1Char(':')) + 1);
    }
    return name;
}

void ViewListItem::save(QDomElement &element) const
{
    element.setAttribute(QStringLiteral("itemtype"), QString::number(type()));
    element.setAttribute(QStringLiteral("tag"), tag());

    if (type() == ItemType_SubView) {
        element.setAttribute(QStringLiteral("viewtype"), viewType());
        element.setAttribute(QStringLiteral("name"), m_viewinfo.name == text(0) ? QLatin1String("") : text(0));
        element.setAttribute(QStringLiteral("tooltip"), m_viewinfo.tip == toolTip(0) ? QStringLiteral(TIP_USE_DEFAULT_TEXT) : toolTip(0));
    } else if (type() == ItemType_Category) {
        debugPlan<<text(0)<<m_viewinfo.name;
        element.setAttribute(QStringLiteral("name"), text(0) == m_viewinfo.name ? QLatin1String("") : text(0));
        element.setAttribute(QStringLiteral("tooltip"), toolTip(0).isEmpty() ? QStringLiteral(TIP_USE_DEFAULT_TEXT) : toolTip(0));
    }
}

ViewListTreeWidget::ViewListTreeWidget(QWidget *parent)
    : QTreeWidget(parent)
{
    header() ->hide();
    setRootIsDecorated(false);
    setItemDelegate(new ViewCategoryDelegate(this, this));
    setItemsExpandable(true);
    setSelectionMode(QAbstractItemView::SingleSelection);

    setDragDropMode(QAbstractItemView::InternalMove);

    //setContextMenuPolicy(Qt::ActionsContextMenu);

    connect(this, &QTreeWidget::itemPressed, this, &ViewListTreeWidget::handleMousePress);
}

void ViewListTreeWidget::drawRow(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QTreeWidget::drawRow(painter, option, index);
}

void ViewListTreeWidget::handleMousePress(QTreeWidgetItem *item)
{
    //debugPlan;
    if (item == nullptr)
        return ;

    if (item->parent() == nullptr) {
        item->setExpanded(!item->isExpanded());
        return ;
    }
}
void ViewListTreeWidget::mousePressEvent (QMouseEvent *event)
{
    if (event->button() == Qt::RightButton) {
        QTreeWidgetItem *item = itemAt(event->pos());
        if (item && item->type() == ViewListItem::ItemType_Category) {
            setCurrentItem(item);
            Q_EMIT customContextMenuRequested(event->pos());
            event->accept();
            return;
        }
    }
    QTreeWidget::mousePressEvent(event);
}

void ViewListTreeWidget::save(QDomElement &element) const
{
    int cnt = topLevelItemCount();
    if (cnt == 0) {
        return;
    }
    QDomElement cs = element.ownerDocument().createElement(QStringLiteral("categories"));
    element.appendChild(cs);
    for (int i = 0; i < cnt; ++i) {
        ViewListItem *itm = static_cast<ViewListItem*>(topLevelItem(i));
        if (itm->type() != ViewListItem::ItemType_Category) {
            continue;
        }
        QDomElement c = cs.ownerDocument().createElement(QStringLiteral("category"));
        cs.appendChild(c);
        Q_EMIT const_cast<ViewListTreeWidget*>(this)->updateViewInfo(itm);
        itm->save(c);
        for (int j = 0; j < itm->childCount(); ++j) {
            ViewListItem *vi = static_cast<ViewListItem*>(itm->child(j));
            if (vi->type() != ViewListItem::ItemType_SubView) {
                continue;
            }
            QDomElement el = c.ownerDocument().createElement(QStringLiteral("view"));
            c.appendChild(el);
            Q_EMIT const_cast<ViewListTreeWidget*>(this)->updateViewInfo(vi);
            vi->save(el);
            QDomElement elm = el.ownerDocument().createElement(QStringLiteral("settings"));
            el.appendChild(elm);
            static_cast<ViewBase*>(vi->view())->saveContext(elm);
        }
    }
}


// </Code mostly nicked from qt designer ;)>

void ViewListTreeWidget::startDrag(Qt::DropActions supportedActions)
{
    QModelIndexList indexes = selectedIndexes();
    if (indexes.count() == 1) {
        ViewListItem *item = static_cast<ViewListItem*>(itemFromIndex(indexes.at(0)));
        Q_ASSERT(item);
        QTreeWidgetItem *root = invisibleRootItem();
        int count = root->childCount();
        if (item && item->type() == ViewListItem::ItemType_Category) {
            root->setFlags(root->flags() | Qt::ItemIsDropEnabled);
            for (int i = 0; i < count; ++i) {
                QTreeWidgetItem * ch = root->child(i);
                ch->setFlags(ch->flags() & ~Qt::ItemIsDropEnabled);
            }
        } else if (item) {
            root->setFlags(root->flags() & ~Qt::ItemIsDropEnabled);
            for (int i = 0; i < count; ++i) {
                QTreeWidgetItem * ch = root->child(i);
                ch->setFlags(ch->flags() | Qt::ItemIsDropEnabled);
            }
        }
    }
    QTreeWidget::startDrag(supportedActions);
}

void ViewListTreeWidget::dropEvent(QDropEvent *event)
{
    QTreeWidget::dropEvent(event);
    if (event->isAccepted()) {
        Q_EMIT modified();
    }
}

ViewListItem *ViewListTreeWidget::findCategory(const QString &cat)
{
    QTreeWidgetItem * item;
    int cnt = topLevelItemCount();
    for (int i = 0; i < cnt; ++i) {
        item = topLevelItem(i);
        if (static_cast<ViewListItem*>(item)->tag() == cat)
            return static_cast<ViewListItem*>(item);
    }
    return nullptr;
}

ViewListItem *ViewListTreeWidget::category(const KoView *view) const
{
    QTreeWidgetItem * item;
    int cnt = topLevelItemCount();
    for (int i = 0; i < cnt; ++i) {
        item = topLevelItem(i);
        for (int c = 0; c < item->childCount(); ++c) {
            if (view == static_cast<ViewListItem*>(item->child(c))->view()) {
                return static_cast<ViewListItem*>(item);
            }
        }
    }
    return nullptr;
}

//-----------------------
ViewListWidget::ViewListWidget(MainDocument *part, QWidget *parent)//QString name, KXmlGuiWindow *parent)
    : QWidget(parent),
    m_part(part),
    m_prev(nullptr),
    m_temp(nullptr)
{
    setObjectName(QStringLiteral("ViewListWidget"));
    setWhatsThis(
              xi18nc("@info:whatsthis",
                     "<title>View Selector</title>"
                     "<para>This is the list of views and editors.</para>"
                     "<para>You can configure the list by using the context menu:"
                     "<list>"
                     "<item>Rename categories or views</item>"
                     "<item>Configure. Move, remove, rename or edit tool tip for categories or views</item>"
                     "<item>Insert categories and views</item>"
                     "</list>"
                     "<nl/><link url='%1'>More...</link>"
                     "</para>", QStringLiteral("plan:main-work-space#the-view-selector")));
    
    m_viewlist = new ViewListTreeWidget(this);
    m_viewlist->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(m_viewlist, &ViewListTreeWidget::modified, this, &ViewListWidget::modified);

    m_currentSchedule = new KComboBox(this);
    m_model.setFlat(true);

    m_sfModel.setSourceModel(&m_model);
    m_currentSchedule->setModel(&m_sfModel);
    m_currentSchedule->setWhatsThis(
              xi18nc("@info:whatsthis",
                     "<title>Schedule selector</title>"
                     "<para>"
                     "Selects the schedule to be used when displaying schedule dependent data."
                     "<nl/><note>Unscheduled tasks are only shown in editors.</note>"
                     "<nl/><link url='%1'>More...</link>"
                     "</para>", QStringLiteral("plan:main-work-space#schedule-selector")));
    

    QVBoxLayout *l = new QVBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);
    l->addWidget(m_viewlist);
    l->addWidget(m_currentSchedule);

    connect(m_viewlist, &QTreeWidget::currentItemChanged, this, &ViewListWidget::slotActivated);

    connect(m_viewlist, &QTreeWidget::itemChanged, this, &ViewListWidget::slotItemChanged);

    setupContextMenus();

    connect(m_currentSchedule, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &ViewListWidget::slotCurrentScheduleChanged);

    connect(&m_model, &ScheduleItemModel::scheduleManagerAdded, this, &ViewListWidget::slotScheduleManagerAdded);

    connect(m_viewlist, &ViewListTreeWidget::updateViewInfo, this, &ViewListWidget::updateViewInfo);
}

ViewListWidget::~ViewListWidget()
{
}

void ViewListWidget::setReadWrite(bool rw)
{
    const QList<ViewListItem*> lst = categories();
    for (ViewListItem *c : lst) {
        for (int i = 0; i < c->childCount(); ++i) {
            static_cast<ViewListItem*>(c->child(i))->setReadWrite(rw);
        }
    }
}

void ViewListWidget::slotItemChanged(QTreeWidgetItem *item, int col)
{
    Q_UNUSED(item)
    Q_UNUSED(col)
    //debugPlan;
}

void ViewListWidget::slotActivated(QTreeWidgetItem *item, QTreeWidgetItem *prev)
{
    if (m_prev) {
        m_prev->setData(0, Qt::BackgroundRole, QVariant());
    }
    if (item && item->type() == ViewListItem::ItemType_Category) {
        return ;
    }
    Q_EMIT activated(static_cast<ViewListItem*>(item), static_cast<ViewListItem*>(prev));
    if (item) {
        QVariant v = QBrush(QColor(Qt::yellow));
        item->setData(0, Qt::BackgroundRole, v);
        m_prev = static_cast<ViewListItem*>(item);
    }
}

ViewListItem *ViewListWidget::addCategory(const QString &tag, const QString& name)
{
    //debugPlan ;
    ViewListItem *item = m_viewlist->findCategory(tag);
    if (item == nullptr) {
        item = new ViewListItem(m_viewlist, tag, QStringList(name), ViewListItem::ItemType_Category);
        item->setExpanded(true);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
    }
    return item;
}

QList<ViewListItem*> ViewListWidget::categories() const
{
    QList<ViewListItem*> lst;
    QTreeWidgetItem *item;
    int cnt = m_viewlist->topLevelItemCount();
    for (int i = 0; i < cnt; ++i) {
        item = m_viewlist->topLevelItem(i);
        if (item->type() == ViewListItem::ItemType_Category)
            lst << static_cast<ViewListItem*>(item);
    }
    return lst;
}

ViewListItem *ViewListWidget::findCategory(const QString &tag) const
{
    return m_viewlist->findCategory(tag);
}

ViewListItem *ViewListWidget::category(const KoView *view) const
{
    return m_viewlist->category(view);
}

QString ViewListWidget::uniqueTag(const QString &seed) const
{
    QString tag = seed;
    for (int i = 1; findItem(tag); ++i) {
        tag = QStringLiteral("%1-%2").arg(seed).arg(i);
    }
    return tag;
}

ViewListItem *ViewListWidget::addView(QTreeWidgetItem *category, const QString &tag, const QString &name, ViewBase *view, KoDocument *doc, const QString &iconName, int index)
{
    ViewListItem * item = new ViewListItem(uniqueTag(tag), QStringList(name), ViewListItem::ItemType_SubView);
    item->setView(view);
    item->setDocument(doc);
    if (! iconName.isEmpty()) {
        item->setData(0, Qt::DecorationRole, QIcon::fromTheme(iconName));
    }
    item->setFlags((item->flags() | Qt::ItemIsEditable) & ~Qt::ItemIsDropEnabled);
    insertViewListItem(item, category, index);

    connect(view, &ViewBase::optionsModified, this, &ViewListWidget::setModified);

    return item;
}

void ViewListWidget::setSelected(QTreeWidgetItem *item)
{
    //debugPlan<<item<<","<<m_viewlist->currentItem();
    if (item == nullptr && m_viewlist->currentItem()) {
        m_viewlist->currentItem()->setSelected(false);
        if (m_prev) {
            m_prev->setData(0, Qt::BackgroundRole, QVariant());
        }
    }
    m_viewlist->setCurrentItem(item);
    //debugPlan<<item<<","<<m_viewlist->currentItem();
}

void ViewListWidget::setCurrentItem(QTreeWidgetItem *item)
{
    m_viewlist->setCurrentItem(item);
    //debugPlan<<item<<","<<m_viewlist->currentItem();
}

ViewListItem *ViewListWidget::currentItem() const
{
    return static_cast<ViewListItem*>(m_viewlist->currentItem());
}

ViewListItem *ViewListWidget::currentCategory() const
{
    ViewListItem *item = static_cast<ViewListItem*>(m_viewlist->currentItem());
    if (item == nullptr) {
        return nullptr;
    }
    if (item->type() == ViewListItem::ItemType_Category) {
        return item;
    }
    return static_cast<ViewListItem*>(item->parent());
}

KoView *ViewListWidget::findView(const QString &tag) const
{
    ViewListItem *i = findItem(tag);
    if (i == nullptr) {
        return nullptr;
    }
    return i->view();
}

ViewListItem *ViewListWidget::findItem(const QString &tag) const
{
    ViewListItem *item = findItem(tag, m_viewlist->invisibleRootItem());
    if (item == nullptr) {
        QTreeWidgetItem *parent = m_viewlist->invisibleRootItem();
        for (int i = 0; i < parent->childCount(); ++i) {
            item = findItem(tag, parent->child(i));
            if (item != nullptr) {
                break;
            }
        }
    }
    return item;
}

ViewListItem *ViewListWidget::findItem(const QString &tag, QTreeWidgetItem *parent) const
{
    if (parent == nullptr) {
        return findItem(tag, m_viewlist->invisibleRootItem());
    }
    for (int i = 0; i < parent->childCount(); ++i) {
        ViewListItem * ch = static_cast<ViewListItem*>(parent->child(i));
        if (ch->tag() == tag) {
            //debugPlan<<ch<<","<<view;
            return ch;
        }
        ch = findItem(tag, ch);
        if (ch) {
            return ch;
        }
    }
    return nullptr;
}

ViewListItem *ViewListWidget::findItem(const ViewBase *view, QTreeWidgetItem *parent) const
{
    if (parent == nullptr) {
        return findItem(view, m_viewlist->invisibleRootItem());
    }
    for (int i = 0; i < parent->childCount(); ++i) {
        ViewListItem * ch = static_cast<ViewListItem*>(parent->child(i));
        if (ch->view() == view) {
            //debugPlan<<ch<<","<<view;
            return ch;
        }
        ch = findItem(view, ch);
        if (ch) {
            return ch;
        }
    }
    return nullptr;
}

int ViewListWidget::indexOf(const QString &catName, const QString &tag) const
{
    const auto cat = findCategory(catName);
    if (!cat) {
        return -1;
    }
    const auto view = findItem(tag, cat);
    if (!view) {
        return -1;
    }
    return cat->indexOfChild(view);
}

void ViewListWidget::slotAddView()
{
    Q_EMIT createView();
}

void ViewListWidget::slotRemoveCategory()
{
    if (m_contextitem == nullptr) {
        return;
    }
    if (m_contextitem->type() != ViewListItem::ItemType_Category) {
        return;
    }
    debugPlan<<m_contextitem<<":"<<m_contextitem->type();
    if (m_contextitem->childCount() > 0) {
        if (KMessageBox::warningContinueCancel(this, i18n("Removing this category will also remove all its views.")) == KMessageBox::Cancel) {
            return;
        }
    }
    // first remove all views in this category
    while (m_contextitem->childCount() > 0) {
        ViewListItem *itm = static_cast<ViewListItem*>(m_contextitem->child(0));
        takeViewListItem(itm);
        delete itm->view();
        delete itm;
    }
    takeViewListItem(m_contextitem);
    delete m_contextitem;
    m_contextitem = nullptr;
    Q_EMIT modified();
}

void ViewListWidget::slotRemoveView()
{
    if (m_contextitem) {
        takeViewListItem(m_contextitem);
        delete m_contextitem->view();
        delete m_contextitem;
        Q_EMIT modified();
    }
}

void ViewListWidget::slotEditViewTitle()
{
    //QTreeWidgetItem *item = m_viewlist->currentItem();
    if (m_contextitem) {
        debugPlan<<m_contextitem<<":"<<m_contextitem->type();
        QString title = m_contextitem->text(0);
        m_viewlist->editItem(m_contextitem);
        if (title != m_contextitem->text(0)) {
            Q_EMIT modified();
        }
    }
}

void ViewListWidget::slotConfigureItem()
{
    if (m_contextitem == nullptr) {
        return;
    }
    KoDialog *dlg = nullptr;
    if (m_contextitem->type() == ViewListItem::ItemType_Category) {
        debugPlan<<m_contextitem<<":"<<m_contextitem->type();
        dlg = new ViewListEditCategoryDialog(*this, m_contextitem, this);
    } else if (m_contextitem->type() == ViewListItem::ItemType_SubView) {
        dlg = new ViewListEditViewDialog(*this, m_contextitem, this);
    }
    if (dlg) {
        connect(dlg, &QDialog::finished, this, &ViewListWidget::slotDialogFinished);
        dlg->open();
    }
}

void ViewListWidget::slotDialogFinished(int result)
{
    if (result == QDialog::Accepted) {
        Q_EMIT modified();
    }
    if (sender()) {
        sender()->deleteLater();
    }
}

void ViewListWidget::slotEditDocumentTitle()
{
    //QTreeWidgetItem *item = m_viewlist->currentItem();
    if (m_contextitem) {
        debugPlan<<m_contextitem<<":"<<m_contextitem->type();
        m_viewlist->editItem(m_contextitem);
    }
}

int ViewListWidget::removeViewListItem(ViewListItem *item)
{
    QTreeWidgetItem *p = item->parent();
    if (p == nullptr) {
        p = m_viewlist->invisibleRootItem();
    }
    int i = p->indexOfChild(item);
    if (i != -1) {
        p->takeChild(i);
        Q_EMIT modified();
    }
    return i;
}

void ViewListWidget::addViewListItem(ViewListItem *item, QTreeWidgetItem *parent, int index)
{
    QTreeWidgetItem *p = parent;
    if (p == nullptr) {
        p = m_viewlist->invisibleRootItem();
    }
    if (index == -1) {
        index = p->childCount();
    }
    p->insertChild(index, item);
    Q_EMIT modified();
}

int ViewListWidget::takeViewListItem(ViewListItem *item)
{
    while (item->childCount() > 0) {
        takeViewListItem(static_cast<ViewListItem*>(item->child(0)));
    }
    int pos = removeViewListItem(item);
    if (pos != -1) {
        Q_EMIT viewListItemRemoved(item);
        if (item == m_prev) {
            m_prev = nullptr;
        }
        if (m_prev) {
            setCurrentItem(m_prev);
        }
    }
    return pos;
}

void ViewListWidget::insertViewListItem(ViewListItem *item, QTreeWidgetItem *parent, int index)
{
    addViewListItem(item, parent, index);
    Q_EMIT viewListItemInserted(item, static_cast<ViewListItem*>(parent), index);
}

void ViewListWidget::setupContextMenus()
{
    // NOTE: can't use xml file as there may not be a factory()
    QAction *action;
    // view actions
    action = new QAction(koIcon("edit-rename"), xi18nc("@action:inmenu rename view", "Rename"), this);
    connect(action, &QAction::triggered, this, &ViewListWidget::slotEditViewTitle);
    m_viewactions.append(action);

    action = new QAction(koIcon("configure"), xi18nc("@action:inmenu configure view", "Configure..."), this);
    connect(action, &QAction::triggered, this, &ViewListWidget::slotConfigureItem);
    m_viewactions.append(action);

    action = new QAction(koIcon("list-remove"), xi18nc("@action:inmenu remove view", "Remove"), this);
    connect(action, &QAction::triggered, this, &ViewListWidget::slotRemoveView);
    m_viewactions.append(action);

    action = new QAction(this);
    action->setSeparator(true);
    m_viewactions.append(action);

    // Category actions
    action = new QAction(koIcon("edit-rename"), xi18nc("@action:inmenu rename view category", "Rename"), this);
    connect(action, &QAction::triggered, this, &ViewListWidget::renameCategory);
    m_categoryactions.append(action);

    action = new QAction(koIcon("configure"), xi18nc("@action:inmenu configure view category", "Configure..."), this);
    connect(action, &QAction::triggered, this, &ViewListWidget::slotConfigureItem);
    m_categoryactions.append(action);

    action = new QAction(koIcon("list-remove"), xi18nc("@action:inmenu Remove view category", "Remove"),this);
    connect(action, &QAction::triggered, this, &ViewListWidget::slotRemoveCategory);
    m_categoryactions.append(action);

    action = new QAction(this);
    action->setSeparator(true);
    m_categoryactions.append(action);

    // list actions
    action = new QAction(koIcon("list-add"), xi18nc("@action:inmenu Insert View", "Insert..."), this);
    connect(action, &QAction::triggered, this, &ViewListWidget::slotAddView);
    m_listactions.append(action);
}

void ViewListWidget::renameCategory()
{
    if (m_contextitem) {
        m_viewlist->editItem(m_contextitem, 0);
    }
}

void ViewListWidget::contextMenuEvent (QContextMenuEvent *event)
{
    QMenu menu;
    QList<QAction*> lst;
    m_contextitem = static_cast<ViewListItem*>(m_viewlist->itemAt(event->pos()));
    if (m_contextitem == nullptr) {
        lst += m_listactions;
    } else {
        if (m_contextitem->type() == ViewListItem::ItemType_Category) {
            lst += m_categoryactions;
        } else if (m_contextitem->type() == ViewListItem::ItemType_SubView) {
            lst += m_viewactions;
            ViewBase *v = dynamic_cast<ViewBase*>(m_contextitem->view());
            if (v) {
                // TODO: review
                //lst += v->viewlistActionList();
            }
        }
        lst += m_listactions;
    }
    if (! lst.isEmpty()) {
        //menu.addTitle(i18n("Edit"));
        for (QAction *a : qAsConst(lst)) {
            menu.addAction(a);
        }
    }
    if (! menu.actions().isEmpty()) {
        menu.exec(event->globalPos());
    }
}

void ViewListWidget::save(QDomElement &element) const
{
    m_viewlist->save(element);
}

void ViewListWidget::setProject(Project *project)
{
    debugPlan<<project;
    m_model.setProject(project);
}

void ViewListWidget::slotCurrentScheduleChanged(int idx)
{
    debugPlan<<idx<<selectedSchedule();
    Q_EMIT selectionChanged(selectedSchedule());
}

ScheduleManager *ViewListWidget::selectedSchedule() const
{
    QModelIndex idx = m_sfModel.index(m_currentSchedule->currentIndex(), m_currentSchedule->modelColumn());
    debugPlan<<idx;
    return m_sfModel.manager(idx);
}

void ViewListWidget::setSelectedSchedule(ScheduleManager *sm)
{
    debugPlan<<sm<<m_model.index(sm);
    QModelIndex idx = m_sfModel.mapFromSource(m_model.index(sm));
    if (sm && ! idx.isValid()) {
        m_temp = sm;
        return;
    }
    m_currentSchedule->setCurrentIndex(idx.row());
    debugPlan<<sm<<idx;
    m_temp = nullptr;
}

void ViewListWidget::slotScheduleManagerAdded(ScheduleManager *sm)
{
    if (m_temp && m_temp == sm) {
        setSelectedSchedule(sm);
        m_temp = nullptr;
    }
}

void ViewListWidget::setModified()
{
    Q_EMIT modified();
}

}  //KPlato namespace
