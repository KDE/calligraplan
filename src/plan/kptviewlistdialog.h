/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2007-2010 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTVIEWLISTDIALOG_H
#define KPTVIEWLISTDIALOG_H

#include "ui_kptviewlistaddview.h"
#include "ui_kptviewlisteditview.h"
#include "ui_kptviewlisteditcategory.h"

#include <KoDialog.h>

#include <QWidget>
#include <QDomDocument>


namespace KPlato
{

class View;
class ViewListWidget;
class ViewListItem;
class AddViewPanel;
class EditViewPanel;
class EditCategoryPanel;
class ViewBase;
class AddReportsViewPanel;

class ViewListDialog : public KoDialog
{
    Q_OBJECT
public:
    ViewListDialog(View *view, ViewListWidget &viewlist, QWidget *parent=nullptr);

protected Q_SLOTS:
    void slotOk();

    void slotViewListItemRemoved(KPlato::ViewListItem *);

Q_SIGNALS:
    void viewCreated(KPlato::ViewBase *view);

private:
    AddViewPanel *m_panel;
};

class AddViewPanel : public QWidget
{
    Q_OBJECT
public:
    AddViewPanel(View *view, ViewListWidget &viewlist, QWidget *parent);

    bool ok();

    Ui::AddViewPanel widget;

Q_SIGNALS:
    void enableButtonOk(bool);
    void viewCreated(KPlato::ViewBase *view);

protected Q_SLOTS:
    void changed();
    void viewtypeChanged(int idx);
    void categoryChanged();
    void fillAfter(KPlato::ViewListItem *cat);

    void viewnameChanged(const QString &text);
    void viewtipChanged(const QString &text);

private:
    View *m_view;
    ViewListWidget &m_viewlist;
    QMap<QString, ViewListItem*> m_categories;
    QStringList m_viewtypes;
    bool m_viewnameChanged;
    bool m_viewtipChanged;
};

class ViewListEditViewDialog : public KoDialog
{
    Q_OBJECT
public:
    ViewListEditViewDialog(ViewListWidget &viewlist, ViewListItem *item, QWidget *parent=nullptr);

protected Q_SLOTS:
    void slotOk();

    void slotViewListItemRemoved(KPlato::ViewListItem*);

private:
    EditViewPanel *m_panel;
};

class EditViewPanel : public QWidget
{
    Q_OBJECT
public:
    EditViewPanel(ViewListWidget &viewlist, ViewListItem *item, QWidget *parent);

    bool ok();

    Ui::EditViewPanel widget;

Q_SIGNALS:
    void enableButtonOk(bool);

protected Q_SLOTS:
    void changed();
    void categoryChanged();
    void fillAfter(KPlato::ViewListItem *cat);

private:
    ViewListItem *m_item;
    ViewListWidget &m_viewlist;
    QMap<QString, ViewListItem*> m_categories;
};

class ViewListEditCategoryDialog : public KoDialog
{
    Q_OBJECT
public:
    ViewListEditCategoryDialog(ViewListWidget &viewlist, ViewListItem *item, QWidget *parent=nullptr);

protected Q_SLOTS:
    void slotOk();

    void slotViewListItemRemoved(KPlato::ViewListItem*);

private:
    EditCategoryPanel *m_panel;
};

class EditCategoryPanel : public QWidget
{
    Q_OBJECT
public:
    EditCategoryPanel(ViewListWidget &viewlist, ViewListItem *item, QWidget *parent);

    bool ok();

    Ui::EditCategoryPanel widget;

Q_SIGNALS:
    void enableButtonOk(bool);

protected Q_SLOTS:
    void changed();
    void fillAfter();

private:
    ViewListItem *m_item;
    ViewListWidget &m_viewlist;
};

#ifdef PLAN_USE_KREPORT
//-------- Reports
class ViewListReportsDialog : public KoDialog
{
    Q_OBJECT
public:
    ViewListReportsDialog(View *view, ViewListWidget &viewlist, const QDomDocument &doc, QWidget *parent=0);

protected Q_SLOTS:
    void slotOk();

    void slotViewListItemRemoved(KPlato::ViewListItem*);

Q_SIGNALS:
    void viewCreated(KPlato::ViewBase *view);

private:
    AddReportsViewPanel *m_panel;
};

class AddReportsViewPanel : public QWidget
{
    Q_OBJECT
public:
    AddReportsViewPanel(View *view, ViewListWidget &viewlist, const QDomDocument &doc, QWidget *parent);

    bool ok();

    Ui::AddViewPanel widget;

Q_SIGNALS:
    void enableButtonOk(bool);
    void viewCreated(KPlato::ViewBase *view);

protected Q_SLOTS:
    void changed();
    void viewtypeChanged(int idx);
    void categoryChanged();
    void fillAfter(KPlato::ViewListItem *cat);

    void viewnameChanged(const QString &text);
    void viewtipChanged(const QString &text);

private:
    View *m_view;
    ViewListWidget &m_viewlist;
    QMap<QString, ViewListItem*> m_categories;
    QStringList m_viewtypes;
    bool m_viewnameChanged;
    bool m_viewtipChanged;
    QDomDocument m_data;
};
#endif

} //KPlato namespace

#endif // CONFIGDIALOG_H
