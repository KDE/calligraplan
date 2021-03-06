/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2007 Dag Andersen <dag.andersen@kdemail.net>
  SPDX-FileCopyrightText: 2011 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTACCOUNTSMODEL_H
#define KPTACCOUNTSMODEL_H

#include "planmodels_export.h"

#include <kptitemmodelbase.h>
#include "kpteffortcostmap.h"

namespace KPlato
{

class Project;
class Account;
class ScheduleManager;
class AccountItemModel;

class PLANMODELS_EXPORT AccountModel : public QObject
{
    Q_OBJECT
public:
    AccountModel();
    ~AccountModel() override {}

    enum Properties {
        Name = 0,
        Description
    };
    Q_ENUM(Properties)
    const QMetaEnum columnMap() const;
    int propertyCount() const;

    virtual QVariant data(const Account *a, int property, int role = Qt::DisplayRole) const; 
    virtual QVariant headerData(int property, int role = Qt::DisplayRole) const; 

protected:
    QVariant name(const Account *account, int role) const;
    QVariant description(const Account *account, int role) const;

private:
    friend class AccountItemModel;
    Project *m_project;
};

class PLANMODELS_EXPORT AccountItemModel : public ItemModelBase
{
    Q_OBJECT
public:
    explicit AccountItemModel(QObject *parent = nullptr);
    ~AccountItemModel() override;

    const QMetaEnum columnMap() const override;
    void setProject(Project *project) override;

    Qt::ItemFlags flags(const QModelIndex & index) const override;

    QModelIndex parent(const QModelIndex & index) const override;
    QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const override;
    QModelIndex index(const Account* account, int column = 0) const;

    int columnCount(const QModelIndex & parent = QModelIndex()) const override; 
    int rowCount(const QModelIndex & parent = QModelIndex()) const override; 

    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override; 
    bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole) override;


    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    Account *account(const QModelIndex &index) const;
    QModelIndex insertAccount(Account *account, Account *parent = nullptr, int index = -1);
    void removeAccounts(QList<Account*> lst);
    
protected Q_SLOTS:
    void slotAccountChanged(KPlato::Account*);
    void slotAccountToBeInserted(const KPlato::Account *parent, int row);
    void slotAccountInserted(const KPlato::Account *account);
    void slotAccountToBeRemoved(const KPlato::Account *account);
    void slotAccountRemoved(const KPlato::Account *account);

protected:
    bool setName(Account *account, const QVariant &value, int role);
    
    bool setDescription(Account *account, const QVariant &value, int role);

private:
    AccountModel m_model;
    Account *m_account; // test for sane operation
};

//---------------
class PLANMODELS_EXPORT CostBreakdownItemModel : public ItemModelBase
{
    Q_OBJECT
public:
    enum PeriodType { Period_Day = 0, Period_Week = 1, Period_Month = 2 };
    enum StartMode { StartMode_Project = 0, StartMode_Date = 1 };
    enum EndMode { EndMode_Project = 0, EndMode_Date = 1, EndMode_CurrentDate = 2 };
    enum ShowMode { ShowMode_Actual = 0, ShowMode_Planned = 1, ShowMode_Both = 2, ShowMode_Deviation = 3 };

    explicit CostBreakdownItemModel(QObject *parent = nullptr);
    ~CostBreakdownItemModel() override;

    enum Properties {
        Name = 0,
        Description,
        Total,
        Planned,
        Actual
    };
    Q_ENUM(Properties)
    const QMetaEnum columnMap() const override;
    int propertyCount() const;

    void setProject(Project *project) override;
    void setScheduleManager(ScheduleManager *sm) override;
    long id() const;

    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QModelIndex parent(const QModelIndex & index) const override;
    QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const override;
    QModelIndex index(const Account* account) const;

    int columnCount(const QModelIndex & parent = QModelIndex()) const override; 
    int rowCount(const QModelIndex & parent = QModelIndex()) const override; 

    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override; 

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    Account *account(const QModelIndex &index) const;

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

    QString formatMoney(double plannedCost, double actualCost) const;
    QString format() const { return m_format; }
    void setFormat(const QString &f) { m_format = f; }
    
protected:
    void fetchData();
    EffortCostMap fetchPlannedCost(Account *account);
    EffortCostMap fetchActualCost(Account *account);
    
    QVariant cost(const Account *a, int offset, int role) const;

protected Q_SLOTS:
    void slotAccountChanged(KPlato::Account*);
    void slotAccountToBeInserted(const KPlato::Account *parent, int row);
    void slotAccountInserted(const KPlato::Account *account);
    void slotAccountToBeRemoved(const KPlato::Account *account);
    void slotAccountRemoved(const KPlato::Account *account);
    
    void slotDataChanged();

private:
    ScheduleManager *m_manager;
    
    bool m_cumulative;
    int m_periodtype;
    int m_startmode;
    int m_endmode;
    QDate m_start;
    QDate m_end;
    int m_showmode;
    QMap<Account*, EffortCostMap> m_plannedCostMap;
    QDate m_plannedStart, m_plannedEnd;
    QMap<Account*, EffortCostMap> m_actualCostMap;
    QDate m_actualStart, m_actualEnd;
    QString m_format;
    
};


}  //KPlato namespace

#endif
