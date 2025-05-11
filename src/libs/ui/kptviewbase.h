/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2006-2010 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTVIEWBASE_H
#define KPTVIEWBASE_H

#include "planui_export.h"
#include "kptitemmodelbase.h"

#include "ui_kptprintingheaderfooter.h"


#include <KoView.h>
#include <KoPrintingDialog.h>
#include <KoPrintJob.h>
#include <KoPageLayout.h>
//#include <KoDockFactoryBase.h>

#include <QMap>
#include <QTreeView>
#include <QSplitter>
#include <QList>
#include <QDockWidget>
#include <QDomDocument>
#include <QAction>

class QMetaEnum;
class QAbstractItemModel;
class QDomElement;
class QModelIndex;

class KoDocument;
class KoPrintJob;
class KoPart;

/// The main namespace
namespace KPlato
{

class Project;
class Node;
class Resource;
class ResourceGroup;
class Relation;
class Calendar;

class ViewBase;
class TreeViewBase;
class DoubleTreeViewBase;

class CommonActionsController;
//------------------

class PLANUI_EXPORT DockWidget : public QDockWidget
{
    Q_OBJECT
public:
    DockWidget(ViewBase *v, const QString &identity, const QString &title);

    void activate(KoMainWindow *mainWindow);
    void deactivate(KoMainWindow *mainWindow);
    bool shown() const;

    bool saveXml(QDomElement &context) const;
    void loadXml(const KoXmlElement &context);

    const ViewBase *view;        /// The view this docker belongs to
    QString id;                  /// Docker identity
    Qt::DockWidgetArea location; /// The area the docker should go when visible
    bool editor;                 /// Editor dockers will not be shown in read only mode

public Q_SLOTS:
    void setShown(bool show);
    void setLocation(Qt::DockWidgetArea area);

private:
    bool m_shown;                /// The dockers visibility when the view is active
};

//------------------
class PLANUI_EXPORT PrintingOptions
{
public:
    PrintingOptions() {
        headerOptions.group = true;
        headerOptions.project = Qt::Checked;
        headerOptions.date = Qt::Checked;
        headerOptions.manager = Qt::Checked;
        headerOptions.page = Qt::Checked;

        footerOptions.group = false;
        footerOptions.project = Qt::Checked;
        footerOptions.date = Qt::Checked;
        footerOptions.manager = Qt::Checked;
        footerOptions.page = Qt::Checked;
    }
    ~PrintingOptions() {}

    bool loadXml(KoXmlElement &element);
    void saveXml(QDomElement &element) const;

    struct Data {
        bool group;
        Qt::CheckState project;
        Qt::CheckState date;
        Qt::CheckState manager;
        Qt::CheckState page;
    };
    struct Data headerOptions;
    struct Data footerOptions;
};

//------------------
class PLANUI_EXPORT PrintingHeaderFooter : public QWidget, public Ui::PrintingHeaderFooter
{
    Q_OBJECT
public:
    explicit PrintingHeaderFooter(const PrintingOptions &opt, QWidget *parent = nullptr);
    ~PrintingHeaderFooter() override;

    void setOptions(const PrintingOptions &options);
    PrintingOptions options() const;

Q_SIGNALS:
    void changed(const KPlato::PrintingOptions&);

protected Q_SLOTS:
    void slotChanged();

private:
    PrintingOptions m_options;
};

//------------------
class PLANUI_EXPORT PrintingDialog : public KoPrintingDialog
{
    Q_OBJECT
public:
    explicit PrintingDialog(ViewBase *view, QPrinter::PrinterMode mode = QPrinter::ScreenResolution);
    ~PrintingDialog() override;

    QList<QWidget*> createOptionWidgets() const override;
//    virtual QList<KoShape*> shapesOnPage(int);

    QRect headerRect() const;
    QRect footerRect() const;
    void paintHeaderFooter(QPainter &p, const PrintingOptions &options, int pageNumber, const Project &project);

    PrintingOptions printingOptions() const;

    QWidget *createPageLayoutWidget() const;
    QAbstractPrintDialog::PrintDialogOptions printDialogOptions() const override;

Q_SIGNALS:
    void changed(const KPlato::PrintingOptions &opt);

public Q_SLOTS:
    void setPrintingOptions(const KPlato::PrintingOptions &opt);
    void setPrinterPageLayout(const KoPageLayout &pagelayout);
    void startPrinting(KoPrintJob::RemovePolicy removePolicy = DoNotDelete) override;

protected:
    virtual void paint(QPainter &p, const PrintingOptions::Data &options, const QRect &rect,  int pageNumber, const Project &project);
    int headerFooterHeight(const PrintingOptions::Data &options) const;
    void drawRect(QPainter &p, const QRect &r, Qt::Edges edges = Qt::LeftEdge | Qt::RightEdge | Qt::BottomEdge);


protected:
    ViewBase *m_view;
    PrintingHeaderFooter *m_widget;
    int m_textheight;
};

class PLANUI_EXPORT ViewActionLists
{
public:
    ViewActionLists() : actionOptions(nullptr) {}
    virtual ~ViewActionLists() {}

    QList<QAction*> contextActionList() const { return m_contextActionList; }
    void addContextAction(QAction *action, qsizetype pos = INT_MAX) {
        m_contextActionList.insert(std::min(pos, m_contextActionList.count()), action);
    }

protected:
    /// List of actions that will be shown in the views header context menu
    QList<QAction*> m_contextActionList;

    // View options context menu
    QAction *actionOptions;
};

/**
 ViewBase is the baseclass of all sub-views to View.

*/
class PLANUI_EXPORT ViewBase : public KoView, public ViewActionLists
{
    Q_OBJECT
public:
    enum OptionTypes {
        OptionExpand = 1,
        OptionCollapse = 2,
        OptionPrint = 4,
        OptionPrintPreview = 8,
        OptionPrintPdf = 16,
        OptionPrintConfig = 32,
        OptionViewConfig = 64,
        OptionAll = 0xffff
    };
    /// Constructor
    ViewBase(KoPart *part, KoDocument *doc, QWidget *parent);
    /// Destructor
    ~ViewBase() override;
    /// Return the part (document) this view handles
    KoDocument *part() const;

    /// Return the page layout used for printing this view
    KoPageLayout pageLayout() const override;

    /// Return the type of view this is (class name)
    QString viewType() const { return QLatin1String(metaObject()->className()); }

    /// Returns true if this view or any child widget has focus
    bool isActive() const;

    /// Return the project
    virtual Project *project() const { return m_proj; }
    /// Return the schedule manager
    virtual ScheduleManager *scheduleManager() const { return m_schedulemanager; }
    /// Draw data from current part / project
    virtual void draw() {}
    /// Draw data from project.
    virtual void draw(Project &/*project*/) {}
    /// Draw changed data from project.
    virtual void drawChanges(Project &project) { draw(project); }
    /// Set readWrite mode
    void updateReadWrite(bool) override;
    bool isReadWrite() const { return m_readWrite; }

    /// Reimplement if your view handles nodes
    virtual Node* currentNode() const { return nullptr; }
    /// Reimplement if your view handles resources
    virtual Resource* currentResource() const { return nullptr; }
    /// Reimplement if your view handles resource groups
    virtual ResourceGroup* currentResourceGroup() const { return nullptr; }
    /// Reimplement if your view handles calendars
    virtual Calendar* currentCalendar() const { return nullptr; }
    /// Reimplement if your view handles relations
    virtual Relation *currentRelation() const { return nullptr; }
    /// Reimplement if your view handles zoom
//    virtual KoZoomController *zoomController() const { return 0; }

    /// Loads context info (printer settings) into this view.
    virtual bool loadContext(const KoXmlElement &context);
    /// Save context info (printer settings) from this view.
    virtual void saveContext(QDomElement &context) const;

    KoPrintJob *createPrintJob() override;
    PrintingOptions printingOptions() const { return m_printingOptions; }
    static QWidget *createPageLayoutWidget(ViewBase *view);
    static PrintingHeaderFooter *createHeaderFooterWidget(ViewBase *view);

    void addDocker(DockWidget *ds);
    QList<DockWidget*> dockers() const;
    DockWidget *findDocker(const QString &id) const;

    void setViewSplitMode(bool split);
    /// Show the listed columns in @p left and @p right
    /// @p right is only used if this view is a double treeview
    /// If @p right is empty, the right view is hidden
    /// Columns are sorted according to the lists
    void showColumns(const QList<int> &left, const QList<int> &right = QList<int>());

    void openPopupMenu(const QString& name, const QPoint &pos);
    QMenu *popupMenu(const QString& name);

    void openContextMenu(const QString& name, const QPoint &pos);

public Q_SLOTS:
    void setPrintingOptions(const KPlato::PrintingOptions &opt) { m_printingOptions = opt; }
    /// Activate/deactivate the gui
    virtual void setGuiActive(bool activate);
    /// Set the project this view shall handle.
    virtual void setProject(KPlato::Project *project);
    /// Set the schedule manager this view shall handle.
    virtual void setScheduleManager(KPlato::ScheduleManager *sm);
    void slotUpdateReadWrite(bool);
    virtual void slotHeaderContextMenuRequested(const QPoint &pos);

    virtual void slotEditCopy() {}
    virtual void slotEditCut() {}
    virtual void slotEditPaste() {}
    virtual void slotRefreshView() {}

    void setPageLayout(const KoPageLayout &layout) override;

Q_SIGNALS:
    /// Emitted when the gui has been activated or deactivated
    void guiActivated(KPlato::ViewBase*, bool);
    /// Request for a context menu popup
    void requestPopupMenu(const QString&, const QPoint &);

    /// Emitted when options are modified
    void optionsModified();

    void projectChanged(KPlato::Project *project);
    void readWriteChanged(bool);

    void expandAll();
    void collapseAll();

    void openDocument(const QUrl &url);
    void openTaskDescription(bool);

protected Q_SLOTS:
    virtual void slotOptions() {}
    virtual void slotOptionsFinished(int result);

protected:
    void createOptionActions(int actions, const QString &prefix = QString());

    bool m_readWrite;
    PrintingOptions m_printingOptions;

    Project *m_proj;
    ScheduleManager *m_schedulemanager;

    KoPageLayout m_pagelayout;

    QList<DockWidget*> m_dockers;

    TreeViewBase *m_singleTreeView;
    DoubleTreeViewBase *m_doubleTreeView;
};

//------------------
class PLANUI_EXPORT TreeViewPrintingDialog : public PrintingDialog
{
    Q_OBJECT
public:
    TreeViewPrintingDialog(ViewBase *view, TreeViewBase *treeview, Project *project = nullptr);
    ~TreeViewPrintingDialog() override {}

    int documentFirstPage() const override { return 1; }
    int documentLastPage() const override;

    QList<QWidget*> createOptionWidgets() const override;

protected:
    void printPage(int pageNumber, QPainter &painter) override;

    int firstRow(int page) const;

private:
    TreeViewBase *m_tree;
    Project *m_project;
    int m_firstRow;
};

//-----------------
class PLANUI_EXPORT TreeViewBase : public QTreeView
{
    Q_OBJECT
public:
    explicit TreeViewBase(QWidget *parent = nullptr);

    void setReadWrite(bool rw);
    virtual void createItemDelegates(ItemModelBase *model);
    void setArrowKeyNavigation(bool on) { m_arrowKeyNavigation = on; }
    bool arrowKeyNavigation() const { return m_arrowKeyNavigation; }

    /// Move move to first visual
    QModelIndex firstColumn(int row, const QModelIndex &parent);
    /// Move move to last visual
    QModelIndex lastColumn(int row, const QModelIndex &parent);
    /// Move from @p current to next item
    QModelIndex nextColumn(const QModelIndex &current);
    /// Move from @p current to next item
    QModelIndex previousColumn(const QModelIndex &current);
    /// Move to first editable index in @p row with @p parent
    QModelIndex firstEditable(int row, const QModelIndex &parent);
    /// Move to last editable index in @p row with @p parent
    QModelIndex lastEditable(int row, const QModelIndex &parent);

    void setAcceptDropsOnView(bool mode) { m_acceptDropsOnView = mode; }

    void setModel(QAbstractItemModel *model) override;

    void setSelectionModel(QItemSelectionModel *model) override;

    void setStretchLastSection(bool);

    void mapToSection(int column, int section);
    int section(int col) const;

    void setColumnsHidden(const QList<int> &list);

    /// Loads context info into this view. Reimplement.
    virtual bool loadContext(const QMetaEnum &map, const KoXmlElement &element, bool expand = true);
    /// Save context info from this view. Reimplement.
    virtual void saveContext(const QMetaEnum &map, QDomElement &context , bool expand = true) const;

    /**
      Reimplemented to fix qt bug 160083: Doesn't scroll horizontally.

      Scroll the contents of the tree view until the given model item
      \a index is visible. The \a hint parameter specifies more
      precisely where the item should be located after the
      operation.
      If any of the parents of the model item are collapsed, they will
      be expanded to ensure that the model item is visible.
    */
    void scrollTo(const QModelIndex &index, ScrollHint hint = EnsureVisible) override;

    void setDefaultColumns(const QList<int> &lst) { m_defaultColumns = lst; }
    QList<int> defaultColumns() const { return m_defaultColumns; }

    KoPrintJob *createPrintJob(ViewBase *parent);

    QModelIndex firstVisibleIndex(const QModelIndex &idx) const;

    ItemModelBase *itemModel() const;

    void setContextMenuIndex(const QModelIndex &idx);

    void loadExpanded(const KoXmlElement &element);
    void saveExpanded(QDomElement &element, const QModelIndex &parent = QModelIndex()) const;
    void expandRecursivly(QDomElement element, const QModelIndex &parent = QModelIndex());
    void doExpand(QDomDocument &doc);

    void setHandleDrag(bool state);
    QList<int> visualColumns() const;
    void setDragPixmap(const QPixmap &pixmap);
    QPixmap dragPixmap() const;

    QMimeData *mimeData() const;
    virtual void editCopy();
    virtual void editPaste();

    QModelIndexList selection() const { return selectedIndexes(); }

public Q_SLOTS:
    void slotExpand();
    void slotCollapse();

Q_SIGNALS:
    /// Context menu requested from viewport at global position @p pos
    void contextMenuRequested(const QModelIndex&, const QPoint &pos, const QModelIndexList&);
    /// Context menu requested from header at global position @p pos
    void headerContextMenuRequested(const QPoint &pos);

    void moveAfterLastColumn(const QModelIndex &);
    void moveBeforeFirstColumn(const QModelIndex &);
    void editAfterLastColumn(const QModelIndex &);
    void editBeforeFirstColumn(const QModelIndex &);

    void dropAllowed(const QModelIndex &index, int dropIndicatorPosition, QDragMoveEvent *event);

protected:
    /// Re-implemented to cater for hidden column 0
    QModelIndexList selectedIndexes() const override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;

    /**
      Reimplemented from QTreeView to make tab/backtab in editor work reasonably well.
      Move the cursor in the way described by \a cursorAction, *not* using the
      information provided by the button \a modifiers.
    */
    QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers) override;
    /// Move cursor from @p index in direction @p cursorAction. @p modifiers is not used.
    QModelIndex moveCursor(const QModelIndex &index, CursorAction cursorAction, Qt::KeyboardModifiers = Qt::NoModifier);
    /// Move from @p index to next editable item, in direction @p cursorAction.
    QModelIndex moveToEditable(const QModelIndex &index, CursorAction cursorAction);

    void contextMenuEvent (QContextMenuEvent * event) override;

    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *e) override;
    void updateSelection(const QModelIndex &oldidx, const QModelIndex &newidx, QKeyEvent *event);

    void expandRecursive(const QModelIndex &parent, bool xpand);

    //Copied from QAbstractItemView
    inline QItemSelectionModel::SelectionFlags selectionBehaviorFlags() const
    {
        switch (selectionBehavior()) {
            case QAbstractItemView::SelectRows: return QItemSelectionModel::Rows;
            case QAbstractItemView::SelectColumns: return QItemSelectionModel::Columns;
            case QAbstractItemView::SelectItems: default: return QItemSelectionModel::NoUpdate;
        }
    }

    void startDrag(Qt::DropActions supportedActions) override;

protected Q_SLOTS:
    /// Close the @p editor, using sender()->endEditHint().
    /// Use @p hint if sender is not of type ItemDelegate.
    void closeEditor(QWidget *editor, QAbstractItemDelegate::EndEditHint hint) override;

    virtual void slotCurrentChanged (const QModelIndex & current, const QModelIndex & previous);
    void slotHeaderContextMenuRequested(const QPoint&);

    void doContextExpanded();
    void doExpanded();

protected:
    bool m_arrowKeyNavigation;
    bool m_acceptDropsOnView;
    QList<int> m_hideList;
    bool m_readWrite;
    QList<int> m_defaultColumns;

    QPersistentModelIndex m_contextMenuIndex;
    QDomDocument m_loadContextDoc;
    QDomDocument m_expandDoc;

    bool m_handleDrag;
    QPixmap m_dragPixmap;
};

//------------------
class PLANUI_EXPORT DoubleTreeViewPrintingDialog : public PrintingDialog
{
    Q_OBJECT
public:
    DoubleTreeViewPrintingDialog(ViewBase *view, DoubleTreeViewBase *treeview, Project *project);
    ~DoubleTreeViewPrintingDialog() override {}

    int documentFirstPage() const override { return 1; }
    int documentLastPage() const override;

    QList<QWidget*> createOptionWidgets() const override;

protected:
    void printPage(int pageNumber, QPainter &painter) override;

    int firstRow(int page) const;

private:
    DoubleTreeViewBase *m_tree;
    Project *m_project;
    int m_firstRow;
};

class PLANUI_EXPORT DoubleTreeViewBase : public QSplitter
{
    Q_OBJECT
public:
    explicit DoubleTreeViewBase(QWidget *parent);
    DoubleTreeViewBase(bool mode, QWidget *parent);
    ~DoubleTreeViewBase() override;

    void setReadWrite(bool rw);
    void closePersistentEditor(const QModelIndex &index);

    void setModel(QAbstractItemModel *model);
    QAbstractItemModel *model() const;

    void setArrowKeyNavigation(bool on) { m_arrowKeyNavigation = on; }
    bool arrowKeyNavigation() const { return m_arrowKeyNavigation; }

    QItemSelectionModel *selectionModel() const { return m_selectionmodel; }
    void setSelectionModel(QItemSelectionModel *model);
    void setSelectionMode(QAbstractItemView::SelectionMode mode);
    void setSelectionBehavior(QAbstractItemView::SelectionBehavior mode);
    virtual void createItemDelegates(ItemModelBase *model);
    void setItemDelegateForColumn(int col, QAbstractItemDelegate * delegate);
    void setEditTriggers (QAbstractItemView::EditTriggers);
    QAbstractItemView::EditTriggers editTriggers() const;

    void setAcceptDrops(bool);
    void setAcceptDropsOnView(bool);
    void setDropIndicatorShown(bool);
    void setDragDropMode(QAbstractItemView::DragDropMode mode);
    void setDragDropOverwriteMode(bool mode);
    void setDragEnabled (bool mode);
    void setDefaultDropAction(Qt::DropAction action);

    void setStretchLastSection(bool);

    /// Hide columns in the @p hideList, show all other columns.
    /// If the hideList.last() == -1, the rest of the columns are hidden.
    void hideColumns(TreeViewBase *view, const QList<int> &hideList);
    void hideColumns(const QList<int> &masterList, const QList<int> &slaveList = QList<int>());
    void hideColumn(int col) {
        m_leftview->hideColumn(col);
        if (m_rightview) m_rightview->hideColumn(col);
    }

    void showColumn(int col) {
        if (col == 0 || m_rightview == nullptr) m_leftview->showColumn(col);
        else m_rightview->showColumn(col);
    }
    bool isColumnHidden(int col) const {
        return m_rightview ? m_rightview->isColumnHidden(col) : m_leftview->isColumnHidden(col);
    }

    TreeViewBase *masterView() const { return m_leftview; }
    TreeViewBase *slaveView() const { return m_rightview; }

    /// Loads context info into this view. Reimplement.
    virtual bool loadContext(const QMetaEnum &map, const KoXmlElement &element);
    /// Save context info from this view. Reimplement.
    virtual void saveContext(const QMetaEnum &map, QDomElement &context) const;

    void setViewSplitMode(bool split);
    bool isViewSplit() const { return m_mode; }
    QAction *actionSplitView() const { return m_actionSplitView; }

    void setRootIsDecorated (bool show);

    KoPrintJob *createPrintJob(ViewBase *parent);

    void setStretchFactors();

    QModelIndex indexAt(const QPoint &pos) const;

    void setParentsExpanded(const QModelIndex &idx, bool expanded);

    void setSortingEnabled(bool on) {
        m_leftview->setSortingEnabled(on);
        m_rightview->setSortingEnabled(on);
    }
    void sortByColumn(int col, Qt::SortOrder order = Qt::AscendingOrder) {
        if (! m_leftview->isColumnHidden(col) ||
             ! m_rightview->isVisible() ||
             m_rightview->isColumnHidden(col) )
        {
            m_leftview->sortByColumn(col, order);
        } else {
            m_rightview->sortByColumn(col, order);
        }
    }

    void setContextMenuIndex(const QModelIndex &idx);

    void handleDrag(Qt::DropActions supportedActions, Qt::DropAction defaultDropAction);
    void setDragPixmap(const QPixmap &pixmap);
    QPixmap dragPixmap() const;

    virtual void editCopy();
    virtual void editPaste();

    QMimeData *mimeData() const;

Q_SIGNALS:
    /// Context menu requested from the viewport, pointer over @p index at global position @p pos
    void contextMenuRequested(const QModelIndex &index, const QPoint& pos, const QModelIndexList&);
    /// Context menu requested from master- or slave header at global position @p pos
    void headerContextMenuRequested(const QPoint &pos);
    /// Context menu requested from master header at global position @p pos
    void masterHeaderContextMenuRequested(const QPoint &pos);
    /// Context menu requested from slave header at global position @p pos
    void slaveHeaderContextMenuRequested(const QPoint &pos);

    void currentChanged (const QModelIndex & current, const QModelIndex & previous);
    void selectionChanged(const QModelIndexList&);

    void dropAllowed(const QModelIndex &index, int dropIndicatorPosition, QDragMoveEvent *event);

public Q_SLOTS:
    void edit(const QModelIndex &index);
    void slotExpand();
    void slotCollapse();

protected Q_SLOTS:
    void slotSelectionChanged(const QItemSelection &sel, const QItemSelection &);
    void slotToRightView(const QModelIndex &index);
    void slotToLeftView(const QModelIndex &index);
    void slotEditToRightView(const QModelIndex &index);
    void slotEditToLeftView(const QModelIndex &index);

    void slotRightHeaderContextMenuRequested(const QPoint &pos);
    void slotLeftHeaderContextMenuRequested(const QPoint &pos);

    void slotLeftSortIndicatorChanged(int logicalIndex, Qt::SortOrder order);
    void slotRightSortIndicatorChanged(int logicalIndex, Qt::SortOrder order);

protected:
    void init();
    QList<int> expandColumnList(const QList<int> &lst) const;

protected:
    TreeViewBase *m_leftview;
    TreeViewBase *m_rightview;
    QItemSelectionModel *m_selectionmodel;
    bool m_arrowKeyNavigation;
    bool m_readWrite;
    bool m_mode;

    QAction *m_actionSplitView;
};

} // namespace KPlato

#endif
