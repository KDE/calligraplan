/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2017 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef PROJECTVIEW_H
#define PROJECTVIEW_H

#include "planui_export.h"

#include "kptglobal.h"
#include "kptviewbase.h"


class KoDocument;

class KActionMenu;

class QWidget;
class QTableView;

namespace KPlato
{


class PLANUI_EXPORT ProjectView : public ViewBase
{
    Q_OBJECT
public:
    ProjectView(KoPart *part, KoDocument *doc, QWidget *parent);

    void setupGui();

    /// Loads context info into this view. Reimplement.
    virtual bool loadContext(const KoXmlElement &/*context*/);
    /// Save context info from this view. Reimplement.
    virtual void saveContext(QDomElement &/*context*/) const;


public Q_SLOTS:
    /// Activate/deactivate the gui
    virtual void setGuiActive(bool activate);

protected:
    void updateActionsEnabled(bool on);
    int selectedRowCount() const;
    QModelIndexList selectedRows() const;

    bool createConnection();

protected Q_SLOTS:
    virtual void slotOptions();

private Q_SLOTS:
    void slotSelectionChanged();
    void slotCurrentChanged(const QModelIndex&, const QModelIndex&);
    void slotContextMenuRequested(const QPoint &pos);

    void slotEnableActions();

private:
    QTableView *m_view;

};


} //namespace KPlato


#endif
