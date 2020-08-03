/*
 * ResourceList.h - TaskJuggler
 *
 * Copyright (c) 2001, 2002, 2003, 2004 by Chris Schlaeger <cs@kde.org>
 * Copyright (c) 2011 by Dag Andersen <dag.andersen@kdemail.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * $Id$
 */

#ifndef _ResourceList_h_
#define _ResourceList_h_

#include "plantj_export.h"

#include "CoreAttributesList.h"

class QString;

namespace TJ
{

class Resource;

/**
 * @short A list of resources.
 * @author Chris Schlaeger <cs@kde.org>
 */
class PLANTJ_EXPORT ResourceList : public CoreAttributesList
{
public:
    ResourceList();
    ~ResourceList() override { }

    Resource* getResource(const QString& id) const;

    static bool isSupportedSortingCriteria(int sc);

    int compareItemsLevel(CoreAttributes* c1, CoreAttributes* c2,
                                  int level) override;
} ;

/**
 * @short Iterator class for ResourceList objects.
 * @author Chris Schlaeger <cs@kde.org>
 */
class ResourceListIterator : public virtual CoreAttributesListIterator
{
public:
    explicit ResourceListIterator(const CoreAttributesList& l) :
        CoreAttributesListIterator(l)
    { }

    ~ResourceListIterator() override { }

    Resource* operator*();
} ;

} // namespace TJ

#endif
