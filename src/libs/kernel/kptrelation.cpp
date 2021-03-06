/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2001 Thomas zander <zander@kde.org>
   SPDX-FileCopyrightText: 2004, 2012 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/
// clazy:excludeall=qstring-arg
#include "kptrelation.h"

#include "kptnode.h"
#include "kptproject.h"
#include "XmlSaveContext.h"
#include "kptxmlloaderobject.h"
#include "kptdebug.h"

#include <KoXmlReader.h>

#include <KLocalizedString>

#include <QStringList>


namespace KPlato
{

Relation::Relation(Node *parent, Node *child, Type type, Duration lag) {
    m_parent=parent;
    m_child=child;
    m_type=type;
    m_lag=lag;
    //debugPlan<<this;
}

Relation::Relation(Node *parent, Node *child, Type type) {
    m_parent=parent;
    m_child=child;
    m_type=type;
    m_lag=Duration();
    //debugPlan<<this;
}

Relation::Relation(Relation *rel) {
    m_parent=rel->parent();
    m_child=rel->child();
    m_type=rel->type();
    m_lag=rel->lag();
    //debugPlan<<this;
}

Relation::~Relation() {
    //debugPlan<<"("<<this<<") parent:"<<(m_parent ? m_parent->name():"none")<<" child:"<<(m_child ? m_child->name():"None");
    if (m_parent)
        m_parent->takeDependChildNode(this);
    if (m_child)
        m_child->takeDependParentNode(this);
}

void Relation::setType(Type type) {
    m_type=type;
}

void Relation::setType(const QString &type)
{
    int t = typeList().indexOf(type);
    if (t == -1) {
        t = FinishStart;
    }
    m_type = static_cast<Type>(t);
}

QString Relation::typeToString(bool trans) const
{
    return typeList(trans).at(m_type);
}

QStringList Relation::typeList(bool trans)
{
    //NOTE: must match enum
    QStringList lst;
    lst << (trans ? i18n("Finish-Start") : "Finish-Start");
    lst << (trans ? i18n("Finish-Finish") : "Finish-Finish");
    lst << (trans ? i18n("Start-Start") : "Start-Start");
    return lst;
}

void Relation::setParent(Node* node)
{
    m_parent = node;
}

void Relation::setChild(Node* node)
{
    m_child = node;
}


bool Relation::load(KoXmlElement &element, XMLLoaderObject &status) {
    const Project &project = status.project();
    m_parent = project.findNode(element.attribute("parent-id"));
    if (m_parent == nullptr) {
        return false;
    }
    m_child = project.findNode(element.attribute("child-id"));
    if (m_child == nullptr) {
        return false;
    }
    if (m_child == m_parent) {
        debugPlan<<"child == parent";
        return false;
    }
    if (m_child == m_parent) {
        debugPlan<<"child == parent";
        return false;
    }
    if (!m_parent->legalToLink(m_child))
        return false;
        
    setType(element.attribute("type"));

    m_lag = Duration::fromString(element.attribute("lag"));

    if (!m_parent->addDependChildNode(this)) {
        errorPlan<<"Failed to add relation: Child="<<m_child->name()<<" parent="<<m_parent->name()<<'\n';
        return false;
    }
    if (!m_child->addDependParentNode(this)) {
        m_parent->takeDependChildNode(this);
        errorPlan<<"Failed to add relation: Child="<<m_child->name()<<" parent="<<m_parent->name()<<'\n';
        return false;
    }

    //debugPlan<<"Added relation: Child="<<m_child->name()<<" parent="<<m_parent->name();
    return true;
}


void Relation::save(QDomElement &element, const XmlSaveContext &context) const
{
    if (!context.saveRelation(this)) {
        return;
    }
    QDomElement me = element.ownerDocument().createElement("relation");
    element.appendChild(me);

    me.setAttribute("parent-id", m_parent->id());
    me.setAttribute("child-id", m_child->id());
    QString type = "Finish-Start";
    switch (m_type) {
        case FinishStart:
            type = "Finish-Start";
            break;
        case FinishFinish:
            type = "Finish-Finish";
            break;
        case StartStart:
            type = "Start-Start";
            break;
        default:
            break;
    }
    me.setAttribute("type", type);
    me.setAttribute("lag", m_lag.toString());
}

#ifndef NDEBUG
void Relation::printDebug(const QByteArray& _indent) { 
    QString indent = _indent;
    indent += "  ";
    debugPlan<<indent<<"  Parent:"<<m_parent->name();
    debugPlan<<indent<<"  Child:"<<m_child->name();
    debugPlan<<indent<<"  Type:"<<m_type;
}
#endif


}  //KPlato namespace

QDebug operator<<(QDebug dbg, const KPlato::Relation *r)
{
    return dbg<<(*r);
}

QDebug operator<<(QDebug dbg, const KPlato::Relation &r)
{
    KPlato::Node *parent = r.parent();
    KPlato::Node *child = r.child();
    QString type = "FS";
    switch (r.type()) {
    case KPlato::Relation::StartStart: type = "SS"; break;
    case KPlato::Relation::FinishFinish: type = "FF"; break;
    default: break;
    }

    KPlato::Duration lag = r.lag();
    dbg<<"Relation["<<parent->name()<<"->"<<child->name()<<type;
    if (lag != 0) {
        dbg<<lag.toString(KPlato::Duration::Format_HourFraction);
    }
    dbg <<']';
    return dbg;
}
