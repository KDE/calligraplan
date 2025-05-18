/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2006-2010, 2012 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kptviewbase.h"
#include "kptitemmodelbase.h"
#include "kptproject.h"
#include "kptdebug.h"
#include "config.h"

#include <KMessageBox>
#include <KActionCollection>
#include <KActionMenu>
#include <KZip>
#include <KStandardAction>
#include <KXMLGUIFactory>

#include <KoIcon.h>
#include <KoDocument.h>
#include <KoPart.h>
#include <KoMainWindow.h>
#include <KoPageLayoutWidget.h>
#include <KoPagePreviewWidget.h>
#include <KoUnit.h>
#include <KoXmlReader.h>

#include <QAbstractItemModel>
#include <QAbstractProxyModel>
#include <QSortFilterProxyModel>
#include <QHeaderView>
#include <QPoint>
#include <QScrollBar>
#include <QAbstractScrollArea>
#include <QMetaEnum>
#include <QPainter>
#include <QAction>
#include <QMenu>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QDragMoveEvent>
#include <QDrag>
#include <QTimer>
#include <QClipboard>

namespace KPlato
{

// sort indexes the way they are displayed
void sort(const QTreeView *view, QModelIndexList &list)
{
    if (list.count() <= 1) {
        return;
    }
    QModelIndexList i; i << list.takeFirst();
    for (QModelIndex idx = view->indexAbove(i.first()); idx.isValid() && !list.isEmpty(); idx = view->indexAbove(idx)) {
        if (list.contains(idx)) {
            i.prepend(idx);
            list.removeOne(idx);
        }
    }
    for (QModelIndex idx = view->indexBelow(i.last()); idx.isValid() && !list.isEmpty(); idx = view->indexBelow(idx)) {
        if (list.contains(idx)) {
            i.append(idx);
            list.removeOne(idx);
        }
    }
    list = i;
}


DockWidget::DockWidget(ViewBase *v, const QString &identity,  const QString &title)
    : QDockWidget(v),
    view(v),
    id(identity),
    location(Qt::RightDockWidgetArea),
    editor(false),
    m_shown(true)
{
    setWindowTitle(title);
    setObjectName(v->objectName() + QLatin1Char('-') + identity);
    toggleViewAction()->setObjectName(objectName());
    connect(this, &QDockWidget::dockLocationChanged, this, &DockWidget::setLocation);
}

void DockWidget::activate(KoMainWindow *mainWindow)
{
    connect(this, &QDockWidget::visibilityChanged, this, &DockWidget::setShown);
    setVisible(m_shown);
    mainWindow->addDockWidget(location, this);

    const KActionCollection *c = mainWindow->actionCollection();
    KActionMenu *a = qobject_cast<KActionMenu*>(c->action(QStringLiteral("settings_dockers_menu")));
    if (a) {
        a->addAction(toggleViewAction());
    }
}

void DockWidget::deactivate(KoMainWindow *mainWindow)
{
    disconnect(this, &QDockWidget::visibilityChanged, this, &DockWidget::setShown);
    mainWindow->removeDockWidget(this);
     // activation re-parents to QMainWindow, so re-parent back to view
    setParent(const_cast<ViewBase*>(view));
    const KActionCollection *c = mainWindow->actionCollection();
    KActionMenu *a = qobject_cast<KActionMenu*>(c->action(QStringLiteral("settings_dockers_menu")));
    if (a) {
        a->removeAction(toggleViewAction());
    }
}

void DockWidget::setShown(bool show)
{
    m_shown = show;
    setVisible(show);
}

bool KPlato::DockWidget::shown() const
{
    return m_shown;
}

void DockWidget::setLocation(Qt::DockWidgetArea area)
{
    location = area;
}

bool DockWidget::saveXml(QDomElement &context) const
{
    QDomElement e = context.ownerDocument().createElement(QStringLiteral("docker"));
    context.appendChild(e);
    e.setAttribute(QStringLiteral("id"), id);
    e.setAttribute(QStringLiteral("location"), QString::number(location));
    e.setAttribute(QStringLiteral("floating"), QString::number(isFloating()));
    e.setAttribute(QStringLiteral("visible"), QString::number(m_shown));
    return true;
}

void DockWidget::loadXml(const KoXmlElement& context)
{
    location = static_cast<Qt::DockWidgetArea>(context.attribute("location", QString::number(0)).toInt());
    setFloating((bool) context.attribute("floating", QString::number(0)).toInt());
    m_shown = context.attribute(QStringLiteral("visible"), QString::number(1)).toInt();
}

//------------------------
bool PrintingOptions::loadXml(KoXmlElement &element)
{
    KoXmlElement e;
    forEachElement(e, element) {
        if (e.tagName() == QStringLiteral("header")) {
            headerOptions.group = e.attribute("group", QString::number(0)).toInt();
            headerOptions.project = static_cast<Qt::CheckState>(e.attribute("project", QString::number(0)).toInt());
            headerOptions.date = static_cast<Qt::CheckState>(e.attribute("date", QString::number(0)).toInt());
            headerOptions.manager = static_cast<Qt::CheckState>(e.attribute("manager", QString::number(0)).toInt());
            headerOptions.page = static_cast<Qt::CheckState>(e.attribute("page", QString::number(0)).toInt());
        } else if (e.tagName() == QStringLiteral("footer")) {
            footerOptions.group = e.attribute("group", QString::number(0)).toInt();
            footerOptions.project = static_cast<Qt::CheckState>(e.attribute("project", QString::number(0)).toInt());
            footerOptions.date = static_cast<Qt::CheckState>(e.attribute("date", QString::number(0)).toInt());
            footerOptions.manager = static_cast<Qt::CheckState>(e.attribute("manager", QString::number(0)).toInt());
            footerOptions.page = static_cast<Qt::CheckState>(e.attribute("page", QString::number(0)).toInt());
        }
    }
    return true;
}

void PrintingOptions::saveXml(QDomElement &element) const
{
    QDomElement me = element.ownerDocument().createElement(QStringLiteral("printing-options"));
    element.appendChild(me);

    QDomElement h = me.ownerDocument().createElement(QStringLiteral("header"));
    me.appendChild(h);
    h.setAttribute(QStringLiteral("group"), QString::number(headerOptions.group));
    h.setAttribute(QStringLiteral("project"), QString::number(headerOptions.project));
    h.setAttribute(QStringLiteral("date"), QString::number(headerOptions.date));
    h.setAttribute(QStringLiteral("manager"), QString::number(headerOptions.manager));
    h.setAttribute(QStringLiteral("page"), QString::number(headerOptions.page));

    QDomElement f = me.ownerDocument().createElement(QStringLiteral("footer"));
    me.appendChild(f);
    f.setAttribute(QStringLiteral("group"), QString::number(footerOptions.group));
    f.setAttribute(QStringLiteral("project"), QString::number(footerOptions.project));
    f.setAttribute(QStringLiteral("date"), QString::number(footerOptions.date));
    f.setAttribute(QStringLiteral("manager"), QString::number(footerOptions.manager));
    f.setAttribute(QStringLiteral("page"), QString::number(footerOptions.page));
}

//----------------------
PrintingHeaderFooter::PrintingHeaderFooter(const PrintingOptions &opt, QWidget *parent)
    : QWidget(parent)
{
    setupUi(this);
    setWindowTitle(i18n("Header and Footer"));
    setOptions(opt);

    connect(ui_header, &QGroupBox::toggled, this, &PrintingHeaderFooter::slotChanged);
    connect(ui_headerProject, &QCheckBox::stateChanged, this, &PrintingHeaderFooter::slotChanged);
    connect(ui_headerPage, &QCheckBox::stateChanged, this, &PrintingHeaderFooter::slotChanged);
    connect(ui_headerManager, &QCheckBox::stateChanged, this, &PrintingHeaderFooter::slotChanged);
    connect(ui_headerDate, &QCheckBox::stateChanged, this, &PrintingHeaderFooter::slotChanged);

    connect(ui_footer, &QGroupBox::toggled, this, &PrintingHeaderFooter::slotChanged);
    connect(ui_footerProject, &QCheckBox::stateChanged, this, &PrintingHeaderFooter::slotChanged);
    connect(ui_footerPage, &QCheckBox::stateChanged, this, &PrintingHeaderFooter::slotChanged);
    connect(ui_footerManager, &QCheckBox::stateChanged, this, &PrintingHeaderFooter::slotChanged);
    connect(ui_footerDate, &QCheckBox::stateChanged, this, &PrintingHeaderFooter::slotChanged);
}

PrintingHeaderFooter::~PrintingHeaderFooter()
{
    //debugPlan;
}

void PrintingHeaderFooter::slotChanged()
{
    debugPlan;
    Q_EMIT changed(options());
}

void PrintingHeaderFooter::setOptions(const PrintingOptions &options)
{
    m_options = options;
    ui_header->setChecked(m_options.headerOptions.group);
    ui_headerProject->setCheckState(m_options.headerOptions.project);
    ui_headerDate->setCheckState(m_options.headerOptions.date);
    ui_headerManager->setCheckState(m_options.headerOptions.manager);
    ui_headerPage->setCheckState(m_options.headerOptions.page);

    ui_footer->setChecked(m_options.footerOptions.group);
    ui_footerProject->setCheckState(m_options.footerOptions.project);
    ui_footerDate->setCheckState(m_options.footerOptions.date);
    ui_footerManager->setCheckState(m_options.footerOptions.manager);
    ui_footerPage->setCheckState(m_options.footerOptions.page);

}

PrintingOptions PrintingHeaderFooter::options() const
{
    //debugPlan;
    PrintingOptions opt;
    opt.headerOptions.group = ui_header->isChecked();
    opt.headerOptions.project = ui_headerProject->checkState();
    opt.headerOptions.date = ui_headerDate->checkState();
    opt.headerOptions.manager = ui_headerManager->checkState();
    opt.headerOptions.page = ui_headerPage->checkState();

    opt.footerOptions.group = ui_footer->isChecked();
    opt.footerOptions.project = ui_footerProject->checkState();
    opt.footerOptions.date = ui_footerDate->checkState();
    opt.footerOptions.manager = ui_footerManager->checkState();
    opt.footerOptions.page = ui_footerPage->checkState();
    return opt;
}

PrintingDialog::PrintingDialog(ViewBase *view, QPrinter::PrinterMode mode)
    : KoPrintingDialog(view, mode),
    m_view(view),
    m_widget(nullptr)
{
    setPrinterPageLayout(view->pageLayout());
    QImage px(100, 600, QImage::Format_Mono);
    int dpm = printer().resolution() * 40;
    px.setDotsPerMeterX(dpm);
    px.setDotsPerMeterY(dpm);
    QPainter p(&px);
    m_textheight = p.boundingRect(QRectF(), Qt::AlignTop, QStringLiteral("Aj")).height();
    debugPlan<<"textheight:"<<m_textheight;
}

PrintingDialog::~PrintingDialog()
{
}

QAbstractPrintDialog::PrintDialogOptions PrintingDialog::printDialogOptions() const
{
    return QAbstractPrintDialog::PrintToFile |
           QAbstractPrintDialog::PrintPageRange |
           QAbstractPrintDialog::PrintCollateCopies;
}

PrintingOptions PrintingDialog::printingOptions() const
{
    return m_view->printingOptions();
}

void PrintingDialog::setPrintingOptions(const PrintingOptions &opt)
{
    debugPlan;
    m_view->setPrintingOptions(opt);
    Q_EMIT changed(opt);
}

void PrintingDialog::setPrinterPageLayout(const KoPageLayout &pagelayout)
{
    QPrinter &p = printer();
    QPageLayout::Orientation o = QPageLayout::Portrait;
    switch (pagelayout.orientation) {
        case KoPageFormat::Portrait: o = QPageLayout::Portrait; break;
        case KoPageFormat::Landscape: o = QPageLayout::Landscape; break;
        default: break;
    }
    p.setPageOrientation(o);
    p.setPageSize(KoPageFormat::qPageSize(pagelayout.format));
    p.setPageMargins(QMarginsF(pagelayout.leftMargin, pagelayout.topMargin, pagelayout.rightMargin, pagelayout.bottomMargin), QPageLayout::Point);
}

void PrintingDialog::startPrinting(RemovePolicy removePolicy)
{
    setPrinterPageLayout(m_view->pageLayout()); // FIXME: Something resets printer().paperSize() to A4 !
    KoPrintingDialog::startPrinting(removePolicy);
}

QWidget *PrintingDialog::createPageLayoutWidget() const
{
    QWidget *w = ViewBase::createPageLayoutWidget(m_view);
    KoPageLayoutWidget *pw = w->findChild<KoPageLayoutWidget*>();
    connect(pw, SIGNAL(layoutChanged(KoPageLayout)), m_view, SLOT(setPageLayout(KoPageLayout)));
    connect(pw, &KoPageLayoutWidget::layoutChanged, this, &PrintingDialog::setPrinterPageLayout);
    connect(pw, SIGNAL(layoutChanged(KoPageLayout)), this, SIGNAL(changed()));
    return w;
}

QList<QWidget*> PrintingDialog::createOptionWidgets() const
{
    //debugPlan;
    PrintingHeaderFooter *w = new PrintingHeaderFooter(printingOptions());
    connect(w, &PrintingHeaderFooter::changed, this, &PrintingDialog::setPrintingOptions);
    const_cast<PrintingDialog*>(this)->m_widget = w;

    return QList<QWidget*>() << w;
}

/*QList<KoShape*> PrintingDialog::shapesOnPage(int)
{
    return QList<KoShape*>();
}*/

void PrintingDialog::drawRect(QPainter &p, const QRect &r, Qt::Edges edges)
{
    p.save();
    QPen pen = p.pen();
    pen.setColor(Qt::gray);
    p.setPen(pen);
    if (edges & Qt::LeftEdge) {
        p.drawLine(r.topLeft(), r.bottomLeft());
    }
    if (edges & Qt::BottomEdge) {
        p.drawLine(r.bottomLeft(), r.bottomRight());
    }
    if (edges & Qt::TopEdge) {
        p.drawLine(r.topRight(), r.bottomRight());
    }
    if (edges & Qt::RightEdge) {
        p.drawLine(r.topRight(), r.bottomRight());
    }
    p.restore();
}

QRect PrintingDialog::headerRect() const
{
    PrintingOptions options =  m_view->printingOptions();
    if (options.headerOptions.group == false) {
        return QRect();
    }
    int height = headerFooterHeight(options.headerOptions);
    const auto resolution = const_cast<PrintingDialog*>(this)->printer().resolution();
    const auto width = const_cast<PrintingDialog*>(this)->printer().pageLayout().paintRectPixels(resolution).width();
    return QRect(0, 0, width, height);
}

QRect PrintingDialog::footerRect() const
{
    PrintingOptions options =  m_view->printingOptions();
    if (options.footerOptions.group == false) {
        return QRect();
    }
    int height = headerFooterHeight(options.footerOptions);
    const auto resolution = const_cast<PrintingDialog*>(this)->printer().resolution();
    QRect r = const_cast<PrintingDialog*>(this)->printer().pageLayout().paintRectPixels(resolution);
    return QRect(0, r.height() - height, r.width(), height);
}

int PrintingDialog::headerFooterHeight(const PrintingOptions::Data &options) const
{
    int height = 0.0;
    if (options.page == Qt::Checked || options.project == Qt::Checked || options.manager == Qt::Checked || options.date == Qt::Checked) {
        height += m_textheight * 1.5;
    }
    if (options.project == Qt::Checked && options.manager == Qt::Checked && (options.date == Qt::Checked || options.page == Qt::Checked)) {
       height *= 2.0;
    }
    debugPlan<<height;
    return height;
}

void PrintingDialog::paintHeaderFooter(QPainter &p, const PrintingOptions &options, int pageNumber, const Project &project)
{
    if (options.headerOptions.group == true) {
        paint(p, options.headerOptions, headerRect(), pageNumber, project);
    }
    if (options.footerOptions.group == true) {
        paint(p, options.footerOptions, footerRect(), pageNumber, project);
    }
}

void PrintingDialog::paint(QPainter &p, const PrintingOptions::Data &options, const QRect &rect,  int pageNumber, const Project &project)
{
    p.save();

    p.setPen(Qt::black);
    p.drawRect(rect);
    QRect projectRect;
    QString projectName = project.name();
    QRect pageRect;
    QString page = i18nc("1=page number, 2=last page number", "%1 (%2)", pageNumber, documentLastPage());
    QRect managerRect;
    QString manager = project.leader();
    QRect dateRect;
    QString date = QLocale().toString(QDate::currentDate(), QLocale::ShortFormat);

    QRect rect_1 = rect;
    QRect rect_2 = rect;
    if (options.project == Qt::Checked) {
        rect_2.setHeight(rect.height() / 2);
        rect_2.translate(0, rect_2.height());
    }
    if ((options.project == Qt::Checked && options.manager == Qt::Checked) && (options.date == Qt::Checked || options.page == Qt::Checked)) {
        rect_1.setHeight(rect.height() / 2);
        p.drawLine(rect_1.bottomLeft(), rect_1.bottomRight());
    }

    if (options.page == Qt::Checked) {
        pageRect = p.boundingRect(rect_1, Qt::AlignRight|Qt::AlignTop, page);
        pageRect.setHeight(rect_1.height());
        rect_1.setRight(pageRect.left() - 2);
        p.save();
        QFont f = p.font();
        f.setPointSize(f.pointSize() * 0.5);
        p.setFont(f);
        QRect r = p.boundingRect(rect_1, Qt::AlignRight|Qt::AlignTop, i18n("Page:"));
        if (r.left() > pageRect.left()) {
            pageRect.setLeft(r.left());
        }
        p.restore();
        if (options.project == Qt::Checked || options.manager == Qt::Checked || options.date == Qt::Checked) {
            p.drawLine(rect_1.topRight(), rect_1.bottomRight());
        }
    }
    if (options.date == Qt::Checked) {
            p.save();
            QFont f = p.font();
            f.setPointSize(f.pointSize() * 0.5);
            p.setFont(f);
            QRect rct = p.boundingRect(rect_1, Qt::AlignRight|Qt::AlignTop, i18n("Date:"));
            p.restore();
        if ((options.project == Qt::Checked && options.manager != Qt::Checked) ||
             (options.project != Qt::Checked && options.manager == Qt::Checked))
        {
            dateRect = p.boundingRect(rect_1, Qt::AlignRight|Qt::AlignTop, date);
            dateRect.setHeight(rect_1.height());
            rect_1.setRight(dateRect.left() - 2);
            if (rct.right() > dateRect.right()) {
                dateRect.setRight(rct.right());
            }
            p.drawLine(rect_1.topRight(), rect_1.bottomRight());
        } else if (options.project == Qt::Checked && options.manager == Qt::Checked) {
            dateRect = p.boundingRect(rect_2, Qt::AlignRight|Qt::AlignTop, date);
            dateRect.setHeight(rect_2.height());
            rect_2.setRight(dateRect.left() - 2);
            if (rct.right() > dateRect.right()) {
                dateRect.setRight(rct.right());
            }
            p.drawLine(rect_2.topRight(), rect_2.bottomRight());
        } else {
            dateRect = p.boundingRect(rect_2, Qt::AlignLeft|Qt::AlignTop, date);
            if (rct.right() > dateRect.right()) {
                dateRect.setRight(rct.right());
            }
            dateRect.setHeight(rect_2.height());
            rect_2.setRight(dateRect.left() - 2);
            if (rect_2.left() != rect.left()) {
                p.drawLine(rect_2.topRight(), rect_2.bottomRight());
            }
        }
    }
    if (options.manager == Qt::Checked) {
        p.save();
        QFont f = p.font();
        f.setPointSize(f.pointSize() * 0.5);
        p.setFont(f);
        QRect rct = p.boundingRect(rect_1, Qt::AlignRight|Qt::AlignTop, i18n("Manager:"));
        p.restore();
        if (options.project != Qt::Checked) {
            managerRect = p.boundingRect(rect_1, Qt::AlignLeft|Qt::AlignTop, manager);
            managerRect.setHeight(rect_1.height());
            if (rct.right() > managerRect.right()) {
                managerRect.setRight(rct.right());
            }
        } else if (options.date != Qt::Checked && options.page != Qt::Checked) {
            managerRect = p.boundingRect(rect_1, Qt::AlignRight|Qt::AlignTop, manager);
            managerRect.setHeight(rect_1.height());
            if (rct.right() > managerRect.right()) {
                managerRect.setRight(rct.right());
            }
            rect_1.setRight(managerRect.left() - 2);
            p.drawLine(rect_1.topRight(), rect_1.bottomRight());
        } else {
            managerRect = p.boundingRect(rect_2, Qt::AlignLeft|Qt::AlignTop, manager);
            managerRect.setHeight(rect_2.height());
            if (rct.right() > managerRect.right()) {
                managerRect.setRight(rct.right());
            }
        }
    }
    if (options.project == Qt::Checked) {
        projectRect = p.boundingRect(rect_1, Qt::AlignLeft|Qt::AlignTop, projectName);
        projectRect.setHeight(rect_1.height());
        p.save();
        QFont f = p.font();
        f.setPointSize(f.pointSize() * 0.5);
        p.setFont(f);
        QRect rct = p.boundingRect(rect_1, Qt::AlignRight|Qt::AlignTop, i18n("Project:"));
        if (rct.right() > projectRect.right()) {
            projectRect.setRight(rct.right());
        }
        p.restore();
    }

    if (options.page == Qt::Checked) {
        p.drawText(pageRect, Qt::AlignHCenter|Qt::AlignBottom, page);
    }
    if (options.project == Qt::Checked) {
        p.drawText(projectRect.adjusted(3, 0, 3, 0), Qt::AlignLeft|Qt::AlignBottom, projectName);
    }
    if (options.date == Qt::Checked) {
        p.drawText(dateRect, Qt::AlignHCenter|Qt::AlignBottom, date);
    }
    if (options.manager == Qt::Checked) {
        p.drawText(managerRect.adjusted(3, 0, 3, 0), Qt::AlignLeft|Qt::AlignBottom, manager);
    }
    QFont f = p.font();
    f.setPointSize(f.pointSize() * 0.5);
    p.setFont(f);
    if (options.page == Qt::Checked) {
        p.drawText(pageRect, Qt::AlignTop|Qt::AlignLeft, i18n("Page:"));
    }
    if (options.project == Qt::Checked) {
        p.drawText(projectRect, Qt::AlignTop|Qt::AlignLeft, i18n("Project:"));
    }
    if (options.date == Qt::Checked) {
        p.drawText(dateRect, Qt::AlignTop|Qt::AlignLeft, i18n("Date:"));
    }
    if (options.manager == Qt::Checked) {
        p.drawText(managerRect, Qt::AlignTop|Qt::AlignLeft, i18n("Manager:"));
    }
    p.restore();
}

//--------------
ViewBase::ViewBase(KoPart *part, KoDocument *doc, QWidget *parent)
    : KoView(part, doc, parent),
    m_readWrite(false),
    m_proj(nullptr),
    m_schedulemanager(nullptr),
    m_singleTreeView(nullptr),
    m_doubleTreeView(nullptr)
{
    if (doc) {
        m_readWrite = doc->isReadWrite();
    }
}

ViewBase::~ViewBase()
{
    if (factory()) {
        factory()->removeClient(this);
    }
    if (koDocument()) {
        //HACK to avoid ~View to access koDocument()
        setDocumentDeleted();
    }
}

void ViewBase::openPopupMenu(const QString &name, const QPoint &pos)
{
    QMenu *menu = popupMenu(name);
    if (menu) {
        menu->exec(pos);
    }
}

QMenu *ViewBase::popupMenu(const QString& name)
{
    //debugPlan;
    if (factory()) {
        auto menu = static_cast<QMenu*>(factory()->container(name, this));
        return menu;
    } else {
        warnPlan<<Q_FUNC_INFO<<this<<"No factory!";
    }
    return nullptr;
}

void ViewBase::setProject(KPlato::Project *project)
{
    m_proj = project;
    Q_EMIT projectChanged(project);
}

void ViewBase::setScheduleManager(KPlato::ScheduleManager *sm)
{
    m_schedulemanager = sm;
}

KoDocument *ViewBase::part() const
{
     return koDocument();
}

KoPageLayout ViewBase::pageLayout() const
{
    return m_pagelayout;
}

void ViewBase::setPageLayout(const KoPageLayout &layout)
{
    m_pagelayout = layout;
}

bool ViewBase::isActive() const
{
    if (hasFocus()) {
        return true;
    }
    const QList<QWidget*> widgets = findChildren<QWidget*>();
    for (QWidget *v : widgets) {
        if (v->hasFocus()) {
            return true;
        }
    }
    return false;
}

void ViewBase::updateReadWrite(bool readwrite)
{
    m_readWrite = readwrite;
    Q_EMIT readWriteChanged(readwrite);
}

void ViewBase::setGuiActive(bool active) // virtual slot
{
    //debugPlan<<active;
    Q_EMIT guiActivated(this, active);
}

void ViewBase::slotUpdateReadWrite(bool rw)
{
    updateReadWrite(rw);
}

KoPrintJob *ViewBase::createPrintJob()
{
    KMessageBox::error(this, i18n("This view does not support printing."));

    return nullptr;
}

/*static*/
QWidget *ViewBase::createPageLayoutWidget(ViewBase *view)
{
    QWidget *widget = new QWidget();
    widget->setWindowTitle(xi18nc("@title:tab", "Page Layout"));

    QHBoxLayout *lay = new QHBoxLayout(widget);

    KoPageLayoutWidget *w = new KoPageLayoutWidget(widget, view->pageLayout());
    w->showPageSpread(false);
    lay->addWidget(w, 1);

    KoPagePreviewWidget *prev = new KoPagePreviewWidget(widget);
    prev->setPageLayout(view->pageLayout());
    lay->addWidget(prev, 1);

    connect (w, &KoPageLayoutWidget::layoutChanged, prev, &KoPagePreviewWidget::setPageLayout);

    return widget;
}

/*static*/
PrintingHeaderFooter *ViewBase::createHeaderFooterWidget(ViewBase *view)
{
    PrintingHeaderFooter *widget = new PrintingHeaderFooter(view->printingOptions());
    widget->setWindowTitle(xi18nc("@title:tab", "Header and Footer"));
    widget->setOptions(view->printingOptions());

    return widget;
}

void ViewBase::slotHeaderContextMenuRequested(const QPoint &pos)
{
    debugPlan;
    QList<QAction*> lst = contextActionList();
    if (! lst.isEmpty()) {
        QMenu::exec(lst, pos, lst.first());
    }
}

void ViewBase::createOptionActions(int actions, const QString &prefix)
{
    Q_UNUSED(prefix)
    QAction *action;
    // These goes at the top
    action = new QAction(this);
    action->setSeparator(true);
    int pos = 0;
    addContextAction(action, pos++);

    if (actions & OptionExpand) {
        action = new QAction(koIcon("arrow-down"), i18n("Expand All"), this);
        actionCollection()->addAction(QStringLiteral("expand_all"), action);
        connect(action, &QAction::triggered, this, &ViewBase::expandAll);
        addContextAction(action, pos++);
    }
    if (actions & OptionCollapse) {
        action = new QAction(koIcon("arrow-up"), i18n("Collapse All"), this);
        actionCollection()->addAction(QStringLiteral("collapse_all"), action);
        connect(action, &QAction::triggered, this, &ViewBase::collapseAll);
        addContextAction(action, pos++);
    }
    action = new QAction(this);
    action->setSeparator(true);
    addContextAction(action, pos++);

    // rest are appended
    action = new QAction(this);
    action->setSeparator(true);
    addContextAction(action);

    if (actions & OptionPrint) {
        action = KStandardAction::create(KStandardAction::Print, mainWindow(), SLOT(slotFilePrint()), this);
        actionCollection()->addAction(QStringLiteral("print"), action);
        addContextAction(action);
    }
    if (actions & OptionPrintPreview) {
        action = KStandardAction::create(KStandardAction::PrintPreview, mainWindow(), SLOT(slotFilePrintPreview()), this);
        actionCollection()->addAction(QStringLiteral("print_preview"), action);
        addContextAction(action);
    }
    if (actions & OptionPrintPdf) {
        action = new QAction(koIcon("application-pdf"), i18n("Print to PDF..."), this);
        actionCollection()->addAction(QStringLiteral("print_pdf"), action);
        connect(action, SIGNAL(triggered()), mainWindow(), SLOT(exportToPdf()));
        addContextAction(action);
    }
    if (actions & OptionPrintConfig) {
        action = new QAction(koIcon("configure"), i18n("Print Options..."), this);
        actionCollection()->addAction(QStringLiteral("print_options"), action);
        connect(action, &QAction::triggered, this, &ViewBase::slotOptions);
        addContextAction(action);
    }

    action = new QAction(this);
    action->setSeparator(true);
    addContextAction(action);

    if (actions & OptionViewConfig) {
        action = new QAction(koIcon("configure"), i18n("Configure View..."), this);
        actionCollection()->addAction(QStringLiteral("configure_view"), action);
        connect(action, &QAction::triggered, this, &ViewBase::slotOptions);
        addContextAction(action);
    }
}

void ViewBase::slotOptionsFinished(int result)
{
    if (result == QDialog::Accepted) {
        Q_EMIT optionsModified();
    }
    if (sender()) {
        sender()->deleteLater();
    }
}

void ViewBase::openContextMenu(const QString& menuname, const QPoint & pos)
{
    QMenu *menu = this->popupMenu(menuname);
    if (menu) {
        QList<QAction*> lst;
        lst = contextActionList();
        debugPlan<<lst;
        if (!lst.isEmpty()) {
            menu->addSeparator();
            for (QAction *a : std::as_const(lst)) {
                menu->addAction(a);
            }
        }
        menu->exec(pos);
        for (QAction *a : std::as_const(lst)) {
            menu->removeAction(a);
        }
    } else {
        warnPlan<<Q_FUNC_INFO<<"Could not find menu:"<<menuname<<xmlFile();
    }
}

bool ViewBase::loadContext(const KoXmlElement &context)
{
    KoXmlElement me;
    forEachElement(me, context) {
        if (me.tagName() == QStringLiteral("page-layout")) {
            m_pagelayout.format = KoPageFormat::formatFromString(me.attribute("format"));
            m_pagelayout.orientation = me.attribute("orientation") == QStringLiteral("landscape") ? KoPageFormat::Landscape : KoPageFormat::Portrait;
            m_pagelayout.width = me.attribute("width", QString::number(0)).toDouble();
            m_pagelayout.height = me.attribute("height", QString::number(0.0)).toDouble();
            m_pagelayout.leftMargin = me.attribute("left-margin", QString::number(MM_TO_POINT(20.0))).toDouble();
            m_pagelayout.rightMargin = me.attribute("right-margin", QString::number(MM_TO_POINT(20.0))).toDouble();
            m_pagelayout.topMargin = me.attribute("top-margin", QString::number(MM_TO_POINT(20.0))).toDouble();
            m_pagelayout.bottomMargin = me.attribute("bottom-margin", QString::number(MM_TO_POINT(20.0))).toDouble();
        } else if (me.tagName() == QStringLiteral("printing-options")) {
            m_printingOptions.loadXml(me);
        } else if (me.tagName() == QStringLiteral("dockers")) {
            KoXmlElement e;
            forEachElement (e, me) {
                DockWidget *ds = findDocker(e.attribute("id"));
                if (ds) {
                    ds->loadXml(e);
                }
            }
        }
    }
    return true;
}

void ViewBase::saveContext(QDomElement &context) const
{
    QDomElement me = context.ownerDocument().createElement(QStringLiteral("page-layout"));
    context.appendChild(me);
    me.setAttribute(QStringLiteral("format"), KoPageFormat::formatString(m_pagelayout.format));
    me.setAttribute(QStringLiteral("orientation"), m_pagelayout.orientation == KoPageFormat::Portrait ? QStringLiteral("portrait") : QStringLiteral("landscape"));
    me.setAttribute(QStringLiteral("width"), QString::number(m_pagelayout.width));
    me.setAttribute(QStringLiteral("height"),QString::number(m_pagelayout. height));
    me.setAttribute(QStringLiteral("left-margin"), QString::number(m_pagelayout.leftMargin));
    me.setAttribute(QStringLiteral("right-margin"), QString::number(m_pagelayout.rightMargin));
    me.setAttribute(QStringLiteral("top-margin"), QString::number(m_pagelayout.topMargin));
    me.setAttribute(QStringLiteral("bottom-margin"),QString::number(m_pagelayout.bottomMargin));

    m_printingOptions.saveXml(context);

    if (! m_dockers.isEmpty()) {
        QDomElement e = context.ownerDocument().createElement(QStringLiteral("dockers"));
        context.appendChild(e);
        for (const DockWidget *ds : std::as_const(m_dockers)) {
            ds->saveXml(e);
        }
    }
}

void ViewBase::addDocker(DockWidget *ds)
{
    //addAction(QStringLiteral("view_docker_list"), ds->toggleViewAction());
    m_dockers << ds;
}

QList<DockWidget*> ViewBase::dockers() const
{
    return m_dockers;
}

DockWidget* ViewBase::findDocker(const QString &id) const
{
    for (DockWidget *ds : std::as_const(m_dockers)) {
        if (ds->id == id) {
            return ds;
        }
    }
    return nullptr;
}

void ViewBase::setViewSplitMode(bool split)
{
    if (m_doubleTreeView) {
        m_doubleTreeView->setViewSplitMode(split);
    }
}

void ViewBase::showColumns(const QList<int> &left, const QList<int> &right)
{
    TreeViewBase *view1 = m_singleTreeView;
    TreeViewBase *view2 = nullptr;
    if (m_doubleTreeView) {
        view1 = m_doubleTreeView->masterView();
        view2 = m_doubleTreeView->slaveView();
        m_doubleTreeView->setViewSplitMode(!right.isEmpty());
    }
    if (view1) {
        const QAbstractItemModel *model = view1->model();
        if (model) {
            int count = model->columnCount();
            for (int i = 0; i < count; ++i) {
                view1->setColumnHidden(i, !left.contains(i));
                if (view2) {
                    view2->setColumnHidden(i, !right.contains(i));
                }
            }
            // sort columns
            for (int i = 0; i < left.count(); ++i) {
                view1->mapToSection(left.at(i), i);
            }
            if (view2) {
                for (int i = 0; i < right.count(); ++i) {
                    view2->mapToSection(right.at(i), i);
                }
            }
        }
    }
}

//----------------------
TreeViewPrintingDialog::TreeViewPrintingDialog(ViewBase *view, TreeViewBase *treeview, Project *project)
    : PrintingDialog(view),
    m_tree(treeview),
    m_project(project),
    m_firstRow(-1)
{
    printer().setFromTo(documentFirstPage(), documentLastPage());
}

int TreeViewPrintingDialog::documentLastPage() const
{
    int page = documentFirstPage();
    while (firstRow(page) != -1) {
        ++page;
    }
    if (page > documentFirstPage()) {
        --page;
    }
    return page;
}

int TreeViewPrintingDialog::firstRow(int page) const
{
    debugPlan<<page;
    int pageNumber = page - documentFirstPage();
    QHeaderView *mh = m_tree->header();
    int height = mh->height();
    int hHeight = headerRect().height();
    int fHeight = footerRect().height();
    const auto resolution = const_cast<TreeViewPrintingDialog*>(this)->printer().resolution();
    auto pageRect = const_cast<TreeViewPrintingDialog*>(this)->printer().pageLayout().paintRectPixels(resolution);

    int gap = 8;
    int pageHeight = pageRect.height() - height;
    if (hHeight > 0) {
        pageHeight -= (hHeight + gap);
    }
    if (fHeight > 0) {
        pageHeight -= (fHeight + gap);
    }
    int rowsPrPage = pageHeight / height;

    int rows = m_tree->model()->rowCount();
    int row = -1;
    for (int i = 0; i < rows; ++i) {
        if (! m_tree->isRowHidden(i, QModelIndex())) {
            row = i;
            break;
        }
    }
    if (row != -1) {
        QModelIndex idx = m_tree->model()->index(row, 0, QModelIndex());
        row = 0;
        while (idx.isValid()) {
            if (row >= rowsPrPage * pageNumber) {
                debugPlan<<page<<pageNumber;
                break;
            }
            ++row;
            idx = m_tree->indexBelow(idx);
        }
        if (! idx.isValid()) {
            row = -1;
        }
    }
    debugPlan<<"Page"<<page<<":"<<(row==-1?"empty":"first row=")<<row<<"("<<rowsPrPage<<")";
    return row;
}

QList<QWidget*> TreeViewPrintingDialog::createOptionWidgets() const
{
    QList<QWidget*> lst;
    lst << createPageLayoutWidget();
    lst += PrintingDialog::createOptionWidgets();
    return  lst;
}

void TreeViewPrintingDialog::printPage(int page, QPainter &painter)
{
    m_firstRow = firstRow(page);

    QHeaderView *mh = m_tree->header();
    int length = mh->length();
    int height = mh->height();
    QRect hRect = headerRect();
    QRect fRect = footerRect();
    const auto resolution = printer().resolution();
    QRect pageRect = printer().pageLayout().paintRectPixels(resolution);
    pageRect.moveTo(0, 0);

    QAbstractItemModel *model = m_tree->model();

    debugPlan<<pageRect;

    painter.translate(pageRect.topLeft());

    painter.setClipping(true);

    if (m_project) {
        paintHeaderFooter(painter, m_view->printingOptions(), page, *(m_project));
    }

    int gap = 8;
    int pageHeight = pageRect.height() - height;
    if (hRect.isValid()) {
        pageHeight -= (hRect.height() + gap);
    }
    if (fRect.isValid()) {
        pageHeight -= (fRect.height() + gap);
    }
    int rowsPrPage = pageHeight / height;

    double sx = pageRect.width() > length ? 1.0 : (double)pageRect.width() / (double)length;
    double sy = 1.0;
    painter.scale(sx, sy);

    int h = 0;

    painter.translate(0, hRect.height() + gap);
    h = hRect.height() + gap;

    painter.setPen(Qt::black);
    painter.setBrush(Qt::lightGray);
    int higestIndex = 0;
    int rightpos = 0;
    for (int i = 0; i < mh->count(); ++i) {
        QString text = model->headerData(i, Qt::Horizontal).toString();
        QVariant a = model->headerData(i, Qt::Horizontal, Qt::TextAlignmentRole);
        int align = a.isValid() ? a.toInt() : (int)(Qt::AlignLeft|Qt::AlignVCenter);
        if (! mh->isSectionHidden(i)) {
            QRect r = QRect(mh->sectionPosition(i), 0, mh->sectionSize(i), height).adjusted(0, 0, 0, -painter.pen().width());
            if (rightpos < r.right()) {
                higestIndex = i;
                rightpos = r.right();
            }
            painter.drawRect(r);
            // FIXME There is a bug somewhere, the text somehow overwrites the rect outline for the first column!
            painter.save();
            painter.setBrush(QBrush());
            painter.drawText(r.adjusted(3, 1, -3, -1), align, text);
            painter.drawRect(r);
            painter.restore();
        }
        //debugPlan<<text<<"hidden="<<h->isSectionHidden(i)<<h->sectionPosition(i);
    }
    if (m_firstRow == -1) {
        debugPlan<<"No data";
        return;
    }
    painter.setBrush(QBrush());
    QModelIndex idx = model->index(m_firstRow, 0, QModelIndex());
    int numRows = 0;
    //debugPlan<<page<<rowsPrPage;
    while (idx.isValid() && numRows < rowsPrPage) {
        painter.translate(0, height);
        h += height;
        for (int i = 0; i < mh->count(); ++i) {
            if (mh->isSectionHidden(i)) {
                continue;
            }
            Qt::Edges edges = Qt::BottomEdge | Qt::LeftEdge;
            QModelIndex index = model->index(idx.row(), i, idx.parent());
            QString text = model->data(index).toString();
            QVariant a = model->data(index, Qt::TextAlignmentRole);
            int align = a.isValid() ? a.toInt() : (int)(Qt::AlignLeft|Qt::AlignVCenter);
            QRect r(mh->sectionPosition(i),  0, mh->sectionSize(i), height);
            if (higestIndex == i) {
                edges |= Qt::RightEdge;
                r.adjust(0, 0, 1, 0);
            }
            drawRect(painter, r, edges);
            painter.drawText(r.adjusted(3, 1, -3, -1) , align, text);
        }
        ++numRows;
        idx = m_tree->indexBelow(idx);
    }
}

/**
 * TreeViewBase is a QTreeView adapted for operation by keyboard only and as components in DoubleTreeViewBase.
 * Note that keyboard navigation and selection behavior may not be fully compliant with QTreeView.
 * If you use other settings than  QAbstractItemView::ExtendedSelection and QAbstractItemView::SelectRows,
 * you should have a look at the implementation keyPressEvent() and updateSelection().
 */

TreeViewBase::TreeViewBase(QWidget *parent)
    : QTreeView(parent),
    m_arrowKeyNavigation(true),
    m_acceptDropsOnView(false),
    m_readWrite(false),
    m_handleDrag(true)

{
    m_dragPixmap = koIcon("application-x-vnd.kde.plan").pixmap(32);
    setDefaultDropAction(Qt::MoveAction);
    setItemDelegate(new ItemDelegate(this));
    setAlternatingRowColors (true);
    setExpandsOnDoubleClick(false);

    header()->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(header(), &QWidget::customContextMenuRequested, this, &TreeViewBase::slotHeaderContextMenuRequested);
}

void TreeViewBase::dropEvent(QDropEvent *e)
{
    debugPlan;
    QTreeView::dropEvent(e);
}

KoPrintJob * TreeViewBase::createPrintJob(ViewBase *parent)
{
    TreeViewPrintingDialog *dia = new TreeViewPrintingDialog(parent, this, parent->project());
    dia->printer().setCreator(QStringLiteral("Plan %1").arg(QStringLiteral(PLAN_VERSION_STRING)));
//    dia->printer().setFullPage(true); // ignore printer margins
    return dia;
}

void TreeViewBase::setReadWrite(bool rw)
{
    m_readWrite = rw;
    if (model()) {
        model()->setData(QModelIndex(), rw, Role::ReadWrite);
    }
}

void TreeViewBase::createItemDelegates(ItemModelBase *model)
{
    for (int c = 0; c < model->columnCount(); ++c) {
        QAbstractItemDelegate *delegate = model->createDelegate(c, this);
        if (delegate) {
            setItemDelegateForColumn(c, delegate);
        }
    }
}

void TreeViewBase::slotHeaderContextMenuRequested(const QPoint& pos)
{
    //debugPlan;
    Q_EMIT headerContextMenuRequested(header()->mapToGlobal(pos));
}

void TreeViewBase::setColumnsHidden(const QList<int> &lst)
{
    //debugPlan<<m_hideList;
    int prev = -1;
    QList<int> xlst;
    for (int c : lst) {
        if (c == -1) {
            // hide rest
            for (int i = prev+1; i < model()->columnCount(); ++i) {
                if (! lst.contains(i)) {
                    xlst << i;
                }
            }
            break;
        }
        xlst << c;
        prev = c;
    }
    for (int c = 0; c < model()->columnCount(); ++c) {
        setColumnHidden(c, xlst.contains(c));
    }
}

QModelIndex TreeViewBase::firstColumn(int row, const QModelIndex &parent)
{
    int s;
    for (s = 0; s < header()->count(); ++s) {
        if (! header()->isSectionHidden(header()->logicalIndex(s))) {
            break;
        }
    }
    if (s == -1) {
        return QModelIndex();
    }
    return model()->index(row, header()->logicalIndex(s), parent);
}

QModelIndex TreeViewBase::lastColumn(int row, const QModelIndex &parent)
{
    int s;
    for (s = header()->count() - 1; s >= 0; --s) {
        if (! header()->isSectionHidden(header()->logicalIndex(s))) {
            break;
        }
    }
    if (s == -1) {
        return QModelIndex();
    }
    return model()->index(row, header()->logicalIndex(s), parent);
}

QModelIndex TreeViewBase::nextColumn(const QModelIndex &curr)
{
    return moveCursor(curr, QAbstractItemView::MoveRight);
}

QModelIndex TreeViewBase::previousColumn(const QModelIndex &curr)
{
    return moveCursor(curr, QAbstractItemView::MoveLeft);
}

QModelIndex TreeViewBase::firstEditable(int row, const QModelIndex &parent)
{
    QModelIndex index = firstColumn(row, parent);
    if (model()->flags(index) & Qt::ItemIsEditable) {
        return index;
    }
    return moveToEditable(index, QAbstractItemView::MoveRight);
}

QModelIndex TreeViewBase::lastEditable(int row, const QModelIndex &parent)
{
    QModelIndex index = lastColumn(row, parent);
    if (model()->flags(index) & Qt::ItemIsEditable) {
        return index;
    }
    return moveToEditable(index, QAbstractItemView::MoveLeft);
}

// Reimplemented to fix qt bug 160083: Doesn't scroll horizontally.
void TreeViewBase::scrollTo(const QModelIndex &index, ScrollHint hint)
{
    //debugPlan<<objectName()<<index<<hint;
    if (! hasFocus()) {
        return;
    }
    QTreeView::scrollTo(index, hint); // scrolls vertically
    if (! index.isValid()) {
        return;
    }
    // horizontal
    int viewportWidth = viewport()->width();
    int horizontalOffset = header()->offset();
    int horizontalPosition = header()->sectionPosition(index.column());
    int cellWidth = header()->sectionSize(index.column());

    if (hint == PositionAtCenter) {
        horizontalScrollBar()->setValue(horizontalPosition - ((viewportWidth - cellWidth) / 2));
    } else {
        if (horizontalPosition - horizontalOffset < 0 || cellWidth > viewportWidth)
            horizontalScrollBar()->setValue(horizontalPosition);
        else if (horizontalPosition - horizontalOffset + cellWidth > viewportWidth)
            horizontalScrollBar()->setValue(horizontalPosition - viewportWidth + cellWidth);
    }
}

void TreeViewBase::focusInEvent(QFocusEvent *event)
{
    //debugPlan<<event->reason();
    QAbstractScrollArea::focusInEvent(event); //NOTE: not QTreeView
    if (event->reason() == Qt::MouseFocusReason) {
        return;
    }
    QModelIndex curr = currentIndex();
    if (! curr.isValid() || ! isIndexHidden(curr)) {
        return;
    }
    QModelIndex idx = curr;
    for (int s = 0; s < header()->count(); ++s) {
        idx = model()->index(curr.row(), header()->logicalIndex(s), curr.parent());
        if (! isIndexHidden(idx)) {
            selectionModel()->setCurrentIndex(idx, QItemSelectionModel::NoUpdate);
            scrollTo(idx);
            break;
        }
    }
}

/*!
    \reimp
 */
void TreeViewBase::keyPressEvent(QKeyEvent *event)
{
    //debugPlan<<objectName()<<event->key()<<","<<m_arrowKeyNavigation;
    if (!m_arrowKeyNavigation) {
        QTreeView::keyPressEvent(event);
        return;
    }
    QModelIndex current = currentIndex();
    if (current.isValid()) {
        switch (event->key()) {
            case Qt::Key_Right: {
                QModelIndex nxt = moveCursor(MoveRight, Qt::NoModifier);
                if (nxt.isValid()) {
                    selectionModel()->setCurrentIndex(nxt, QItemSelectionModel::NoUpdate);
                } else {
                    Q_EMIT moveAfterLastColumn(current);
                }
                event->accept();
                return;
                break;
            }
            case Qt::Key_Left: {
                QModelIndex prv = moveCursor(MoveLeft, Qt::NoModifier);
                if (prv.isValid()) {
                    selectionModel()->setCurrentIndex(prv, QItemSelectionModel::NoUpdate);
                } else {
                    Q_EMIT moveBeforeFirstColumn(current);
                }
                event->accept();
                return;
                break;
            }
            case Qt::Key_Down: {
                QModelIndex i = moveCursor(MoveDown, Qt::NoModifier);
                updateSelection(current, i, event);
                event->accept();
                return;
                break;
            }
            case Qt::Key_Up: {
                QModelIndex i = moveCursor(MoveUp, Qt::NoModifier);
                updateSelection(current, i, event);
                event->accept();
                return;
                break;
            }
            default: break;
        }
    }
    QTreeView::keyPressEvent(event);
}

void TreeViewBase::updateSelection(const QModelIndex &oldidx, const QModelIndex &newidx, QKeyEvent *event)
{
    if (newidx == oldidx || ! newidx.isValid()) {
        return;
    }
    if (!hasFocus() && QApplication::focusWidget() == indexWidget(oldidx)) {
        setFocus();
    }
    QItemSelectionModel::SelectionFlags command;
    // NoUpdate on Key movement and Ctrl
    Qt::KeyboardModifiers modifiers = static_cast<const QKeyEvent*>(event)->modifiers();
    switch (static_cast<const QKeyEvent*>(event)->key()) {
    case Qt::Key_Backtab:
        modifiers = modifiers & ~Qt::ShiftModifier; // special case for backtab
        Q_FALLTHROUGH();
    case Qt::Key_Down:
    case Qt::Key_Up:
    case Qt::Key_Left:
    case Qt::Key_Right:
        if (modifiers & Qt::ControlModifier)
            command = QItemSelectionModel::NoUpdate;
        else if (modifiers & Qt::ShiftModifier)
            command = QItemSelectionModel::Select | selectionBehaviorFlags();
        else
            command = QItemSelectionModel::ClearAndSelect | selectionBehaviorFlags();
        break;
    default:
        break;
    }
    selectionModel()->setCurrentIndex(newidx, command);
}

void TreeViewBase::mousePressEvent(QMouseEvent *event)
{
    // If  the mouse is pressed outside any item, the current item should be/remain selected
    QPoint pos = event->pos();
    QModelIndex index = indexAt(pos);
    debugPlan<<index<<event->pos()<<index.flags();
    if (! index.isValid()) {
        index = selectionModel()->currentIndex();
        if (index.isValid() && ! selectionModel()->isSelected(index)) {
            pos = visualRect(index).center();
            QMouseEvent e(event->type(), pos, mapToGlobal(pos), event->button(), event->buttons(), event->modifiers());
            QTreeView::mousePressEvent(&e);
            event->setAccepted(e.isAccepted());
            debugPlan<<index<<e.pos();
        }
        return;
    }
    QTreeView::mousePressEvent(event);
}

void TreeViewBase::closeEditor(QWidget *editor, QAbstractItemDelegate::EndEditHint hint)
{
    //debugPlan<<editor<<hint;
    ItemDelegate *delegate = ::qobject_cast<ItemDelegate*>(sender());
    if (delegate == nullptr) {
        warnPlan<<"Not a KPlato::ItemDelegate, try standard treatment"<<editor<<hint;
        return QTreeView::closeEditor(editor, hint);
    }
    // Hacky, if only hint was an int!
    Delegate::EndEditHint endHint = delegate->endEditHint();
    // Close editor, do nothing else
    QTreeView::closeEditor(editor, QAbstractItemDelegate::NoHint);


    QModelIndex index;
    switch (endHint) {
        case Delegate::EditLeftItem:
            index = moveToEditable(currentIndex(), MoveLeft);
            break;
        case Delegate::EditRightItem:
            index = moveToEditable(currentIndex(), MoveRight);
            break;
        case Delegate::EditDownItem:
            index = moveToEditable(currentIndex(), MoveDown);
            break;
        case Delegate::EditUpItem:
            index = moveToEditable(currentIndex(), MoveUp);
            break;
        default:
            //debugPlan<<"Standard treatment"<<editor<<hint;
            return QTreeView::closeEditor(editor, hint); // standard treatment
    }
    if (index.isValid()) {
        QItemSelectionModel::SelectionFlags flags = QItemSelectionModel::ClearAndSelect | selectionBehaviorFlags();
        //debugPlan<<flags;
        QPersistentModelIndex persistent(index);
        selectionModel()->setCurrentIndex(persistent, flags);
        // currentChanged signal would have already started editing
        if (!(editTriggers() & QAbstractItemView::CurrentChanged)) {
            edit(persistent);
        }
    }
}

QModelIndex TreeViewBase::moveToEditable(const QModelIndex &index, CursorAction cursorAction)
{
    QModelIndex ix = index;
    do {
        ix = moveCursor(ix, cursorAction);
    } while (ix.isValid() &&  ! (model()->flags(ix) & Qt::ItemIsEditable));
    //debugPlan<<ix;
    if (! ix.isValid()) {
        switch (cursorAction) {
            case MovePrevious:
            case MoveLeft: Q_EMIT editBeforeFirstColumn(index); break;
            case MoveNext:
            case MoveRight: Q_EMIT editAfterLastColumn(index); break;
            default: break;
        }
    }
    return ix;
}


/*
    Reimplemented from QTreeView to make tab/backtab in editor work reasonably well.
    Move the cursor in the way described by \a cursorAction, *not* using the
    information provided by the button \a modifiers.
 */

QModelIndex TreeViewBase::moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers)
{
    QModelIndex current = currentIndex();
    //debugPlan<<cursorAction<<current;
    if (!current.isValid()) {
        return QTreeView::moveCursor(cursorAction, modifiers);
    }
    return moveCursor(current, cursorAction, modifiers);
}

QModelIndex TreeViewBase::moveCursor(const QModelIndex &index, CursorAction cursorAction, Qt::KeyboardModifiers modifiers)
{
    executeDelayedItemsLayout();
    QModelIndex current = index;
    int col = current.column();
    QModelIndex ix;
    switch (cursorAction) {
        case MoveDown: {
            // TODO: span

            // Fetch the index below current.
            // This should be the next non-hidden row, same column as current,
            // that has a column in current.column()
            ix = indexBelow(current);
            while (ix.isValid() && col >= model()->columnCount(ix.parent())) {
                //debugPlan<<col<<model()->columnCount(ix.parent())<<ix;
                ix = indexBelow(ix);
            }
            if (ix.isValid()) {
                ix = model()->index(ix.row(), col, ix.parent());
            } // else Here we could go to the top
            return ix;
        }
        case MoveUp: {
            // TODO: span

            // Fetch the index above current.
            // This should be the previous non-hidden row, same column as current,
            // that has a column in current.column()
            ix = indexAbove(current);
            while (ix.isValid() && col >= model()->columnCount(ix.parent())) {
                ix = indexAbove(ix);
            }
            if (ix.isValid()) {
                ix = model()->index(ix.row(), col, ix.parent());
            } // else Here we could go to the bottom
            return ix;
        }
        case MovePrevious:
        case MoveLeft: {
            for (int s = header()->visualIndex(col) - 1; s >= 0; --s) {
                if (! header()->isSectionHidden(header()->logicalIndex(s))) {
                    ix = model()->index(current.row(), header()->logicalIndex(s), current.parent());
                    break;
                }
            }
            return ix;
        }
        case MoveNext:
        case MoveRight: {
            for (int s = header()->visualIndex(col) + 1; s < header()->count(); ++s) {
                if (! header()->isSectionHidden(header()->logicalIndex(s))) {
                    ix = model()->index(current.row(), header()->logicalIndex(s), current.parent());
                    break;
                }
            }
            return ix;
        }
        case MovePageUp:
        case MovePageDown: {
            ix = QTreeView::moveCursor(cursorAction, modifiers);
            // Now we are at the correct row, so move to correct column
            if (ix.isValid()) {
                ix = model()->index(ix.row(), col, ix.parent());
            } // else Here we could go to the bottom
            return ix;
        }
        case MoveHome: {
            if ((modifiers & Qt::ControlModifier) == 0) {
                ix = QTreeView::moveCursor(cursorAction, modifiers); // move to first row
            } else { //stay at this row
                ix = current;
            }
            for (int s = 0; s < header()->count(); ++s) {
                int logicalIndex = header()->logicalIndex(s);
                if (! isColumnHidden(logicalIndex)) {
                    ix = model()->index(ix.row(), header()->logicalIndex(s), ix.parent());
                    break;
                }
            }
            return ix;
        }
        case MoveEnd: {
            if ((modifiers & Qt::ControlModifier) == 0) {
                ix = QTreeView::moveCursor(cursorAction, modifiers); // move to last row
            } else { //stay at this row
                ix = current;
            }
            for (int s = header()->count() - 1; s >= 0; --s) {
                int logicalIndex = header()->logicalIndex(s);
                if (! isColumnHidden(logicalIndex)) {
                    ix = model()->index(ix.row(), logicalIndex, ix.parent());
                    break;
                }
            }
            return ix;
        }
        default: break;
    }
    return ix;
}

void TreeViewBase::contextMenuEvent (QContextMenuEvent *event)
{
    debugPlan<<selectionModel()->selectedRows();
    Q_EMIT contextMenuRequested(indexAt(event->pos()), event->globalPos(), selectionModel()->selectedRows());
}

void TreeViewBase::slotCurrentChanged(const QModelIndex &current, const QModelIndex &)
{
    if (current.isValid()) {
        scrollTo(current);
    }
}

void TreeViewBase::setModel(QAbstractItemModel *model)
{
    if (selectionModel()) {
        disconnect(selectionModel(), &QItemSelectionModel::currentChanged, this, &TreeViewBase::slotCurrentChanged);
    }
    QTreeView::setModel(model);
    if (selectionModel()) {
        connect(selectionModel(), &QItemSelectionModel::currentChanged, this, &TreeViewBase::slotCurrentChanged);
    }
    setReadWrite(m_readWrite);
}

void TreeViewBase::setSelectionModel(QItemSelectionModel *model)
{
    if (selectionModel()) {
        disconnect(selectionModel(), &QItemSelectionModel::currentChanged, this, &TreeViewBase::slotCurrentChanged);
    }
    QTreeView::setSelectionModel(model);
    if (selectionModel()) {
        connect(selectionModel(), &QItemSelectionModel::currentChanged, this, &TreeViewBase::slotCurrentChanged);
    }
}

void TreeViewBase::setStretchLastSection(bool mode)
{
    header()->setStretchLastSection(mode);
}

void TreeViewBase::mapToSection(int col, int section)
{
    header()->moveSection(header()->visualIndex(col), section);
}

int TreeViewBase::section(int col) const
{
    return header()->visualIndex(col);
}

void TreeViewBase::dragMoveEvent(QDragMoveEvent *event)
{
    //debugPlan;
    if (dragDropMode() == InternalMove
        && (event->source() != this || !(event->possibleActions() & Qt::MoveAction))) {
        //debugPlan<<"Internal:"<<event->isAccepted();
        return;
    }
    QTreeView::dragMoveEvent(event);
    if (dropIndicatorPosition() == QAbstractItemView::OnViewport) {
        if (! m_acceptDropsOnView) {
            event->ignore();
        }
        debugPlan<<"On viewport:"<<event->isAccepted();
    } else {
        QModelIndex index = indexAt(event->position().toPoint());
        if (index.isValid() && m_readWrite) {
            Q_EMIT dropAllowed(index, dropIndicatorPosition(), event);
        } else {
            event->ignore();
            debugPlan<<"Invalid index:"<<event->isAccepted();
        }
    }
    if (event->isAccepted()) {
        if (viewport()->cursor().shape() == Qt::ForbiddenCursor) {
            viewport()->unsetCursor();
        }
    } else if (viewport()->cursor().shape() != Qt::ForbiddenCursor) {
        viewport()->setCursor(Qt::ForbiddenCursor);
    }
    debugPlan<<event->isAccepted()<<viewport()->cursor().shape();
}

QModelIndex TreeViewBase::firstVisibleIndex(const QModelIndex &idx) const
{
    int count = model()->columnCount();
    for (int c = 0; c < count; ++c) {
        if (! isColumnHidden(c)) {
            return model()->index(idx.row(), c, model()->parent(idx));
        }
    }
    return QModelIndex();
}


bool TreeViewBase::loadContext(const QMetaEnum &map, const KoXmlElement &element, bool expand)
{
    debugPlan<<this;
    header()->setStretchLastSection((bool)(element.attribute(QStringLiteral("stretch-last-column"), QString::number(1)).toInt()));
    KoXmlElement e = element.namedItem("columns").toElement();
    if (! e.isNull()) {
        if (! map.isValid()) {
            // try numbers
            debugPlan<<"invalid map";
            for (int i = model()->columnCount() - 1; i >= 0; --i) {
                QString s = e.attribute(QStringLiteral("column-%1").arg(i), QString());
                if (s == QStringLiteral("hidden")) {
                    hideColumn(i);
                } else if (s == QStringLiteral("shown")) {
                    showColumn(i);
                } else debugPlan<<objectName()<<"Unknown column:"<<s;
            }
        } else {
            for (int i = model()->columnCount() - 1; i >= 0; --i) {
                QString n = QLatin1String(map.key(i));
                //debugPlan<<i<<"="<<n;
                if (! n.isEmpty()) {
                    QString s = e.attribute(n, QString());
                    if (s == QStringLiteral("hidden")) {
                        hideColumn(i);
                    } else if (s == QStringLiteral("shown")) {
                        showColumn(i);
                    } else debugPlan<<objectName()<<"Unknown column:"<<s;
                } else debugPlan<<"Column not in enum:"<<i<<map.name()<<map.keyCount();
            }
        }
    }
    e = element.namedItem("sections").toElement();
    if (! e.isNull()) {
        QHeaderView *h = header();
        QString s = QStringLiteral("section-%1");
        if (! map.isValid()) {
            // try numbers
            for (int i = 0; i < h->count(); ++i) {
                if (e.hasAttribute(s.arg(i))) {
                    int index = e.attribute(s.arg(i), QString::number(-1)).toInt();
                    if (index >= 0 && index < h->count()) {
                        header()->moveSection(h->visualIndex(index), i);
                    }
                }
            }
        } else {
            QMap<int, int > m; // QMap<destination, column>85
            for (int i = 0; i < h->count(); ++i) {
                QString n = e.attribute(s.arg(i));
                if (n.isEmpty()) {
                    continue;
                }
                int col = map.keyToValue(n.toUtf8().constData());
                if (col >= 0 && col < h->count()) {
                    m.insert(i, col);
                }
            }
            for (QMap<int, int>::const_iterator it = m.constBegin(); it != m.constEnd(); ++it) {
                int current = h->visualIndex(it.value());
                header()->moveSection(current, it.key());
            }
        }
    }
    if (expand) {
        loadExpanded(element);
    }
    if (!e.isNull()) {
        // FIXME: This only works for column 0
        QHeaderView *h = header();
        QString s = QStringLiteral("size-%1");
        for (int i = 0; i < model()->columnCount(); ++i) {
            if (!h->isSectionHidden(i) && e.hasAttribute(s.arg(i))) {
                int size = e.attribute(s.arg(i)).toInt();
                if (size > 0) {
                    h->resizeSection(i, size);
                }
            }
        }
    }
    return true;
}

void TreeViewBase::saveContext(const QMetaEnum &map, QDomElement &element, bool expand) const
{
    //debugPlan<<objectName();
    element.setAttribute(QStringLiteral("stretch-last-column"), QString::number(header()->stretchLastSection()));
    QDomElement e = element.ownerDocument().createElement(QStringLiteral("columns"));
    element.appendChild(e);
    for (int i = 0; i < model()->columnCount(); ++i) {
        bool h = isColumnHidden(i);
        if (! map.isValid()) {
            debugPlan<<"invalid map";
            e.setAttribute(QStringLiteral("column-%1").arg(i), h ? QStringLiteral("hidden") : QStringLiteral("shown"));
        } else {
            QString n = QLatin1String(map.key(i));
            //debugPlan<<i<<"="<<n;
            if (! n.isEmpty()) {
                e.setAttribute(n, h ? QStringLiteral("hidden") : QStringLiteral("shown"));
            }
        }
    }
    e = element.ownerDocument().createElement(QStringLiteral("sections"));
    element.appendChild(e);
    QHeaderView *h = header();
    for (int i = 0; i < h->count(); ++i) {
        if (! isColumnHidden(h->logicalIndex(i))) {
            if (! map.isValid()) {
                e.setAttribute(QStringLiteral("section-%1").arg(i), h->logicalIndex(i));
            } else {
                QString n = QLatin1String(map.key(h->logicalIndex(i)));
                if (! n.isEmpty()) {
                    e.setAttribute(QStringLiteral("section-%1").arg(i), n);
                    e.setAttribute(QStringLiteral("size-%1").arg(i), h->sectionSize(h->logicalIndex(i)));
                }
            }
        }
    }
    if (expand) {
        QDomElement expanded = element.ownerDocument().createElement(QStringLiteral("expanded"));
        element.appendChild(expanded);
        saveExpanded(expanded);
    }
}

ItemModelBase *TreeViewBase::itemModel() const
{
    QAbstractItemModel *m = model();
    QAbstractProxyModel *p = qobject_cast<QAbstractProxyModel*>(m);
    while (p) {
        m = p->sourceModel();
        p = qobject_cast<QAbstractProxyModel*>(m);
    }
    return qobject_cast<ItemModelBase*>(m);
}

void TreeViewBase::expandRecursive(const QModelIndex &idx, bool xpand)
{
    int rowCount = model()->rowCount(idx);
    if (rowCount == 0) {
        return;
    }
    xpand ? expand(idx) : collapse(idx);
    for (int r = 0; r < rowCount; ++r) {
        QModelIndex i = model()->index(r, 0, idx);
        Q_ASSERT(i.isValid());
        expandRecursive(i, xpand);
    }
}

void TreeViewBase::slotExpand()
{
// NOTE: Do not use this, KGantt does not like it
//     if (!m_contextMenuIndex.isValid()) {
//         expandAll();
//         return;
//     }
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QModelIndex idx = m_contextMenuIndex;
    if (idx.column() > 0) {
        idx = idx.model()->index(idx.row(), idx.column(), idx.parent()); //NOLINT(clang-analyzer-core.CallAndMessage)
    }
    expandRecursive(idx, true);
    QApplication::restoreOverrideCursor();
}

void TreeViewBase::slotCollapse()
{
// NOTE: Do not use this, KGantt does not like it
//     if (!m_contextMenuIndex.isValid()) {
//         collapseAll();
//         return;
//     }
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QModelIndex idx = m_contextMenuIndex;
    if (idx.column() > 0) {
        idx = idx.model()->index(idx.row(), 0, idx.parent()); //NOLINT(clang-analyzer-core.CallAndMessage)
    }
    expandRecursive(idx, false);
    QApplication::restoreOverrideCursor();
}

void TreeViewBase::setContextMenuIndex(const QModelIndex &idx)
{
    m_contextMenuIndex = idx;
}

void TreeViewBase::loadExpanded(const KoXmlElement &element)
{
    // we get here on loadContext()
    m_loadContextDoc.clear();
    KoXmlElement expanded = element.namedItem("expanded").toElement();
    if (expanded.isNull()) {
        return;
    }
    KoXml::asQDomElement(m_loadContextDoc, expanded);

    // FIXME:
    // if data is dependent on schedule manager
    // we cannot do anything until schedulemanager is set,
    // so we wait a bit and hope everything is ok
    QTimer::singleShot(500, this, &TreeViewBase::doContextExpanded);
}

void TreeViewBase::expandRecursivly(QDomElement element, const QModelIndex &parent)
{
    if (element.isNull()) {
        return;
    }
    for(QDomNode n = element.firstChild(); !n.isNull(); n = n.nextSibling()) {
        QDomElement e = n.toElement();
        if (e.tagName() != QStringLiteral("item")) {
            continue;
        }
        int childRow = e.attribute(QStringLiteral("row"), QString::number(-1)).toInt();
        if (childRow > -1) {
            QModelIndex idx = model()->index(childRow, 0, parent);
            if (idx.isValid()) {
                setExpanded(idx, true);
                expandRecursivly(e, idx);
            }
        }
    }
}

void TreeViewBase::doExpand(QDomDocument &doc)
{
    // we get here on setScheduleManager()
    m_expandDoc = doc;
    QTimer::singleShot(0, this, &TreeViewBase::doExpanded);
}

void TreeViewBase::doContextExpanded()
{
    expandRecursivly(m_loadContextDoc.documentElement());
}

void TreeViewBase::doExpanded()
{
    expandRecursivly(m_expandDoc.documentElement());
}

void TreeViewBase::saveExpanded(QDomElement &element, const QModelIndex &parent) const
{
    for (int r = 0; r < model()->rowCount(parent); ++r) {
        QModelIndex idx = model()->index(r, 0, parent);
        if (isExpanded(idx)) {
            QDomElement e = element.ownerDocument().createElement(QStringLiteral("item"));
            e.setAttribute(QStringLiteral("row"), r);
            element.appendChild(e);
            saveExpanded(e, idx);
        }
    }
}

void TreeViewBase::setHandleDrag(bool state)
{
    m_handleDrag = state;
}

void TreeViewBase::startDrag(Qt::DropActions supportedActions)
{
    Qt::DropAction defaultDropAction = Qt::IgnoreAction;
    if (this->defaultDropAction() != Qt::IgnoreAction && (supportedActions & this->defaultDropAction())) {
        defaultDropAction = this->defaultDropAction();
    } else if (supportedActions & Qt::CopyAction && dragDropMode() != QAbstractItemView::InternalMove) {
        defaultDropAction = Qt::CopyAction;
    }
    if (m_handleDrag) {
        QMimeData *data = mimeData();
        if (!data) {
            debugPlan<<"No mimedata";
            return;
        }
        QDrag *drag = new QDrag(this);
        drag->setPixmap(m_dragPixmap);
        drag->setMimeData(data);
        drag->exec(supportedActions, defaultDropAction);
    } else {
        static_cast<DoubleTreeViewBase*>(parent())->handleDrag(supportedActions, defaultDropAction);
    }
}

QList<int> TreeViewBase::visualColumns() const
{
    if (!isVisible()) {
        return QList<int>();
    }
    QMap<int, int> columns;
    for (int i = 0; i < model()->columnCount(); ++i) {
        if (!isColumnHidden(i)) {
            columns.insert(header()->visualIndex(i), i);
        }
    }
    return columns.values();
}

void TreeViewBase::setDragPixmap(const QPixmap &pixmap)
{
    m_dragPixmap = pixmap;
}

QPixmap TreeViewBase::dragPixmap() const
{
    return m_dragPixmap;
}

QMimeData *TreeViewBase::mimeData() const
{
    QModelIndexList rows = selectionModel()->selectedRows();
    sort(this, rows);
    if (rows.isEmpty()) {
        debugPlan<<"No rows selected";
        return nullptr;
    }
    QList<int> columns;;
    columns = visualColumns();
    QModelIndexList indexes;
    for (int r = 0; r < rows.count(); ++r) {
        int row = rows.at(r).row();
        const QModelIndex &parent = rows.at(r).parent();
        for (int i = 0; i < columns.count(); ++i) {
            indexes << model()->index(row, columns.at(i), parent);
        }
    }
    return model()->mimeData(indexes);
}


void TreeViewBase::editCopy()
{
    QMimeData *data = mimeData();
    if (!data) {
        debugPlan<<"No mimedata";
        return;
    }
    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setMimeData(data);
}

void TreeViewBase::editPaste()
{
}

QModelIndexList TreeViewBase::selectedIndexes() const
{
    QModelIndexList viewSelected;
    QModelIndexList modelSelected;
    if (selectionModel())
        modelSelected = selectionModel()->selectedIndexes();
    for (int i = 0; i < modelSelected.count(); ++i) {
        // check that neither the parents nor the index is hidden before we add
        QModelIndex index = modelSelected.at(i);
        while (index.isValid() && !isIndexHidden(index)) {
            int column = index.column();
            index = index.parent();
            if (index.isValid() && column != index.column()) {
                index = index.sibling(index.row(), column);
            }
        }
        if (index.isValid())
            continue;
        viewSelected.append(modelSelected.at(i));
    }
    return viewSelected;
}

//----------------------
DoubleTreeViewPrintingDialog::DoubleTreeViewPrintingDialog(ViewBase *view, DoubleTreeViewBase *treeview, Project *project)
    : PrintingDialog(view),
    m_tree(treeview),
    m_project(project),
    m_firstRow(-1)
{
    printer().setFromTo(documentFirstPage(), documentLastPage());
}

int DoubleTreeViewPrintingDialog::documentLastPage() const
{
    debugPlan<<KoPageFormat::formatString(m_view->pageLayout().format);
    int page = documentFirstPage();
    while (firstRow(page) != -1) {
        ++page;
    }
    if (page > documentFirstPage()) {
        --page;
    }
    return page;
}

int DoubleTreeViewPrintingDialog::firstRow(int page) const
{
    debugPlan<<page;
    int pageNumber = page - documentFirstPage();
    QHeaderView *mh = m_tree->masterView()->header();
    QHeaderView *sh = m_tree->slaveView()->header();
    int height = mh->height() > sh->height() ? mh->height() : sh->height();
    int hHeight = headerRect().height();
    int fHeight = footerRect().height();
    const auto resolution = const_cast<DoubleTreeViewPrintingDialog*>(this)->printer().resolution();
    QRect pageRect = const_cast<DoubleTreeViewPrintingDialog*>(this)->printer().pageLayout().paintRectPixels(resolution);

    int gap = 8;
    int pageHeight = pageRect.height() - height;
    if (hHeight > 0) {
        pageHeight -= (hHeight + gap);
    }
    if (fHeight > 0) {
        pageHeight -= (fHeight + gap);
    }
    int rowsPrPage = pageHeight / height;

    debugPlan<<"rowsPrPage"<<rowsPrPage;
    Q_ASSERT(rowsPrPage > 0);

    int rows = m_tree->model()->rowCount();
    int row = -1;
    for (int i = 0; i < rows; ++i) {
        if (! m_tree->masterView()->isRowHidden(i, QModelIndex())) {
            row = i;
            break;
        }
    }
    if (row != -1) {
        QModelIndex idx = m_tree->model()->index(row, 0, QModelIndex());
        row = 0;
        while (idx.isValid()) {
            if (row >= rowsPrPage * pageNumber) {
                debugPlan<<page<<pageNumber;
                break;
            }
            ++row;
            idx = m_tree->masterView()->indexBelow(idx);
        }
        if (! idx.isValid()) {
            row = -1;
        }
    }
    debugPlan<<"Page"<<page<<":"<<(row==-1?"empty":"first row=")<<row<<"("<<rowsPrPage<<")";
    return row;
}

QList<QWidget*> DoubleTreeViewPrintingDialog::createOptionWidgets() const
{
    QList<QWidget*> lst;
    lst << createPageLayoutWidget();
    lst += PrintingDialog::createOptionWidgets();
    return  lst;
}

void DoubleTreeViewPrintingDialog::printPage(int page, QPainter &painter)
{
    debugPlan<<page<<"paper size:"<<printer().pageLayout().pageSize().id()<<"---------------------------";
    setPrinterPageLayout(m_view->pageLayout());
    const auto resolution = printer().resolution();
    auto pageRect = printer().pageLayout().paintRectPixels(resolution);
    debugPlan<<page<<"page size:"<<pageRect;
    painter.save();

    m_firstRow = firstRow(page);

    QHeaderView *mh = m_tree->masterView()->header();
    QHeaderView *sh = m_tree->slaveView()->header();
    int length = mh->length() + sh->length();
    int height = mh->height() > sh->height() ? mh->height() : sh->height();
    QRect hRect = headerRect();
    QRect fRect = footerRect();
    pageRect.moveTo(0, 0);

    QAbstractItemModel *model = m_tree->model();
    Q_ASSERT(model != nullptr);
    debugPlan<<pageRect;

    painter.translate(pageRect.topLeft());

    painter.setClipping(true);

    if (m_project) {
        paintHeaderFooter(painter, printingOptions(), page, *(m_project));
    }
    int gap = 8;
    int pageHeight = pageRect.height() - height;
    if (hRect.isValid()) {
        pageHeight -= (hRect.height() + gap);
    }
    if (fRect.isValid()) {
        pageHeight -= (fRect.height() + gap);
    }
    int rowsPrPage = pageHeight / height;

    double sx = pageRect.width() > length ? 1.0 : (double)pageRect.width() / (double)length;
    double sy = 1.0;
    painter.scale(sx, sy);

    int h = 0;

    painter.translate(0, hRect.height() + gap);
    h = hRect.height() + gap;

    painter.setPen(Qt::black);
    painter.setBrush(Qt::lightGray);
    int higestIndex = 0;
    int rightpos = 0;
    for (int i = 0; i < mh->count(); ++i) {
        QString text = model->headerData(i, Qt::Horizontal).toString();
        QVariant a = model->headerData(i, Qt::Horizontal, Qt::TextAlignmentRole);
        int align = a.isValid() ? a.toInt() : (int)(Qt::AlignLeft|Qt::AlignVCenter);
        if (! mh->isSectionHidden(i)) {
            QRect r = QRect(mh->sectionPosition(i), 0, mh->sectionSize(i), height).adjusted(0, 0, 0, -painter.pen().width());
            if (rightpos < r.right()) {
                higestIndex = i;
                rightpos = r.right();
            }
            painter.drawRect(r);
            // FIXME There is a bug somewhere, the text somehow overwrites the rect outline for the first column!
            painter.save();
            painter.setBrush(QBrush());
            painter.drawText(r.adjusted(3, 1, -3, -1), align, text);
            painter.drawRect(r);
            painter.restore();
        }
        if (! sh->isSectionHidden(i)) {
            QRect r = QRect(sh->sectionPosition(i) + mh->length(), 0, sh->sectionSize(i), height).adjusted(0, 0, 0, -painter.pen().width());
            if (rightpos < r.right()) {
                higestIndex = i;
                rightpos = r.right();
            }
            painter.drawRect(r);
            painter.drawText(r.adjusted(3, 1, -3, -1), align, text);
        }
        //debugPlan<<text<<"hidden="<<h->isSectionHidden(i)<<h->sectionPosition(i);
    }
    if (m_firstRow == -1 || model->rowCount() == 0) {
        debugPlan<<"No data";
        painter.restore();
        return;
    }
    painter.setBrush(QBrush());

    QModelIndex idx = model->index(0, 0);
    for (int r = 0; r < m_firstRow && idx.isValid(); ++r) {
        idx = m_tree->masterView()->indexBelow(idx);
    }
    int numRows = 0;
    //debugPlan<<page<<rowsPrPage;
    while (idx.isValid() && numRows < rowsPrPage) {
        debugPlan<<"print:"<<idx;
        painter.translate(0, height);
        h += height;
        for (int i = 0; i < mh->count(); ++i) {
            if (mh->isSectionHidden(i) &&  sh->isSectionHidden(i)) {
                continue;
            }
            Qt::Edges edges = Qt::BottomEdge | Qt::LeftEdge;
            QModelIndex index = model->index(idx.row(), i, idx.parent());
            QString text = model->data(index).toString();
            QVariant a = model->data(index, Qt::TextAlignmentRole);
            int align = a.isValid() ? a.toInt() : (int)(Qt::AlignLeft|Qt::AlignVCenter);
            if (! mh->isSectionHidden(i)) {
                QRect r(mh->sectionPosition(i),  0, mh->sectionSize(i), height);
                if (higestIndex == i) {
                    edges |= Qt::RightEdge;
                    r.adjust(0, 0, 1, 0);
                }
                drawRect(painter, r, edges);
                painter.drawText(r.adjusted(3, 1, -3, -1) , align, text);
            }
            if (! sh->isSectionHidden(i)) {
                QRect r(sh->sectionPosition(i) + mh->length(), 0, sh->sectionSize(i), height);
                if (higestIndex == i) {
                    edges |= Qt::RightEdge;
                    r.adjust(0, 0, 1, 0);
                }
                drawRect(painter, r, edges);
                painter.drawText(r.adjusted(3, 1, -3, -1), align, text);
            }
        }
        ++numRows;
        idx = m_tree->masterView()->indexBelow(idx);
    }
    painter.restore();
}


/**
 * DoubleTreeViewBase is a QSplitter containing two treeviews.
 * This makes it possible to keep columns visible in one view when scrolling the other view horizontally.
 */

DoubleTreeViewBase::DoubleTreeViewBase(bool /*mode*/, QWidget *parent)
    : QSplitter(parent),
    m_rightview(nullptr),
    m_selectionmodel(nullptr),
    m_readWrite(false),
    m_mode(false)
{
    init();
}

DoubleTreeViewBase::DoubleTreeViewBase(QWidget *parent)
    : QSplitter(parent),
    m_rightview(nullptr),
    m_selectionmodel(nullptr),
    m_mode(false)
{
    init();
}

DoubleTreeViewBase::~DoubleTreeViewBase()
{
}

KoPrintJob *DoubleTreeViewBase::createPrintJob(ViewBase *parent)
{
    DoubleTreeViewPrintingDialog *dia = new DoubleTreeViewPrintingDialog(parent, this, parent->project());
    dia->printer().setCreator(QStringLiteral("Plan %1").arg(QStringLiteral(PLAN_VERSION_STRING)));
//    dia->printer().setFullPage(true); // ignore printer margins
    return dia;
}

void DoubleTreeViewBase::slotExpand()
{
    m_leftview->slotExpand();
}

void DoubleTreeViewBase::slotCollapse()
{
    m_leftview->slotCollapse();
}

void DoubleTreeViewBase::setParentsExpanded(const QModelIndex &idx, bool expanded)
{
    //debugPlan<<idx<<m_leftview->isExpanded(idx)<<m_rightview->isExpanded(idx);
    QModelIndex p = model()->parent(idx);
    QList<QModelIndex> lst;
    while (p.isValid()) {
        lst << p;
        p = model()->parent(p);
    }
    while (! lst.isEmpty()) {
        p = lst.takeLast();
        m_leftview->setExpanded(p, expanded);
        m_rightview->setExpanded(m_rightview->firstVisibleIndex(p), expanded); //HACK: qt can't handle that column 0 is hidden!
        //debugPlan<<p<<m_leftview->isExpanded(p)<<m_rightview->isExpanded(p);
    }
}

void DoubleTreeViewBase::init()
{
    setOrientation(Qt::Horizontal);
    setHandleWidth(3);
    m_leftview = new TreeViewBase(this);
    m_leftview->setObjectName(QStringLiteral("Left view"));
    m_leftview->setHandleDrag(false);
    addWidget(m_leftview);
    setStretchFactor(0, 1);
    m_rightview = new TreeViewBase(this);
    m_rightview->setObjectName(QStringLiteral("Right view"));
    m_rightview->setHandleDrag(false);
    addWidget(m_rightview);
    setStretchFactor(1, 1);

    m_leftview->setTreePosition(-1); // always visual index 0

    connect(m_leftview, &TreeViewBase::contextMenuRequested, this, &DoubleTreeViewBase::contextMenuRequested);
    connect(m_leftview, &TreeViewBase::headerContextMenuRequested, this, &DoubleTreeViewBase::slotLeftHeaderContextMenuRequested);

    connect(m_rightview, &TreeViewBase::contextMenuRequested, this, &DoubleTreeViewBase::contextMenuRequested);
    connect(m_rightview, &TreeViewBase::headerContextMenuRequested, this, &DoubleTreeViewBase::slotRightHeaderContextMenuRequested);

    connect(m_leftview->verticalScrollBar(), &QAbstractSlider::valueChanged, m_rightview->verticalScrollBar(), &QAbstractSlider::setValue);

    connect(m_rightview->verticalScrollBar(), &QAbstractSlider::valueChanged, m_leftview->verticalScrollBar(), &QAbstractSlider::setValue);

    connect(m_leftview, &TreeViewBase::moveAfterLastColumn, this, &DoubleTreeViewBase::slotToRightView);
    connect(m_rightview, &TreeViewBase::moveBeforeFirstColumn, this, &DoubleTreeViewBase::slotToLeftView);

    connect(m_leftview, &TreeViewBase::editAfterLastColumn, this, &DoubleTreeViewBase::slotEditToRightView);
    connect(m_rightview, &TreeViewBase::editBeforeFirstColumn, this, &DoubleTreeViewBase::slotEditToLeftView);

    connect(m_leftview, &QTreeView::expanded, m_rightview, &QTreeView::expand);
    connect(m_leftview, &QTreeView::collapsed, m_rightview, &QTreeView::collapse);

    connect(m_rightview, &QTreeView::expanded, m_leftview, &QTreeView::expand);
    connect(m_rightview, &QTreeView::collapsed, m_leftview, &QTreeView::collapse);

    connect(m_leftview, &TreeViewBase::dropAllowed, this, &DoubleTreeViewBase::dropAllowed);
    connect(m_rightview, &TreeViewBase::dropAllowed, this, &DoubleTreeViewBase::dropAllowed);

    m_actionSplitView = new QAction(koIcon("view-split-left-right"), QString(), this);
    m_actionSplitView->setObjectName(QStringLiteral("split_view"));
    setViewSplitMode(true);

    connect(m_leftview->header(), &QHeaderView::sortIndicatorChanged, this, &DoubleTreeViewBase::slotLeftSortIndicatorChanged);

    connect(m_rightview->header(), &QHeaderView::sortIndicatorChanged, this, &DoubleTreeViewBase::slotRightSortIndicatorChanged);
}

void DoubleTreeViewBase::slotLeftSortIndicatorChanged(int logicalIndex, Qt::SortOrder /*order*/)
{
    QSortFilterProxyModel *sf = qobject_cast<QSortFilterProxyModel*>(model());
    if (sf) {
        ItemModelBase *m = m_rightview->itemModel();
        if (m) {
            sf->setSortRole(m->sortRole(logicalIndex));
        }
    }
    m_leftview->header()->setSortIndicatorShown(true);
    // sorting controlled by left treeview, turn right off
    m_rightview->header()->setSortIndicatorShown(false);
}

void DoubleTreeViewBase::slotRightSortIndicatorChanged(int logicalIndex, Qt::SortOrder /*order*/)
{
    QSortFilterProxyModel *sf = qobject_cast<QSortFilterProxyModel*>(model());
    if (sf) {
        ItemModelBase *m = m_rightview->itemModel();
        if (m) {
            sf->setSortRole(m->sortRole(logicalIndex));
        }
    }
    m_rightview->header()->setSortIndicatorShown(true);
    // sorting controlled by right treeview, turn left off
    m_leftview->header()->setSortIndicatorShown(false);
}


QList<int> DoubleTreeViewBase::expandColumnList(const QList<int> &lst) const
{
    QList<int> mlst = lst;
    if (! mlst.isEmpty()) {
        int v = 0;
        if (mlst.last() == -1 && mlst.count() > 1) {
            v = mlst[ mlst.count() - 2 ] + 1;
            mlst.removeLast();
        }
        for (int c = v; c < model()->columnCount(); ++c) {
            mlst << c;
        }
    }
    return mlst;
}

void DoubleTreeViewBase::hideColumns(TreeViewBase *view, const QList<int> &list)
{
    view->setColumnsHidden(list);
}

void DoubleTreeViewBase::hideColumns(const QList<int> &masterList, const QList<int> &slaveList)
{
    m_leftview->setColumnsHidden(masterList);
    m_rightview->setColumnsHidden(slaveList);
    if (m_rightview->isHidden()) {
        QList<int> mlst = expandColumnList(masterList);
        QList<int> slst = expandColumnList(slaveList);
        QList<int> lst;
        for (int c = 0; c < model()->columnCount(); ++c) {
            // only hide columns hidden in *both* views
            //debugPlan<<c<<(mlst.indexOf(c))<<(slst.indexOf(c));
            if ((mlst.indexOf(c) >= 0) && (slst.indexOf(c) >= 0)) {
                lst << c;
            }
        }
        //debugPlan<<lst;
        m_leftview->setColumnsHidden(lst);
    } else {
        setStretchFactors();
    }
}

void DoubleTreeViewBase::slotToRightView(const QModelIndex &index)
{
    //debugPlan<<index.column();
    QModelIndex nxt = m_rightview->firstColumn(index.row(), model()->parent(index));
    m_rightview->setFocus();
    if (nxt.isValid()) {
        m_selectionmodel->setCurrentIndex(nxt, QItemSelectionModel::NoUpdate);
    }
}

void DoubleTreeViewBase::slotToLeftView(const QModelIndex &index)
{
    //debugPlan<<index.column();
    QModelIndex prv = m_leftview->lastColumn(index.row(), model()->parent(index));
    m_leftview->setFocus();
    if (prv.isValid()) {
        m_selectionmodel->setCurrentIndex(prv, QItemSelectionModel::NoUpdate);
    }
}

void DoubleTreeViewBase::slotEditToRightView(const QModelIndex &index)
{
    //debugPlan<<index.column()<<'\n';
    if (m_rightview->isHidden()) {
        return;
    }
    m_rightview->setFocus();
    QModelIndex nxt = m_rightview->firstEditable(index.row(), model()->parent (index));
    if (nxt.isValid() && (model()->flags(nxt) & Qt::ItemIsEditable)) {
        m_selectionmodel->setCurrentIndex(nxt, QItemSelectionModel::NoUpdate);
        m_rightview->edit(nxt);
    } else {
        slotToRightView(index);
    }
}

void DoubleTreeViewBase::slotEditToLeftView(const QModelIndex &index)
{
    //debugPlan<<index.column()<<'\n';
    if (m_leftview->isHidden()) {
        return;
    }
    m_leftview->setFocus();
    QModelIndex nxt = m_leftview->lastEditable(index.row(), model()->parent (index));
    if (nxt.isValid() && (model()->flags(nxt) & Qt::ItemIsEditable)) {
        m_selectionmodel->setCurrentIndex(nxt, QItemSelectionModel::NoUpdate);
        m_leftview->edit(nxt);
    } else {
        slotToLeftView(index);
    }
}


void DoubleTreeViewBase::setReadWrite(bool rw)
{
    m_readWrite = rw;
    m_leftview->setReadWrite(rw);
    m_rightview->setReadWrite(rw);
}

void DoubleTreeViewBase::closePersistentEditor(const QModelIndex &index)
{
    m_leftview->closePersistentEditor(index);
    m_rightview->closePersistentEditor(index);
}

void DoubleTreeViewBase::setModel(QAbstractItemModel *model)
{
    m_leftview->setModel(model);
    m_rightview->setModel(model);
    if (m_selectionmodel) {
        disconnect(m_selectionmodel, &QItemSelectionModel::selectionChanged, this, &DoubleTreeViewBase::slotSelectionChanged);

        disconnect(m_selectionmodel, &QItemSelectionModel::currentChanged, this, &DoubleTreeViewBase::currentChanged);
    }
    m_selectionmodel = m_leftview->selectionModel();
    m_rightview->setSelectionModel(m_selectionmodel);

    connect(m_selectionmodel, &QItemSelectionModel::selectionChanged, this, &DoubleTreeViewBase::slotSelectionChanged);

    connect(m_selectionmodel, &QItemSelectionModel::currentChanged, this, &DoubleTreeViewBase::currentChanged);

    setReadWrite(m_readWrite);
}

QAbstractItemModel *DoubleTreeViewBase::model() const
{
    return m_leftview->model();
}

void DoubleTreeViewBase::slotSelectionChanged(const QItemSelection &, const QItemSelection &)
{
    Q_EMIT selectionChanged(selectionModel()->selectedIndexes());
}

void DoubleTreeViewBase::setSelectionModel(QItemSelectionModel *model)
{
    m_leftview->setSelectionModel(model);
    m_rightview->setSelectionModel(model);
}

void DoubleTreeViewBase::setSelectionMode(QAbstractItemView::SelectionMode mode)
{
    m_leftview->setSelectionMode(mode);
    m_rightview->setSelectionMode(mode);
}

void DoubleTreeViewBase::setSelectionBehavior(QAbstractItemView::SelectionBehavior mode)
{
    m_leftview->setSelectionBehavior(mode);
    m_rightview->setSelectionBehavior(mode);
}

void DoubleTreeViewBase::setItemDelegateForColumn(int col, QAbstractItemDelegate * delegate)
{
    m_leftview->setItemDelegateForColumn(col, delegate);
    m_rightview->setItemDelegateForColumn(col, delegate);
}

void DoubleTreeViewBase::createItemDelegates(ItemModelBase *model)
{
    m_leftview->createItemDelegates(model);
    m_rightview->createItemDelegates(model);
}

void DoubleTreeViewBase::setEditTriggers(QAbstractItemView::EditTriggers mode)
{
    m_leftview->setEditTriggers(mode);
    m_rightview->setEditTriggers(mode);
}

QAbstractItemView::EditTriggers DoubleTreeViewBase::editTriggers() const
{
    return m_leftview->editTriggers();
}

void DoubleTreeViewBase::setStretchLastSection(bool mode)
{
    m_rightview->header()->setStretchLastSection(mode);
    if (m_rightview->isHidden()) {
        m_leftview->header()->setStretchLastSection(mode);
    }
}

void DoubleTreeViewBase::edit(const QModelIndex &index)
{
    if (! m_leftview->isColumnHidden(index.column())) {
        m_leftview->edit(index);
    } else if (! m_rightview->isHidden() && ! m_rightview->isColumnHidden(index.column())) {
        m_rightview->edit(index);
    }
}

void DoubleTreeViewBase::setDragDropMode(QAbstractItemView::DragDropMode mode)
{
    m_leftview->setDragDropMode(mode);
    m_rightview->setDragDropMode(mode);
}

void DoubleTreeViewBase::setDragDropOverwriteMode(bool mode)
{
    m_leftview->setDragDropOverwriteMode(mode);
    m_rightview->setDragDropOverwriteMode(mode);
}

void DoubleTreeViewBase::setDropIndicatorShown(bool mode)
{
    m_leftview->setDropIndicatorShown(mode);
    m_rightview->setDropIndicatorShown(mode);
}

void DoubleTreeViewBase::setDragEnabled (bool mode)
{
    m_leftview->setDragEnabled(mode);
    m_rightview->setDragEnabled(mode);
}

void DoubleTreeViewBase::setAcceptDrops(bool mode)
{
    m_leftview->setAcceptDrops(mode);
    m_rightview->setAcceptDrops(mode);
}

void DoubleTreeViewBase::setAcceptDropsOnView(bool mode)
{
    m_leftview->setAcceptDropsOnView(mode);
    m_rightview->setAcceptDropsOnView(mode);
}

void DoubleTreeViewBase::setDefaultDropAction(Qt::DropAction action)
{
    m_leftview->setDefaultDropAction(action);
    m_rightview->setDefaultDropAction(action);
}

void DoubleTreeViewBase::slotRightHeaderContextMenuRequested(const QPoint &pos)
{
    //debugPlan;
    Q_EMIT slaveHeaderContextMenuRequested(pos);
    Q_EMIT headerContextMenuRequested(pos);
}

void DoubleTreeViewBase::slotLeftHeaderContextMenuRequested(const QPoint &pos)
{
    //debugPlan;
    Q_EMIT masterHeaderContextMenuRequested(pos);
    Q_EMIT headerContextMenuRequested(pos);
}

void DoubleTreeViewBase::setStretchFactors()
{
    int lc = m_leftview->header()->count() - m_leftview->header()->hiddenSectionCount();
    int rc = m_rightview->header()->count() - m_rightview->header()->hiddenSectionCount();
    setStretchFactor(indexOf(m_rightview), qMax(1, qMin(4, rc / qMax(1, lc))));
    //debugPlan<<this<<"set stretch factor="<<qMax(1, qMin(4, rc / lc));
}

bool DoubleTreeViewBase::loadContext(const QMetaEnum &map, const KoXmlElement &element)
{
    //debugPlan;
    KoXmlElement slave = element.namedItem("slave").toElement();
    if (!slave.isNull()) {
        if (slave.attribute(QStringLiteral("hidden"), QStringLiteral("false")) == QStringLiteral("true")) {
            setViewSplitMode(false);
        } else {
            setViewSplitMode(true);
            setStretchFactors();
        }
        m_rightview->loadContext(map, slave, false);
    }
    KoXmlElement master = element.namedItem("master").toElement();
    if (!master.isNull()) {
        m_leftview->loadContext(map, master);
    }
    return true;
}

void DoubleTreeViewBase::saveContext(const QMetaEnum &map, QDomElement &element) const
{
    QDomElement master = element.ownerDocument().createElement(QStringLiteral("master"));
    element.appendChild(master);
    m_leftview->saveContext(map, master);

    QDomElement slave = element.ownerDocument().createElement(QStringLiteral("slave"));
    element.appendChild(slave);
    if (m_rightview->isHidden()) {
        slave.setAttribute(QStringLiteral("hidden"), QStringLiteral("true"));
    }
    m_rightview->saveContext(map, slave, false);
}

void DoubleTreeViewBase::setViewSplitMode(bool split)
{
    if (split) {
        m_actionSplitView->setText(i18n("Unsplit View"));
        m_actionSplitView->setIcon(koIcon("view-close"));
    } else {
        m_actionSplitView->setText(i18n("Split View"));
        m_actionSplitView->setIcon(koIcon("view-split-left-right"));
    }

    if (m_mode == split) {
        return;
    }

    m_mode = split;
    if (split) {
        m_leftview->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_leftview->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        if (model()) {
            m_rightview->setColumnHidden(0, true);
            m_leftview->resizeColumnToContents(0);
            for (int c = 1; c < m_rightview->model()->columnCount(); ++c) {
                if (m_leftview->isColumnHidden(c)) {
                    m_rightview->setColumnHidden(c, true);
                } else {
                    m_rightview->setColumnHidden(c, false);
                    m_rightview->mapToSection(c, m_leftview->section(c));
                    m_leftview->setColumnHidden(c, true);
                    m_rightview->resizeColumnToContents(c);
                }
            }
        }
        m_rightview->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        m_rightview->show();
    } else if (model()) {
        QMap<int, int> sections;
        const int count = model()->columnCount();
        for (int i = 0; i < count; ++i) {
            if (!m_leftview->isColumnHidden(i)) {
                auto vindex = m_leftview->header()->visualIndex(i);
                Q_ASSERT(vindex >= 0);
                sections.insert(vindex, i);
            }
        }
        int offset = sections.count();
        for (int i = 0; i < count; ++i) {
            if (!m_rightview->isColumnHidden(i)) {
                auto vindex = m_rightview->header()->visualIndex(i);
                Q_ASSERT(vindex >= 0);
                if (sections.key(i, -1) == -1) {
                    sections.insert(vindex + offset, i);
                }
            }
        }
        for (int i = 0; i < count; ++i) {
            m_leftview->setColumnHidden(i, (sections.key(i, -1) == -1));
        }
        // sort columns
        for (QMap<int, int>::const_iterator it = sections.constBegin(); it != sections.constEnd(); ++it) {
            m_leftview->mapToSection(it.value(), it.key());
        }
        m_rightview->hide();
        m_leftview->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        m_leftview->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    }
}

void DoubleTreeViewBase::setRootIsDecorated (bool show)
{
    m_leftview->setRootIsDecorated(show);
    m_rightview->setRootIsDecorated(show);
}

QModelIndex DoubleTreeViewBase::indexAt(const QPoint &pos) const
{
    QModelIndex idx = m_leftview->indexAt(pos);
    if (! idx.isValid()) {
        idx = m_rightview->indexAt(pos);
    }
    return idx;
}

void DoubleTreeViewBase::setContextMenuIndex(const QModelIndex &idx)
{
    m_leftview->setContextMenuIndex(idx);
    m_rightview->setContextMenuIndex(idx);
}

QMimeData *DoubleTreeViewBase::mimeData() const
{
    QModelIndexList rows = selectionModel()->selectedRows();
    if (rows.isEmpty()) {
        debugPlan<<"No rows selected";
        return nullptr;
    }
    sort(m_leftview, rows);
    QList<int> columns;;
    columns = m_leftview->visualColumns() + m_rightview->visualColumns();
    QModelIndexList indexes;
    for (int r = 0; r < rows.count(); ++r) {
        int row = rows.at(r).row();
        const QModelIndex &parent = rows.at(r).parent();
        for (int i = 0; i < columns.count(); ++i) {
            indexes << model()->index(row, columns.at(i), parent);
        }
    }
    return model()->mimeData(indexes);
}

void DoubleTreeViewBase::handleDrag(Qt::DropActions supportedActions, Qt::DropAction defaultDropAction)
{
    QMimeData *data = mimeData();
    if (!data) {
        debugPlan<<"No mimedata";
        return;
    }
    QDrag *drag = new QDrag(this);
    drag->setPixmap(m_leftview->dragPixmap());
    drag->setMimeData(data);
    drag->exec(supportedActions, defaultDropAction);
}

void DoubleTreeViewBase::setDragPixmap(const QPixmap &pixmap)
{
    m_leftview->setDragPixmap(pixmap);
}

QPixmap DoubleTreeViewBase::dragPixmap() const
{
    return m_leftview->dragPixmap();
}

void DoubleTreeViewBase::editCopy()
{
    QMimeData *data = mimeData();
    if (!data) {
        debugPlan<<"No mimedata";
        return;
    }
    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setMimeData(data);
}

void DoubleTreeViewBase::editPaste()
{
}

} // namespace KPlato
