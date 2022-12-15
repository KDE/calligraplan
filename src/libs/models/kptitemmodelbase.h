/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2006-2009 Dag Andersen <calligra-devel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTITEMMODELBASE_H
#define KPTITEMMODELBASE_H

#include "planmodels_export.h"

#include "kptglobal.h"
#include "kpttreecombobox.h"

#include <QAbstractItemModel>
#include <QStyledItemDelegate>
#include <QMetaEnum>
#include <QMimeData>
#include <QSortFilterProxyModel>

#include <KoXmlReaderForward.h>

class KUndo2Command;


/// The main namespace
namespace KPlato
{

class Project;
class ScheduleManager;

/// Namespace for item delegate specific enums
namespace Delegate
{
    /// For selector delegate
    enum EditorType { EnumEditor, TimeEditor };
    /// Controls action when editor is closed. See QAbstractItemDelegate::EndEditHint.
    enum EndEditHint { 
        NoHint = QAbstractItemDelegate::NoHint,
        EditNextItem = QAbstractItemDelegate::EditNextItem,
        EditPreviousItem = QAbstractItemDelegate::EditPreviousItem,
        SubmitModelCache = QAbstractItemDelegate::SubmitModelCache,
        RevertModelCache = QAbstractItemDelegate::RevertModelCache,
        EditLeftItem = 100,
        EditRightItem = 101,
        EditDownItem = 102,
        EditUpItem = 103
    };
}

/// ItemDelegate implements improved control over closeEditor
class PLANMODELS_EXPORT ItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    /// Constructor
    explicit ItemDelegate(QObject *parent = nullptr)
    : QStyledItemDelegate(parent),
    m_lastHint(Delegate::NoHint)
    {}
    
    /// Extend EndEditHint for movement from edited item to next item to edit
    Delegate::EndEditHint endEditHint() const { return m_lastHint; }
    /// Increase the sizehint height a little to give room for editors
    QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const override;

protected:
    /// Implements arrow key navigation
    bool eventFilter(QObject *object, QEvent *event) override;
    /// Draw custom focus
//    virtual void drawFocus(QPainter *painter, const QStyleOptionViewItem &option, const QRect &rect) const;
    
private:
    Delegate::EndEditHint m_lastHint;
};

class PLANMODELS_EXPORT CheckStateItemDelegate : public ItemDelegate
{
    Q_OBJECT
public:
    explicit CheckStateItemDelegate(QObject *parent = nullptr);

protected:
    bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index) override;
};

class PLANMODELS_EXPORT DateTimeCalendarDelegate : public ItemDelegate
{
  Q_OBJECT
public:
    explicit DateTimeCalendarDelegate(QObject *parent = nullptr);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

};

class PLANMODELS_EXPORT ProgressBarDelegate : public ItemDelegate
{
  Q_OBJECT
public:
    explicit ProgressBarDelegate(QObject *parent = nullptr);

    ~ProgressBarDelegate() override;

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

protected:
    void initStyleOptionProgressBar(QStyleOptionProgressBar *option, const QModelIndex &index) const;

};

class Slider : public QSlider {
    Q_OBJECT
public:
    explicit Slider(QWidget *parent);
private Q_SLOTS:
    void updateTip(int value);
};

class PLANMODELS_EXPORT SelectorDelegate : public ItemDelegate
{
    Q_OBJECT
public:
    explicit SelectorDelegate(QObject *parent = nullptr);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;

    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

class PLANMODELS_EXPORT EnumDelegate : public ItemDelegate
{
    Q_OBJECT
public:
    explicit EnumDelegate(QObject *parent = nullptr);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;

    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

class PLANMODELS_EXPORT DurationSpinBoxDelegate : public ItemDelegate
{
    Q_OBJECT
public:
    explicit DurationSpinBoxDelegate(QObject *parent = nullptr);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;

    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

class PLANMODELS_EXPORT SpinBoxDelegate : public ItemDelegate
{
    Q_OBJECT
public:
    explicit SpinBoxDelegate(QObject *parent = nullptr);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;

    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

class PLANMODELS_EXPORT DoubleSpinBoxDelegate : public ItemDelegate
{
    Q_OBJECT
public:
    explicit DoubleSpinBoxDelegate(QObject *parent = nullptr);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;

    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

class PLANMODELS_EXPORT MoneyDelegate : public ItemDelegate
{
    Q_OBJECT
public:
    explicit MoneyDelegate(QObject *parent = nullptr);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;

    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

class PLANMODELS_EXPORT TimeDelegate : public ItemDelegate
{
    Q_OBJECT
public:
    explicit TimeDelegate(QObject *parent = nullptr);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;

    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

class PLANMODELS_EXPORT ItemModelBase : public QAbstractItemModel
{
    Q_OBJECT
public:
    // FIXME: Refactor, This is a copy from protected enum in QAbstractItemView
    enum DropIndicatorPosition {
        OnItem, /*QAbstractItemView::OnItem*/  /// The item will be dropped on the index.
        AboveItem, /*QAbstractItemView::AboveItem*/ /// The item will be dropped above the index.
        BelowItem, /*QAbstractItemView::BelowItem*/  /// The item will be dropped below the index.
        OnViewport /*QAbstractItemView::OnViewport*/ /// The item will be dropped onto a region of the viewport with no items if acceptDropsOnView is set.
    };

    explicit ItemModelBase(QObject *parent = nullptr);
    ~ItemModelBase() override;

    virtual const QMetaEnum columnMap() const { return QMetaEnum(); }
    Project *project() const { return m_project; }
    ScheduleManager *scheduleManager() const { return m_manager; }
    bool isReadWrite() { return m_readWrite; }
    void setReadOnly(int column, bool ro) { m_columnROMap[ column ] = ro; }
    /// Returns true if @p column has been set to ReadOnly.
    bool isColumnReadOnly(int column) const { return m_columnROMap.contains(column) && m_columnROMap[ column ]; }

    /**
     * Check if the @p data is allowed to be dropped on @p index,
     * @p dropIndicatorPosition indicates position relative @p index.
     *
     * Base implementation checks flags and mimetypes.
     */
    virtual bool dropAllowed(const QModelIndex &index, int dropIndicatorPosition, const QMimeData *data);
    
    /// Create the correct delegate for @p column. @p parent is the delegates parent widget.
    /// If default should be used, return 0.
    virtual QAbstractItemDelegate *createDelegate(int column, QWidget *parent) const { Q_UNUSED(column); Q_UNUSED(parent); return nullptr; }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    /// Return the sortorder to be used for @p column
    virtual int sortRole(int /*column*/) const { return Qt::DisplayRole; }

    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;
    void writeText(QMimeData *m, const QModelIndexList &indexes) const;

Q_SIGNALS:
    /// Connect to this signal if your model modifies data using undo commands.
    void executeCommand(KUndo2Command*);
    void managerChanged(KPlato::ScheduleManager *sm);

public Q_SLOTS:
    virtual void setProject(KPlato::Project *project);
    virtual void setScheduleManager(KPlato::ScheduleManager *sm);
    virtual void setReadWrite(bool rw) { m_readWrite = rw; }
    /// Reimplement if your model can be refreshed
    virtual void refresh() {}

protected Q_SLOTS:
    virtual void slotLayoutToBeChanged();
    virtual void slotLayoutChanged();

    void projectDeleted();

protected:
    Project *m_project;
    ScheduleManager *m_manager;
    bool m_readWrite;
    QMap<int, bool> m_columnROMap;
};


} // namespace KPlato

#endif
