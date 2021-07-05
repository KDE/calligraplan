/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2007 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTITEMVIEWSETTUP_H
#define KPTITEMVIEWSETTUP_H

#include "planui_export.h"

#include "ui_kptitemviewsettings.h"

#include <QListWidgetItem>

#include <kpagedialog.h>

class KoPageLayoutWidget;

namespace KPlato
{

class DoubleTreeViewBase;
class TreeViewBase;
class ViewBase;
class PrintingHeaderFooter;

class PLANUI_EXPORT ItemViewSettup : public QWidget, public Ui::ItemViewSettings
{
    Q_OBJECT
public:
    explicit ItemViewSettup(TreeViewBase *view, bool includeColumn0, QWidget *parent = nullptr);
    
    class Item : public QListWidgetItem
    {
    public:
        Item(int column, const QString &text);
        int column() const;
        bool operator<(const QListWidgetItem & other) const override;
    private:
        int m_column;
    };
    
Q_SIGNALS:
    void enableButtonOk(bool);
    
public Q_SLOTS:
    void slotChanged();
    void slotOk();
    void setDefault();
    
private:
    TreeViewBase *m_view;
    bool m_includeColumn0;
};

class PLANUI_EXPORT ItemViewSettupDialog : public KPageDialog
{
    Q_OBJECT
public:
    explicit ItemViewSettupDialog(ViewBase *view, TreeViewBase *treeview, bool includeColumn0 = false, QWidget *parent = nullptr);

    KPageWidgetItem *insertWidget(int before, QWidget *widget, const QString &name, const QString &header);
    void addPrintingOptions(bool setAsCurrent = false);

protected Q_SLOTS:
    virtual void slotOk();

protected:
    ViewBase *m_view;
    TreeViewBase *m_treeview;
    KoPageLayoutWidget *m_pagelayout;
    PrintingHeaderFooter *m_headerfooter;
    QList<KPageWidgetItem*> m_pageList;
    ItemViewSettup *m_panel;
};

class PLANUI_EXPORT SplitItemViewSettupDialog : public KPageDialog
{
    Q_OBJECT
public:
    explicit SplitItemViewSettupDialog(ViewBase *view, DoubleTreeViewBase *treeview, QWidget *parent = nullptr);

    KPageWidgetItem *insertWidget(int before, QWidget *widget, const QString &name, const QString &header);
    void addPrintingOptions(bool setAsCurrent = false);

protected Q_SLOTS:
    virtual void slotOk();

private:
    ViewBase *m_view;
    DoubleTreeViewBase *m_treeview;
    QList<KPageWidgetItem*> m_pageList;
    ItemViewSettup *m_page1;
    ItemViewSettup *m_page2;
    KoPageLayoutWidget *m_pagelayout;
    PrintingHeaderFooter *m_headerfooter;
};

} //namespace KPlato

#endif
