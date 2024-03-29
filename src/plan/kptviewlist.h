/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2007-2010 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTVIEWLIST_H
#define KPTVIEWLIST_H

#include "plan_export.h"

#include "kptschedulemodel.h"

#include <QTreeWidget>

class QDomElement;

class KoDocument;
class KoView;

class QComboBox;

namespace KPlato
{

class View;
class ViewBase;
class ViewListItem;
class ViewListWidget;

class MainDocument;
class Context;
class ScheduleManager;

#define TIP_USE_DEFAULT_TEXT "TIP_USE_DEFAULT_TEXT"

//--------------
struct ViewInfo
{
    QString name;
    QString tip;
};

//--------------
class PLAN_EXPORT ViewListItem : public QTreeWidgetItem
{
    public:
        enum ItemType { ItemType_Category = Type, ItemType_SubView = UserType };

        enum DataRole { DataRole_View = Qt::UserRole, DataRole_Document };

        ViewListItem(const QString &tag, const QStringList &strings, int type = ItemType_Category);
        ViewListItem(QTreeWidget *parent, const QString &tag, const QStringList &strings, int type = ItemType_Category);
        ViewListItem(QTreeWidgetItem *parent, const QString &tag, const QStringList &strings, int type = ItemType_Category);

        void setView(ViewBase *view);
        ViewBase *view() const;
        void setDocument(KoDocument *doc);
        KoDocument *document() const;

        void setViewInfo(const ViewInfo &vi) { m_viewinfo = vi; }
        QString viewType() const;
        QString tag() const { return m_tag; }
        void save(QDomElement &element) const;

        void setReadWrite(bool rw);

    private:
        QString m_tag;
        ViewInfo m_viewinfo;
};

class PLAN_EXPORT ViewListTreeWidget : public QTreeWidget
{
    Q_OBJECT
public:
    explicit ViewListTreeWidget(QWidget *parent);
    ViewListItem *findCategory(const QString &cat);

    /// Return the category of @p view
    ViewListItem *category(const KoView *view) const;

    void save(QDomElement &element) const;

protected:
    void drawRow(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void mousePressEvent (QMouseEvent *event) override;
    /// Setup drop enabled/disabled dependent on the selected item
    void startDrag(Qt::DropActions supportedActions) override;
    /// If modified by the drop, Q_EMIT modified
    void dropEvent(QDropEvent *event) override;

Q_SIGNALS:
    void updateViewInfo(KPlato::ViewListItem *itm);
    void modified();

private Q_SLOTS:
    void handleMousePress(QTreeWidgetItem *item);
};

class PLAN_EXPORT ViewListWidget : public QWidget
{
Q_OBJECT
public:
    ViewListWidget(MainDocument *part, QWidget *parent);//QString name, KXmlGuiWindow *parent);
    ~ViewListWidget() override;

    /// Set read/write permission on all views.
    void setReadWrite(bool rw);
    /// Add a category if it does not already exist
    ViewListItem *addCategory(const QString &tag, const QString& name);
    /// Return a list of all categories
    QList<ViewListItem*> categories() const;

    /// Return the category with @p tag
    ViewListItem *findCategory(const QString &tag) const;
    /// Return the category of @p view
    ViewListItem *category(const KoView *view) const;

    /// Create a unique tag
    QString uniqueTag(const QString &seed) const;
    /// Add a sub-view
    ViewListItem *addView(QTreeWidgetItem *category, const QString &tag, const QString &name, ViewBase *view, KoDocument *doc, const QString &iconName = QString(), int index = -1);

    void setSelected(QTreeWidgetItem *item);
    ViewListItem *currentItem() const;
    void setCurrentItem(QTreeWidgetItem *item);
    ViewListItem *currentCategory() const;
    KoView *findView(const QString &tag) const;
    ViewListItem *findItem(const QString &tag) const;
    ViewListItem *findItem(const QString &tag, QTreeWidgetItem* parent) const;
    ViewListItem *findItem(const ViewBase *view, QTreeWidgetItem* parent = nullptr) const;
    int indexOf(const QString &catecory, const QString &tag) const;

    /// Remove @p item, don't Q_EMIT signal
    int removeViewListItem(ViewListItem *item);
    /// Add @p item to @p parent at @p index, don't Q_EMIT signal
    void addViewListItem(ViewListItem *item, QTreeWidgetItem *parent, int index);

    /// Remove @p item, Q_EMIT signal
    int takeViewListItem(ViewListItem *item);
    /// Add @p item to @p parent at @ index, Q_EMIT signal
    void insertViewListItem(ViewListItem *item, QTreeWidgetItem *parent, int index);

    void save(QDomElement &element) const;

    ViewListItem *previousViewItem() const { return m_prev; }

    ScheduleManager *selectedSchedule() const;

Q_SIGNALS:
    void activated(KPlato::ViewListItem*, KPlato::ViewListItem*);
    void createView();
    void viewListItemRemoved(KPlato::ViewListItem *item);
    void viewListItemInserted(KPlato::ViewListItem *item, KPlato::ViewListItem *parent, int index);

    void selectionChanged(KPlato::ScheduleManager*);

    void updateViewInfo(KPlato::ViewListItem *itm);

    void modified();

public Q_SLOTS:
    void setProject(KPlato::Project *project);
    void setSelectedSchedule(KPlato::ScheduleManager *sm);
    void setModified();

protected Q_SLOTS:
    void slotActivated(QTreeWidgetItem *item, QTreeWidgetItem *prev);
    void slotItemChanged(QTreeWidgetItem *item, int col);
    void renameCategory();
    void slotAddView();
    void slotRemoveCategory();
    void slotRemoveView();
    void slotEditViewTitle();
    void slotEditDocumentTitle();
    void slotConfigureItem();

    void slotCurrentScheduleChanged(int);
    void slotScheduleManagerAdded(KPlato::ScheduleManager*);

    void slotDialogFinished(int result);

protected:
    void contextMenuEvent (QContextMenuEvent *event) override;

private:
    void setupContextMenus();

private:
    MainDocument *m_part;
    ViewListTreeWidget *m_viewlist;
    QComboBox *m_currentSchedule;
    ScheduleSortFilterModel m_sfModel;
    ScheduleItemModel m_model;

    ViewListItem *m_contextitem;
    QList<QAction*> m_categoryactions;
    QList<QAction*> m_viewactions;
    QList<QAction*> m_listactions;

    ViewListItem *m_prev;

    ScheduleManager *m_temp;
};

} //Kplato namespace

#endif
