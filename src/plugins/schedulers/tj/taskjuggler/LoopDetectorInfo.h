/*
 * LoopDetectorInfo.h - TaskJuggler
 *
 * SPDX-FileCopyrightText: 2002, 2003, 2004, 2005, 2006 Chris Schlaeger <cs@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * $Id$
 */

#ifndef _LoopDetectorInfo_h_
#define _LoopDetectorInfo_h_

namespace TJ
{


class LoopDetectorInfo;

/**
 * This class stores information about a waypoint the dependency loop
 * detector passes when looking for loops.
 *
 * @short Utility class for the dependency loop detector.
 * @author Chris Schlaeger <cs@kde.org>
 */
class LoopDetectorInfo
{
    friend class LDIList;
public:
    LoopDetectorInfo() :
        nextLDI(nullptr),
        prevLDI(nullptr),
        t(nullptr),
        atEnd(false)
    { }

    LoopDetectorInfo(const Task* _t, bool ae) :
        nextLDI(nullptr),
        prevLDI(nullptr),
        t(_t),
        atEnd(ae)
    { }

    ~LoopDetectorInfo() { }

    bool operator==(const LoopDetectorInfo& ldi) const
    {
        return t == ldi.t && atEnd == ldi.atEnd;
    }
    bool operator!=(const LoopDetectorInfo& ldi) const
    {
        return t != ldi.t || atEnd != ldi.atEnd;
    }
    const Task* getTask() const { return t; }
    bool getAtEnd() const { return atEnd; }
    LoopDetectorInfo* next() const { return nextLDI; }
    LoopDetectorInfo* prev() const { return prevLDI; }
protected:
    LoopDetectorInfo* nextLDI;
    LoopDetectorInfo* prevLDI;
private:
    const Task* t;
    bool atEnd;
} ;

/**
 * This class stores the waypoints the dependency loop detector passes when
 * looking for loops. Since it is very performance critical we use a
 * handrolled list class instead of a Qt class.
 *
 * @short Waypoint list of the dependency loop detector.
 * @author Chris Schlaeger <cs@kde.org>
 */
class LDIList
{
public:
    LDIList() :
        items(0),
        root(nullptr),
        leaf(nullptr)
    { }

    LDIList(const LDIList& list) :
        items(0),
        root(nullptr),
        leaf(nullptr)
    {
        for (LoopDetectorInfo* p = list.root; p; p = p->nextLDI)
            append(new LoopDetectorInfo(p->t, p->atEnd));
    }

    virtual ~LDIList()
    {
        for (LoopDetectorInfo* p = root; p; p = root)
        {
            root = p->nextLDI;
            delete p;
        }
    }
    LoopDetectorInfo* first() const { return root; }
    LoopDetectorInfo* last() const { return leaf; }
    long count() const { return items; }

    bool find(const LoopDetectorInfo* ref) const
    {
        for (LoopDetectorInfo* p = root; p; p = p->nextLDI)
            if (*p == *ref)
                return true;

        return false;
    }

    void append(LoopDetectorInfo* p)
    {
        if (root == nullptr)
        {
            root = leaf = p;
            leaf->prevLDI = nullptr;
        }
        else
        {
            leaf->nextLDI = p;
            p->prevLDI = leaf;
            leaf = leaf->nextLDI;
        }
        leaf->nextLDI = nullptr;
        ++items;
    }
    void removeLast()
    {
        if (leaf == root)
        {
            delete leaf;
            root = leaf = nullptr;
        }
        else
        {
            leaf = leaf->prevLDI;
            delete leaf->nextLDI;
            leaf->nextLDI = nullptr;
        }
        --items;
    }
    LoopDetectorInfo* popLast()
    {
        LoopDetectorInfo* lst = leaf;
        if (leaf == root)
            root = leaf = nullptr;
        else
        {
            leaf = leaf->prevLDI;
            leaf->nextLDI = nullptr;
        }
        --items;
        lst->prevLDI = lst->nextLDI = nullptr;
        return lst;
    }
private:
    long items;
    LoopDetectorInfo* root;
    LoopDetectorInfo* leaf;
} ;

} // namespace TJ

#endif

