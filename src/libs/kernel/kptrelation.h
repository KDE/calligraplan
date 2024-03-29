/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2001 Thomas Zander zander @kde.org
   SPDX-FileCopyrightText: 2004-2007 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTRELATION_H
#define KPTRELATION_H

#include "plankernel_export.h"

#include "kptduration.h"

#include <KoXmlReaderForward.h>

#include <QString>

class QDomElement;

/// The main namespace
namespace KPlato
{

class Node;
class Project;
class XmlSaveContext;
class XMLLoaderObject;

/**
  * The relation class couples 2 nodes together which are dependent on each other.
  * If for example you have a project to build a house, the node that represents the 
  * adding of the roof is dependent on the node that represents the building of the walls;
  * the roof can't be put up if the walls are not there yet.
  * We actually have a number of relationtypes so this relation can be used in different manners.
  */
class PLANKERNEL_EXPORT Relation {
public:
    enum Type { FinishStart, FinishFinish, StartStart };

    Relation(Node *parent, Node *child, Type type, Duration lag);
    explicit Relation(Node *parent=nullptr, Node *child=nullptr, Type type=FinishStart);
    explicit Relation(Relation *rel);
    
    /** 
    *  When deleted the relation will remove itself from 
    *  the parent- and child nodes lists
    */
    virtual ~Relation();

    /// Set relation type
    void setType(Type);
    /// Set relation type
    void setType(const QString &type);
    /// Return relation type
    Type type() const { return m_type; }
    /// Return relation type as a string. Translated if @p trans = true.
    QString typeToString(bool trans = false) const;
    /// Convert @p type to a valid relation type
    static Type typeFromString(const QString &type);
    /// Return a stringlist of relation types. Translated if @p trans = true
    static QStringList typeList(bool trans = false);
    
    /**
    * Returns the lag.
    * The lag of a relation is the time it takes between the parent starting/stopping
    * and the start of the child.
    */
    const Duration &lag() const { return m_lag; }
    /// Set relation time lag
    void setLag(Duration lag) { m_lag = lag; }

    /**
     * @return The parent dependent node.
     */
    Node *parent() const { return m_parent; }
    void setParent(Node *node);
    /**
     * @return The child dependent node.
     */
    Node *child() const { return m_child; }
    void setChild(Node *node);

    // NOTE: Saving is done here, loading is done using the XmlLoaderObject
    void save(QDomElement &element, const XmlSaveContext &context) const;

protected: // variables
    Node *m_parent;
    Node *m_child;
    Type m_type;
    Duration m_lag;

private:
    QString m_parentId;
    
#ifndef NDEBUG
public:
    void printDebug(const QByteArray& indent);
#endif

};

class PLANKERNEL_EXPORT ProxyRelation : public Relation
{
public:
    ProxyRelation(Node *parent, Node *child, Relation::Type type, Duration lag) 
    : Relation(parent, child, type, lag) 
    {}

    ~ProxyRelation() override { m_parent = nullptr; m_child = nullptr;}
};

}  //KPlato namespace

PLANKERNEL_EXPORT QDebug operator<<(QDebug dbg, const KPlato::Relation* r);
PLANKERNEL_EXPORT QDebug operator<<(QDebug dbg, const KPlato::Relation& r);

#endif
