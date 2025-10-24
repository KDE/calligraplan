/*
 * CoreAttributes.h - TaskJuggler
 *
 * SPDX-FileCopyrightText: 2001, 2002, 2003, 2004, 2005 Chris Schlaeger <cs@kde.org>
 * SPDX-FileCopyrightText: 2011 Dag Andersen <dag.andersen@kdemail.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * $Id$
 */

// clazy:excludeall=qstring-arg
#include "CoreAttributes.h"
#include "CoreAttributesList.h"
#include "CustomAttributeDefinition.h"
// #include "TextAttribute.h"
// #include "ReferenceAttribute.h"
#include "Task.h"

namespace TJ
{


CoreAttributes::CoreAttributes(Project* p, const QString& i,
                               const QString& n,
                               CoreAttributes* parent_, const QString& df,
                               uint dl) :
    project(p),
    id(i),
    name(n),
    parent(parent_),
    definitionFile(df),
    definitionLine(dl),
    sequenceNo(0),
    hierarchNo(0),
    index(-1),
    hierarchIndex(0),
    sub(new CoreAttributesList()),
    flags(),
    customAttributes()
{
//     customAttributes.setAutoDelete(true);
    if (parent_)
        parent_->sub->append(this);
    
//     qDebug()<<"CoreAttributes:"<<this;
}

CoreAttributes::~CoreAttributes()
{
    //qDebug()<<"~CoreAttributes:"<<this;
    while (!sub->isEmpty())
        delete sub->takeFirst();
    if (parent && parent->sub->contains(this))
        parent->sub->removeAt(parent->sub->indexOf(this));
    delete sub;
    qDeleteAll(customAttributes);
}

uint
CoreAttributes::treeLevel() const
{
    uint tl = 0;
    for (CoreAttributes* c = parent; c; c = c->parent)
        tl++;
    return tl;
}

CoreAttributesList
CoreAttributes::getSubList() const
{
   return *sub;
}

CoreAttributesListIterator
CoreAttributes::getSubListIterator() const
{
    return CoreAttributesListIterator(*sub);
}

bool
CoreAttributes::hasSubs() const
{
    return !sub->isEmpty();
}

void
CoreAttributes::setHierarchNo(uint no)
{
    hierarchNo = no;
    uint hNo = 1;
    for (CoreAttributes *a : std::as_const(*sub)) {
        a->setHierarchNo(hNo++);
    }
}

QString
CoreAttributes::getHierarchNo() const
{
    QString text;
    const CoreAttributes* ca = this;
    do
    {
        if (!text.isEmpty())
            text.prepend(QLatin1Char('.'));
        text = QString("%1").arg(ca->hierarchNo) + text;
        ca = ca->getParent();
    }
    while (ca);
    return text;
}

void
CoreAttributes::setHierarchIndex(uint no)
{
    if (no == 0)
    {
        hierarchIndex = 0;
        return;
    }
    /* If there is no parent, we take the passed number. */
    if (!parent)
    {
        hierarchIndex = no;
        return;
    }

    /* Find the highest hierarchIndex of all children of this CAs parent. */
    uint max = 0;
    const auto lst = *parent->sub;
    for (CoreAttributes *a : lst) {
        if (a->hierarchIndex > max)
            max = a->hierarchIndex;
    }
    /* The index is then the highest found + 1. */
    hierarchIndex = max + 1;
}

QString
CoreAttributes::getHierarchIndex() const
{
    QString text;
    const CoreAttributes* ca = this;
    while (ca)
    {
        if (!text.isEmpty())
            text.prepend(QLatin1Char('.'));
        text = QString("%1").arg(ca->hierarchIndex) + text;
        ca = ca->getParent();
    }
    return text;
}

QString
CoreAttributes::getHierarchLevel() const
{
    return QString("%1").arg(treeLevel());
}

void
CoreAttributes::getFullName(QString& fullName) const
{
    fullName.clear();
    for (const CoreAttributes* c = this; c != nullptr; c = c->parent)
        fullName = c->name + QLatin1Char('.') + fullName;
    // Remove trailing dot.
    fullName.remove(fullName.length() - 1, 1);
}

QString
CoreAttributes::getFullId() const
{
    QString fullID = id;
    for (const CoreAttributes* c = parent; c != nullptr; c = c->parent)
        fullID = c->id + QLatin1Char('.') + fullID;
    return fullID;
}

bool
CoreAttributes::hasSameAncestor(const CoreAttributes* c) const
{
    if (c == nullptr)
        return false;

    CoreAttributes const* p1;
    for (p1 = this; p1->parent; p1 = p1->parent)
        ;
    CoreAttributes const* p2;
    for (p2 = c; p2->parent; p2 = p2->parent)
        ;
    return p1 == p2;
}

bool
CoreAttributes::isDescendantOf(const CoreAttributes* c) const
{
    if (c == nullptr)
        return false;

    for (CoreAttributes const* p = this->parent; p; p = p->parent)
        if (p == c)
            return true;

    return false;
}

bool
CoreAttributes::isParentOf(const CoreAttributes* c) const
{
    if (!c)
        return false;

    for (CoreAttributes const* p = c->parent; p; p = p->parent)
        if (p == this)
            return true;

    return false;
}

bool
CoreAttributes::isLeaf() const
{
    return sub->isEmpty();
}

void
CoreAttributes::addCustomAttribute(const QString& id, CustomAttribute* ca)
{
    customAttributes.insert(id, ca);
}

const CustomAttribute*
CoreAttributes::getCustomAttribute(const QString& id) const
{
    return customAttributes[id];
}

void
CoreAttributes::inheritCustomAttributes
(const QMap<QString, CustomAttributeDefinition*>& dict)
{
    QMap<QString, CustomAttributeDefinition*>::const_iterator cadi = dict.constBegin();
    for (; cadi != dict.constEnd(); ++cadi)
    {
        const CustomAttribute* custAttr;
        if (cadi.value()->getInherit() &&
                (custAttr = parent->getCustomAttribute(cadi.key())))
        {
            switch (custAttr->getType())
            {
            case CAT_Text:
/*                addCustomAttribute(cadi.key(), new TextAttribute
                                   (*(static_cast<const TextAttribute*>(custAttr))));
                break;*/
            case CAT_Reference:
/*                addCustomAttribute(cadi.key(), new ReferenceAttribute
                                   (*(static_cast<const ReferenceAttribute*>(custAttr))));
                break;*/
            default:
                qFatal("CoreAttributes::inheritCustomAttributes: "
                       "Unknown CAT %d", custAttr->getType());
                break;
            }
        }
    }
}

} // namespace TJ

QDebug operator<<(QDebug dbg, const TJ::CoreAttributes* t)
{
    if (t == nullptr) {
        return dbg << (void*)t;
    }
    return operator<<(dbg, *t);
}

QDebug operator<<(QDebug dbg, const TJ::CoreAttributes& t)
{
    switch (t.getType()) {
        case CA_Task: dbg << "Task[" << t.getName() << "]"; break;
        case CA_Resource: dbg << "Resource[" << t.getName() << "]"; break;
        case CA_Account: dbg << "Account[" << t.getName() << "]"; break;
        case CA_Shift: dbg << "Shift[" << t.getName() << "]"; break;
        case CA_Scenario: dbg << "Scenario[" << t.getName() << "]"; break;
        default: dbg << "CoreAttribute[" << t.getName() << "]"; break;
    }
    return dbg;
}
