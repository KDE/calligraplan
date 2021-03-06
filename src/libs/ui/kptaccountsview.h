/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2005 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTACCOUNTSVIEW_H
#define KPTACCOUNTSVIEW_H

#include "planui_export.h"

#include "kptviewbase.h"

#include <KoDocument.h>

#include <QDate>

#include "kptaccount.h"
#include "kpteffortcostmap.h"


namespace KPlato
{

class Account;
class Project;
class ScheduleManager;
class CostBreakdownItemModel;

class PLANUI_EXPORT AccountsTreeView : public DoubleTreeViewBase
{
    Q_OBJECT
public:
    explicit AccountsTreeView(QWidget *parent = nullptr);
    
    CostBreakdownItemModel *model() const;
    
    bool cumulative() const;
    void setCumulative(bool on);
    int periodType() const;
    void setPeriodType(int period);
    int startMode() const;
    void setStartMode(int mode);
    int endMode() const;
    void setEndMode(int mode);
    QDate startDate() const;
    void setStartDate(const QDate &date);
    QDate endDate() const;
    void setEndDate(const QDate &date);
    int showMode() const;
    void setShowMode(int show);

protected Q_SLOTS:
    void slotModelReset();

private:
    QList<int> m_leftHidden;
};

class PLANUI_EXPORT AccountsView : public ViewBase
{
    Q_OBJECT
public:
    AccountsView(KoPart *part, Project *project, KoDocument *doc, QWidget *parent);

    //~AccountsView();
    void setupGui();
    Project *project() const override { return m_project; }
    virtual void setZoom(double zoom);
    void setProject(Project *project) override;

    bool loadContext(const KoXmlElement &context) override;
    void saveContext(QDomElement &context) const override;

    CostBreakdownItemModel *model() const;

    KoPrintJob *createPrintJob() override;
    
public Q_SLOTS:
    void setScheduleManager(KPlato::ScheduleManager *sm) override;
    void slotEditCopy() override;

protected Q_SLOTS:
    void slotContextMenuRequested(const QModelIndex&, const QPoint &pos);
    void slotHeaderContextMenuRequested(const QPoint &pos) override;
    void slotOptions() override;
    
private:
    void init();
    
private:
    Project *m_project;
    ScheduleManager *m_manager;
    AccountsTreeView *m_view;
    
    QDate m_date;
    int m_period;
    bool m_cumulative;
    QDomDocument m_domdoc;
    
};

}  //KPlato namespace

#endif
