/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2001 Thomas zander <zander@kde.org>
   SPDX-FileCopyrightText: 2002-2010, 2012 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/
// clazy:excludeall=qstring-arg
#include "kptnode.h"

#include "kptappointment.h"
#include "kptaccount.h"
#include "kptwbsdefinition.h"
#include "kptresource.h"
#include "kptschedule.h"
#include "kptxmlloaderobject.h"
#include "XmlSaveContext.h"
#include "kptdebug.h"

#include <KoXmlReader.h>

#include <KLocalizedString>

#include <QListIterator>


namespace KPlato
{

Node::Node(Node *parent) 
    : QObject(nullptr), // We don't use qobjects parent
      m_nodes(), m_dependChildNodes(), m_dependParentNodes(),
      m_estimate(nullptr),
      m_blockChanged(false)
{
    //debugPlan<<"("<<this<<")";
    m_parent = parent;
    init();
    m_id.clear(); // Not mapped
}

Node::Node(const Node &node, Node *parent) 
    : QObject(nullptr), // Don't set parent, we handle parent/child ourselves
      m_nodes(), 
      m_dependChildNodes(), 
      m_dependParentNodes(),
      m_estimate(nullptr),
      m_blockChanged(false)
{
    //debugPlan<<"("<<this<<")";
    m_parent = parent;
    init();
    m_name = node.name();
    m_leader = node.leader();
    m_description = node.description();
    m_constraint = (ConstraintType) node.constraint();
    m_constraintStartTime = node.constraintStartTime();
    m_constraintEndTime = node.constraintEndTime();
    
    m_runningAccount = node.runningAccount();
    m_startupAccount = node.startupAccount();
    m_shutdownAccount = node.shutdownAccount();

    m_startupCost = node.startupCost();
    m_shutdownCost = node.shutdownCost();
    
    setObjectName(node.objectName());
}

Node::~Node() {
    //debugPlan<<"("<<this<<")"<<m_name;
    delete m_estimate;
    while (!m_nodes.isEmpty())
        delete m_nodes.takeFirst();

    while (!m_dependParentNodes.isEmpty()) {
        delete m_dependParentNodes.value(0);
    }
    while (!m_dependChildNodes.isEmpty()) {
        delete m_dependChildNodes.value(0);
    }
    if (m_runningAccount)
        m_runningAccount->removeRunning(*this);
    if (m_startupAccount)
        m_startupAccount->removeStartup(*this);
    if (m_shutdownAccount)
        m_shutdownAccount->removeShutdown(*this);

    for (Schedule *s : std::as_const(m_schedules)) {
        delete s;
    }
    m_schedules.clear();
    m_parent = nullptr; //safety
}

void Node::init() {
    m_priority = 0;
    m_documents.node = this;
    m_currentSchedule = nullptr;
    m_constraint = Node::ASAP;
    m_estimate = nullptr;
    m_visitedForward = false;
    m_visitedBackward = false;
    
    m_runningAccount = nullptr;
    m_startupAccount = nullptr;
    m_shutdownAccount = nullptr;
    m_startupCost = 0.0;
    m_shutdownCost = 0.0;
}

void Node::setPriority(int priority)
{
    m_priority = priority;
    changed(PriorityProperty);
}

int Node::priority() const
{
    return m_priority;
}

ulong Node::granularity() const
{
    return parentNode() ? parentNode()->granularity() : 0;
}

QString Node::typeToString(bool trans) const
{
    return typeToString((Node::NodeTypes)type(), trans);
}

// static
QString Node::typeToString(Node::NodeTypes typ, bool trans)
{
    return typeToStringList(trans).at(typ);
}

// static
QStringList Node::typeToStringList(bool trans)
{
    return QStringList() 
            << (trans ? i18n("None") : QStringLiteral("None"))
            << (trans ? i18n("Project") : QStringLiteral("Project"))
            << (trans ? i18n("Sub-Project") : QStringLiteral("Sub-Project"))
            << (trans ? i18n("Task") : QStringLiteral("Task"))
            << (trans ? i18n("Milestone") : QStringLiteral("Milestone"))
            << (trans ? i18n("Periodic") : QStringLiteral("Periodic"))
            << (trans ? i18n("Summary") : QStringLiteral("Summary-Task"));
}

void Node::setName(const QString &n) 
{
#ifndef NDEBUG
    setObjectName(n);
#endif
     m_name = n;
     changed(this, NameProperty);
}

void Node::setLeader(const QString &l)
{
     m_leader = l;
     changed(this, LeaderProperty);
}

void Node::setDescription(const QString &d)
{
     m_description = d;
     changed(this, DescriptionProperty);
}

Node *Node::projectNode() {
    if ((type() == Type_Project) || (type() == Type_Subproject)) {
        return this;
    }
    if (m_parent)
        return m_parent->projectNode();

    // This happens for default tasks
    return nullptr;
}

const Node *Node::projectNode() const {
    if ((type() == Type_Project) || (type() == Type_Subproject)) {
        return this;
    }
    if (m_parent)
        return m_parent->projectNode();

    // This happens for default tasks
    return nullptr;
}

void Node::takeChildNode(Node *node) {
    //debugPlan<<"find="<<m_nodes.indexOf(node);
    int t = type();
    int i = m_nodes.indexOf(node);
    if (i != -1) {
        m_nodes.removeAt(i);
    }
    node->setParentNode(nullptr);
    if (t != type()) {
        changed(TypeProperty);
    }
}

void Node::takeChildNode(int number) {
    int t = type();
    if (number >= 0 && number < m_nodes.size()) {
        Node *n = m_nodes.takeAt(number);
        //debugPlan<<(n?n->id():"null")<<" :"<<(n?n->name():"");
        if (n) {
            n->setParentNode(nullptr);
        }
    }
    if (t != type()) {
        changed(TypeProperty);
    }
}

void Node::insertChildNode(int index, Node *node) {
    int t = type();
    if (index == -1)
        m_nodes.append(node);
    else
        m_nodes.insert(index,node);
    node->setParentNode(this);
    if (t != type()) {
        changed(TypeProperty);
    }
}

void Node::addChildNode(Node *node, Node *after) {
    int t = type();
    int index = m_nodes.indexOf(after);
    if (index == -1) {
        m_nodes.append(node);
        node->setParentNode(this);
        if (t != type()) {
            changed(TypeProperty);
        }
        return;
    }
    m_nodes.insert(index+1, node);
    node->setParentNode(this);
    if (t != type()) {
        changed(TypeProperty);
    }
}

int Node::findChildNode(const Node* node) const
{
    return m_nodes.indexOf(const_cast<Node*>(node));
}

bool Node::isChildOf(const Node* node) const
{
    if (node == nullptr || m_parent == nullptr) {
        return false;
    }
    if (node == m_parent) {
        return true;
    }
    return m_parent->isChildOf(node);
}


Node* Node::childNode(int number)
{
    //debugPlan<<number;
    return m_nodes.value(number);
}

const Node* Node::childNode(int number) const
{
    if (number < 0 || number >= m_nodes.count()) {
        return nullptr;
    }
    return m_nodes.at(number);
}

int Node::indexOf(const Node *node) const
{
    return m_nodes.indexOf(const_cast<Node*>(node));
}


Duration *Node::getDelay() {
    /* TODO
       Calculate the delay of this node. Use the calculated startTime and the set startTime.
    */
    return nullptr;
}

void Node::addDependChildNode(Node *node, Relation::Type p) {
    addDependChildNode(node,p,Duration());
}

void Node::addDependChildNode(Node *node, Relation::Type p, Duration lag) {
    Relation *relation = new Relation(this, node, p, lag);
    if (node->addDependParentNode(relation))
        m_dependChildNodes.append(relation);
    else
        delete relation;
}

void Node::insertDependChildNode(unsigned int index, Node *node, Relation::Type p) {
    Relation *relation = new Relation(this, node, p, Duration());
    if (node->addDependParentNode(relation))
        m_dependChildNodes.insert(index, relation);
    else
        delete relation;
}

bool Node::addDependChildNode(Relation *relation) {
    if(m_dependChildNodes.indexOf(relation) != -1)
        return false;
    m_dependChildNodes.append(relation);
    return true;
}

void Node::takeDependChildNode(Relation *rel) {
    int i = m_dependChildNodes.indexOf(rel);
    if (i != -1) {
        //debugPlan<<m_name<<": ("<<rel<<")";
        m_dependChildNodes.removeAt(i);
    }
}

void Node::addDependParentNode(Node *node, Relation::Type p) {
    addDependParentNode(node,p,Duration());
}

void Node::addDependParentNode(Node *node, Relation::Type p, Duration lag) {
    Relation *relation = new Relation(node, this, p, lag);
    if (node->addDependChildNode(relation))
        m_dependParentNodes.append(relation);
    else
        delete relation;
}

void Node::insertDependParentNode(unsigned int index, Node *node, Relation::Type p) {
    Relation *relation = new Relation(this, node, p, Duration());
    if (node->addDependChildNode(relation))
        m_dependParentNodes.insert(index,relation);
    else
        delete relation;
}

bool Node::addDependParentNode(Relation *relation) {
    if(m_dependParentNodes.indexOf(relation) != -1)
        return false;
    m_dependParentNodes.append(relation);
    return true;
}

void Node::takeDependParentNode(Relation *rel) {
    int i = m_dependParentNodes.indexOf(rel);
    if (i != -1) {
        //debugPlan<<m_name<<": ("<<rel<<")";
        m_dependParentNodes.removeAt(i);
    }
}

bool Node::isParentOf(const Node *node) const
{
    if (m_nodes.indexOf(const_cast<Node*>(node)) != -1)
        return true;

    QListIterator<Node*> nit(childNodeIterator());
    while (nit.hasNext()) {
        if (nit.next()->isParentOf(node))
            return true;
    }
    return false;
}

Relation *Node::findParentRelation(const Node *node) const
{
    for (int i=0; i<numDependParentNodes(); i++) {
        Relation *rel = getDependParentNode(i);
        if (rel->parent() == node)
            return rel;
    }
    return (Relation *)nullptr;
}

Relation *Node::findChildRelation(const Node *node) const
{
    for (int i=0; i<numDependChildNodes(); i++) {
        Relation *rel = getDependChildNode(i);
        if (rel->child() == node)
            return rel;
    }
    return (Relation *)nullptr;
}

Relation *Node::findRelation(const Node *node) const
{
    Relation *rel = findParentRelation(node);
    if (!rel)
        rel = findChildRelation(node);
    return rel;
}

bool Node::isDependChildOf(const Node *node) const
{
    return isSuccessorof(node, false, true);
}

bool Node::isSuccessorof(const Node *node, bool checkProxies, bool checkSummarytasks) const
{
    //debugPlan<<this<<"checking against"<<node;
    if (!checkSummarytasks && (type() == Type_Summarytask || node->type() == Type_Summarytask)) {
        //debugPlan<<this<<type()<<"skip check"<<node<<node->type();
        return false;
    }
    //debugPlan<<this<<type()<<"checking against"<<node<<node->type();
    for (const auto r : std::as_const(m_dependParentNodes)) {
        if (r->parent() == node) {
            //debugPlan<<"deps"<<r<<true;
            return true;
        }
        if (r->parent()->isSuccessorof(node, checkProxies, checkSummarytasks)) {
            //debugPlan<<"deps"<<true;
            return true;
        }
    }
    if (checkProxies) {
        for (const auto r : std::as_const(m_parentProxyRelations)) {
            if (r->parent() == node) {
                //debugPlan<<"proxy"<<true<<r;
                return true;
            }
            if (r->parent()->isSuccessorof(node, checkProxies, checkSummarytasks)) {
                //debugPlan<<"proxy"<<true;
                return true;
            }
        }
    }
    //debugPlan<<this<<"->"<<node<<false<<m_dependParentNodes<<"proxy"<<m_parentProxyRelations;;
    return false;
}

bool Node::isPredecessorof(const Node *node, bool checkProxies, bool checkSummarytasks) const
{
    if (!checkSummarytasks && (type() == Type_Summarytask || node->type() == Type_Summarytask)) {
        //debugPlan<<this<<type()<<"skip check"<<node<<node->type();
        return false;
    }
    //debugPlan<<this<<type()<<"checking against"<<node<<node->type();
    for (const auto r : m_dependChildNodes) {
        if (r->child() == node) {
            //debugPlan<<"deps"<<true<<r;
            return true;
        }
        if (r->child()->isPredecessorof(node, checkProxies, checkSummarytasks)) {
            //debugPlan<<"deps"<<true;
            return true;
        }
    }
    if (checkProxies) {
        for (const auto r : std::as_const(m_childProxyRelations)) {
            if (r->child() == node) {
                //debugPlan<<"proxy"<<true<<r;
                return true;
            }
            if (r->child()->isPredecessorof(node, checkProxies, checkSummarytasks)) {
                //debugPlan<<"proxy"<<true;
                return true;
            }
        }
    }
    //debugPlan<<this<<"->"<<node<<false<<"dep:"<<m_dependChildNodes<<"proxy"<<m_childProxyRelations;
    return false;
}

QList<Node*> Node::getParentNodes()
{
    this->m_parentNodes.clear();
    const QList<Relation*> relations = this->dependParentNodes();
    for (Relation * currentRelation : relations)
    {
        if (!this->m_parentNodes.contains(currentRelation->parent())) 
        {
            this->m_parentNodes.append(currentRelation->parent());
        }
    }
    return this->m_parentNodes;
}

bool Node::canMoveTo(const Node *newParent) const
{
    if (m_parent == newParent) {
        return true;
    }
    if (newParent->isChildOf(this)) {
        return false;
    }
    if (isDependChildOf(newParent) || newParent->isDependChildOf(this)) {
        debugPlan<<"Can't move, node is dependent on new parent";
        return false;
    }
    for (Node *n : std::as_const(m_nodes)) {
        if (!n->canMoveTo(newParent)) {
            return false;
        }
    }
    return true;
}

void Node::makeAppointments() {
    QListIterator<Node*> nit(m_nodes);
    while (nit.hasNext()) {
        nit.next()->makeAppointments();
    }
}

void Node::calcResourceOverbooked() {
    QListIterator<Node*> nit(m_nodes);
    while (nit.hasNext()) {
        nit.next()->calcResourceOverbooked();
    }
}

// Returns the (previously) calculated duration
Duration Node::duration(long id) const
{
    Schedule *s = schedule(id);
    return s ? s->duration : Duration::zeroDuration;
}

double Node::variance(long id, Duration::Unit unit) const
{
    double d = deviation(id, unit);
    return d * d;
}

double Node::deviation(long id, Duration::Unit unit) const
{
    Schedule *s = schedule(id);
    double d = 0.0;
    if (s && m_estimate) {
        d = s->duration.toDouble(unit);
        double o = (d *  (100 + m_estimate->optimisticRatio())) / 100;
        double p = (d * (100 + m_estimate->pessimisticRatio())) / 100;
        d =  (p - o) / 6;
    }
    return d;
}

DateTime Node::startTime(long id) const
{
    Schedule *s = schedule(id);
    return s ? s->startTime : DateTime();
}
DateTime Node::endTime(long id) const
{
    Schedule *s = schedule(id);
    return s ? s->endTime : DateTime();
}

DateTime Node::appointmentStartTime(long id) const
{
    Schedule *s = schedule(id);
    return s ? s->appointmentStartTime() : DateTime();
}
DateTime Node::appointmentEndTime(long id) const
{
    Schedule *s = schedule(id);
    return s ? s->appointmentEndTime() : DateTime();
}

void Node::setDuration(const Duration &duration, long id)
{
    Schedule *s = schedule(id);
    if (s) {
        s->duration = duration;
    }
}

void Node::setEarlyStart(const DateTime &dt, long id)
{
    Schedule *s = schedule(id);
    if (s) s->earlyStart = dt;
}

DateTime Node::earlyStart(long id) const
{
    Schedule *s = schedule(id);
    return s ? s->earlyStart : DateTime();
}

void Node::setLateStart(const DateTime &dt, long id)
{
    Schedule *s = schedule(id);
    if (s) s->lateStart = dt;
}

DateTime Node::lateStart(long id) const
{
    Schedule *s = schedule(id);
    return s ? s->lateStart : DateTime();
}
    
void Node::setEarlyFinish(const DateTime &dt, long id)
{
    Schedule *s = schedule(id);
    if (s) s->earlyFinish = dt;
}

DateTime Node::earlyFinish(long id) const
{
    Schedule *s = schedule(id);
    return s ? s->earlyFinish : DateTime();
}

void Node::setLateFinish(const DateTime &dt, long id)
{
    Schedule *s = schedule(id);
    if (s) s->lateFinish = dt;
}

DateTime Node::lateFinish(long id) const
{
    Schedule *s = schedule(id);
    return s ? s->lateFinish : DateTime();
}
    
DateTime Node::workStartTime(long id) const
{
    Schedule *s = schedule(id);
    return s ? s->workStartTime : DateTime();
}

void Node::setWorkStartTime(const DateTime &dt, long id)
{
    Schedule *s = schedule(id);
    if (s) s->workStartTime = dt;
}

DateTime Node::workEndTime(long id) const
{
    Schedule *s = schedule(id);
    return s ? s->workEndTime : DateTime();
}

void Node::setWorkEndTime(const DateTime &dt, long id)
{
    Schedule *s = schedule(id);
    if (s) s->workEndTime = dt;
}
    
bool Node::inCriticalPath(long id) const
{
    Schedule *s = schedule(id);
    return s ? s->inCriticalPath : false;
}

QStringList Node::schedulingStatus(long id, bool trans) const 
{
    Schedule *s = schedule(id);
    QStringList lst;
    if (s) {
        lst = s->state();
    }
    if (lst.isEmpty()) {
        lst.append(trans ? i18n("Not scheduled") : QStringLiteral("Not scheduled"));
    }
    return lst;
}

bool Node::resourceError(long id) const 
{
    Schedule *s = schedule(id);
    return s ? s->resourceError : false;
}
    
bool Node::resourceOverbooked(long id) const
{
    Schedule *s = schedule(id);
    return s ? s->resourceOverbooked : false;
}
    
bool Node::resourceNotAvailable(long id) const
{
    Schedule *s = schedule(id);
    return s ? s->resourceNotAvailable : false;
}

bool Node::constraintError(long id) const
{
    Schedule *s = schedule(id);
    return s ? s->constraintError : false;
}

bool Node::schedulingError(long id) const
{
    Schedule *s = schedule(id);
    return s ? s->schedulingError : false;
}

bool Node::notScheduled(long id) const
{
    if (type() == Type_Summarytask) {
        // i am scheduled if al least on child is scheduled
        for (Node *n : std::as_const(m_nodes)) {
            if (! n->notScheduled(id)) {
                return false;
            }
        }
        return true;
    }
    Schedule *s = schedule(id);
    return s == nullptr || s->isDeleted() || s->notScheduled;
}

QStringList Node::overbookedResources(long id) const
{
    Schedule *s = schedule(id);
    return s ? s->overbookedResources() : QStringList();
}

void Node::saveWorkPackageXML(QDomElement &, long) const
{
    return;
}

void Node::saveRelations(QDomElement &element, const XmlSaveContext &context) const
{
    QListIterator<Relation*> it(m_dependChildNodes);
    while (it.hasNext()) {
        Relation *r = it.next();
        if (context.saveRelation(r)) {
            r->save(element, context);
        }
    }
    QListIterator<Node*> nodes(m_nodes);
    while (nodes.hasNext()) {
        nodes.next()->saveRelations(element, context);
    }
}

void Node::setConstraint(Node::ConstraintType type)
{ 
    m_constraint = type;
    changed(this, ConstraintTypeProperty);
}

void Node::setConstraint(const QString &type) {
    // Do not i18n these, they are used in load()
    if (type == QStringLiteral("ASAP"))
        setConstraint(ASAP);
    else if (type == QStringLiteral("ALAP"))
        setConstraint(ALAP);
    else if (type == QStringLiteral("MustStartOn"))
        setConstraint(MustStartOn);
    else if (type == QStringLiteral("MustFinishOn"))
        setConstraint(MustFinishOn);
    else if (type == QStringLiteral("StartNotEarlier"))
        setConstraint(StartNotEarlier);
    else if (type == QStringLiteral("FinishNotLater"))
        setConstraint(FinishNotLater);
    else if (type == QStringLiteral("FixedInterval"))
        setConstraint(FixedInterval);
    else
        setConstraint(ASAP);  // default
}

QString Node::constraintToString(bool trans) const {
    return constraintList(trans).at(m_constraint);
}

QStringList Node::constraintList(bool trans) {
    // keep these in the same order as the enum!
    return QStringList() 
            << (trans ? i18n("As Soon As Possible") : QStringLiteral("ASAP"))
            << (trans ? i18n("As Late As Possible") : QStringLiteral("ALAP"))
            << (trans ? i18n("Must Start On") : QStringLiteral("MustStartOn"))
            << (trans ? i18n("Must Finish On") : QStringLiteral("MustFinishOn"))
            << (trans ? i18n("Start Not Earlier") : QStringLiteral("StartNotEarlier"))
            << (trans ? i18n("Finish Not Later") : QStringLiteral("FinishNotLater"))
            << (trans ? i18n("Fixed Interval") : QStringLiteral("FixedInterval"));
}

void Node::propagateEarliestStart(DateTime &time) {
    if (m_currentSchedule == nullptr) {
        return;
    }
    if (type() != Type_Project) {
        m_currentSchedule->earlyStart = time;
        if (m_currentSchedule->lateStart.isValid() && m_currentSchedule->lateStart < time) {
            m_currentSchedule->lateStart = time;
        }
        //m_currentSchedule->logDebug("propagateEarliestStart: " + time.toString());
        switch (m_constraint) {
            case FinishNotLater:
            case MustFinishOn:
                if (m_constraintEndTime < time) {
                    m_currentSchedule->logWarning(i18n("Task constraint outside project constraint"));
#ifndef PLAN_NLOGDEBUG
                    m_currentSchedule->logDebug(QStringLiteral("%1: end constraint %2 < %3").arg(constraintToString(true), m_constraintEndTime.toString(), time.toString()));
#endif
                }
                break;
            case MustStartOn:
            case FixedInterval:
                if (m_constraintStartTime < time) {
                    m_currentSchedule->logWarning(i18n("Task constraint outside project constraint"));
#ifndef PLAN_NLOGDEBUG
                    m_currentSchedule->logDebug(QStringLiteral("%1: start constraint %2 < %3").arg(constraintToString(true), m_constraintEndTime.toString(), time.toString()));
#endif
                }
                break;
            default:
                break;
        }
    }
    //debugPlan<<m_name<<":"<<m_currentSchedule->earlyStart;
    QListIterator<Node*> it = m_nodes;
    while (it.hasNext()) {
        it.next()->propagateEarliestStart(time);
    }
}

void Node::propagateLatestFinish(DateTime &time) {
    if (m_currentSchedule == nullptr) {
        return;
    }
    if (type() != Type_Project) {
        m_currentSchedule->lateFinish = time;
        if (m_currentSchedule->earlyFinish.isValid() && m_currentSchedule->earlyFinish > time) {
            m_currentSchedule->earlyFinish = time;
        }
        switch (m_constraint) {
            case StartNotEarlier:
            case MustStartOn:
                if (m_constraintStartTime > time) {
                    m_currentSchedule->logWarning(i18n("Task constraint outside project constraint"));
#ifndef PLAN_NLOGDEBUG
                    m_currentSchedule->logDebug(QStringLiteral("%1: start constraint %2 < %3").arg(constraintToString(true), m_constraintEndTime.toString(), time.toString()));
#endif
                }
                break;
            case MustFinishOn:
            case FixedInterval:
                if (m_constraintEndTime > time) {
                    m_currentSchedule->logWarning(i18n("Task constraint outside project constraint"));
#ifndef PLAN_NLOGDEBUG
                    m_currentSchedule->logDebug(QStringLiteral("%1: end constraint %2 > %3").arg(constraintToString(true), m_constraintEndTime.toString(), time.toString()));
#endif
                }
                break;
            default:
                break;
        }
    }
    //debugPlan<<m_name<<":"<<m_currentSchedule->lateFinish;
    QListIterator<Node*> it = m_nodes;
    while (it.hasNext()) {
        it.next()->propagateLatestFinish(time);
    }
}

void Node::moveEarliestStart(DateTime &time) {
    if (m_currentSchedule == nullptr)
        return;
    if (m_currentSchedule->earlyStart < time) {
        //m_currentSchedule->logDebug("moveEarliestStart: " + m_currentSchedule->earlyStart.toString() + " -> " + time.toString());
        m_currentSchedule->earlyStart = time;
    }
    QListIterator<Node*> it = m_nodes;
    while (it.hasNext()) {
        it.next()->moveEarliestStart(time);
    }
}

void Node::moveLatestFinish(DateTime &time) {
    if (m_currentSchedule == nullptr)
        return;
    if (m_currentSchedule->lateFinish > time)
        m_currentSchedule->lateFinish = time;
    QListIterator<Node*> it = m_nodes;
    while (it.hasNext()) {
        it.next()->moveLatestFinish(time);
    }
}

void Node::initiateCalculation(MainSchedule &sch) {
    m_visitedForward = false;
    m_visitedBackward = false;
    m_durationForward = Duration::zeroDuration;
    m_durationBackward = Duration::zeroDuration;
    m_earlyStart = DateTime();
    m_earlyFinish = DateTime();
    m_lateFinish = DateTime();
    QListIterator<Node*> it = m_nodes;
    while (it.hasNext()) {
        it.next()->initiateCalculation(sch);
    }
}

void Node::resetVisited() {
    m_visitedForward = false;
    m_visitedBackward = false;
    QListIterator<Node*> it = m_nodes;
    while (it.hasNext()) {
        it.next()->resetVisited();
    }
}

Node *Node::siblingBefore() {
    //debugPlan;
    if (parentNode())
        return parentNode()->childBefore(this);
    return nullptr;
}

Node *Node::childBefore(Node *node) {
    //debugPlan;
    int index = m_nodes.indexOf(node);
    if (index > 0){
        return m_nodes.at(index-1);
    }
    return nullptr;
}

Node *Node::siblingAfter() {
    //debugPlan;
    if (parentNode())
        return parentNode()->childAfter(this);
    return nullptr;
}

Node *Node::childAfter(Node *node)
{
    //debugPlan;
    Q_ASSERT(m_nodes.contains(node));
    int index = m_nodes.indexOf(node);
    if (index < m_nodes.count()-1) {
        return m_nodes.at(index+1);
    }
    return nullptr;
}

bool Node::moveChildUp(Node* node)
{
    if (findChildNode(node) == -1)
        return false; // not my node!
    Node *sib = node->siblingBefore();
    if (!sib)
        return false;
    sib = sib->siblingBefore();
    takeChildNode(node);
    if (sib) {
        addChildNode(node, sib);
    } else {
        insertChildNode(0, node);
    }        
    return true;
}

bool Node::moveChildDown(Node* node)
{
    if (findChildNode(node) == -1)
        return false; // not my node!
    Node *sib = node->siblingAfter();
    if (!sib)
        return false;
    takeChildNode(node);
    addChildNode(node, sib);
    return true;
}

bool Node::legalToLink(const Node *node) const
{
    Node *p = const_cast<Node*>(this)->projectNode();
    if (p)
        return p->legalToLink(this, node);
    return false;
}

bool Node::isEndNode() const {
    return m_dependChildNodes.isEmpty();
}
bool Node::isStartNode() const {
    return m_dependParentNodes.isEmpty();
}

void Node::setId(const QString& id) {
    //debugPlan<<id;
    m_id = id;
}

void Node::setStartTime(const DateTime &startTime, long id)
{ 
    Schedule *s = schedule(id);
    if (s)
        s->startTime = startTime;
}

void Node::setEndTime(const DateTime &endTime, long id)
{
    Schedule *s = schedule(id);
    if (s)
        s->endTime = endTime;
}

void Node::saveAppointments(QDomElement &element, long id) const {
    //debugPlan<<m_name<<" id="<<id;
    QListIterator<Node*> it(m_nodes);
    while (it.hasNext()) {
        it.next()->saveAppointments(element, id);
    }
}

QList<Appointment*> Node::appointments(long id)
{
    Schedule *s = schedule(id);
    QList<Appointment*> lst;
    if (s)
        lst = s->appointments();
    return lst;
}

QList<Resource*> Node::assignedResources(long id) const {
    Schedule *s = schedule(id);
    QList<Resource*> res;
    if (s) {
        const QList<Appointment*> appointments = s->appointments();
        for (Appointment *a : appointments) {
            res << a->resource()->resource();
        }
    }
    return res;
}


// Appointment *Node::findAppointment(Resource *resource) {
//     if (m_currentSchedule)
//         return m_currentSchedule->findAppointment(resource);
//     return 0;
// }
// bool Node::addAppointment(Appointment *appointment) {
//     if (m_currentSchedule)
//         return m_currentSchedule->add(appointment);
//     return false;
// }
// 
// called from resource side when resource adds appointment
bool Node::addAppointment(Appointment *appointment, Schedule &main) {
    Schedule *s = findSchedule(main.id());
    if (s == nullptr) {
        s = createSchedule(&main);
    }
    appointment->setNode(s);
    //debugPlan<<this<<":"<<appointment<<","<<s<<","<<s->id()<<","<<main.id();
    return s->add(appointment);
}

void Node::addAppointment(ResourceSchedule *resource, const DateTime &start, const DateTime &end, double load) {
    Schedule *node = findSchedule(resource->id());
    if (node == nullptr) {
        node = createSchedule(resource->parent());
    }
    node->setCalculationMode(resource->calculationMode());
    node->addAppointment(resource, start, end, load);
}

bool Node::isBaselined(long id) const
{
    Schedule *s = schedule(id);
    return s ? s->isBaselined() : false;
}

void Node::takeSchedule(const Schedule *schedule) {
    if (schedule == nullptr)
        return;
    if (m_currentSchedule == schedule)
        m_currentSchedule = nullptr;
    m_schedules.take(schedule->id());
}

void Node::addSchedule(Schedule *schedule) {
    if (schedule == nullptr)
        return;
    m_schedules.insert(schedule->id(), schedule);
}

Schedule *Node::createSchedule(const QString& name, Schedule::Type type, long id) {
    //debugPlan<<name<<" type="<<type<<" id="<<(int)id;
    NodeSchedule *sch = new NodeSchedule(this, name, type, id);
    addSchedule(sch);
    return sch;
}

Schedule *Node::createSchedule(Schedule *parent) {
    //debugPlan<<name<<" type="<<type<<" id="<<(int)id;
    NodeSchedule *sch = new NodeSchedule(parent, this);
    addSchedule(sch);
    return sch;
}

Schedule *Node::schedule(long id) const
{
    switch (id) {
        case ANYSCHEDULED: {
            for (Schedule *s : std::as_const(m_schedules)) {
                if (s->isScheduled()) {
                    return s;
                }
            }
            return nullptr;
        }
        case CURRENTSCHEDULE:
            return m_currentSchedule;
        case NOTSCHEDULED:
            return nullptr;
        case BASELINESCHEDULE: {
            for (Schedule *s : std::as_const(m_schedules)) {
                if (s->isBaselined()) {
                    return s;
                }
            }
            return nullptr;
        }
        default:
            break;
    }
    return findSchedule(id);
}

Schedule *Node::findSchedule(long id) const
{
    return m_schedules.value(id);
}

Schedule *Node::findSchedule(const QString &name, const Schedule::Type type) {
    const QHash<long, Schedule*> it;
    for (Schedule *sch : it) {
        if (!sch->isDeleted() && 
            sch->name() == name && sch->type() == type)
            return sch;
    }
    return nullptr;
}

Schedule *Node::findSchedule(const QString &name) {
    for (Schedule *sch : std::as_const(m_schedules)) {
        if (!sch->isDeleted() && sch->name() == name)
            return sch;
    }
    return nullptr;
}


Schedule *Node::findSchedule(const Schedule::Type type) {
    //debugPlan<<m_name<<" find type="<<type<<" nr="<<m_schedules.count();
    for (Schedule *sch : std::as_const(m_schedules)) {
        if (!sch->isDeleted() && sch->type() == type) {
            return sch;
        }
    }
    return nullptr;
}

void Node::setScheduleDeleted(long id, bool on) {
    Schedule *ns = findSchedule(id);
    if (ns == nullptr) {
        errorPlan<<m_name<<" Could not find schedule with id="<<id;
    } else {
        ns->setDeleted(on);
    }
}

void Node::setParentSchedule(Schedule *sch) {
    Schedule *s = findSchedule(sch->id());
    if (s) {
        s->setParent(sch);
    }
    QListIterator<Node*> it = m_nodes;
    while (it.hasNext()) {
        it.next()->setParentSchedule(sch);
    }
}

bool Node::calcCriticalPath(bool fromEnd) {
    if (m_currentSchedule == nullptr)
        return false;
    //debugPlan<<m_name;
    if (!isCritical()) {
        return false;
    }
    if (!fromEnd && isStartNode()) {
        m_currentSchedule->inCriticalPath = true;
        return true;
    }
    if (fromEnd && isEndNode()) {
        m_currentSchedule->inCriticalPath = true;
        return true;
    }
    QListIterator<Relation*> pit(m_dependParentNodes);
    while (pit.hasNext()) {
        if (pit.next()->parent()->calcCriticalPath(fromEnd)) {
            m_currentSchedule->inCriticalPath = true;
        }
    }
    return m_currentSchedule->inCriticalPath;
}

void Node::calcFreeFloat() {
    for (Node *n : std::as_const(m_nodes)) {
        n->calcFreeFloat();
    }
    return;
}

int Node::level() const {
    const Node *n = parentNode();
    return n ? n->level() + 1 : 0;
}

QString Node::generateWBSCode(QList<int> &indexes, bool sortable) const {
    //debugPlan<<m_name<<indexes;
    if (m_parent == nullptr) {
        return QString();
    }
    indexes.insert(0, m_parent->indexOf(this));
    return m_parent->generateWBSCode(indexes, sortable);
}

QString Node::wbsCode(bool sortable) const {
    //debugPlan<<m_name;
    QList<int> indexes;
    return generateWBSCode(indexes, sortable);
}

bool Node::isScheduled(long id) const
{
    Schedule *s = schedule(id);
    return s != nullptr && s->isScheduled();
}

void Node::setCurrentSchedule(long id) {
    QListIterator<Node*> it = m_nodes;
    while (it.hasNext()) {
        it.next()->setCurrentSchedule(id);
    }
    //debugPlan<<m_name<<" id:"<<id<<"="<<m_currentSchedule;
}

void Node::setStartupCost(double cost)
{
    m_startupCost = cost;
    changed(StartupCostProperty);
}

void Node::setStartupAccount(Account *acc)
{
    //debugPlan<<m_name<<"="<<acc;
    if (m_startupAccount) {
        m_startupAccount->removeStartup(*this);
    }
    m_startupAccount = acc;
    changed(StartupAccountProperty);
}

void Node::setShutdownCost(double cost)
{
    m_shutdownCost = cost;
    changed(ShutdownCostProperty);
}

void Node::setShutdownAccount(Account *acc)
{
    //debugPlan<<m_name<<"="<<acc;
    if (m_shutdownAccount) {
        m_shutdownAccount->removeShutdown(*this);
    }
    m_shutdownAccount = acc;
    changed(ShutdownAccountProperty);
}

void Node::setRunningAccount(Account *acc)
{
    //debugPlan<<m_name<<"="<<acc;
    if (m_runningAccount) {
        m_runningAccount->removeRunning(*this);
    }
    m_runningAccount = acc;
    changed(RunningAccountProperty);
}

void Node::blockChanged(bool on)
{
    m_blockChanged = on;
}

void Node::changed(Node *node, int property) {
    if (m_blockChanged) {
        return;
    }
    switch (property) {
        case TypeProperty:
        case StartupCostProperty:
        case ShutdownCostProperty:
        case CompletionEntryProperty:
        case CompletionStartedProperty:
        case CompletionFinishedProperty:
        case CompletionStartTimeProperty:
        case CompletionFinishTimeProperty:
        case CompletionPercentageProperty:
        case CompletionRemainingEffortProperty:
        case CompletionActualEffortProperty:
        case CompletionUsedEffortProperty:
            for (Schedule *s : std::as_const(m_schedules)) {
                s->clearPerformanceCache();
            }
        break;
        default: break;
    }
    if (m_parent) {
        m_parent->changed(node, property);
    }
}

Duration Node::plannedEffort(const Resource *resource, long id, EffortCostCalculationType type) const
{
    Duration e;
    for (Node *n : std::as_const(m_nodes)) {
        e += n->plannedEffort(resource, id, type);
    }
    return e;
}

Duration Node::plannedEffort(const Resource *resource, QDate date, long id, EffortCostCalculationType type) const
{
    Duration e;
    for (Node *n : std::as_const(m_nodes)) {
        e += n->plannedEffort(resource, date, id, type);
    }
    return e;
}

Duration Node::plannedEffortTo(const Resource *resource, QDate date, long id, EffortCostCalculationType type) const
{
    Duration e;
    for (Node *n : std::as_const(m_nodes)) {
        e += n->plannedEffortTo(resource, date, id, type);
    }
    return e;
}

EffortCost Node::plannedCost(long id, EffortCostCalculationType type) const
{
    EffortCost ec;
    for (Node *n : std::as_const(m_nodes)) {
        ec += n->plannedCost(id, type);
    }
    return ec;
}

EffortCostMap Node::bcwsPrDay(long int id, EffortCostCalculationType type) const
{
    return const_cast<Node*>(this)->bcwsPrDay(id, type);
}

EffortCostMap Node::bcwsPrDay(long int id, EffortCostCalculationType type)
{
    Schedule *s = schedule(id);
    if (s == nullptr) {
        return EffortCostMap();
    }
    EffortCostCache &ec = s->bcwsPrDayCache(type);
    if (! ec.cached) {
        ec.effortcostmap = EffortCostMap();
        for (Node *n : std::as_const(m_nodes)) {
            ec.effortcostmap += n->bcwsPrDay(id, type);
        }
        ec.cached = true;
    }
    return ec.effortcostmap;
}

EffortCostMap Node::bcwpPrDay(long int id, EffortCostCalculationType type) const
{
    return const_cast<Node*>(this)->bcwpPrDay(id, type);
}

EffortCostMap Node::bcwpPrDay(long int id, EffortCostCalculationType type)
{
    Schedule *s = schedule(id);
    if (s == nullptr) {
        return EffortCostMap();
    }
    EffortCostCache &ec = s->bcwpPrDayCache(type);
    if (! ec.cached) {
        ec.effortcostmap = EffortCostMap();
        for (Node *n : std::as_const(m_nodes)) {
            ec.effortcostmap += n->bcwpPrDay(id, type);
        }
        ec.cached = true;
    }
    return ec.effortcostmap;
}

EffortCostMap Node::acwp(long id, EffortCostCalculationType type) const
{
    return const_cast<Node*>(this)->acwp(id, type);
}

EffortCostMap Node::acwp(long id, EffortCostCalculationType type)
{
    Schedule *s = schedule(id);
    if (s == nullptr) {
        return EffortCostMap();
    }
    EffortCostCache &ec = s->acwpCache(type);
    if (! ec.cached) {
        ec.effortcostmap = EffortCostMap();
        for (Node *n : std::as_const(m_nodes)) {
            ec.effortcostmap += n->acwp(id, type);
        }
        ec.cached = true;
    }
    return ec.effortcostmap;
}

EffortCost Node::acwp(QDate date, long id) const
{
    EffortCost ec;
    for (Node *n : std::as_const(m_nodes)) {
        ec += n->acwp(date, id);
    }
    return ec;
}

void Node::slotStandardWorktimeChanged(KPlato::StandardWorktime*)
{
    //debugPlan<<m_estimate;
    if (m_estimate) {
        m_estimate->m_expectedCached = false;
        m_estimate->m_optimisticCached = false;
        m_estimate->m_pessimisticCached = false;
    }
}

void Node::emitDocumentAdded(Node *node, Document *doc, int idx)
{
    if (m_parent) {
        m_parent->emitDocumentAdded(node, doc, idx);
    }
}

void Node::emitDocumentRemoved(Node *node, Document *doc, int idx)
{
    if (m_parent) {
        m_parent->emitDocumentRemoved(node, doc, idx);
    }
}

void Node::emitDocumentChanged(Node *node, Document *doc, int idx)
{
    if (m_parent) {
        m_parent->emitDocumentChanged(node, doc, idx);
    }
}

//////////////////////////   Estimate   /////////////////////////////////

Estimate::Estimate(Node *parent)
    : m_parent(parent)
{
    m_pertCached = false;

    setUnit(Duration::Unit_h);
    setExpectedEstimate(8.0);
    setPessimisticEstimate(8.0);
    setOptimisticEstimate(8.0);
    
    m_type = Type_Effort;
    m_calendar = nullptr;
    m_risktype = Risk_None;
}

Estimate::Estimate(const Estimate &estimate, Node *parent)
    : m_parent(parent)
{
    copy(estimate);
}

Estimate::~Estimate()
{
}

void Estimate::clear()
{
    m_pertCached = false;

    setExpectedEstimate(0.0);
    setPessimisticEstimate(0.0);
    setOptimisticEstimate(0.0);
    
    m_type = Type_Effort;
    m_calendar = nullptr;
    m_risktype = Risk_None;
    m_unit = Duration::Unit_h;
    changed(Node::EstimateProperty);
}

Estimate &Estimate::operator=(const Estimate &estimate)
{
    copy(estimate);
    return *this;
}

void Estimate::copy(const Estimate &estimate)
{
    //m_parent = 0; // don't touch
    m_expectedEstimate = estimate.m_expectedEstimate;
    m_optimisticEstimate = estimate.m_optimisticEstimate;
    m_pessimisticEstimate = estimate.m_pessimisticEstimate;
    
    m_expectedValue = estimate.m_expectedValue;
    m_optimisticValue = estimate.m_optimisticValue;
    m_pessimisticValue = estimate.m_pessimisticValue;
    
    m_expectedCached = estimate.m_expectedCached;
    m_optimisticCached = estimate.m_optimisticCached;
    m_pessimisticCached = estimate.m_pessimisticCached;
    
    m_pertExpected = estimate.m_pertExpected;
    m_pertCached = estimate.m_pertCached;

    m_type = estimate.m_type;
    m_calendar = estimate.m_calendar;
    m_risktype = estimate.m_risktype;
    m_unit = estimate.m_unit;
    changed(Node::EstimateProperty);
}

double Estimate::variance() const
{
    double d = deviation();
    return d * d;
}

double Estimate::variance(Duration::Unit unit) const
{
    double d = deviation(unit);
    return d * d;
}

double Estimate::deviation() const
{
    return (m_pessimisticEstimate - m_optimisticEstimate) / 6;
}

double Estimate::deviation(Duration::Unit unit) const
{
    if (unit == m_unit) {
        return deviation();
    }
    double p = pessimisticValue().toDouble(unit);
    double o = optimisticValue().toDouble(unit);
    double v = (p - o) / 6;
    return v;
}

Duration Estimate::pertExpected() const {
    if (m_risktype == Risk_Low) {
        if (! m_pertCached) {
            m_pertExpected = (optimisticValue() + pessimisticValue() + (expectedValue()*4))/6;
            m_pertCached = true;
        }
        return m_pertExpected;
    } else if (m_risktype == Risk_High) {
        if (! m_pertCached) {
            m_pertExpected = (optimisticValue() + (pessimisticValue()*2) + (expectedValue()*4))/7;
            m_pertCached = true;
        }
        return m_pertExpected;
    }
    return expectedValue(); // risk==none
}

Duration Estimate::pertOptimistic() const {
    if (m_risktype != Risk_None) {
        return pertExpected() - Duration(variance(Duration::Unit_ms));
    }
    return optimisticValue();
}

Duration Estimate::pertPessimistic() const {
    if (m_risktype != Risk_None) {
        return pertExpected() + Duration(variance(Duration::Unit_ms));
    }
    return pessimisticValue();
}

Duration Estimate::value(int valueType, bool pert) const {
    if (valueType == Estimate::Use_Expected) {
        return pert ? pertExpected() : expectedValue();
    } else if (valueType == Estimate::Use_Optimistic) {
        return pert ? pertOptimistic() : optimisticValue();
    } else if (valueType == Estimate::Use_Pessimistic) {
        return pert ? pertPessimistic() : pessimisticValue();
    }
    return expectedValue();
}

void Estimate::setUnit(Duration::Unit unit)
{
    m_unit = unit;
    m_expectedCached = false;
    m_optimisticCached = false;
    m_pessimisticCached = false;
    m_pertCached = false;
    changed(Node::EstimateProperty);
}

bool Estimate::load(KoXmlElement &element, XMLLoaderObject &status) {
    setType(element.attribute(QStringLiteral("type")));
    setRisktype(element.attribute(QStringLiteral("risk")));
    if (status.version() <= QStringLiteral("0.6")) {
        m_unit = (Duration::Unit)(element.attribute(QStringLiteral("display-unit"), QString().number(Duration::Unit_h)).toInt());
        QList<qint64> s = status.project().standardWorktime()->scales();
        m_expectedEstimate = scale(Duration::fromString(element.attribute(QStringLiteral("expected"))), m_unit, s);
        m_optimisticEstimate = scale(Duration::fromString(element.attribute(QStringLiteral("optimistic"))), m_unit, s);
        m_pessimisticEstimate = scale(Duration::fromString(element.attribute(QStringLiteral("pessimistic"))), m_unit, s);
    } else {
        if (status.version() <= QStringLiteral("0.6.2")) {
            // 0 pos in unit is now Unit_Y, so add 3 to get the correct new unit
            m_unit = (Duration::Unit)(element.attribute(QStringLiteral("unit"), QString().number(Duration::Unit_ms - 3)).toInt() + 3);
        } else {
            m_unit = Duration::unitFromString(element.attribute(QStringLiteral("unit")));
        }
        m_expectedEstimate = element.attribute(QStringLiteral("expected"), QStringLiteral("0.0")).toDouble();
        m_optimisticEstimate = element.attribute(QStringLiteral("optimistic"), QStringLiteral("0.0")).toDouble();
        m_pessimisticEstimate = element.attribute(QStringLiteral("pessimistic"), QStringLiteral("0.0")).toDouble();
        
        m_calendar = status.project().findCalendar(element.attribute(QStringLiteral("calendar-id")));
    }
    return true;
}

void Estimate::save(QDomElement &element) const {
    QDomElement me = element.ownerDocument().createElement(QStringLiteral("estimate"));
    element.appendChild(me);
    me.setAttribute(QStringLiteral("expected"), QString::number(m_expectedEstimate));
    me.setAttribute(QStringLiteral("optimistic"), QString::number(m_optimisticEstimate));
    me.setAttribute(QStringLiteral("pessimistic"), QString::number(m_pessimisticEstimate));
    me.setAttribute(QStringLiteral("type"), typeToString());
    if (m_calendar) {
        me.setAttribute(QStringLiteral("calendar-id"), m_calendar->id());
    }
    me.setAttribute(QStringLiteral("risk"), risktypeToString());
    me.setAttribute(QStringLiteral("unit"), Duration::unitToString(m_unit));
}

QString Estimate::typeToString(bool trans) const {
    return typeToStringList(trans).at(m_type);
}

QString Estimate::typeToString(Estimate::Type typ, bool trans)
{
    return typeToStringList(trans).value(typ);
}

QStringList Estimate::typeToStringList(bool trans) {
    return QStringList() 
            << (trans ? i18n("Effort") : QStringLiteral("Effort"))
            << (trans ? i18n("Duration") : QStringLiteral("Duration"));
}

void Estimate::setType(Type type)
{
    m_type = type;
    m_expectedCached = false;
    m_optimisticCached = false;
    m_pessimisticCached = false;
    m_pertCached = false;
    changed(Node::EstimateProperty);
}

void Estimate::setType(const QString& type) {
    if (type == QStringLiteral("Effort"))
        setType(Type_Effort);
    else if (type == QStringLiteral("Duration") || /*old format*/ type == QStringLiteral("FixedDuration"))
        setType(Type_Duration);
    else if (/*old format*/type == QStringLiteral("Length"))
        setType(Type_Duration);
    else if (type == QStringLiteral("Type_FixedDuration")) // Typo, keep old xml files working
        setType(Type_Duration);
    else
        setType(Type_Effort); // default
}

QString Estimate::risktypeToString(bool trans) const {
    return risktypeToStringList(trans).at(m_risktype);
}

QStringList Estimate::risktypeToStringList(bool trans) {
    return QStringList() 
            << (trans ? i18n("None") : QStringLiteral("None"))
            << (trans ? i18n("Low") : QStringLiteral("Low"))
            << (trans ? i18n("High") : QStringLiteral("High"));
}

void Estimate::setRisktype(const QString& type) {
    if (type == QStringLiteral("High"))
        setRisktype(Risk_High);
    else if (type == QStringLiteral("Low"))
        setRisktype(Risk_Low);
    else
        setRisktype(Risk_None); // default
}

void Estimate::setRisktype(Risktype type)
{
    m_pertCached = false;
    m_risktype = type;
    changed(Node::EstimateRiskProperty);
}

void Estimate::setCalendar(Calendar *calendar)
{
    m_calendar = calendar;
    m_expectedCached = false;
    m_optimisticCached = false;
    m_pessimisticCached = false;
    m_pertCached = false;
    changed(Node::EstimateProperty);
}

void Estimate::setExpectedEstimate(double value)
{
    m_expectedEstimate = value;
    m_expectedCached = false;
    m_pertCached = false;
    changed(Node::EstimateProperty);
}

void Estimate::setOptimisticEstimate(double value)
{
    m_optimisticEstimate = value;
    m_optimisticCached = false;
    m_pertCached = false;
    changed(Node::EstimateOptimisticProperty);
}

void Estimate::setPessimisticEstimate(double value)
{
    m_pessimisticEstimate = value;
    m_pessimisticCached = false;
    m_pertCached = false;
    changed(Node::EstimatePessimisticProperty);
}

void Estimate::setOptimisticRatio(int percent)
{
    int p = percent>0 ? -percent : percent;
    m_optimisticValue = expectedValue()*(100+p)/100;
    m_optimisticEstimate = scale(m_optimisticValue, m_unit, scales());
    m_optimisticCached = true;
    m_pertCached = false;
    changed(Node::EstimateOptimisticProperty);
}

int Estimate::optimisticRatio() const {
    if (m_expectedEstimate == 0.0)
        return 0;
    return (int)((optimisticValue()*100)/expectedValue())-100;
}

void Estimate::setPessimisticRatio(int percent) 
{
    int p = percent<0 ? -percent : percent;
    m_pessimisticValue = expectedValue()*(100+p)/100;
    m_pessimisticEstimate = scale(m_pessimisticValue, m_unit, scales());
    m_pessimisticCached = true;
    m_pertCached = false;
    changed(Node::EstimatePessimisticProperty);
}

int Estimate::pessimisticRatio() const {
    if (m_expectedEstimate == 0.0)
        return 0;
    return (int)((pessimisticValue()*100)/expectedValue())-100;
}

// internal
void Estimate::setOptimisticValue()
{
    m_optimisticValue = scale(m_optimisticEstimate, m_unit, scales());
    m_optimisticCached = true;
    m_pertCached = false;
}

// internal
void Estimate::setExpectedValue()
{
    m_expectedValue = scale(m_expectedEstimate, m_unit, scales());
    m_expectedCached = true;
    m_pertCached = false;
}

// internal
void Estimate::setPessimisticValue()
{
    m_pessimisticValue = scale(m_pessimisticEstimate, m_unit, scales());
    m_pessimisticCached = true;
    m_pertCached = false;
}

Duration Estimate::optimisticValue() const
{
    if (! m_optimisticCached) {
        const_cast<Estimate*>(this)->setOptimisticValue();
    }
    return m_optimisticValue;
}

Duration Estimate::pessimisticValue() const
{
    if (! m_pessimisticCached) {
        const_cast<Estimate*>(this)->setPessimisticValue();
    }
    return m_pessimisticValue;
}

Duration Estimate::expectedValue() const
{
    if (! m_expectedCached) {
        const_cast<Estimate*>(this)->setExpectedValue();
    }
    return m_expectedValue;
}

double Estimate::scale(const Duration &value, Duration::Unit unit, const QList<qint64> &scales)
{
    //debugPlan<<value.toDouble(unit)<<","<<unit<<scales;
    QList<qint64> lst = scales;
    switch (lst.count()) {
        case Duration::Unit_Y:
            lst << (qint64)(365 * 24) * 60 * 60 * 1000; // add milliseconds in a year
            Q_FALLTHROUGH();
        case Duration::Unit_M:
            lst << (qint64)(30 * 24) * 60 * 60 * 1000; // add milliseconds in a month
            Q_FALLTHROUGH();
        case Duration::Unit_w:
            lst << (qint64)(7 * 24) * 60 * 60 * 1000; // add milliseconds in a week
            Q_FALLTHROUGH();
        case Duration::Unit_d:
            lst << 24 * 60 * 60 * 1000; // add milliseconds in day
            Q_FALLTHROUGH();
        case Duration::Unit_h:
            lst << 60 * 60 * 1000; // add milliseconds in hour
            Q_FALLTHROUGH();
        case Duration::Unit_m:
            lst << 60 * 1000; // add milliseconds in minute
            Q_FALLTHROUGH();
        case Duration::Unit_s:
            lst << 1000; // add milliseconds in second
            Q_FALLTHROUGH();
        case Duration::Unit_ms:
            lst << 1; // add milliseconds in a millisecond
            Q_FALLTHROUGH();
        default:
            break;
    }
    double v = (double)(value.milliseconds());
    v /= lst[ unit ];
    //debugPlan<<value.toString()<<","<<unit<<"="<<v;
    return v;
}

Duration Estimate::scale(double value, Duration::Unit unit, const QList<qint64> &scales)
{
    //debugPlan<<value<<","<<unit<<scales;
    QList<qint64> lst = scales;
    switch (lst.count()) {
        case Duration::Unit_Y:
            lst << (qint64)(365 * 24) * 60 * 60 * 1000; // add milliseconds in a year
            Q_FALLTHROUGH();
        case Duration::Unit_M:
            lst << (qint64)(30 * 24) * 60 * 60 * 1000; // add milliseconds in a month
            Q_FALLTHROUGH();
        case Duration::Unit_w:
            lst << (qint64)(7 * 24) * 60 * 60 * 1000; // add milliseconds in a week
            Q_FALLTHROUGH();
        case Duration::Unit_d:
            lst << 24 * 60 * 60 * 1000; // add milliseconds in day
            Q_FALLTHROUGH();
        case Duration::Unit_h:
            lst << 60 * 60 * 1000; // add milliseconds in hour
            Q_FALLTHROUGH();
        case Duration::Unit_m:
            lst << 60 * 1000; // add milliseconds in minute
            Q_FALLTHROUGH();
        case Duration::Unit_s:
            lst << 1000; // add milliseconds in second
            Q_FALLTHROUGH();
        case Duration::Unit_ms:
            lst << 1; // add milliseconds in a millisecond
            Q_FALLTHROUGH();
        default:
            break;
    }
    qint64 v = (qint64)(value * lst[ unit ]);
    //debugPlan<<value<<","<<unit<<"="<<v;
    return Duration(v, Duration::Unit_ms);
}

//static
QList<qint64> Estimate::defaultScales()
{
    QList<qint64> lst;
    lst << (qint64)(365 * 24) * 60 * 60 * 1000  // add milliseconds in a year
        << (qint64)(30 * 24) * 60 * 60 * 1000   // add milliseconds in a month
        << (qint64)(7 * 24) * 60 * 60 * 1000    // add milliseconds in a week
        << 24 * 60 * 60 * 1000                  // add milliseconds in day
        << 60 * 60 * 1000                       // add milliseconds in hour
        << 60 * 1000                            // add milliseconds in minute
        << 1000                                 // add milliseconds in second
        << 1;                                   // add milliseconds in a millisecond
    return lst;
}

QList<qint64> Estimate::scales() const
{
    QList<qint64> s;
    if (m_type == Type_Duration && m_calendar == nullptr) {
        return s; // Use default scaling (24h a day...)
    }
    if (m_parent == nullptr) {
        return s;
    }
    Project *p = static_cast<Project*>(m_parent->projectNode());
    if (p == nullptr) {
        return s;
    }
    s << p->standardWorktime()->scales();
    return s;
}


}  //KPlato namespace
