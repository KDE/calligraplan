/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2002 The calligra team <calligra@kde.org>
   SPDX-FileCopyrightText: 2003-2010 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTRELATIONDIALOG_H
#define KPTRELATIONDIALOG_H

#include "planui_export.h"

#include "ui_relationpanel.h"
#include <KoDialog.h>

#include <QWidget>

namespace KPlato
{

class RelationPanel;

class Relation;
class Project;
class Node;
class MacroCommand;

class RelationPanel : public QWidget, public Ui_RelationPanel
{
    Q_OBJECT
public:
    explicit RelationPanel(QWidget *parent = nullptr);
};

class PLANUI_EXPORT AddRelationDialog : public KoDialog
{
    Q_OBJECT
public:
    AddRelationDialog(Project &project, Relation *rel, QWidget *p, const QString& caption=QString(), ButtonCodes buttons=Ok|Cancel);
    ~AddRelationDialog() override;

    virtual MacroCommand *buildCommand();
    int selectedRelationType() const;

protected Q_SLOTS:
    void slotOk();
    void lagChanged();
    void typeClicked(int);
    void slotFinishStartToggled(bool);
    void slotFinishFinishToggled(bool);
    void slotStartStartToggled(bool);
    void slotNodeRemoved(KPlato::Node *node);

protected:
    Project &m_project;
    RelationPanel *m_panel;
    Relation *m_relation;
    bool m_deleterelation;
};


class PLANUI_EXPORT ModifyRelationDialog : public AddRelationDialog
{
    Q_OBJECT
public:
    explicit ModifyRelationDialog(Project &project, Relation *rel, QWidget *p=nullptr);

    MacroCommand *buildCommand() override;
    bool relationIsDeleted() { return m_deleted; }

protected Q_SLOTS:
    void slotUser1();

    void slotRelationRemoved(KPlato::Relation *relation);

private:
    bool m_deleted;
};

}  //KPlato namespace

#endif // RELATIONDIALOG_H
