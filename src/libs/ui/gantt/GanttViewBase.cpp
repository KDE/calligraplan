/* This file is part of the KDE project
 *  SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
 *  SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 *  SPDX-FileCopyrightText: 2005 Dag Andersen <dag.andersen@kdemail.net>
 *  SPDX-FileCopyrightText: 2006 Raphael Langerhorst <raphael.langerhorst@kdemail.net>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "GanttViewBase.h"
#include "kptnodeitemmodel.h"
#include "DateTimeTimeLine.h"
#include "kptganttitemdelegate.h"
#include "DateTimeGrid.h"
#include <kptproject.h>

#include <KoXmlReader.h>
#include <KoPageLayoutWidget.h>
#include <KoIcon.h>

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
#include <QGraphicsTextItem>
#include <QLineEdit>
#include <QPaintEvent>

/// The main namespace
namespace KPlato
{

class GanttItemDelegate;

// Define a printer friendly palette
#define VeryLightGray   "#f8f8f8"
#define LightLightGray  "#f0f0f0"
#define DarkDarkGray    "#b3b3b3"
#define VeryDarkGray    "#838383"
class PrintPalette {
public:
    PrintPalette() {
        orig = QApplication::palette();
        QPalette palette = orig;
        // define a palette that works when printing on white paper
        palette.setColor(QPalette::Window, Qt::white);
        palette.setColor(QPalette::WindowText, Qt::black);
        palette.setColor(QPalette::Base, Qt::white);
        palette.setColor(QPalette::AlternateBase, VeryLightGray); // clazy:exclude=qcolor-from-literal
        palette.setColor(QPalette::ToolTipBase, Qt::white);
        palette.setColor(QPalette::ToolTipText, Qt::black);
        palette.setColor(QPalette::Text, Qt::black);
        palette.setColor(QPalette::Button, Qt::lightGray);
        palette.setColor(QPalette::ButtonText, Qt::black);
        palette.setColor(QPalette::BrightText, Qt::white);
        palette.setColor(QPalette::Link, Qt::blue);
        palette.setColor(QPalette::Highlight, Qt::blue);
        palette.setColor(QPalette::HighlightedText, Qt::white);
        palette.setColor(QPalette::Light, QColor(VeryLightGray)); // clazy:exclude=qcolor-from-literal
        palette.setColor(QPalette::Midlight, QColor(LightLightGray)); // clazy:exclude=qcolor-from-literal
        palette.setColor(QPalette::Dark, QColor(DarkDarkGray)); // clazy:exclude=qcolor-from-literal
        palette.setColor(QPalette::Mid, QColor(VeryDarkGray)); // clazy:exclude=qcolor-from-literal
        palette.setColor(QPalette::Shadow, Qt::black);
        QApplication::setPalette(palette);
    }
    ~PrintPalette() {
        QApplication::setPalette(orig);
    }
    QPalette orig;
};


//-------------------------
GanttPrintingOptions::GanttPrintingOptions()
    : useStartTime(false)
    , useEndTime(false)
{
}

GanttPrintingOptions::GanttPrintingOptions(const GanttPrintingOptions &other)
{
    operator=(other);
}

GanttPrintingOptions &GanttPrintingOptions::operator=(const GanttPrintingOptions &other)
{
    context = other.context;
    useStartTime = other.useStartTime;
    diagramStart = other.diagramStart;
    useEndTime = other.useEndTime;
    diagramEnd = other.diagramEnd;
    return *this;
}

bool GanttPrintingOptions::loadContext(const KoXmlElement &settings)
{
    KoXmlElement e = settings.namedItem("print-options").toElement();
    if (! e.isNull()) {
        context.setDrawRowLabels((bool)(e.attribute(QStringLiteral("print-rowlabels"), QString::number(1)).toInt()));
        context.setDrawColumnLabels((bool)(e.attribute(QStringLiteral("print-columnlabels"), QString::number(1)).toInt()));
        if ((e.attribute(QStringLiteral("print-singlepage"), QString::number(0)).toInt())) {
            context.setFitting(KGantt::PrintingContext::FitSinglePage);
        } else if ((bool)(e.attribute(QStringLiteral("print-pageheight"), QString::number(0)).toInt())) {
            context.setFitting(KGantt::PrintingContext::FitPageHeight);
        } else if ((bool)(e.attribute(QStringLiteral("print-nofitting"), QString::number(1)).toInt())) {
            context.setFitting(KGantt::PrintingContext::NoFitting);
        }
        useStartTime = (bool)(e.attribute(QStringLiteral("print-use-starttime"), QString::number(0)).toInt());
        if (e.hasAttribute(QStringLiteral("print-starttime"))) {
            diagramStart = QDateTime::fromString(e.attribute(QStringLiteral("print-starttime")), Qt::ISODate);
        }
        useEndTime = (bool)(e.attribute(QStringLiteral("print-use-endtime"), QString::number(0)).toInt());
        if (e.hasAttribute(QStringLiteral("print-endtime"))) {
            diagramEnd = QDateTime::fromString(e.attribute(QStringLiteral("print-endtime")), Qt::ISODate);
        }
    }
    return true;
}

void GanttPrintingOptions::saveContext(QDomElement &settings) const
{
    QDomElement e = settings.ownerDocument().createElement(QStringLiteral("print-options"));
    settings.appendChild(e);
    e.setAttribute(QStringLiteral("print-rowlabels"), QString::number(context.drawRowLabels()));
    e.setAttribute(QStringLiteral("print-columnlabels"), QString::number(context.drawColumnLabels()));
    if (context.fitting() & KGantt::PrintingContext::FitSinglePage) {
        e.setAttribute(QStringLiteral("print-singlepage"), QString::number(1));
    } else if (context.fitting() & KGantt::PrintingContext::FitPageHeight) {
        e.setAttribute(QStringLiteral("print-pageheight"), QString::number(1));
    } else {
        e.setAttribute(QStringLiteral("print-nofitting"), QString::number(1));
    }
    e.setAttribute(QStringLiteral("print-use-starttime"), QString::number(useStartTime));
    e.setAttribute(QStringLiteral("print-starttime"), diagramStart.toString(Qt::ISODate));
    e.setAttribute(QStringLiteral("print-use-endtime"), QString::number(useEndTime));
    e.setAttribute(QStringLiteral("print-endtime"), diagramEnd.toString(Qt::ISODate));
}



GanttPrintingOptionsWidget::GanttPrintingOptionsWidget(GanttViewBase *gantt, QWidget *parent)
    : QWidget(parent)
{
    setupUi(this);
    setWindowTitle(xi18nc("@title:tab", "Chart"));

    QRectF rect = gantt->graphicsView()->sceneRect();
    QDateTime start = qobject_cast<KGantt::DateTimeGrid*>(gantt->grid())->mapToDateTime(rect.left());
    QDateTime end = qobject_cast<KGantt::DateTimeGrid*>(gantt->grid())->mapToDateTime(rect.right());
    ui_startTime->setDateTime(start);
    ui_chartStartTime->setDateTime(start);
    ui_chartStartTime->setEnabled(false);
    ui_endTime->setDateTime(end);
    ui_chartEndTime->setDateTime(end);
    ui_chartEndTime->setEnabled(false);
    setOptions(gantt->printingOptions());
}

GanttPrintingOptions GanttPrintingOptionsWidget::options() const
{
    GanttPrintingOptions opt;
    opt.context.setDrawRowLabels(printRowLabels());
    opt.context.setDrawColumnLabels(printColumnLabels());
    opt.context.setFitting(fitting());
    opt.useStartTime = ui_startTimeBtn->isChecked();
    opt.diagramStart = ui_startTime->dateTime();
    opt.useEndTime = ui_endTimeBtn->isChecked();
    opt.diagramEnd = ui_endTime->dateTime();
    return opt;
}

void GanttPrintingOptionsWidget::setOptions(const GanttPrintingOptions &opt)
{
    setPrintRowLabels(opt.context.drawRowLabels());
    setPrintColumnLabels(opt.context.drawColumnLabels());
    setFitting(opt.context.fitting());
    ui_projectStartBtn->setChecked(!opt.useStartTime);
    ui_startTimeBtn->setChecked(opt.useStartTime);
    ui_startTime->setDateTime(opt.diagramStart);
    ui_projectEndBtn->setChecked(!opt.useEndTime);
    ui_endTimeBtn->setChecked(opt.useEndTime);
    ui_endTime->setDateTime(opt.diagramEnd);
}


//----------------
GanttPrintingDialog::GanttPrintingDialog(ViewBase *view, GanttViewBase *gantt)
    : PrintingDialog(view, QPrinter::HighResolution),
    m_gantt(gantt),
    m_options(nullptr)
{
    const auto resolution = printer().resolution();
    const auto pageRect = printer().pageLayout().paintRectPixels(resolution);
    m_headerHeight = gantt->treeView()->header()->height(); // same header height
    m_sceneRect = gantt->graphicsView()->sceneRect();
    const GanttPrintingOptions opt = m_gantt->m_printOptions;
    if (opt.useStartTime) {
        qreal x = qobject_cast<KGantt::DateTimeGrid*>(m_gantt->grid())->mapFromDateTime(opt.diagramStart);
        m_sceneRect.setX(x);
    }
    if (opt.useEndTime) {
        qreal x = qobject_cast<KGantt::DateTimeGrid*>(m_gantt->grid())->mapFromDateTime(opt.diagramEnd);
        m_sceneRect.setRight(x);
    }
    m_horPages = 1;
    qreal c = m_sceneRect.width() - pageRect.width();
    while (c > 0) {
        ++m_horPages;
        c -= pageRect.width();
    }
    m_vertPages = 1;
    c = m_sceneRect.height() - pageRect.height() - m_headerHeight;
    while (c > 0) {
        ++m_vertPages;
        c -= pageRect.height();
    }
    debugPlan<<m_sceneRect<<pageRect<<m_horPages<<m_vertPages;
    printer().setFromTo(documentFirstPage(), lastPageNumber());
}

QRectF GanttPrintingDialog::calcSceneRect(const QDateTime &startDateTime, const QDateTime &endDateTime) const
{
    //qInfo()<<Q_FUNC_INFO<<startDateTime<<endDateTime;
    QRectF rect = m_gantt->graphicsView()->sceneRect();
    if (!startDateTime.isValid() && !endDateTime.isValid()) {
        return rect;
    }
    const KGantt::DateTimeGrid *grid = qobject_cast<KGantt::DateTimeGrid*>(m_gantt->grid());
    Q_ASSERT(grid);

    QDateTime startTime = startDateTime.isValid() ? startDateTime : grid->mapToDateTime(rect.left());
    rect.setLeft(std::max(rect.left(), grid->mapFromDateTime(startTime)));

    QDateTime endTime = endDateTime.isValid() ? endDateTime : grid->mapToDateTime(rect.right());
    rect.setRight(std::min(rect.right(), grid->mapFromDateTime(endTime)));

    QTreeView *tv = qobject_cast<QTreeView*>(m_gantt->leftView());
    Q_ASSERT(tv);
    QModelIndexList items;
    for (QModelIndex idx = tv->indexAt(QPoint(0, 0)); idx.isValid(); idx = tv->indexBelow(idx)) {
        items << idx;
    }
    const int startRole = static_cast<KGantt::ProxyModel*>(m_gantt->ganttProxyModel())->role(KGantt::StartTimeRole);
    const int endRole = static_cast<KGantt::ProxyModel*>(m_gantt->ganttProxyModel())->role(KGantt::EndTimeRole);
    const int startCol = static_cast<KGantt::ProxyModel*>(m_gantt->ganttProxyModel())->column(KGantt::StartTimeRole);
    const int endCol = static_cast<KGantt::ProxyModel*>(m_gantt->ganttProxyModel())->column(KGantt::EndTimeRole);

    // Find the first item that end after rect.left AND start before rect.right
    // Remove all items that is completely outside rect
    bool startFound = false;
    QModelIndexList lst = items;
    for (const QModelIndex &idx : std::as_const(lst)) {
        const QDateTime sdt = idx.sibling(idx.row(), startCol).data(startRole).toDateTime();
        const QDateTime edt = idx.sibling(idx.row(), endCol).data(endRole).toDateTime();
        //qInfo()<<Q_FUNC_INFO<<idx<<"start:"<<startTime<<idx.data().toString()<<idx.sibling(idx.row(), endCol).data(endRole);
        if (edt <= startTime) {
            items.removeAll(idx);
            continue;
        }
        if (!startFound && sdt >= endTime) {
            items.removeAll(idx);
            continue;
        }
        if (!startFound) {
            rect.setTop(tv->visualRect(idx).top());
            startFound = true;
            //qInfo()<<Q_FUNC_INFO<<idx<<"start first:"<<idx.data().toString()<<rect.top();
        }
    }
    // All items ending before rect.left has been removed,
    // so now find the first item (from bottom) that start before rect.right
    for (int i = items.count() - 1; i >= 0; --i) {
        const QModelIndex idx = items.at(i);
        //qInfo()<<Q_FUNC_INFO<<idx<<"end:"<<endTime<<idx.data().toString()<<idx.sibling(idx.row(), startCol).data(startRole);
        const QDateTime sdt = idx.sibling(idx.row(), startCol).data(startRole).toDateTime();
        if (!sdt.isValid() || endTime < sdt) {
            continue;
        }
        rect.setBottom(tv->visualRect(idx).bottom());
        //qInfo()<<Q_FUNC_INFO<<idx<<"end last:"<<idx.data().toString()<<rect.bottom();
        break;
    }
    return rect;
}

qreal GanttPrintingDialog::rowLabelsWidth(const QPaintDevice *device) const
{
    qreal labelsWidth = 0.0;
    QFont sceneFont(m_gantt->graphicsView()->font(), device);
    QGraphicsTextItem dummyTextItem( QStringLiteral("X") );
    dummyTextItem.adjustSize();
    QFontMetricsF fm(dummyTextItem.font());
    sceneFont.setPixelSize( fm.height() );

    /* row labels */
    qreal charWidth = QFontMetricsF(sceneFont).boundingRect(QString::fromLatin1("X")).width();
    const auto *summaryHandlingModel = m_gantt->graphicsView()->summaryHandlingModel();
    const auto *rowController = m_gantt->graphicsView()->rowController();
    QModelIndex sidx = summaryHandlingModel->mapToSource(summaryHandlingModel->index(0, 0));
    do {
        QModelIndex idx = summaryHandlingModel->mapFromSource(sidx);
        //const KGantt::Span rg=rowController->rowGeometry(sidx);
        const QString txt = idx.data(Qt::DisplayRole).toString();
        qreal textWidth = QFontMetricsF(sceneFont).boundingRect(txt).width() + charWidth;
        labelsWidth = std::max(labelsWidth, textWidth);
    } while ((sidx = rowController->indexBelow(sidx)).isValid());

    return labelsWidth;
}

void GanttPrintingDialog::startPrinting(RemovePolicy removePolicy)
{
    Q_UNUSED(removePolicy)

    if (printer().fromPage() <= 0) {
        return;
    }
    KGantt::PrintingContext ctx = m_gantt->m_printOptions.context;
    //qInfo()<<Q_FUNC_INFO<<m_gantt->m_printOptions.context<<':'<<ctx;
    QDateTime start, end;
    if (m_gantt->m_printOptions.useStartTime) {
        start = m_gantt->m_printOptions.diagramStart;
    }
    if (m_gantt->m_printOptions.useEndTime) {
        end = m_gantt->m_printOptions.diagramEnd;
    }
    ctx.setSceneRect(calcSceneRect(start, end));
    //qInfo()<<Q_FUNC_INFO<<m_gantt->m_printOptions.context<<':'<<ctx;
    printer().setFullPage(true);
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    PrintPalette p;
    m_gantt->printDiagram(&printer(), ctx);
    QApplication::restoreOverrideCursor();
}

QList<QWidget*> GanttPrintingDialog::createOptionWidgets() const
{
    //debugPlan;
    GanttPrintingOptionsWidget *w = new GanttPrintingOptionsWidget(m_gantt);
    const_cast<GanttPrintingDialog*>(this)->m_options = w;

    return QList<QWidget*>() << createPageLayoutWidget() << m_options;
}

void GanttPrintingDialog::slotPrintRowLabelsToogled(bool on)
{
    m_gantt->m_printOptions.context.setDrawRowLabels(on);
}

void GanttPrintingDialog::slotSinglePageToogled(bool on)
{
    Q_UNUSED(on)
    //     m_gantt->m_printOptions.singlePage = on;
    //     printer().setFromTo(documentFirstPage(), documentLastPage());
}

int GanttPrintingDialog::documentLastPage() const
{
    return lastPageNumber();
}

int GanttPrintingDialog::lastPageNumber() const
{
    //debugPlan<<m_gantt->m_printOptions.singlePage<<m_horPages<<m_vertPages;
    return (m_gantt->m_printOptions.context.fitting() & KGantt::PrintingContext::FitSinglePage) ? documentFirstPage() : m_horPages * m_vertPages;
}


void GanttPrintingDialog::printPage(int page, QPainter &painter)
{
    Q_UNUSED(page)
    Q_UNUSED(painter)
}

//---------------------
HeaderView::HeaderView(QWidget* parent)
    : QHeaderView(Qt::Horizontal, parent)
{
}

QSize HeaderView::sizeHint() const
{
    QSize s = QHeaderView::sizeHint(); s.rheight() *= 2; return s;
}

GanttTreeView::GanttTreeView(QWidget* parent)
    : TreeViewBase(parent)
{
    disconnect(header());
    auto header = new HeaderView;
    setHeader(header);

    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setTreePosition(-1); // always visual index 0

    header->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(header, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(slotHeaderContextMenuRequested(QPoint)));
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
    g->setWeekStart(QLocale().firstDayOfWeek());
}

GanttViewBase::~GanttViewBase()
{
    // HACK: avoid crash due to access of graphicsview scrollbar after death
    // KGantt tries to sync leftview scrollbar with graphicsview scrollbar
    // and seems sometimes graphicsview has already been deleted.
    // Note: this will be fixed in next KGantt release
    leftView()->verticalScrollBar()->disconnect();
}

void GanttViewBase::setProject(Project *project)
{
    if (m_project) {
        disconnect(m_project, &KPlato::Project::freedaysCalendarChanged, this, nullptr);
    }
    m_project = project;
    if (m_project) {
        connect(m_project, &KPlato::Project::freedaysCalendarChanged, this, [this](Calendar *calendar) {
            if (m_freedaysType == 1) {
                setCalendar(1, calendar);
            }
        });
    }
}

void GanttViewBase::setCalendar(int type, Calendar *calendar)
{
    m_freedaysType = std::min(type, 2);
    switch (m_freedaysType) {
        case 0: // no freedays
            m_freedaysCalendar = nullptr;
            break;
        case 1: // project freedays
            m_freedaysCalendar = m_project->freedaysCalendar();
            break;
        default:
            m_freedaysCalendar = calendar;
            break;
    }
    auto g = qobject_cast<DateTimeGrid*>(grid());
    if (g) {
        g->setCalendar(m_freedaysCalendar);
    }
}

Calendar *GanttViewBase::calendar() const
{
    Calendar *c = nullptr;
    auto g = qobject_cast<DateTimeGrid*>(grid());
    if (g) {
        c = g->calendar();
    }
    return c;
}

int GanttViewBase::freedaysType() const
{
    return m_freedaysType;
}

DateTimeGrid *GanttViewBase::dateTimeGrid() const
{
    return qobject_cast<DateTimeGrid*>(grid());
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

bool GanttViewBase::loadContext(const KoXmlElement &settings)
{
    KGantt::DateTimeGrid *g = static_cast<KGantt::DateTimeGrid*>(grid());
    g->setScale(static_cast<KGantt::DateTimeGrid::Scale>(settings.attribute(QStringLiteral("chart-scale"), QString::number(0)).toInt()));
    g->setDayWidth(settings.attribute(QStringLiteral("chart-daywidth"), QString::number(30)).toDouble());

    if (m_project) {
        const auto freedays = settings.namedItem(QStringLiteral("freedays")).toElement();
        if (freedays.isNull()) {
            // if loading an old project, set to project freedays
            setCalendar(1, nullptr);
        } else {
            setCalendar(freedays.attribute(QStringLiteral("type")).toInt(), m_project->findCalendar(freedays.attribute(QStringLiteral("calendar-id"))));
        }
    }
    DateTimeTimeLine::Options opt;
    opt.setFlag(DateTimeTimeLine::Foreground, settings.attribute(QStringLiteral("timeline-foreground")).toInt());
    opt.setFlag(DateTimeTimeLine::Background, settings.attribute(QStringLiteral("timeline-background")).toInt());
    opt.setFlag(DateTimeTimeLine::UseCustomPen, settings.attribute(QStringLiteral("timeline-custom")).toInt());
    timeLine()->setOptions(opt);

    timeLine()->setInterval(settings.attribute(QStringLiteral("timeline-interval")).toInt() * 60000);

    QPen pen;
    pen.setWidth(settings.attribute(QStringLiteral("timeline-width")).toInt());
    pen.setColor(QColor(settings.attribute(QStringLiteral("timeline-color"))));
    timeLine()->setPen(pen);

    setShowRowSeparators(settings.attribute(QStringLiteral("show-rowseparators"), QString::number(0)).toInt());

    return true;
}

void GanttViewBase::saveContext(QDomElement &settings) const
{
    KGantt::DateTimeGrid *g = static_cast<KGantt::DateTimeGrid*>(grid());
    settings.setAttribute(QStringLiteral("chart-scale"), QString::number(g->scale()));
    settings.setAttribute(QStringLiteral("chart-daywidth"), QString::number(g->dayWidth()));

    auto freedays = settings.ownerDocument().createElement(QStringLiteral("freedays"));
    settings.appendChild(freedays);
    freedays.setAttribute(QStringLiteral("type"), m_freedaysType);
    freedays.setAttribute(QStringLiteral("calendar-id"), m_freedaysCalendar ? m_freedaysCalendar->id() : QLatin1String());

    settings.setAttribute(QStringLiteral("timeline-foreground"), timeLine()->options() & DateTimeTimeLine::Foreground);
    settings.setAttribute(QStringLiteral("timeline-background"), timeLine()->options() & DateTimeTimeLine::Background);
    settings.setAttribute(QStringLiteral("timeline-interval"), timeLine()->interval() / 60000);
    settings.setAttribute(QStringLiteral("timeline-custom"), timeLine()->options() & DateTimeTimeLine::UseCustomPen);
    settings.setAttribute(QStringLiteral("timeline-width"), timeLine()->pen().width());
    settings.setAttribute(QStringLiteral("timeline-color"), timeLine()->pen().color().name());

    settings.setAttribute(QStringLiteral("show-rowseparators"), showRowSeparators());
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
    Q_EMIT contextMenuRequested(idx, globalPos);
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
