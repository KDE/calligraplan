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

// clazy:excludeall=qstring-arg
#include "GanttViewBase.h"
#include "kptnodeitemmodel.h"
#include "DateTimeTimeLine.h"
#include "kptganttitemdelegate.h"
#include "DateTimeGrid.h"

#include <KoXmlReader.h>
#include <KoPageLayoutWidget.h>

#include <KGanttProxyModel>
#include <KGanttConstraintModel>
#include <KGanttConstraint>
#include <KGanttGraphicsView>
#include <KGanttTreeViewRowController>
#include <KGanttDateTimeGrid>

#include <QDomElement>
#include <QMouseEvent>
#include <QDialog>
#include <QDrag>
#include <QPushButton>
#include <QHeaderView>
#include <QScrollBar>
#include <QClipboard>

/// The main namespace
namespace KPlato
{

class GanttItemDelegate;

//-------------------------------------------------
GanttChartDisplayOptionsPanel::GanttChartDisplayOptionsPanel(GanttViewBase *gantt, GanttItemDelegate *delegate, QWidget *parent)
    : QWidget(parent)
    , m_delegate(delegate)
    , m_gantt(gantt)
{
    setupUi(this);
    setValues(*delegate);

    connect(ui_showTaskName, &QCheckBox::stateChanged, this, &GanttChartDisplayOptionsPanel::changed);
    connect(ui_showResourceNames, &QCheckBox::stateChanged, this, &GanttChartDisplayOptionsPanel::changed);
    connect(ui_showDependencies, &QCheckBox::stateChanged, this, &GanttChartDisplayOptionsPanel::changed);
    connect(ui_showPositiveFloat, &QCheckBox::stateChanged, this, &GanttChartDisplayOptionsPanel::changed);
    connect(ui_showNegativeFloat, &QCheckBox::stateChanged, this, &GanttChartDisplayOptionsPanel::changed);
    connect(ui_showCriticalPath, &QCheckBox::stateChanged, this, &GanttChartDisplayOptionsPanel::changed);
    connect(ui_showCriticalTasks, &QCheckBox::stateChanged, this, &GanttChartDisplayOptionsPanel::changed);
    connect(ui_showCompletion, &QCheckBox::stateChanged, this, &GanttChartDisplayOptionsPanel::changed);
    connect(ui_showSchedulingError, &QCheckBox::stateChanged, this, &GanttChartDisplayOptionsPanel::changed);
    connect(ui_showTimeConstraint, &QCheckBox::stateChanged, this, &GanttChartDisplayOptionsPanel::changed);
    connect(ui_showRowSeparators, &QCheckBox::stateChanged, this, &GanttChartDisplayOptionsPanel::changed);
}

void GanttChartDisplayOptionsPanel::slotOk()
{
    m_delegate->showTaskName = ui_showTaskName->checkState() == Qt::Checked;
    m_delegate->showResources = ui_showResourceNames->checkState() == Qt::Checked;
    m_delegate->showTaskLinks = ui_showDependencies->checkState() == Qt::Checked;
    m_delegate->showPositiveFloat = ui_showPositiveFloat->checkState() == Qt::Checked;
    m_delegate->showNegativeFloat = ui_showNegativeFloat->checkState() == Qt::Checked;
    m_delegate->showCriticalPath = ui_showCriticalPath->checkState() == Qt::Checked;
    m_delegate->showCriticalTasks = ui_showCriticalTasks->checkState() == Qt::Checked;
    m_delegate->showProgress = ui_showCompletion->checkState() == Qt::Checked;
    m_delegate->showSchedulingError = ui_showSchedulingError->checkState() == Qt::Checked;
    m_delegate->showTimeConstraint = ui_showTimeConstraint->checkState() == Qt::Checked;

    DateTimeTimeLine *timeline = m_gantt->timeLine();

    timeline->setInterval(ui_timeLineInterval->value() * 60000);
    QPen pen;
    pen.setWidth(ui_timeLineStroke->value());
    pen.setColor(ui_timeLineColor->color());
    timeline->setPen(pen);

    DateTimeTimeLine::Options opt = timeline->options();
    opt.setFlag(DateTimeTimeLine::Foreground, ui_timeLineForeground->isChecked());
    opt.setFlag(DateTimeTimeLine::Background, ui_timeLineBackground->isChecked());
    opt.setFlag(DateTimeTimeLine::UseCustomPen, ui_timeLineUseCustom->isChecked());
    timeline->setOptions(opt);

    m_gantt->setShowRowSeparators(ui_showRowSeparators->checkState());
}

void GanttChartDisplayOptionsPanel::setValues(const GanttItemDelegate &del)
{
    ui_showTaskName->setCheckState(del.showTaskName ? Qt::Checked : Qt::Unchecked);
    ui_showResourceNames->setCheckState(del.showResources ? Qt::Checked : Qt::Unchecked);
    ui_showDependencies->setCheckState(del.showTaskLinks ? Qt::Checked : Qt::Unchecked);
    ui_showPositiveFloat->setCheckState(del.showPositiveFloat ? Qt::Checked : Qt::Unchecked);
    ui_showNegativeFloat->setCheckState(del.showNegativeFloat ? Qt::Checked : Qt::Unchecked);
    ui_showCriticalPath->setCheckState(del.showCriticalPath ? Qt::Checked : Qt::Unchecked);
    ui_showCriticalTasks->setCheckState(del.showCriticalTasks ? Qt::Checked : Qt::Unchecked);
    ui_showCompletion->setCheckState(del.showProgress ? Qt::Checked : Qt::Unchecked);
    ui_showSchedulingError->setCheckState(del.showSchedulingError ? Qt::Checked : Qt::Unchecked);
    ui_showTimeConstraint->setCheckState(del.showTimeConstraint ? Qt::Checked : Qt::Unchecked);

    DateTimeTimeLine *timeline = m_gantt->timeLine();

    ui_timeLineInterval->setValue(timeline->interval() / 60000);

    QPen pen = timeline->pen();
    ui_timeLineStroke->setValue(pen.width());
    ui_timeLineColor->setColor(pen.color());

    ui_timeLineHide->setChecked(true);
    DateTimeTimeLine::Options opt = timeline->options();
    ui_timeLineBackground->setChecked(opt & DateTimeTimeLine::Background);
    ui_timeLineForeground->setChecked(opt & DateTimeTimeLine::Foreground);
    ui_timeLineUseCustom->setChecked(opt & DateTimeTimeLine::UseCustomPen);

    ui_showRowSeparators->setCheckState(m_gantt->showRowSeparators() ? Qt::Checked : Qt::Unchecked);
}

void GanttChartDisplayOptionsPanel::setDefault()
{
    GanttItemDelegate del;
    setValues(del);
}

//----
GanttViewSettingsDialog::GanttViewSettingsDialog(GanttViewBase *gantt, GanttItemDelegate *delegate, ViewBase *view, bool selectPrint)
    : ItemViewSettupDialog(view, gantt->treeView(), true, view),
    m_gantt(gantt)
{
    GanttChartDisplayOptionsPanel *panel = new GanttChartDisplayOptionsPanel(gantt, delegate);
    /*KPageWidgetItem *page = */insertWidget(1, panel, i18n("Chart"), i18n("Gantt Chart Settings"));
    QTabWidget *tab = new QTabWidget();
    QWidget *w = ViewBase::createPageLayoutWidget(view);
    tab->addTab(w, w->windowTitle());
    m_pagelayout = w->findChild<KoPageLayoutWidget*>();
    Q_ASSERT(m_pagelayout);

    m_printingoptions = new GanttPrintingOptionsWidget(this);
    m_printingoptions->setOptions(gantt->printingOptions());
    tab->addTab(m_printingoptions, m_printingoptions->windowTitle());
    KPageWidgetItem *page = insertWidget(2, tab, i18n("Printing"), i18n("Printing Options"));
    if (selectPrint) {
        setCurrentPage(page);
    }
    connect(this, SIGNAL(accepted()), this, SLOT(slotOk()));
    connect(this, &QDialog::accepted, panel, &GanttChartDisplayOptionsPanel::slotOk);
    connect(button(QDialogButtonBox::RestoreDefaults), &QAbstractButton::clicked, panel, &GanttChartDisplayOptionsPanel::setDefault);
}

void GanttViewSettingsDialog::slotOk()
{
    debugPlan;
    m_gantt->setPrintingOptions(m_printingoptions->options());
    ItemViewSettupDialog::slotOk();
}

//-------------------------
GanttPrintingOptions::GanttPrintingOptions()
    : printRowLabels(true),
    singlePage(true)
{
}

bool GanttPrintingOptions::loadContext(const KoXmlElement &settings)
{
    KoXmlElement e = settings.namedItem("print-options").toElement();
    if (! e.isNull()) {
        printRowLabels = (bool)(e.attribute("print-rowlabels", "0").toInt());
        singlePage = (bool)(e.attribute("print-singlepage", "0").toInt());
    }
    debugPlan <<"..........."<<printRowLabels<<singlePage;
    return true;
}

void GanttPrintingOptions::saveContext(QDomElement &settings) const
{
    QDomElement e = settings.ownerDocument().createElement("print-options");
    settings.appendChild(e);
    e.setAttribute("print-rowlabels", QString::number(printRowLabels));
    e.setAttribute("print-singlepage", QString::number(singlePage));
}

GanttPrintingOptionsWidget::GanttPrintingOptionsWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi(this);
    setWindowTitle(xi18nc("@title:tab", "Chart"));
}

GanttPrintingOptions GanttPrintingOptionsWidget::options() const
{
    GanttPrintingOptions opt;
    opt.printRowLabels = printRowLabels();
    opt.singlePage = singlePage();
    return opt;
}

void GanttPrintingOptionsWidget::setOptions(const GanttPrintingOptions &opt)
{
    setPrintRowLabels(opt.printRowLabels);
    setSinglePage(opt.singlePage);
}


//----------------
GanttPrintingDialog::GanttPrintingDialog(ViewBase *view, GanttViewBase *gantt)
    : PrintingDialog(view),
    m_gantt(gantt),
    m_options(nullptr)
{
    m_headerHeight = gantt->treeView()->header()->height(); // same header hight
    m_sceneRect = gantt->graphicsView()->sceneRect();
    m_horPages = 1;
    qreal c = m_sceneRect.width() - printer().pageRect().width();
    while (c > 0) {
        ++m_horPages;
        c -= printer().pageRect().width();
    }
    m_vertPages = 1;
    c = m_sceneRect.height() - printer().pageRect().height() - m_headerHeight;
    while (c > 0) {
        ++m_vertPages;
        c -= printer().pageRect().height();
    }
    debugPlan<<m_sceneRect<<printer().pageRect()<<m_horPages<<m_vertPages;
    printer().setFromTo(documentFirstPage(), documentLastPage());
}

void GanttPrintingDialog::startPrinting(RemovePolicy removePolicy)
{
    QList<int> pages;
    if (printer().fromPage() > 0) {
        pages << printer().fromPage();
        if (! m_gantt->m_printOptions.singlePage) {
            int last = printer().toPage();
            for (int i = pages.first() + 1; i <= last; ++i) {
                pages << i;
            }
            if (m_vertPages > 1) {
                m_image = QImage(m_sceneRect.width(), m_sceneRect.height() + m_headerHeight, QImage::Format_ARGB32);
                m_image.fill(Qt::white);
                QPainter p(&m_image);
                m_gantt->print(&p, m_image.rect(), m_gantt->m_printOptions.printRowLabels, true);
            }
        }
    }
    setPageRange(pages);

    PrintingDialog::startPrinting(removePolicy);
}

QList<QWidget*> GanttPrintingDialog::createOptionWidgets() const
{
    //debugPlan;
    GanttPrintingOptionsWidget *w = new GanttPrintingOptionsWidget();
    w->setPrintRowLabels(m_gantt->m_printOptions.printRowLabels);
    connect(w->ui_printRowLabels, &QAbstractButton::toggled, this, &GanttPrintingDialog::slotPrintRowLabelsToogled);
    w->setSinglePage(m_gantt->m_printOptions.singlePage);
    connect(w->ui_singlePage, &QAbstractButton::toggled, this, &GanttPrintingDialog::slotSinglePageToogled);
    const_cast<GanttPrintingDialog*>(this)->m_options = w;

    return QList<QWidget*>() << createPageLayoutWidget() << m_options;
}

void GanttPrintingDialog::slotPrintRowLabelsToogled(bool on)
{
    m_gantt->m_printOptions.printRowLabels = on;
}

void GanttPrintingDialog::slotSinglePageToogled(bool on)
{
    m_gantt->m_printOptions.singlePage = on;
    printer().setFromTo(documentFirstPage(), documentLastPage());
}

int GanttPrintingDialog::documentLastPage() const
{
    //debugPlan<<m_gantt->m_printOptions.singlePage<<m_horPages<<m_vertPages;
    return m_gantt->m_printOptions.singlePage ? documentFirstPage() : m_horPages * m_vertPages;
}


void GanttPrintingDialog::printPage(int page, QPainter &painter)
{
    debugPlan<<"page:"<<page<<"first"<<documentFirstPage()<<"last:"<<documentLastPage()<<m_horPages<<m_vertPages;
    int p = page - documentFirstPage();
    QRectF pageRect = printer().pageRect();
    pageRect.moveTo(0, 0);
    bool singlePage = m_gantt->m_printOptions.singlePage;
    int vert = singlePage ? 0 : p / m_horPages;
    int hor = singlePage ? 0 : p % m_horPages;
//     painter.setClipRect(pageRect.adjusted(-1.0, -1.0, 1.0, 1.0));
    if (singlePage) {
        // single page: use KGantt
        m_gantt->print(&painter, m_sceneRect.left(), m_sceneRect.right(), pageRect, m_gantt->m_printOptions.printRowLabels, true);
    } else if (m_vertPages == 1) {
        // single vertical page: use KGantt
        qreal hh = vert == 0 ? m_headerHeight : 0;
        qreal ho = vert > 0 ? m_headerHeight : 0;
        QRectF sourceRect = QRectF(m_sceneRect.x() + (pageRect.width() * hor), m_sceneRect.y() + ((pageRect.height() * vert) - ho), pageRect.width(), pageRect.height() - hh);
        debugPlan<<p<<hor<<vert<<sourceRect;
        m_gantt->print(&painter, sourceRect.left(), sourceRect.right(), pageRect, hor == 0 && m_gantt->m_printOptions.printRowLabels, true);
    } else {
        // print on multiple vertical pages: use pixmap
        // QT5TODO Make KGantt able to print multiple pages vertically
        QRectF sourceRect = m_image.rect();
        qreal hh = vert == 0 ? m_headerHeight : 0;
        qreal ho = vert > 0 ? m_headerHeight : 0;
        sourceRect = QRectF(sourceRect.x() + (pageRect.width() * hor), sourceRect.y() + ((pageRect.height() * vert) - ho), pageRect.width(), pageRect.height() - hh);
        debugPlan<<p<<hor<<vert<<sourceRect;
        painter.drawImage(pageRect, m_image, sourceRect);
    }
}

//---------------------
class HeaderView : public QHeaderView
{
public:
    explicit HeaderView(QWidget* parent=nullptr) : QHeaderView(Qt::Horizontal, parent) {
    }

    QSize sizeHint() const override { QSize s = QHeaderView::sizeHint(); s.rheight() *= 2; return s; }
};

GanttTreeView::GanttTreeView(QWidget* parent)
    : TreeViewBase(parent)
{
    disconnect(header());
    setHeader(new HeaderView);

    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setTreePosition(-1); // always visual index 0

    header()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(header(), SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(slotHeaderContextMenuRequested(QPoint)));
}

//-------------------------------------------
GanttZoomWidget::GanttZoomWidget(QWidget *parent)
    : QSlider(parent), m_hide(true), m_grid(nullptr)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setGeometry(0, 0, 200, minimumSizeHint().height());
    setContextMenuPolicy(Qt::PreventContextMenu);
    setOrientation(Qt::Horizontal);
    setPageStep(5);
    setMaximum(125);
    connect(this, &QAbstractSlider::valueChanged, this, &GanttZoomWidget::sliderValueChanged);
}

void GanttZoomWidget::setEnableHideOnLeave(bool hide)
{
    m_hide = hide;
}

void GanttZoomWidget::setGrid(KGantt::DateTimeGrid *grid)
{
    m_grid = grid;
    if (grid) {
        int pos = -1; // daywidth always >= 0.1
        for (qreal dw = grid->dayWidth(); dw >= 0.1 && pos < maximum(); ++pos) {
            dw *= 1.0 / 1.1;
        }
        blockSignals(true);
        setValue(pos);
        blockSignals(false);
    }
}

void GanttZoomWidget::mousePressEvent(QMouseEvent *e)
{
    QSlider::mousePressEvent(e);
    setRepeatAction(QAbstractSlider::SliderNoAction);
    e->accept();
}

void GanttZoomWidget::mouseMoveEvent(QMouseEvent *e)
{
    QSlider::mouseMoveEvent(e);
    e->accept();
}

void GanttZoomWidget::mouseReleaseEvent(QMouseEvent *e)
{
    QSlider::mouseReleaseEvent(e);
    e->accept();
}

void GanttZoomWidget::sliderValueChanged(int value)
{
    //debugPlan<<m_grid<<value;
    if (m_grid) {
        int v = qMax(1.0, qPow(1.1, value) * 0.1);
        m_grid->setDayWidth(v);
    }
}

class MyGraphicsView : public KGantt::GraphicsView
{
public:
    MyGraphicsView(GanttViewBase *parent) : KGantt::GraphicsView(parent), m_parent(parent)
    {
        setMouseTracking(true);
    }
protected:
    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) {
            event->ignore();
            return;
        }
        KGantt::GraphicsView::mousePressEvent(event);
    }
    void mouseMoveEvent(QMouseEvent *event) override {
        if (event->buttons() & Qt::LeftButton) {
            event->ignore();
            return;
        }
        KGantt::GraphicsView::mouseMoveEvent(event);
    }
    void mouseReleaseEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) {
            event->ignore();
            return;
        }
        KGantt::GraphicsView::mouseMoveEvent(event);
    }
    void contextMenuEvent(QContextMenuEvent *event) override {
        m_parent->handleContextMenuEvent(indexAt(event->pos()), event->globalPos());
    }
private:
    GanttViewBase *m_parent;
};
//-------------------------------------------
GanttViewBase::GanttViewBase(QWidget *parent)
    : KGantt::View(parent)
    , m_mouseButton(Qt::NoButton)
{
    DateTimeGrid *g = new DateTimeGrid();
    setGrid(g);
    setGraphicsView(new MyGraphicsView(this));
    g->setUserDefinedUpperScale(new KGantt::DateTimeScaleFormatter(KGantt::DateTimeScaleFormatter::Month, QString::fromLatin1("yyyy-MMMM")));
    g->setUserDefinedLowerScale(new KGantt::DateTimeScaleFormatter(KGantt::DateTimeScaleFormatter::Day, QString::fromLatin1("ddd")));

    g->timeNow()->setInterval(5000);

    QLocale locale;

    g->setWeekStart(locale.firstDayOfWeek());

    const QList<Qt::DayOfWeek> weekdays = locale.weekdays();
    QSet<Qt::DayOfWeek> fd;
    for (int i = Qt::Monday; i <= Qt::Sunday; ++i) {
        if (!weekdays.contains(static_cast<Qt::DayOfWeek>(i))) {
            fd << static_cast<Qt::DayOfWeek>(i);
        }
    }
    g->setFreeDays(fd);

    m_zoomwidget = new GanttZoomWidget(graphicsView());
    m_zoomwidget->setGrid(g);
    m_zoomwidget->setEnableHideOnLeave(true);
    m_zoomwidget->hide();
    m_zoomwidget->move(6, 6);

    graphicsView()->installEventFilter(this);
}

GanttViewBase::~GanttViewBase()
{
    // HACK: avoid crash due to access of graphicsview scrollbar after death
    // KGantt tries to sync leftview scrollbar with graphicsview scrollbar
    // and seems sometimes graphicsview has already been deleted.
    // Note: this will be fixed in next KGantt release
    leftView()->verticalScrollBar()->disconnect();
}

void GanttViewBase::editCopy()
{
    QMimeData *mimeData = new QMimeData;
    QPixmap pixmap(size());
    render(&pixmap);
    mimeData->setImageData(pixmap);
    QGuiApplication::clipboard()->setMimeData(mimeData);
}

DateTimeTimeLine *GanttViewBase::timeLine() const
{
    DateTimeGrid *g = static_cast<DateTimeGrid*>(grid());
    return g->timeNow();
}

GanttTreeView *GanttViewBase::treeView() const
{
    GanttTreeView *tv = qobject_cast<GanttTreeView*>(const_cast<QAbstractItemView*>(leftView()));
    Q_ASSERT(tv);
    return tv;
}

bool GanttViewBase::eventFilter(QObject *obj, QEvent *event)
{
    if (obj != graphicsView()) {
        return false;
    }
    if (event->type() == QEvent::HoverMove) {
        QHoverEvent *e = static_cast<QHoverEvent*>(event);
        bool zoom = m_zoomwidget->geometry().contains(e->pos());
        if (zoom && m_zoomwidget->isVisible()) {
            return true;
        }
        if (m_mouseButton == Qt::NoButton && zoom && !m_zoomwidget->isVisible()) {
            m_zoomwidget->show();
            m_zoomwidget->raise();
            m_zoomwidget->setFocus();
        }
        if (!zoom && m_zoomwidget->isVisible()) {
            m_zoomwidget->hide();
            graphicsView()->update();
        }
    }
    return QObject::eventFilter(obj, event);
}

bool GanttViewBase::loadContext(const KoXmlElement &settings)
{
    KGantt::DateTimeGrid *g = static_cast<KGantt::DateTimeGrid*>(grid());
    g->setScale(static_cast<KGantt::DateTimeGrid::Scale>(settings.attribute("chart-scale", "0").toInt()));
    g->setDayWidth(settings.attribute("chart-daywidth", "30").toDouble());

    DateTimeTimeLine::Options opt;
    opt.setFlag(DateTimeTimeLine::Foreground, settings.attribute("timeline-foreground").toInt());
    opt.setFlag(DateTimeTimeLine::Background, settings.attribute("timeline-background").toInt());
    opt.setFlag(DateTimeTimeLine::UseCustomPen, settings.attribute("timeline-custom").toInt());
    timeLine()->setOptions(opt);

    timeLine()->setInterval(settings.attribute("timeline-interval", nullptr).toInt() * 60000);

    QPen pen;
    pen.setWidth(settings.attribute("timeline-width").toInt());
    pen.setColor(QColor(settings.attribute("timeline-color")));
    timeLine()->setPen(pen);

    setShowRowSeparators(settings.attribute("show-rowseparators", "0").toInt());

    return true;
}

void GanttViewBase::saveContext(QDomElement &settings) const
{
    KGantt::DateTimeGrid *g = static_cast<KGantt::DateTimeGrid*>(grid());
    settings.setAttribute("chart-scale", QString::number(g->scale()));
    settings.setAttribute("chart-daywidth", QString::number(g->dayWidth()));

    settings.setAttribute("timeline-foreground", timeLine()->options() & DateTimeTimeLine::Foreground);
    settings.setAttribute("timeline-background", timeLine()->options() & DateTimeTimeLine::Background);
    settings.setAttribute("timeline-interval", timeLine()->interval() / 60000);
    settings.setAttribute("timeline-custom", timeLine()->options() & DateTimeTimeLine::UseCustomPen);
    settings.setAttribute("timeline-width", timeLine()->pen().width());
    settings.setAttribute("timeline-color", timeLine()->pen().color().name());

    settings.setAttribute("show-rowseparators", showRowSeparators());
}

void GanttViewBase::mousePressEvent(QMouseEvent *event)
{
    m_mouseButton = event->button();
    if (event->button() == Qt::LeftButton) {
        m_dragStartPosition = event->pos();
    } else if (event->button() == Qt::RightButton) {
        // contextmenu is activated, so we do not get a mouseReleaseEvent
        m_mouseButton = Qt::NoButton;
    }
    event->ignore();
}

void GanttViewBase::mouseMoveEvent(QMouseEvent *event)
{
    if (!(event->buttons() & Qt::LeftButton)) {
        event->ignore();
        return;
    }
    if ((event->pos() - m_dragStartPosition).manhattanLength() < QApplication::startDragDistance()) {
        event->ignore();
        return;
    }
    QDrag *drag = new QDrag(this);
    QMimeData *mimeData = new QMimeData;
    QPixmap pixmap(size());
    render(&pixmap);
    mimeData->setImageData(pixmap);
    drag->setMimeData(mimeData);
    drag->exec(Qt::CopyAction);
    m_mouseButton = Qt::NoButton;
}

void GanttViewBase::mouseReleaseEvent(QMouseEvent *event)
{
    m_mouseButton = Qt::NoButton;
    event->ignore();
}

void GanttViewBase::handleContextMenuEvent(const QModelIndex &idx, const QPoint &globalPos)
{
    emit contextMenuRequested(idx, globalPos);
}

void GanttViewBase::setShowRowSeparators(bool enable)
{
    KGantt::DateTimeGrid *g = qobject_cast<KGantt::DateTimeGrid*>(grid());
    Q_ASSERT(g);
    g->setRowSeparators(enable);
}

bool GanttViewBase::showRowSeparators() const
{
    KGantt::DateTimeGrid *g = qobject_cast<KGantt::DateTimeGrid*>(grid());
    Q_ASSERT(g);
    return g->rowSeparators();
}


}  //KPlato namespace
