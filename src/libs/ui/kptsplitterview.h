/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2007 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTSPLITTERVIEW_H
#define KPTSPLITTERVIEW_H

#include "planui_export.h"

#include "kptviewbase.h"

#include <KoXmlReaderForward.h>


class KoDocument;

class QSplitter;
class QTabWidget;

/// The main namespace
namespace KPlato
{

class Project;
class Node;
class Resource;
class ResourceGroup;
class Calendar;
class Relation;
class Context;

/**
 * SplitterView is a view with a vertical QSplitter that can contain
 * other ViewBase based views and/or QTabWidgets that can hold
 * ViewBase based views as well.
 * This splitter view is created by the main View, and subviews can then be
 * added with addView(). A QTabWidget is added with addTabWidget().
 *
 * To have context info loaded, views added to this splitter must have a
 * unique objectName().
*/
class PLANUI_EXPORT SplitterView : public ViewBase
{
    Q_OBJECT
public:
    /// Constructor
    SplitterView(KoPart *part, KoDocument *doc, QWidget *parent);
    /// Destructor
    ~SplitterView() override {}

    /// Set the project this view shall handle.
    void setProject(Project *project) override;
    /// Draw data from current part / project
    void draw() override;
    /// Draw data from project.
    void draw(Project &project) override;
    /// Set readWrite mode
    void updateReadWrite(bool) override;

    /// Return the view that has focus
    ViewBase *focusView() const;
    /// Returns the list of context menu actions for the active view
    virtual QList<QAction*> contextActionList() const;

    /// Sets context info to this view. Reimplement.
    virtual bool setContext(const Context &/*context*/) { return false; }
    /// Gets context info from this view. Reimplement.
    virtual void getContext(Context &/*context*/) const {}
    
    /// Add a QTabWidget to the splitter
    QTabWidget *addTabWidget();
    /// Add the @p view to the splitter
    void addView(ViewBase *view);
    /// Add the @p view to the @p tab. Set the tabs label to @p label
    void addView(ViewBase *view, QTabWidget *tab, const QString &label);
    /// Return the active view at @p pos
    ViewBase *findView(const QPoint &pos) const;

    /// Loads context info into this view. Reimplement.
    bool loadContext(const KoXmlElement &/*context*/) override;
    /// Save context info from this view. Reimplement.
    void saveContext(QDomElement &/*context*/) const override;
    
    Node* currentNode() const override;
    
    Resource* currentResource() const override;
    
    ResourceGroup* currentResourceGroup() const override;

    Calendar* currentCalendar() const override;

    Relation *currentRelation() const override;

public Q_SLOTS:
    /// Activate/deactivate the gui (also of subviews)
    void setGuiActive(bool activate) override;
    void setScheduleManager(KPlato::ScheduleManager *sm) override;
    void slotEditCopy() override;

protected Q_SLOTS:
    virtual void slotGuiActivated(KPlato::ViewBase *v, bool active);
    virtual void currentTabChanged(int i);
    
protected:
    QSplitter *m_splitter;
    ViewBase *m_activeview;
};


} // namespace KPlato

#endif
