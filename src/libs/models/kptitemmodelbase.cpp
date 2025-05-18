/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2006-2007, 2012 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kptitemmodelbase.h"

#include "kptproject.h"
#include "kptschedule.h"
#include "kptdurationspinbox.h"
#include "kptnodeitemmodel.h"
#include "kptdebug.h"

#include <QApplication>
#include <QComboBox>
#include <QKeyEvent>
#include <QModelIndex>
#include <QItemSelection>
#include <QStyleOptionViewItem>
#include <QTimeEdit>
#include <QPainter>
#include <QToolTip>
#include <QTreeView>
#include <QStylePainter>
#include <QMimeData>
#include <QTextDocument>
#include <QTextCursor>
#include <QTextTableFormat>
#include <QVector>
#include <QTextLength>
#include <QTextTable>

#include <QBuffer>
#include <KoStore.h>
#include <KoXmlWriter.h>
#include <KoOdfWriteStore.h>

#include <KComboBox>
#include <KLineEdit>
#include <KLocalizedString>

using namespace KPlato;

//--------------------------------------
bool ItemDelegate::eventFilter(QObject *object, QEvent *event)
{
    QWidget *editor = ::qobject_cast<QWidget*>(object);
    if (!editor) {
        return false;
    }
    m_lastHint = Delegate::NoHint;
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *e = static_cast<QKeyEvent *>(event);
        if (e->modifiers() & Qt::AltModifier && e->modifiers() & Qt::ControlModifier) {
            switch (e->key()) {
                case Qt::Key_Left:
                    m_lastHint = Delegate::EditLeftItem;
                    Q_EMIT commitData(editor);
                    Q_EMIT closeEditor(editor, QAbstractItemDelegate::NoHint);
                    return true;
                case Qt::Key_Right:
                    m_lastHint = Delegate::EditRightItem;
                    Q_EMIT commitData(editor);
                    Q_EMIT closeEditor(editor, QAbstractItemDelegate::NoHint);
                    return true;
                case Qt::Key_Down:
                    m_lastHint = Delegate::EditDownItem;
                    Q_EMIT commitData(editor);
                    Q_EMIT closeEditor(editor, QAbstractItemDelegate::NoHint);
                    return true;
                case Qt::Key_Up:
                    m_lastHint = Delegate::EditUpItem;
                    Q_EMIT commitData(editor);
                    Q_EMIT closeEditor(editor, QAbstractItemDelegate::NoHint);
                    return true;
                default:
                    break;
            }
        }
    }
    return QStyledItemDelegate::eventFilter(object, event);
}

QSize ItemDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    // Decoration (like baseline icon) increases height so check height of column 0 in case...
    QSize s = QStyledItemDelegate::sizeHint(option, index);
    return QSize(s.width(), qMax(s.height(), QStyledItemDelegate::sizeHint(option, index.sibling(index.row(), 0)).height()));
}

//----------------------
CheckStateItemDelegate::CheckStateItemDelegate(QObject *parent)
    : ItemDelegate(parent)
{
}

bool CheckStateItemDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    Q_ASSERT(event);
    Q_ASSERT(model);
    debugPlan;

    Qt::ItemFlags flags = model->flags(index);
    if (! (option.state & QStyle::State_Enabled) || ! (flags & Qt::ItemIsEnabled)) {
        return false;
    }

    // make sure that we have a check state
    QVariant value = index.data(Qt::EditRole);
    if (! value.isValid()) {
        return false;
    }

    QStyle *style = QApplication::style();

    // make sure that we have the right event type
    if ((event->type() == QEvent::MouseButtonRelease) || (event->type() == QEvent::MouseButtonDblClick) || (event->type() == QEvent::MouseButtonPress)) {
        QStyleOptionViewItem viewOpt(option);
        initStyleOption(&viewOpt, index);
        QRect checkRect = style->subElementRect(QStyle::SE_ItemViewItemDecoration, &viewOpt, nullptr);
        QMouseEvent *me = static_cast<QMouseEvent*>(event);
        if (me->button() != Qt::LeftButton || ! checkRect.contains(me->pos())) {
            return false;
        }
        if ((event->type() == QEvent::MouseButtonPress) || (event->type() == QEvent::MouseButtonDblClick)) {
            return true;
        }
    } else if (event->type() == QEvent::KeyPress) {
        if (static_cast<QKeyEvent*>(event)->key() != Qt::Key_Space && static_cast<QKeyEvent*>(event)->key() != Qt::Key_Select) {
            return false;
        }
    } else {
        return false;
    }
    Qt::CheckState state = (static_cast<Qt::CheckState>(value.toInt()) == Qt::Checked ? Qt::Unchecked : Qt::Checked);
    return model->setData(index, state, Qt::CheckStateRole);
}


//----------------------
DateTimeCalendarDelegate::DateTimeCalendarDelegate(QObject *parent)
    : ItemDelegate(parent)
{
}

QWidget *DateTimeCalendarDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/* option */, const QModelIndex &/* index */) const
{
    QDateTimeEdit *editor = new QDateTimeEdit(parent);
    editor->setCalendarPopup(true);
    editor->installEventFilter(const_cast<DateTimeCalendarDelegate*>(this));
    return editor;
}

void DateTimeCalendarDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QDateTime value = index.model()->data(index, Qt::EditRole).toDateTime();

    QDateTimeEdit *e = static_cast<QDateTimeEdit*>(editor);
    e->setDateTime(value);
}

void DateTimeCalendarDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                const QModelIndex &index) const
{
    QDateTimeEdit *e = static_cast<QDateTimeEdit*>(editor);
    model->setData(index, e->dateTime(), Qt::EditRole);
}

void DateTimeCalendarDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
    debugPlan<<editor<<":"<<option.rect<<","<<editor->sizeHint();
    QRect r = option.rect;
    //r.setHeight(r.height() 50);
    editor->setGeometry(r);
}


//-----------------------------
ProgressBarDelegate::ProgressBarDelegate(QObject *parent)
 : ItemDelegate(parent)
{
}

ProgressBarDelegate::~ProgressBarDelegate()
{
}

void ProgressBarDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
 const QModelIndex &index) const
{
    auto style = option.widget ? option.widget->style() : QApplication::style();

    // draw background
    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, painter);

    bool ok;
    index.data().toInt(&ok);
    if (ok) {
        // only draw progress bar if a valid int is returned and
        const auto idx = index.siblingAtColumn(NodeModel::NodeStatus);
        int state = idx.data(Qt::EditRole).toInt();
        if (state & Node::State_NotReadyToStart || state & Node::State_NotScheduled) {
            QStyleOptionViewItem opt = option;
            initStyleOption(&opt, index);
            opt.text = idx.data().toString();
            style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);
        } else {
            QStyleOptionProgressBar pbOption;
            pbOption.QStyleOption::operator=(option);
            initStyleOptionProgressBar(&pbOption, index);
            style->drawControl(QStyle::CE_ProgressBar, &pbOption, painter);
        }
    } else {
        // draw the returned text (if any)
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);
        opt.text = index.data().toString();
        style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);
    }
}

QSize ProgressBarDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyle *style = option.widget ? option.widget->style() : QApplication::style();

    QStyleOptionProgressBar pbOption;
    pbOption.QStyleOption::operator=(option);
    initStyleOptionProgressBar(&pbOption, index);

    return style->sizeFromContents(QStyle::CT_ProgressBar, &pbOption, QSize(), option.widget);
}

void ProgressBarDelegate::initStyleOptionProgressBar(QStyleOptionProgressBar *option, const QModelIndex &index) const
{
    option->rect.adjust(0, 1, 0, -1);
    option->minimum = 0;
    int max = index.data(Role::Maximum).toInt();
    option->maximum = max > option->minimum ? max : option->minimum + 100;
    option->progress = index.data().toInt();
    option->text = i18nc("task progress", "%1%", (option->progress * 100) / (option->maximum - option->minimum));
    option->textAlignment = Qt::AlignCenter; // TODO find out why this does not work
    option->textVisible = true;
    option->state.setFlag(QStyle::State_Horizontal);
}

QWidget *ProgressBarDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &) const
{
    Slider *slider = new Slider(parent);
    slider->setAutoFillBackground(true);
    slider->setBackgroundRole(QPalette::Highlight);
    slider->setRange(0, 100);
    slider->setOrientation(Qt::Horizontal);
    //debugPlan<<slider->minimumSizeHint()<<slider->minimumSize();
    return slider;
}

void ProgressBarDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QSlider *slider = static_cast<QSlider *>(editor);
    slider->setValue(index.data(Qt::EditRole).toInt());
}

void ProgressBarDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QSlider *slider = static_cast<QSlider *>(editor);
    model->setData(index, slider->value());
}

void ProgressBarDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &) const
{
    editor->setGeometry(option.rect);
    //debugPlan<<editor->minimumSizeHint()<<editor->minimumSize()<<editor->geometry()<<editor->size();
}

Slider::Slider(QWidget *parent)
  : QSlider(parent)
{
    connect(this, &QAbstractSlider::valueChanged, this, &Slider::updateTip);
}

void Slider::updateTip(int value)
{
    QPoint p;
    p.setY(height() / 2);
    p.setX(style()->sliderPositionFromValue (minimum(), maximum(), value, width()));

    QString text = QString::number(value) + QLatin1Char('%');
    QToolTip::showText(mapToGlobal(p), text, this);
}

//--------------------------------------
// Hmmm, a bit hacky, but this makes it possible to use index specific editors...
SelectorDelegate::SelectorDelegate(QObject *parent)
    : ItemDelegate(parent)
{
}

QWidget *SelectorDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/* option */, const QModelIndex & index) const
{
    switch (index.model()->data(index, Role::EditorType).toInt()) {
        case Delegate::EnumEditor: {
            QComboBox *editor = new KComboBox(parent);
            editor->installEventFilter(const_cast<SelectorDelegate*>(this));
            return editor;
        }
        case Delegate::TimeEditor: {
            QTimeEdit *editor = new QTimeEdit(parent);
            editor->installEventFilter(const_cast<SelectorDelegate*>(this));
            return editor;
        }
    }
    return nullptr; // FIXME: What to do?
}

void SelectorDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    switch (index.model()->data(index, Role::EditorType).toInt()) {
        case Delegate::EnumEditor: {
            QStringList lst = index.model()->data(index, Role::EnumList).toStringList();
            int value = index.model()->data(index, Role::EnumListValue).toInt();
            QComboBox *box = static_cast<QComboBox*>(editor);
            box->addItems(lst);
            box->setCurrentIndex(value);
            return;
        }
        case Delegate::TimeEditor:
            QTime value = index.model()->data(index, Qt::EditRole).toTime();
            QTimeEdit *e = static_cast<QTimeEdit*>(editor);
            e->setMinimumTime(index.model()->data(index, Role::Minimum).toTime());
            e->setMaximumTime(index.model()->data(index, Role::Maximum).toTime());
            e->setTime(value);
            return;
    }
}

void SelectorDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                const QModelIndex &index) const
{
    switch (index.model()->data(index, Role::EditorType).toInt()) {
        case Delegate::EnumEditor: {
            QComboBox *box = static_cast<QComboBox*>(editor);
            int value = box->currentIndex();
            model->setData(index, value, Qt::EditRole);
            return;
        }
        case Delegate::TimeEditor: {
            QTimeEdit *e = static_cast<QTimeEdit*>(editor);
            model->setData(index, e->time(), Qt::EditRole);
            return;
        }
    }
}

void SelectorDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
    QRect r = option.rect;
    editor->setGeometry(r);
}

EnumDelegate::EnumDelegate(QObject *parent)
    : ItemDelegate(parent)
{
}

QWidget *EnumDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/* option */, const QModelIndex &/* index */) const
{
    QComboBox *editor = new KComboBox(parent);
    editor->installEventFilter(const_cast<EnumDelegate*>(this));
    return editor;
}

void EnumDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QStringList lst = index.model()->data(index, Role::EnumList).toStringList();
    int value = index.model()->data(index, Role::EnumListValue).toInt();

    QComboBox *box = static_cast<QComboBox*>(editor);
    box->addItems(lst);
    box->setCurrentIndex(value);
}

void EnumDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                const QModelIndex &index) const
{
    QComboBox *box = static_cast<QComboBox*>(editor);
    int value = box->currentIndex();
    model->setData(index, value, Qt::EditRole);
}

void EnumDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
    debugPlan<<editor<<":"<<option.rect<<","<<editor->sizeHint();
    QRect r = option.rect;
    //r.setHeight(r.height() 50);
    editor->setGeometry(r);
}

//-------------------------------
DurationSpinBoxDelegate::DurationSpinBoxDelegate(QObject *parent)
    : ItemDelegate(parent)
{
}

QWidget *DurationSpinBoxDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/* option */, const QModelIndex &/* index */) const
{
    DurationSpinBox *editor = new DurationSpinBox(parent);
    editor->installEventFilter(const_cast<DurationSpinBoxDelegate*>(this));
    return editor;
}

void DurationSpinBoxDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    DurationSpinBox *dsb = static_cast<DurationSpinBox*>(editor);
//    dsb->setScales(index.model()->data(index, Role::DurationScales));
    dsb->setMinimumUnit((Duration::Unit)(index.data(Role::Minimum).toInt()));
    dsb->setMaximumUnit((Duration::Unit)(index.data(Role::Maximum).toInt()));
    dsb->setUnit((Duration::Unit)(index.model()->data(index, Role::DurationUnit).toInt()));
    dsb->setValue(index.model()->data(index, Qt::EditRole).toDouble());
}

void DurationSpinBoxDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                const QModelIndex &index) const
{
    DurationSpinBox *dsb = static_cast<DurationSpinBox*>(editor);
    QVariantList lst;
    lst << QVariant(dsb->value()) << QVariant((int)(dsb->unit()));
    model->setData(index, QVariant(lst), Qt::EditRole);
}

void DurationSpinBoxDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
    debugPlan<<editor<<":"<<option.rect<<","<<editor->sizeHint();
    QRect r = option.rect;
    //r.setHeight(r.height() + 50);
    editor->setGeometry(r);
}

//---------------------------
SpinBoxDelegate::SpinBoxDelegate(QObject *parent)
    : ItemDelegate(parent)
{
}

QWidget *SpinBoxDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/* option */, const QModelIndex &/* index */) const
{
    QSpinBox *editor = new QSpinBox(parent);
    editor->installEventFilter(const_cast<SpinBoxDelegate*>(this));
    return editor;
}

void SpinBoxDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    int value = index.model()->data(index, Qt::EditRole).toInt();
    auto min = index.model()->data(index, Role::Minimum);
    auto max = index.model()->data(index, Role::Maximum);

    QSpinBox *box = static_cast<QSpinBox*>(editor);
    if (min.isValid()) {
        box->setMinimum(min.toInt());
    } else {
        box->setMinimum(std::numeric_limits<int>::min());
    }
    if (max.isValid()) {
        box->setMaximum(max.toInt());
    } else {
        box->setMaximum(std::numeric_limits<int>::max());
    }
    box->setValue(value);
}

void SpinBoxDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                const QModelIndex &index) const
{
    QSpinBox *box = static_cast<QSpinBox*>(editor);
    model->setData(index, box->value(), Qt::EditRole);
}

void SpinBoxDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
    debugPlan<<editor<<":"<<option.rect<<","<<editor->sizeHint();
    QRect r = option.rect;
    //r.setHeight(r.height() + 50);
    editor->setGeometry(r);
}

//---------------------------
DoubleSpinBoxDelegate::DoubleSpinBoxDelegate(QObject *parent)
    : ItemDelegate(parent)
{
}

QWidget *DoubleSpinBoxDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/* option */, const QModelIndex &/* index */) const
{
    QDoubleSpinBox *editor = new QDoubleSpinBox(parent);
    editor->installEventFilter(const_cast<DoubleSpinBoxDelegate*>(this));
    return editor;
}

void DoubleSpinBoxDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    double value = index.model()->data(index, Qt::EditRole).toDouble();
    double min = 0.0;//index.model()->data(index, Role::Minimum).toInt();
    double max = 24.0;//index.model()->data(index, Role::Maximum).toInt();

    QDoubleSpinBox *box = static_cast<QDoubleSpinBox*>(editor);
    box->setDecimals(1);
    box->setRange(min, max);
    box->setValue(value);
    box->selectAll();
}

void DoubleSpinBoxDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                const QModelIndex &index) const
{
    QDoubleSpinBox *box = static_cast<QDoubleSpinBox*>(editor);
    model->setData(index, box->value(), Qt::EditRole);
}

void DoubleSpinBoxDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
    QRect r = option.rect;
    editor->setGeometry(r);
}

//---------------------------
MoneyDelegate::MoneyDelegate(QObject *parent)
    : ItemDelegate(parent)
{
}

QWidget *MoneyDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/* option */, const QModelIndex &/* index */) const
{
    KLineEdit *editor = new KLineEdit(parent);
    //TODO: validator
    editor->installEventFilter(const_cast<MoneyDelegate*>(this));
    return editor;
}

void MoneyDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QString value = index.model()->data(index, Qt::EditRole).toString();
    KLineEdit *e = static_cast<KLineEdit*>(editor);
    e->setText(value);
}

void MoneyDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                const QModelIndex &index) const
{
    KLineEdit *e = static_cast<KLineEdit*>(editor);
    model->setData(index, e->text(), Qt::EditRole);
}

void MoneyDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
    QRect r = option.rect;
    editor->setGeometry(r);
}

//---------------------------
TimeDelegate::TimeDelegate(QObject *parent)
    : ItemDelegate(parent)
{
}

QWidget *TimeDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/* option */, const QModelIndex &/* index */) const
{
    QTimeEdit *editor = new QTimeEdit(parent);
    editor->installEventFilter(const_cast<TimeDelegate*>(this));
    return editor;
}

void TimeDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QTime value = index.model()->data(index, Qt::EditRole).toTime();
    QTimeEdit *e = static_cast<QTimeEdit*>(editor);
    e->setMinimumTime(index.model()->data(index, Role::Minimum).toTime());
    e->setMaximumTime(index.model()->data(index, Role::Maximum).toTime());
    e->setTime(value);
}

void TimeDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                const QModelIndex &index) const
{
    QTimeEdit *e = static_cast<QTimeEdit*>(editor);
    model->setData(index, e->time(), Qt::EditRole);
}

void TimeDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
    QRect r = option.rect;
    editor->setGeometry(r);
}

//--------------------------
ItemModelBase::ItemModelBase(QObject *parent)
    : QAbstractItemModel(parent),
    m_project(nullptr),
    m_manager(nullptr),
    m_readWrite(false)//part->isReadWrite())
{
}

ItemModelBase::~ItemModelBase()
{
}

void ItemModelBase::setProject(Project *project)
{
    m_project = project;
}

void ItemModelBase::setScheduleManager(ScheduleManager *sm)
{
    bool changed = m_manager != sm;
    m_manager = sm;
    if (changed) {
        Q_EMIT managerChanged(sm);
    }
}

void ItemModelBase::slotLayoutChanged()
{
    debugPlan;
    Q_EMIT layoutAboutToBeChanged();
    Q_EMIT layoutChanged();
}

void ItemModelBase::slotLayoutToBeChanged()
{
    debugPlan;
    Q_EMIT layoutAboutToBeChanged();
}

bool ItemModelBase::dropAllowed(const QModelIndex &index, int, const QMimeData *data)
{
    if (flags(index) & Qt::ItemIsDropEnabled) {
        const QList<QString> formats = data->formats();
        for (const QString &s : formats) {
            if (mimeTypes().contains(s)) {
                return true;
            }
        }
    }
    return false;
}

QVariant ItemModelBase::data(const QModelIndex &index, int role) const
{
    if (index.isValid() && role == Role::ColumnTag) {
        return QLatin1String(columnMap().key(index.column()));
    }
    return QVariant();
}

QVariant ItemModelBase::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(orientation);
    if (role == Role::ColumnTag) {
        return QLatin1String(columnMap().key(section));
    }
    return QVariant();
}

bool ItemModelBase::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_UNUSED(index);
    if (role == Role::ReadWrite) {
        setReadWrite(value.toBool());
        return true;
    }
    return false;
}

void ItemModelBase::projectDeleted()
{
    setProject(nullptr);
}

int numColumns(const QModelIndexList &indexes)
{
    int c = 0;
    int r = indexes.first().row();
    QModelIndex currentParent = indexes.first().parent();
    for (const QModelIndex &idx : indexes) {
        if (idx.row() == r && currentParent == idx.parent()) {
            ++c;
        } else {
            break;
        }
    }
    return c;
}

int numRows(const QModelIndexList &indexes)
{
    int rows = 1;
    int currentrow = indexes.value(0).row();
    QModelIndex currentParent = indexes.value(0).parent();
    for (int i = 1; i < indexes.count(); ++i) {
        if (currentParent != indexes.at(i).parent() || currentrow != indexes.at(i).row()) {
            ++rows;
            currentrow = indexes.at(i).row();
            currentParent = indexes.at(i).parent();
        }
    }
    return rows;
}

void ItemModelBase::writeText(QMimeData *m, const QModelIndexList &indexes) const
{
    if (indexes.isEmpty()) {
        return;
    }
    int cols = numColumns(indexes);
    int rows = numRows(indexes);
    if (mimeTypes().contains(QStringLiteral("text/html"))) {
        QTextDocument doc;
        QTextCursor cursor(&doc);
        cursor.insertTable(rows+1, cols);
        QTextTableFormat tableFormat;
        QVector<QTextLength> v;
        for (int i = 0; i < cols; ++i) {
            QTextLength l(QTextLength::PercentageLength, 100 / cols);
            v << l;
        }
        tableFormat.setColumnWidthConstraints(v);
        cursor.currentTable()->setFormat(tableFormat);
        // headers
        for (int i = 0; i < cols; ++i) {
            cursor.insertText(headerData(indexes.at(i).column(), Qt::Horizontal).toString());
            cursor.movePosition(QTextCursor::NextCell);
        }
        // data
        for (int i = 0; i < indexes.count(); ++i) {
            cursor.insertText(indexes.at(i).data().toString());
            cursor.movePosition(QTextCursor::NextCell);
        }
        m->setData(QStringLiteral("text/html"), doc.toHtml().toUtf8());
    }
    if (mimeTypes().contains(QStringLiteral("text/plain"))) {
        QVector<QStringList> text(rows+1);
        // headers
        for (int c = 0; c < cols; ++c) {
            text[0] << headerData(indexes.at(c).column(), Qt::Horizontal).toString();
        }
        // data
        for (int i = 0; i < rows; ++i) {
            for (int c = 0; c < cols; ++c) {
                text[i+1] << indexes.at(i*cols+c).data().toString();
            }
        }
        if (mimeTypes().contains(QStringLiteral("text/plain"))) {
            QString s;
            for (const auto &t : std::as_const(text)) {
                s += t.join(QLatin1Char('\t'));
                s += QLatin1Char('\n');
            }
            m->setData(QStringLiteral("text/plain"), s.toUtf8());
        }
    }
}

QStringList ItemModelBase::mimeTypes() const
{
    return QStringList();
}

QMimeData *ItemModelBase::mimeData(const QModelIndexList &indexes) const
{
    debugPlan<<indexes;
    QMimeData *m = new QMimeData();
    if (indexes.isEmpty() || mimeTypes().isEmpty()) {
        debugPlan<<"No indexes or no mimeTypes";
        return m;
    }
    writeText(m, indexes);
    return m;
}
