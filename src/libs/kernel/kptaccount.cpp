/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2005, 2006, 2007, 2012 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kptaccount.h"

#include "kptduration.h"
#include "kptproject.h"
#include "kptdebug.h"

#include <KoXmlReader.h>

#include <KLocalizedString>

#include <QDate>

namespace KPlato
{

Account::Account()
    : m_name(),
      m_description(),
      m_list(nullptr),
      m_parent(nullptr),
      m_accountList(),
      m_costPlaces() {
    
}

Account::Account(const QString& name, const QString& description)
    : m_name(name),
      m_description(description),
      m_list(nullptr),
      m_parent(nullptr),
      m_accountList(),
      m_costPlaces() {
    
}

Account::~Account() {
    if (m_list) {
        m_list->accountDeleted(this); // default account
    }
    while (!m_accountList.isEmpty()) {
        delete m_accountList.takeFirst();
    }
    while (!m_costPlaces.isEmpty()) {
        delete m_costPlaces.takeFirst();
    }
    take(this);
}
    
bool Account::isDefaultAccount() const
{
    return m_list == nullptr ? false : m_list->defaultAccount() == this;
}

void Account::changed() {
    if (m_list) {
        m_list->accountChanged(this);
    }
}

void Account::setName(const QString& name) {
    if (findAccount() == this) {
        removeId();
    }
    m_name = name;
    insertId();
    changed();
}

void Account::setDescription(const QString& desc)
{
    m_description = desc;
    changed();
}

void Account::insert(Account *account, int index) {
    Q_ASSERT(account);
    int i = index == -1 ? m_accountList.count() : index;
    m_accountList.insert(i, account);
    account->setList(m_list);
    account->setParent(this);
    insertId(account);
    account->insertChildren();
}

void Account::insertChildren() {
    for (Account *a : std::as_const(m_accountList)) {
        a->setList(m_list);
        a->setParent(this);
        insertId(a);
        a->insertChildren();
    }
}

void Account::take(Account *account) {
    if (account == nullptr) {
        return;
    }
    if (account->parent()) {
        if (account->m_list) {
            // emits remove signals,
            // sets account->m_list = 0,
            // calls us again
            account->m_list->take(account);
        } else {
            // child account has been removed from Accounts
            // so now we can remove child account from us
            // if still there
            if (m_accountList.contains(account)) {
                m_accountList.removeAt(m_accountList.indexOf(account));
            }
        }
    } else if (account->m_list) {
        // we are top level so just needs Accounts to remove us
        account->m_list->take(account);
    }
    //debugPlan<<account->name();
}

bool Account::isChildOf(const Account *account) const
{
    if (m_parent == nullptr) {
        return false;
    }
    if (m_parent == account) {
        return true;
    }
    return  m_parent->isChildOf(account);
}

bool Account::isBaselined(long id) const
{
    for (CostPlace *p : std::as_const(m_costPlaces)) {
        if (p->isBaselined(id)) {
            return true;
        }
    }
    return false;
}

bool Account::load(KoXmlElement &element, Project &project) {
    m_name = element.attribute(QStringLiteral("name"));
    m_description = element.attribute(QStringLiteral("description"));
    KoXmlNode n = element.firstChild();
    for (; ! n.isNull(); n = n.nextSibling()) {
        if (! n.isElement()) {
            continue;
        }
        KoXmlElement e = n.toElement();
        if (e.tagName() == QStringLiteral("costplace")) {
            Account::CostPlace *child = new Account::CostPlace(this);
            if (child->load(e, project)) {
                append(child);
            } else {
                delete child;
            }
        } else if (e.tagName() == QStringLiteral("account")) {
            Account *child = new Account();
            if (child->load(e, project)) {
                m_accountList.append(child);
            } else {
                // TODO: Complain about this
                warnPlan<<"Loading failed";
                delete child;
            }
        }
    }
    return true;
}

void Account::save(QDomElement &element) const {
    QDomElement me = element.ownerDocument().createElement(QStringLiteral("account"));
    element.appendChild(me);
    me.setAttribute(QStringLiteral("name"), m_name);
    me.setAttribute(QStringLiteral("description"), m_description);
    for (Account::CostPlace *cp : std::as_const(m_costPlaces)) {
        cp->save(me);
    }
    for (Account *a : std::as_const(m_accountList)) {
        a->save(me);
    }
}

Account::CostPlace *Account::findCostPlace(const Resource &resource) const
{
    for (Account::CostPlace *cp : std::as_const(m_costPlaces)) {
        if (&resource == cp->resource()) {
            return cp;
        }
    }
    return nullptr;
}

Account::CostPlace *Account::findRunning(const Resource &resource) const {
    for (Account::CostPlace *cp : std::as_const(m_costPlaces)) {
        if (&resource == cp->resource() && cp->running()) {
            return cp;
        }
    }
    return nullptr;
}

void Account::removeRunning(const Resource &resource) {
    Account::CostPlace *cp = findRunning(resource);
    if (cp && cp->running()) {
        cp->setRunning(false);
        if (cp->isEmpty()) {
            deleteCostPlace(cp);
        }
    }
}

void Account::addRunning(Resource &resource) {
    Account::CostPlace *cp = findRunning(resource);
    if (cp) {
        cp->setRunning(true);
        changed();
        return;
    }
    cp = new CostPlace(this, &resource, true);
    append(cp);
    changed();
}

Account::CostPlace *Account::findCostPlace(const Node &node) const
{
    for (Account::CostPlace *cp : std::as_const(m_costPlaces)) {
        if (&node == cp->node()) {
            return cp;
        }
    }
    return nullptr;
}

Account::CostPlace *Account::findRunning(const Node &node) const
{
    for (CostPlace *cp : m_costPlaces) {
        if (cp->node() == &node && cp->running()) {
            return cp;
        }
    }
    return nullptr;
}

void Account::removeRunning(const Node &node) {
    CostPlace *cp = findRunning(node);
    if (cp) {
        cp->setRunning(false);
        if (cp->isEmpty()) {
            deleteCostPlace(cp);
        }
        changed();
    }
}

void Account::addRunning(Node &node) {
    Account::CostPlace *cp = findCostPlace(node);
    if (cp) {
        cp->setRunning(true);
        changed();
        return;
    }
    cp = new CostPlace(this, &node, true);
    append(cp);
    changed();
}


Account::CostPlace *Account::findStartup(const Node &node) const {
    for (CostPlace *cp : m_costPlaces) {
        if (cp->node() == &node && cp->startup()) {
            return cp;
        }
    }
    return nullptr;
}

void Account::removeStartup(const Node &node) {
    Account::CostPlace *cp = findStartup(node);
    if (cp) {
        cp->setStartup(false);
        if (cp->isEmpty()) {
            deleteCostPlace(cp);
        }
        changed();
    }
}

void Account::addStartup(Node &node) {
    Account::CostPlace *cp = findCostPlace(node);
    if (cp) {
        cp->setStartup(true);
        changed();
        return;
    }
    cp = new CostPlace(this, &node, false, true, false);
    append(cp);
    changed();
}

Account::CostPlace *Account::findShutdown(const Node &node) const {
    for (CostPlace *cp : m_costPlaces) {
        if (cp->node() == &node && cp->shutdown()) {
            return cp;
        }
    }
    return nullptr;
}

void Account::removeShutdown(const Node &node) {
    Account::CostPlace *cp = findShutdown(node);
    if (cp) {
        cp->setShutdown(false);
        if (cp->isEmpty()) {
            deleteCostPlace(cp);
        }
        changed();
    }
}

void Account::addShutdown(Node &node) {
    Account::CostPlace *cp = findCostPlace(node);
    if (cp) {
        cp->setShutdown(true);
        changed();
        return;
    }
    cp = new CostPlace(this, &node, false, false, true);
    append(cp);
    changed();
}

Account *Account::findAccount(const QString &id) const {
    if (m_list) 
        return m_list->findAccount(id);
    return nullptr;
}

bool Account::removeId(const QString &id) {
    return (m_list ? m_list->removeId(id) : false);
}

bool Account::insertId() {
    return insertId(this);
}

bool Account::insertId(Account *account) {
    return (m_list ? m_list->insertId(account) : false);
}

void Account::deleteCostPlace(CostPlace *cp) {
    //debugPlan;
    int i = m_costPlaces.indexOf(cp);
    if (i != -1)
        m_costPlaces.removeAt(i);
    delete cp;
}

EffortCostMap Account::plannedCost(long id) const
{
    return plannedCost(QDate(), QDate(), id);
}

EffortCostMap Account::plannedCost(const QDate &start, const QDate &end, long id) const {
    EffortCostMap ec;
    if (! isElement()) {
        for (Account *a : std::as_const(m_accountList)) {
            ec += a->plannedCost(start, end, id);
        }
    }
    for (Account::CostPlace *cp : std::as_const(m_costPlaces)) {
        ec += plannedCost(*cp, start, end, id);
    }
    if (isDefaultAccount()) {
        const QList<Node*> list = m_list == nullptr ? QList<Node*>() : m_list->allNodes();
        for (Node *n : list) {
            if (n->numChildren() > 0) {
                continue;
            }
            if (n->runningAccount() == nullptr) {
                ec += n->plannedEffortCostPrDay(start, end, id);
            }
            if (n->startupAccount() == nullptr) {
                if ((! start.isValid() || n->startTime(id).date() >= start) &&
                     (! end.isValid() || n->startTime(id).date() <= end)) {
                    ec.add(n->startTime(id).date(), EffortCost(Duration::zeroDuration, n->startupCost()));
                }
            }
            if (n->shutdownAccount() == nullptr) {
                if ((! start.isValid() || n->endTime(id).date() >= start) &&
                     (! end.isValid() || n->endTime(id).date() <= end)) {
                    ec.add(n->endTime(id).date(), EffortCost(Duration::zeroDuration, n->shutdownCost()));
                }
            }
        }
    }
    return ec;
}

EffortCostMap Account::plannedCost(const Account::CostPlace &cp, const QDate &start, const QDate &end, long id) const
{
    EffortCostMap ec;
    if (cp.node()) {
        Node &node = *(cp.node());
        //debugPlan<<"n="<<n->name();
        if (cp.running()) {
            ec += node.plannedEffortCostPrDay(start, end, id);
        }
        if (cp.startup()) {
            if ((! start.isValid() || node.startTime(id).date() >= start) &&
                 (! end.isValid() || node.startTime(id).date() <= end)) {
                ec.add(node.startTime(id).date(), EffortCost(Duration::zeroDuration, node.startupCost()));
            }
        }
        if (cp.shutdown()) {
            if ((! start.isValid() || node.endTime(id).date() >= start) &&
                 (! end.isValid() || node.endTime(id).date() <= end)) {
                ec.add(node.endTime(id).date(), EffortCost(Duration::zeroDuration, node.shutdownCost()));
            }
        }
    } else if (cp.resource()) {
        if (cp.running()) {
            ec += cp.resource()->plannedEffortCostPrDay(start, end, id);
        }
    }
    return ec;
}

EffortCostMap Account::actualCost(long id) const
{
    return actualCost(QDate(), QDate(), id);
}

EffortCostMap Account::actualCost(const QDate &start, const QDate &end, long id) const
{
    EffortCostMap ec;
    if (! isElement()) {
        for (Account *a : std::as_const(m_accountList)) {
            ec += a->actualCost(start, end, id);
        }
    }
    const auto costs = costPlaces();
    for (Account::CostPlace *cp : costs) {
        ec += actualCost(*cp, start, end, id);
    }
    if (isDefaultAccount()) {
        const QList<Node*> list = m_list == nullptr ? QList<Node*>() : m_list->allNodes();
        for (Node *n : list) {
            if (n->numChildren() > 0) {
                continue;
            }
            if (n->runningAccount() == nullptr) {
                //debugPlan<<"default, running:"<<n->name();
                ec += n->actualEffortCostPrDay(start, end, id);
            }
            Task *t = dynamic_cast<Task*>(n); // only tasks have completion
            if (t) {
                if (n->startupAccount() == nullptr && t->completion().isStarted()) {
                    const QDate startDate = t->completion().startTime().date();
                    if ((! start.isValid() || startDate >= start) &&
                        (! end.isValid() || startDate <= end)) {
                        ec.add(startDate, EffortCost(Duration::zeroDuration, n->startupCost()));
                    }
                }
                if (n->shutdownAccount() == nullptr && t->completion().isFinished()) {
                    //debugPlan<<"default, shutdown:"<<n->name();
                    const QDate finishDate = t->completion().finishTime().date();
                    if ((! start.isValid() || finishDate >= start) &&
                        (! end.isValid() || finishDate <= end)) {
                        ec.add(finishDate, EffortCost(Duration::zeroDuration, n->shutdownCost()));
                    }
                }
            }
        }
    }
    return ec;
}

EffortCostMap Account::actualCost(const Account::CostPlace &cp, const QDate &start, const QDate &end, long id) const
{
    EffortCostMap ec;
    if (cp.node()) {
        Node &node = *(cp.node());
        if (cp.running()) {
            ec += node.actualEffortCostPrDay(start, end, id);
        }
        Task *t = dynamic_cast<Task*>(&node); // only tasks have completion
        if (t) {
            if (cp.startup() && t->completion().isStarted()) {
                const QDate startDate = t->completion().startTime().date();
                if ((! start.isValid() || startDate >= start) &&
                    (! end.isValid() || startDate <= end)) {
                    ec.add(startDate, EffortCost(Duration::zeroDuration, node.startupCost()));
                }
            }
            if (cp.shutdown() && t->completion().isFinished()) {
                const QDate finishDate = t->completion().finishTime().date();
                if ((! start.isValid() || finishDate >= start) &&
                    (! end.isValid() || finishDate <= end)) {
                    ec.add(finishDate, EffortCost(Duration::zeroDuration, node.shutdownCost()));
                }
            }
        }
    } else if (cp.resource() && m_list) {
        const auto nodes = m_list->allNodes();
        for (Node *n : nodes) {
            if (n->type() == Node::Type_Task) {
                ec += n->actualEffortCostPrDay(cp.resource(), start, end, id);
            }
        }
    }
    return ec;
}


//------------------------------------
Account::CostPlace::CostPlace(Account *acc, Node *node, bool running, bool strtup, bool shutdown)
    : m_account(acc), 
    m_objectId(node->id()),
    m_node(node),
    m_resource(nullptr),
    m_running(false),
    m_startup(false),
    m_shutdown(false)
{
    if (node) {
        if (running) setRunning(running);
        if (strtup) setStartup(strtup);
        if (shutdown) setShutdown(shutdown);
    }
}

Account::CostPlace::CostPlace(Account *acc, Resource *resource, bool running)
    : m_account(acc), 
    m_objectId(resource->id()),
    m_node(nullptr),
    m_resource(resource),
    m_running(false),
    m_startup(false),
    m_shutdown(false)
{
    if (resource) {
        if (running) setRunning(running);
    }
}

Account::CostPlace::~CostPlace() {

    if (m_node) {
        if (m_running) {
            m_running = false;
            m_node->setRunningAccount(nullptr);
        }
        if (m_startup) {
            m_startup = false;
            m_node->setStartupAccount(nullptr);
        }
        if (m_shutdown) {
            m_shutdown = false;
            m_node->setShutdownAccount(nullptr);
        }
    }
    if (m_resource) {
        if (m_running) {
            m_running = false;
            m_resource->setAccount(nullptr);
        }
    }
}

bool Account::CostPlace::isBaselined(long id) const
{
    if (m_node) {
        if (m_running) {
            if (m_node->isBaselined(id)) {
                return true;
            }
        }
        if (m_startup) {
            if (m_node->isBaselined(id)) {
                return true;
            }
        }
        if (m_shutdown) {
            if (m_node->isBaselined(id)) {
                return true;
            }
        }
    }
    if (m_resource) {
        if (m_running) {
            if (m_resource->isBaselined(id)) {
                return true;
            }
        }
    }
    return false;
}

void Account::CostPlace::setNode(Node* node)
{
    Q_ASSERT(! m_node);
    m_node = node;
}

void Account::CostPlace::setResource(Resource* resource)
{
    Q_ASSERT(! m_resource);
    m_resource = resource;
}

void Account::CostPlace::setRunning(bool on)
{
    if (m_running == on) {
        return;
    }
    m_running = on;
    if (m_node) {
        m_node->setRunningAccount(on ? m_account : nullptr);
    } else if (m_resource) {
        m_resource->setAccount(on ? m_account : nullptr);
    }
}

void Account::CostPlace::setStartup(bool on)
{
    if (m_startup == on) {
        return;
    }
    m_startup = on;
    if (m_node)
        m_node->setStartupAccount(on ? m_account : nullptr);
}

void Account::CostPlace::setShutdown(bool on)
{
    if (m_shutdown == on) {
        return;
    }
    m_shutdown = on;
    if (m_node)
        m_node->setShutdownAccount(on ? m_account : nullptr);
}

//TODO
bool Account::CostPlace::load(KoXmlElement &element, Project &project) {
    //debugPlan;
    m_objectId = element.attribute(QStringLiteral("object-id"));
    if (m_objectId.isEmpty()) {
        // check old format
        m_objectId = element.attribute(QStringLiteral("node-id"));
        if (m_objectId.isEmpty()) {
            errorPlan<<"No object id";
            return false;
        }
    }
    m_node = project.findNode(m_objectId);
    if (m_node == nullptr) {
        m_resource = project.findResource(m_objectId);
        if (m_resource == nullptr) {
            errorPlan<<"Cannot find object with id: "<<m_objectId;
            return false;
        }
    }
    bool on = (bool)(element.attribute(QStringLiteral("running-cost")).toInt());
    if (on) setRunning(on);
    on = (bool)(element.attribute(QStringLiteral("startup-cost")).toInt());
    if (on) setStartup(on);
    on = (bool)(element.attribute(QStringLiteral("shutdown-cost")).toInt());
    if (on) setShutdown(on);
    return true;
}

void Account::CostPlace::save(QDomElement &element) const {
    //debugPlan;
    QDomElement me = element.ownerDocument().createElement(QStringLiteral("costplace"));
    element.appendChild(me);
    me.setAttribute(QStringLiteral("object-id"), m_objectId);
    me.setAttribute(QStringLiteral("running-cost"), QString::number(m_running));
    me.setAttribute(QStringLiteral("startup-cost"), QString::number(m_startup));
    me.setAttribute(QStringLiteral("shutdown-cost"), QString::number(m_shutdown));
    
}

void Account::CostPlace::setObjectId(const QString& id)
{
    m_objectId = id;
}

QString Account::CostPlace::objectId() const
{
    return m_objectId;
}

//---------------------------------
Accounts::Accounts(Project &project)
    : m_project(project),
      m_accountList(),
      m_idDict(),
      m_defaultAccount(nullptr) {
      
}

Accounts::~Accounts() {
    //debugPlan;
    m_defaultAccount = nullptr;
    while (!m_accountList.isEmpty()) {
        delete m_accountList.takeFirst();
    }
}

EffortCostMap Accounts::plannedCost(const Account &account, long id) const
{
    return account.plannedCost(id);
}

EffortCostMap Accounts::plannedCost(const Account &account, const QDate &start, const QDate &end, long id) const
{
    return account.plannedCost(start, end, id);
}

EffortCostMap Accounts::actualCost(const Account &account, long id) const
{
    return account.actualCost(id);
}

EffortCostMap Accounts::actualCost(const Account &account, const QDate &start, const QDate &end, long id) const
{
    return account.actualCost(start, end, id);
}

void Accounts::insert(Account *account, Account *parent, int index) {
    Q_ASSERT(account);
    if (parent == nullptr) {
        int i = index == -1 ? m_accountList.count() : index;
        Q_EMIT accountToBeAdded(parent, i);
        m_accountList.insert(i, account);
        account->setList(this);
        account->setParent(nullptr); // incase...
        insertId(account);
        account->insertChildren();
    } else {
        int i = index == -1 ? parent->accountList().count() : index;
        Q_EMIT accountToBeAdded(parent, i);
        parent->insert(account, i);
    }
    //debugPlan<<account->name();
    Q_EMIT accountAdded(account);
}

void Accounts::take(Account *account){
    if (account == nullptr) {
        return;
    }
    account->m_list = nullptr;
    removeId(account->name());
    if (account->parent()) {
        Q_EMIT accountToBeRemoved(account);
        account->parent()->take(account);
        Q_EMIT accountRemoved(account);
        //debugPlan<<account->name();
        return;
    }
    int i = m_accountList.indexOf(account);
    if (i != -1) {
        Q_EMIT accountToBeRemoved(account);
        m_accountList.removeAt(i);
        Q_EMIT accountRemoved(account);
    }
    //debugPlan<<account->name();
}
    
bool Accounts::load(KoXmlElement &element, Project &project) {
    KoXmlNode n = element.firstChild();
    for (; ! n.isNull(); n = n.nextSibling()) {
        if (! n.isElement()) {
            continue;
        }
        KoXmlElement e = n.toElement();
        if (e.tagName() == QStringLiteral("account")) {
            Account *child = new Account();
            if (child->load(e, project)) {
                insert(child);
            } else {
                // TODO: Complain about this
                warnPlan<<"Loading failed";
                delete child;
            }
        }
    }
    if (element.hasAttribute(QStringLiteral("default-account"))) {
        m_defaultAccount = findAccount(element.attribute(QStringLiteral("default-account")));
        if (m_defaultAccount == nullptr) {
            warnPlan<<"Could not find default account.";
        }
    }
    return true;
}

void Accounts::save(QDomElement &element) const {
    QDomElement me = element.ownerDocument().createElement(QStringLiteral("accounts"));
    element.appendChild(me);
    if (m_defaultAccount) {
        me.setAttribute(QStringLiteral("default-account"), m_defaultAccount->name());
    }
    for (Account *a : std::as_const(m_accountList)) {
        a->save(me);
    }
}

QStringList Accounts::costElements() const {
    QStringList l;
    const auto keys = m_idDict.keys();
    for (const QString &key : keys) {
        if (m_idDict[key]->isElement())
            l << key;
    }
    return l;
}
    

QStringList Accounts::nameList() const {
    return m_idDict.keys();
}

Account *Accounts::findRunningAccount(const Resource &resource) const {
    const auto  accounts = m_idDict.values();
    for (Account *a : accounts) {
        if (a->findRunning(resource)) {
            return a;
        }
    }
    return nullptr;
}

Account *Accounts::findRunningAccount(const Node &node) const {
    const auto  accounts = m_idDict.values();
    for (Account *a : accounts) {
        if (a->findRunning(node))
            return a;
    }
    return nullptr;
}

Account *Accounts::findStartupAccount(const Node &node) const {
    const auto  accounts = m_idDict.values();
    for (Account *a : accounts) {
        if (a->findStartup(node))
            return a;
    }
    return nullptr;
}

Account *Accounts::findShutdownAccount(const Node &node) const {
    const auto  accounts = m_idDict.values();
    for (Account *a : accounts) {
        if (a->findShutdown(node))
            return a;
    }
    return nullptr;
}

Account *Accounts::findAccount(const QString &id) const {
    return m_idDict.value(id);
}

bool Accounts::insertId(Account *account) {
    Q_ASSERT(account);
    Account *a = findAccount(account->name());
    if (a == nullptr) {
        //debugPlan<<"'"<<account->name()<<"' inserted";
        m_idDict.insert(account->name(), account);
        return true;
    }
    if (a == account) {
        debugPlan<<"'"<<a->name()<<"' already exists";
        return true;
    }
    //TODO: Create unique id?
    warnPlan<<"Insert failed, creating unique id";
    account->setName(uniqueId(account->name())); // setName() calls insertId !!
    return false;
}

bool Accounts::removeId(const QString &id) {
    bool res = m_idDict.remove(id);
    //debugPlan<<id<<": removed="<<res;
    return res;
}

QString Accounts::uniqueId(const QString &seed) const
{
    QString s = seed.isEmpty() ? i18n("Account") + QStringLiteral(".%1") : QString(seed + QStringLiteral(".%1"));
    int i = 1;
    QString n = s.arg(i);
    while (findAccount(n)) {
        n = s.arg(++i);
    }
    return n;
}

void Accounts::setDefaultAccount(Account *account)
{
    Account *a = m_defaultAccount;
    m_defaultAccount = account;
    if (a) {
        Q_EMIT changed(a);
    }
    if (account) {
        Q_EMIT changed(account);
    }
    if (a != account) {
        Q_EMIT defaultAccountChanged();
    }
}

void Accounts::accountDeleted(Account *account)
{
    if (account == m_defaultAccount) {
        m_defaultAccount = nullptr;
    }
}

void Accounts::accountChanged(Account *account) 
{
    Q_EMIT changed(account);
}

QList<Node*> Accounts::allNodes() const
{
    return m_project.allNodes();
}

#ifndef NDEBUG
void Accounts::printDebug(const QString& indent) {
    debugPlan<<indent<<"Accounts:"<<this<<m_accountList.count()<<" children";
    for(Account *a : std::as_const(m_accountList)) {
        a->printDebug(indent + QStringLiteral("    !"));
    }
}
void Account::printDebug(const QString& indent) {
    debugPlan<<indent<<"--- Account:"<<this<<m_name<<":"<<m_accountList.count()<<" children";
    for(Account *a : std::as_const(m_accountList)) {
        a->printDebug(indent + QStringLiteral("    !"));
    }
}
#endif
} //namespace KPlato
