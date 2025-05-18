/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2007, 2012 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kptaccountsmodel.h"

#include "kptglobal.h"
#include "kptlocale.h"
#include "kptcommonstrings.h"
#include "kptcommand.h"
#include "kptduration.h"
#include "kptnode.h"
#include "kptproject.h"
#include "kpttask.h"
#include "kptaccount.h"
#include "kptdatetime.h"
#include "kptschedule.h"
#include "kptdebug.h"

#include <KoIcon.h>

#include <KLocalizedString>

namespace KPlato
{

//--------------------------------------
AccountModel::AccountModel()
    : QObject(),
    m_project(nullptr)
{
}

const QMetaEnum AccountModel::columnMap() const
{
    return metaObject()->enumerator(metaObject()->indexOfEnumerator("Properties"));
}

int AccountModel::propertyCount() const
{
    return columnMap().keyCount();
}

QVariant AccountModel::data(const Account *a, int property, int role) const
{
    QVariant result;
    if (a == nullptr) {
        return QVariant();
    }
    switch (property) {
        case AccountModel::Name: result = name(a, role); break;
        case AccountModel::Description: result = description(a, role); break;
        default:
            debugPlan<<"data: invalid display value column"<<property;
            return QVariant();
    }
    return result;
}

QVariant AccountModel::name(const Account *a, int role) const
{
    //debugPlan<<a->name()<<","<<role;
    switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            return a->name();
        case Qt::ToolTipRole:
            if (a->isDefaultAccount()) {
                return xi18nc("1=account name", "%1 (Default account)", a->name());
            }
            return a->name();
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
         case Qt::CheckStateRole:
             if (a->isDefaultAccount()) {
                 return m_project && m_project->isBaselined() ? Qt::PartiallyChecked : Qt::Checked;
             }
             return  m_project && m_project->isBaselined() ? QVariant() : Qt::Unchecked;
         case Qt::DecorationRole:
             if (a->isBaselined()) {
                return koIcon("view-time-schedule-baselined");
             }
             break;
    }
    return QVariant();
}

QVariant AccountModel::description(const Account *a, int role) const
{
    //debugPlan<<res->name()<<","<<role;
    switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
        case Qt::ToolTipRole:
            return a->description();
            break;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant AccountModel::headerData(int property, int role) const
{
    if (role == Qt::DisplayRole) {
        switch (property) {
            case AccountModel::Name: return i18n("Name");
            case AccountModel::Description: return i18n("Description");
            default: return QVariant();
        }
    }
    if (role == Qt::TextAlignmentRole) {
        return QVariant();
    }
    if (role == Qt::ToolTipRole) {
        switch (property) {
            case AccountModel::Name: return ToolTip::accountName();
            case AccountModel::Description: return ToolTip::accountDescription();
            default: return QVariant();
        }
    }
    return QVariant();
}

//----------------------------------------
AccountItemModel::AccountItemModel(QObject *parent)
    : ItemModelBase(parent),
    m_account(nullptr)
{
}

AccountItemModel::~AccountItemModel()
{
}

const QMetaEnum AccountItemModel::columnMap() const
{
    return m_model.columnMap();
}

void AccountItemModel::slotAccountToBeInserted(const Account *parent, int row)
{
    //debugPlan<<parent->name();
    Q_ASSERT(m_account == nullptr);
    m_account = const_cast<Account*>(parent);
    beginInsertRows(index(parent), row, row);
}

void AccountItemModel::slotAccountInserted(const Account *account)
{
    //debugPlan<<account->name();
    Q_ASSERT(account->parent() == m_account); Q_UNUSED(account);
    endInsertRows();
    m_account = nullptr;
}

void AccountItemModel::slotAccountToBeRemoved(const Account *account)
{
    //debugPlan<<account->name();
    Q_ASSERT(m_account == nullptr);
    m_account = const_cast<Account*>(account);
    int row = index(account).row();
    beginRemoveRows(index(account->parent()), row, row);
}

void AccountItemModel::slotAccountRemoved(const Account *account)
{
    //debugPlan<<account->name();
    Q_ASSERT(account == m_account); Q_UNUSED(account);
    endRemoveRows();
    m_account = nullptr;
}

void AccountItemModel::setProject(Project *project)
{
    if (m_project) {
        Accounts *acc = &(m_project->accounts());
        disconnect(acc , &Accounts::changed, this, &AccountItemModel::slotAccountChanged);

        disconnect(acc, &Accounts::accountAdded, this, &AccountItemModel::slotAccountInserted);
        disconnect(acc, &Accounts::accountToBeAdded, this, &AccountItemModel::slotAccountToBeInserted);

        disconnect(acc, &Accounts::accountRemoved, this, &AccountItemModel::slotAccountRemoved);
        disconnect(acc, &Accounts::accountToBeRemoved, this, &AccountItemModel::slotAccountToBeRemoved);
    }
    m_project = project;
    m_model.m_project = project;
    if (project) {
        Accounts *acc = &(project->accounts());
        debugPlan<<acc;
        connect(acc, &Accounts::changed, this, &AccountItemModel::slotAccountChanged);

        connect(acc, &Accounts::accountAdded, this, &AccountItemModel::slotAccountInserted);
        connect(acc, &Accounts::accountToBeAdded, this, &AccountItemModel::slotAccountToBeInserted);

        connect(acc, &Accounts::accountRemoved, this, &AccountItemModel::slotAccountRemoved);
        connect(acc, &Accounts::accountToBeRemoved, this, &AccountItemModel::slotAccountToBeRemoved);
    }
}

Qt::ItemFlags AccountItemModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = ItemModelBase::flags(index);
    flags &= ~Qt::ItemIsEditable;
    if (! m_readWrite) {
        return flags;
    }
    if (! index.isValid() || ! m_project) {
        return flags;
    }
    flags |= Qt::ItemIsDragEnabled;
    Account *a = account(index);
    if (a) {
        switch (index.column()) {
            case AccountModel::Name: {
                if (! a->isBaselined()) {
                    flags |= Qt::ItemIsEditable;
                    flags |= Qt::ItemIsUserCheckable;
                }
                break;
            }
            default: flags |= Qt::ItemIsEditable; break;
        }
    }
    return flags;
}


QModelIndex AccountItemModel::parent(const QModelIndex &index) const
{
    if (!index.isValid() || m_project == nullptr) {
        return QModelIndex();
    }
    //debugPlan<<index.internalPointer()<<":"<<index.row()<<","<<index.column();
    Account *a = account(index);
    if (a == nullptr) {
        return QModelIndex();
    }
    Account *par = a->parent();
    if (par) {
        a = par->parent();
        int row = -1;
        if (a) {
            row = a->accountList().indexOf(par);
        } else {
            row = m_project->accounts().accountList().indexOf(par);
        }
        //debugPlan<<par->name()<<":"<<row;
        return createIndex(row, 0, par);
    }
    return QModelIndex();
}

QModelIndex AccountItemModel::index(int row, int column, const QModelIndex &parent) const
{
    if (m_project == nullptr || column < 0 || column >= columnCount() || row < 0) {
        return QModelIndex();
    }
    Account *par = account(parent);
    if (par == nullptr) {
        if (row < m_project->accounts().accountList().count()) {
            return createIndex(row, column, m_project->accounts().accountList().at(row));
        }
    } else if (row < par->accountList().count()) {
        return createIndex(row, column, par->accountList().at(row));
    }
    return QModelIndex();
}

QModelIndex AccountItemModel::index(const Account *account, int column) const
{
    Account *a = const_cast<Account*>(account);
    if (m_project == nullptr || account == nullptr) {
        return QModelIndex();
    }
    int row = -1;
    Account *par = a->parent();
    if (par == nullptr) {
         row = m_project->accounts().accountList().indexOf(a);
    } else {
        row = par->accountList().indexOf(a);
    }
    if (row == -1) {
        return QModelIndex();
    }
    return createIndex(row, column, a);

}

int AccountItemModel::columnCount(const QModelIndex &) const
{
    return m_model.propertyCount();
}

int AccountItemModel::rowCount(const QModelIndex &parent) const
{
    if (m_project == nullptr) {
        return 0;
    }
    Account *par = account(parent);
    if (par == nullptr) {
        return m_project->accounts().accountList().count();
    }
    return par->accountList().count();
}

bool AccountItemModel::setName(Account *a, const QVariant &value, int role)
{
    switch (role) {
        case Qt::EditRole:
            if (value.toString() != a->name()) {
                Q_EMIT executeCommand(new RenameAccountCmd(a, value.toString(), kundo2_i18n("Modify account name")));
            }
            return true;
        case Qt::CheckStateRole: {
            switch (value.toInt()) {
                case Qt::Unchecked:
                    if (a->isDefaultAccount()) {
                        Q_EMIT executeCommand(new ModifyDefaultAccountCmd(m_project->accounts(), a, nullptr, kundo2_i18n("De-select as default account")));
                        return true;
                    }
                    break;
                case Qt::Checked:
                    if (! a->isDefaultAccount()) {
                        Q_EMIT executeCommand(new ModifyDefaultAccountCmd(m_project->accounts(), m_project->accounts().defaultAccount(), a, kundo2_i18n("Select as default account")));
                        return true;
                    }
                    break;
                default: break;
            }
        }
        default: break;
    }
    return false;
}

bool AccountItemModel::setDescription(Account *a, const QVariant &value, int role)
{
    switch (role) {
        case Qt::EditRole:
            if (value.toString() != a->description()) {
                Q_EMIT executeCommand(new ModifyAccountDescriptionCmd(a, value.toString(), kundo2_i18n("Modify account description")));
            }
            return true;
    }
    return false;
}

QVariant AccountItemModel::data(const QModelIndex &index, int role) const
{
    QVariant result;
    Account *a = account(index);
    if (a == nullptr) {
        return QVariant();
    }
    result = m_model.data(a, index.column(), role);
    return result;
}

bool AccountItemModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (! index.isValid()) {
        return ItemModelBase::setData(index, value, role);
    }
    if ((flags(index) &(Qt::ItemIsEditable | Qt::CheckStateRole)) == 0) {
        Q_ASSERT(true);
        return false;
    }
    Account *a = account(index);
    debugPlan<<a->name()<<value<<role;
    switch (index.column()) {
        case AccountModel::Name: return setName(a, value, role);
        case AccountModel::Description: return setDescription(a, value, role);
        default:
            qWarning("data: invalid display value column %d", index.column());
            return false;
    }
    return false;
}

QVariant AccountItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal) {
        return m_model.headerData(section, role);
    }
    return ItemModelBase::headerData(section, orientation, role);
}

Account *AccountItemModel::account(const QModelIndex &index) const
{
    return static_cast<Account*>(index.internalPointer());
}

void AccountItemModel::slotAccountChanged(Account *account)
{
    Account *par = account->parent();
    if (par) {
        int row = par->accountList().indexOf(account);
        Q_EMIT dataChanged(createIndex(row, 0, account), createIndex(row, columnCount() - 1, account));
    } else {
        int row = m_project->accounts().accountList().indexOf(account);
        Q_EMIT dataChanged(createIndex(row, 0, account), createIndex(row, columnCount() - 1, account));
    }
}

QModelIndex AccountItemModel::insertAccount(Account *account, Account *parent, int index)
{
    debugPlan;
    if (account->name().isEmpty() || m_project->accounts().findAccount(account->name())) {
        QString s = parent == nullptr ? account->name() : parent->name();
        account->setName(m_project->accounts().uniqueId(s));
        //m_project->accounts().insertId(account);
    }
    Q_EMIT executeCommand(new AddAccountCmd(*m_project, account, parent, index, kundo2_i18n("Add account")));
    int row = -1;
    if (parent) {
        row = parent->accountList().indexOf(account);
    } else {
        row = m_project->accounts().accountList().indexOf(account);
    }
    if (row != -1) {
        //debugPlan<<"Inserted:"<<account->name();
        return createIndex(row, 0, account);
    }
    debugPlan<<"Can't find"<<account->name();
    return QModelIndex();
}

void AccountItemModel::removeAccounts(QList<Account*> lst)
{
    MacroCommand *cmd = nullptr;
    KUndo2MagicString c = kundo2_i18np("Delete Account", "Delete %1 Accounts", lst.count());
    while (! lst.isEmpty()) {
        bool del = true;
        Account *acc = lst.takeFirst();
        for (Account *a : std::as_const(lst)) {
            if (acc->isChildOf(a)) {
                del = false; // acc will be deleted when a is deleted
                break;
            }
        }
        if (del) {
            if (cmd == nullptr) cmd = new MacroCommand(c);
            cmd->addCommand(new RemoveAccountCmd(*m_project, acc));
        }
    }
    if (cmd)
        Q_EMIT executeCommand(cmd);
}

//----------------------------------------
CostBreakdownItemModel::CostBreakdownItemModel(QObject *parent)
    : ItemModelBase(parent),
    m_manager(nullptr),
    m_cumulative(false),
    m_periodtype(Period_Day),
    m_startmode(StartMode_Project),
    m_endmode(EndMode_Project),
    m_showmode(ShowMode_Both)
{
    m_format = QStringLiteral("%1 [%2]");
}

CostBreakdownItemModel::~CostBreakdownItemModel()
{
}

const QMetaEnum CostBreakdownItemModel::columnMap() const
{
    return metaObject()->enumerator(metaObject()->indexOfEnumerator("Properties"));
}

int CostBreakdownItemModel::propertyCount() const
{
    return columnMap().keyCount();
}

void CostBreakdownItemModel::slotAccountToBeInserted(const Account *parent, int row)
{
    //debugPlan<<parent->name();
    beginInsertRows(index(parent), row, row);
}

void CostBreakdownItemModel::slotAccountInserted(const Account *account)
{
    Q_UNUSED(account);
    //debugPlan<<account->name();
    endInsertRows();
}

void CostBreakdownItemModel::slotAccountToBeRemoved(const Account *account)
{

    //debugPlan<<account->name();
    int row = index(account).row();
    beginRemoveRows(index(account->parent()), row, row);
}

void CostBreakdownItemModel::slotAccountRemoved(const Account *account)
{
    Q_UNUSED(account);
    //debugPlan<<account->name();
    endRemoveRows();
}

void CostBreakdownItemModel::slotDataChanged()
{
    fetchData();
    QMap<Account*, EffortCostMap>::const_iterator it;
    for (it = m_plannedCostMap.constBegin(); it != m_plannedCostMap.constEnd(); ++it) {
        QModelIndex idx1 = index(it.key());
        QModelIndex idx2 = index(idx1.row(), columnCount() - 1, parent(idx1));
        //debugPlan<<a->name()<<idx1<<idx2;
        Q_EMIT dataChanged(idx1, idx2);
    }
}

void CostBreakdownItemModel::setProject(Project *project)
{
    if (m_project) {
        Accounts *acc = &(m_project->accounts());
        disconnect(acc , &Accounts::changed, this, &CostBreakdownItemModel::slotAccountChanged);

        disconnect(acc, &Accounts::accountAdded, this, &CostBreakdownItemModel::slotAccountInserted);
        disconnect(acc, &Accounts::accountToBeAdded, this, &CostBreakdownItemModel::slotAccountToBeInserted);

        disconnect(acc, &Accounts::accountRemoved, this, &CostBreakdownItemModel::slotAccountRemoved);
        disconnect(acc, &Accounts::accountToBeRemoved, this, &CostBreakdownItemModel::slotAccountToBeRemoved);

        disconnect(m_project, &Project::aboutToBeDeleted, this, &CostBreakdownItemModel::projectDeleted);
        disconnect(m_project, &Project::nodeChanged, this, &CostBreakdownItemModel::slotDataChanged);
        disconnect(m_project, &Project::nodeAdded, this, &CostBreakdownItemModel::slotDataChanged);
        disconnect(m_project, &Project::nodeRemoved, this, &CostBreakdownItemModel::slotDataChanged);

        disconnect(m_project, &Project::resourceChanged, this, &CostBreakdownItemModel::slotDataChanged);
        disconnect(m_project, &Project::resourceAdded, this, &CostBreakdownItemModel::slotDataChanged);
        disconnect(m_project, &Project::resourceRemoved, this, &CostBreakdownItemModel::slotDataChanged);
    }
    m_project = project;
    if (project) {
        Accounts *acc = &(project->accounts());
        debugPlan<<acc;
        connect(acc, &Accounts::changed, this, &CostBreakdownItemModel::slotAccountChanged);

        connect(acc, &Accounts::accountAdded, this, &CostBreakdownItemModel::slotAccountInserted);
        connect(acc, &Accounts::accountToBeAdded, this, &CostBreakdownItemModel::slotAccountToBeInserted);

        connect(acc, &Accounts::accountRemoved, this, &CostBreakdownItemModel::slotAccountRemoved);
        connect(acc, &Accounts::accountToBeRemoved, this, &CostBreakdownItemModel::slotAccountToBeRemoved);

        connect(m_project, &Project::aboutToBeDeleted, this, &CostBreakdownItemModel::projectDeleted);
        connect(m_project, &Project::nodeChanged, this, &CostBreakdownItemModel::slotDataChanged);
        connect(m_project, &Project::nodeAdded, this, &CostBreakdownItemModel::slotDataChanged);
        connect(m_project, &Project::nodeRemoved, this, &CostBreakdownItemModel::slotDataChanged);

        connect(m_project, &Project::resourceChanged, this, &CostBreakdownItemModel::slotDataChanged);
        connect(m_project, &Project::resourceAdded, this, &CostBreakdownItemModel::slotDataChanged);
        connect(m_project, &Project::resourceRemoved, this, &CostBreakdownItemModel::slotDataChanged);
    }
}

void CostBreakdownItemModel::setScheduleManager(ScheduleManager *sm)
{
    debugPlan<<m_project<<m_manager<<sm;
    if (m_manager != sm) {
        beginResetModel();
        m_manager = sm;
        fetchData();
        endResetModel();
    }
}

long CostBreakdownItemModel::id() const
{
    return m_manager == nullptr ? -1 : m_manager->scheduleId();
}

EffortCostMap CostBreakdownItemModel::fetchPlannedCost(Account *account)
{
    EffortCostMap ec;
    ec = account->plannedCost(id());
    m_plannedCostMap.insert(account, ec);
    QDate s = ec.startDate();
    if (! m_plannedStart.isValid() || s < m_plannedStart) {
        m_plannedStart = s;
    }
    QDate e = ec.endDate();
    if (! m_plannedEnd.isValid() || e > m_plannedEnd) {
        m_plannedEnd = e;
    }
    return ec;
}

EffortCostMap CostBreakdownItemModel::fetchActualCost(Account *account)
{
    debugPlan<<account->name();
    EffortCostMap ec;
    ec = account->actualCost(id());
    m_actualCostMap.insert(account, ec);
    QDate s = ec.startDate();
    if (! m_actualStart.isValid() || s < m_actualStart) {
        m_actualStart = s;
    }
    QDate e = ec.endDate();
    if (! m_actualEnd.isValid() || e > m_actualEnd) {
        m_actualEnd = e;
    }
    debugPlan<<account->name()<<ec.totalEffort().toDouble(Duration::Unit_h)<<ec.totalCost();
    return ec;
}

void CostBreakdownItemModel::fetchData()
{
    //debugPlan<<m_start<<m_end;
    m_plannedCostMap.clear();
    m_plannedStart = m_plannedEnd = QDate();
    m_actualStart = m_actualEnd = QDate();
    if (m_project == nullptr || m_manager == nullptr) {
        return;
    }
    const QList<Account*> accounts = m_project->accounts().allAccounts();
    for (Account *a : accounts) {
        fetchPlannedCost(a);
        fetchActualCost(a);
    }
}

Qt::ItemFlags CostBreakdownItemModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = ItemModelBase::flags(index);
    flags &= ~Qt::ItemIsEditable;
    if (index.isValid()) {
        flags |= Qt::ItemIsDragEnabled;
    }
    return flags;
}

QModelIndex CostBreakdownItemModel::parent(const QModelIndex &index) const
{
    if (!index.isValid() || m_project == nullptr) {
        return QModelIndex();
    }
    //debugPlan<<index.internalPointer()<<":"<<index.row()<<","<<index.column();
    Account *a = account(index);
    if (a == nullptr) {
        return QModelIndex();
    }
    Account *par = a->parent();
    if (par) {
        a = par->parent();
        int row = -1;
        if (a) {
            row = a->accountList().indexOf(par);
        } else {
            row = m_project->accounts().accountList().indexOf(par);
        }
        //debugPlan<<par->name()<<":"<<row;
        return createIndex(row, 0, par);
    }
    return QModelIndex();
}

QModelIndex CostBreakdownItemModel::index(int row, int column, const QModelIndex &parent) const
{
    if (m_project == nullptr || column < 0 || column >= columnCount() || row < 0) {
        return QModelIndex();
    }
    Account *par = account(parent);
    if (par == nullptr) {
        if (row < m_project->accounts().accountList().count()) {
            return createIndex(row, column, m_project->accounts().accountList().at(row));
        }
    } else if (row < par->accountList().count()) {
        return createIndex(row, column, par->accountList().at(row));
    }
    return QModelIndex();
}

QModelIndex CostBreakdownItemModel::index(const Account *account) const
{
    Account *a = const_cast<Account*>(account);
    if (m_project == nullptr || account == nullptr) {
        return QModelIndex();
    }
    int row = -1;
    Account *par = a->parent();
    if (par == nullptr) {
        row = m_project->accounts().accountList().indexOf(a);
    } else {
        row = par->accountList().indexOf(a);
    }
    if (row == -1) {
        return QModelIndex();
    }
    return createIndex(row, 0, a);

}

int CostBreakdownItemModel::columnCount(const QModelIndex &) const
{
    int c = propertyCount();
    if (startDate().isValid() && endDate().isValid()) {
        switch (m_periodtype) {
            case Period_Day: {
                c += startDate().daysTo(endDate()) + 1;
                break;
            }
            case Period_Week: {
                int days = QLocale().firstDayOfWeek() - startDate().dayOfWeek();
                if (days > 0) {
                    days -= 7;
                }
                QDate start = startDate().addDays(days);
                c += (start.daysTo(endDate()) / 7) + 1;
                break;
            }
            case Period_Month: {
                int days = startDate().daysInMonth() - startDate().day() + 1;
                for (QDate d = startDate(); d < endDate(); d = d.addDays(days)) {
                    ++c;
                    days = qMin(d.daysTo(endDate()), static_cast<qint64>(d.daysInMonth()));
                }
                break;
            }
        }
    }
    return c;
}

int CostBreakdownItemModel::rowCount(const QModelIndex &parent) const
{
    if (m_project == nullptr) {
        return 0;
    }
    Account *par = account(parent);
    if (par == nullptr) {
        return m_project->accounts().accountList().count();
    }
    return par->accountList().count();
}

QString CostBreakdownItemModel::formatMoney(double cost1, double cost2) const
{
    if (m_showmode == ShowMode_Planned) {
        return m_project->locale()->formatMoney(cost1, QString(), 0);
    }
    if (m_showmode == ShowMode_Actual) {
        return m_project->locale()->formatMoney(cost2, QString(), 0);
    }
    if (m_showmode == ShowMode_Both) {
        return QString(m_format).arg(m_project->locale()->formatMoney(cost2, QString(), 0), m_project->locale()->formatMoney(cost1, QString(), 0));
    }
    if (m_showmode == ShowMode_Deviation) {
        return m_project->locale()->formatMoney(cost1 - cost2, QString(), 0);
    }
    return QLatin1String("");
}

QVariant CostBreakdownItemModel::data(const QModelIndex &index, int role) const
{
    Account *a = account(index);
    if (a == nullptr) {
        return QVariant();
    }
    if (role == Qt::DisplayRole) {
        switch (index.column()) {
            case Name: return a->name();
            case Description: return a->description();
            case Total: {
                return formatMoney(m_plannedCostMap.value(a).totalCost(), m_actualCostMap.value(a).totalCost());
            }
            case Planned:
                return m_project->locale()->formatMoney(m_plannedCostMap.value(a).totalCost(), QString(), 0);
            case Actual:
                return m_project->locale()->formatMoney(m_actualCostMap.value(a).totalCost(), QString(), 0);
            default: {
                int col = index.column() - propertyCount();
                EffortCostMap pc = m_plannedCostMap.value(a);
                EffortCostMap ac = m_actualCostMap.value(a);
                switch (m_periodtype) {
                    case Period_Day: {
                        double planned = 0.0;
                        if (m_cumulative) {
                            planned = pc.costTo(startDate().addDays(col));
                        } else {
                            planned = pc.costOnDate(startDate().addDays(col));
                        }
                        double actual = 0.0;
                        if (m_cumulative) {
                            actual = ac.costTo(startDate().addDays(col));
                        } else {
                            actual = ac.costOnDate(startDate().addDays(col));
                        }
                        return formatMoney(planned, actual);
                    }
                    case Period_Week: {
                        int days = QLocale().firstDayOfWeek() - startDate().dayOfWeek();
                        if (days > 0) {
                            days -= 7; ;
                        }
                        QDate start = startDate().addDays(days);
                        int week = col;
                        double planned = 0.0;
                        if (m_cumulative) {
                            planned = pc.costTo(start.addDays(++week * 7));
                        } else {
                            planned = week == 0 ? pc.cost(startDate(), startDate().daysTo(start.addDays(7))) : pc.cost(start.addDays(week * 7));
                        }
                        double actual = 0.0;
                        if (m_cumulative) {
                            actual = ac.costTo(start.addDays(++week * 7));
                        } else {
                            actual = week == 0 ? ac.cost(startDate(), startDate().daysTo(start.addDays(7))) : ac.cost(start.addDays(week * 7));
                        }
                        return formatMoney(planned, actual);
                    }
                    case Period_Month: {
                        int days = startDate().daysInMonth() - startDate().day() + 1;
                        QDate start = startDate();
                        for (int i = 0; i < col; ++i) {
                            start = start.addDays(days);
                            days = start.daysInMonth();
                        }
                        int planned = 0.0;
                        if (m_cumulative) {
                            planned = pc.costTo(start.addDays(start.daysInMonth() - start.day() + 1));
                        } else {
                            planned = pc.cost(start, start.daysInMonth() - start.day() + 1);
                        }
                        int actual = 0.0;
                        if (m_cumulative) {
                            actual = ac.costTo(start.addDays(start.daysInMonth() - start.day() + 1));
                        } else {
                            actual = ac.cost(start, start.daysInMonth() - start.day() + 1);
                        }
                        return formatMoney(planned, actual);
                    }
                    default:
                        return 0.0;
                        break;
                }
            }
        }
    } else if (role == Qt::ToolTipRole) {
        switch (index.column()) {
            case Name: return a->name();
            case Description: return a->description();
            case Total: {
                double act = m_actualCostMap.value(a).totalCost();
                double pl = m_plannedCostMap.value(a).totalCost();
                return i18n("Actual total cost: %1, planned total cost: %2", m_project->locale()->formatMoney(act, QString(), 0), m_project->locale()->formatMoney(pl, QString(), 0));
            }
            case Planned:
            case Actual:
            default: break;
        }
    } else if (role == Qt::TextAlignmentRole) {
        return headerData(index.column(), Qt::Horizontal, role);
    } else {
        switch (index.column()) {
            case Name:
            case Description:
            case Planned:
            case Actual: return QVariant();
            default: {
                return cost(a, index.column() - propertyCount(), role);
            }
        }
    }
    return QVariant();
}

QVariant CostBreakdownItemModel::cost(const Account *a, int offset, int role) const
{
    EffortCostMap costmap;
    if (role == Role::Planned) {
        costmap = m_plannedCostMap.value(const_cast<Account*>(a));
    } else if (role == Role::Actual) {
        costmap = m_actualCostMap.value(const_cast<Account*>(a));
    } else {
        return QVariant();
    }
    double cost = 0.0;
    switch (m_periodtype) {
        case Period_Day: {
            if (m_cumulative) {
                cost = costmap.costTo(startDate().addDays(offset));
            } else {
                cost = costmap.costOnDate(startDate().addDays(offset));
            }
            break;
        }
        case Period_Week: {
            int days = QLocale().firstDayOfWeek() - startDate().dayOfWeek();
            if (days > 0) {
                days -= 7; ;
            }
            QDate start = startDate().addDays(days);
            int week = offset;
            if (m_cumulative) {
                cost = costmap.costTo(start.addDays(++week * 7));
            } else {
                cost = week == 0 ? costmap.cost(startDate(), startDate().daysTo(start.addDays(7))) : costmap.cost(start.addDays(week * 7));
            }
            break;
        }
        case Period_Month: {
            int days = startDate().daysInMonth() - startDate().day() + 1;
            QDate start = startDate();
            for (int i = 0; i < offset; ++i) {
                start = start.addDays(days);
                days = start.daysInMonth();
            }
            if (m_cumulative) {
                cost = costmap.costTo(start.addDays(start.daysInMonth() - start.day() + 1));
            } else {
                cost = costmap.cost(start, start.daysInMonth() - start.day() + 1);
            }
            break;
        }
        default:
            break;
    }
    return cost;
}

int CostBreakdownItemModel::periodType() const
{
    return m_periodtype;
}

void CostBreakdownItemModel::setPeriodType(int period)
{
    if (m_periodtype != period) {
        beginResetModel();
        m_periodtype = period;
        endResetModel();
    }
}

int CostBreakdownItemModel::startMode() const
{
    return m_startmode;
}

void CostBreakdownItemModel::setStartMode(int mode)
{
    beginResetModel();
    m_startmode = mode;
    endResetModel();
}

int CostBreakdownItemModel::endMode() const
{
    return m_endmode;
}

void CostBreakdownItemModel::setEndMode(int mode)
{
    beginResetModel();
    m_endmode = mode;
    endResetModel();
}

QDate CostBreakdownItemModel::startDate() const
{
    if (m_project == nullptr || m_manager == nullptr) {
        return m_start;
    }
    switch (m_startmode) {
        case StartMode_Project: {
            QDate d = m_project->startTime(id()).date();
            if (m_plannedStart.isValid() && m_plannedStart < d) {
                d = m_plannedStart;
            }
            if (m_actualStart.isValid() && m_actualStart < d) {
                d = m_actualStart;
            }
            return d;
        }
        default: break;
    }
    return m_start;
}


void CostBreakdownItemModel::setStartDate(const QDate &date)
{
    beginResetModel();
    m_start = date;
    endResetModel();
}

QDate CostBreakdownItemModel::endDate() const
{
    if (m_project == nullptr || m_manager == nullptr) {
        return m_end;
    }
    switch (m_endmode) {
        case EndMode_Project: {
            QDate d = m_project->endTime(id()).date();
            if (m_plannedEnd.isValid() && m_plannedEnd > d) {
                d = m_plannedEnd;
            }
            if (m_actualEnd.isValid() && m_actualEnd > d) {
                d = m_actualEnd;
            }
            return d;
        }
        case EndMode_CurrentDate: return QDate::currentDate();
        default: break;
    }
    return m_end;
}

void CostBreakdownItemModel::setEndDate(const QDate &date)
{
    beginResetModel();
    m_end = date;
    endResetModel();
}

bool CostBreakdownItemModel::cumulative() const
{
    return m_cumulative;
}

void CostBreakdownItemModel::setCumulative(bool on)
{
    beginResetModel();
    m_cumulative = on;
    endResetModel();
}

int CostBreakdownItemModel::showMode() const
{
    return m_showmode;
}
void CostBreakdownItemModel::setShowMode(int show)
{
    m_showmode = show;
}

QVariant CostBreakdownItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal) {
        if (role == Qt::DisplayRole) {
            switch (section) {
                case Name: return i18n("Name");
                case Description: return i18n("Description");
                case Total: return i18n("Total");
                case Planned: return i18n("Planned");
                case Actual: return i18n("Actual");
                default: break;
            }
            int col = section - propertyCount();
            switch (m_periodtype) {
                case Period_Day: {
                    return startDate().addDays(col).toString(Qt::ISODate);
                }
                case Period_Week: {
                    const auto date = startDate().addDays((col) * 7);
                    int week = date.weekNumber();
                    const auto y = date.toString(QStringLiteral("yyyy"));
                    return i18nc("@title:column week-year", "%1-%2", week, y);
                }
                case Period_Month: {
                    int days = startDate().daysInMonth() - startDate().day() + 1;
                    QDate start = startDate();
                    for (int i = 0; i < col; ++i) {
                        start = start.addDays(days);
                        days = start.daysInMonth();
                    }
                    return start.toString(QStringLiteral("MMM-yyyy"));
                }
                default:
                    return section;
                    break;
            }
            return QVariant();
        }
        if (role == Qt::EditRole) {
            switch (section) {
                case Name: return QStringLiteral("Name");
                case Description: return QStringLiteral("Description");
                case Total: return QStringLiteral("Total");
                case Planned: return QStringLiteral("Planned");
                case Actual: return QStringLiteral("Actual");
                default: break;
            }
            int col = section - propertyCount();
            switch (m_periodtype) {
                case Period_Day: {
                    return startDate().addDays(col);
                }
                case Period_Week: {
                    return startDate().addDays((col) * 7).weekNumber();
                }
                case Period_Month: {
                    int days = startDate().daysInMonth() - startDate().day() + 1;
                    QDate start = startDate();
                    for (int i = 0; i < col; ++i) {
                        start = start.addDays(days);
                        days = start.daysInMonth();
                    }
                    return start.month();
                }
                default:
                    return section;
                    break;
            }
            return QVariant();
        }
        if (role == Qt::ToolTipRole) {
            switch (section) {
                case Name: return ToolTip::accountName();
                case Description: return ToolTip::accountDescription();
                case Total: return i18n("The total cost for the account shown as: Actual cost [ Planned cost ]");
                case Planned:
                case Actual:
                default: {
                    int col = section - propertyCount();
                    switch (m_periodtype) {
                        case Period_Day: {
                            return startDate().addDays(col).toString();
                        }
                        case Period_Week: {
                            const auto date = startDate().addDays((col) * 7);
                            int week = date.weekNumber();
                            const auto y = date.toString(QStringLiteral("yyyy"));
                            return i18nc("@info:tooltip week-year", "Week %1-%2", week, y);
                        }
                        case Period_Month: {
                            int days = startDate().daysInMonth() - startDate().day() + 1;
                            QDate start = startDate();
                            for (int i = 0; i < col; ++i) {
                                start = start.addDays(days);
                                days = start.daysInMonth();
                            }
                            return start.toString(QStringLiteral("MMMM-yyyy"));
                        }
                        default:
                            return section;
                            break;
                    }
                    return QVariant();
                }
            }
        }
        if (role == Qt::TextAlignmentRole) {
            switch (section) {
                case Name: return QVariant();
                case Description: return QVariant();
                default: return (int)(Qt::AlignRight|Qt::AlignVCenter);
            }
            return QVariant();
        }
    }
    return ItemModelBase::headerData(section, orientation, role);
}

Account *CostBreakdownItemModel::account(const QModelIndex &index) const
{
    return static_cast<Account*>(index.internalPointer());
}

void CostBreakdownItemModel::slotAccountChanged(Account *account)
{
    Q_UNUSED(account);
    fetchData();
    QMap<Account*, EffortCostMap>::const_iterator it;
    for (it = m_plannedCostMap.constBegin(); it != m_plannedCostMap.constEnd(); ++it) {
        QModelIndex idx1 = index(it.key());
        QModelIndex idx2 = index(idx1.row(), columnCount() - 1, parent(idx1));
        //debugPlan<<a->name()<<idx1<<idx2;
        Q_EMIT dataChanged(idx1, idx2);
    }
}


} // namespace KPlato
