/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2009 Dag Andersen <calligra-devel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTTASKDESCRIPTIONDIALOG_H
#define KPTTASKDESCRIPTIONDIALOG_H

#include "planui_export.h"
#include "PlanMacros.h"

#include "ui_kpttaskdescriptionpanelbase.h"

#include <KoDialog.h>

namespace KPlato
{

class TaskDescriptionPanel;
class Task;
class Node;
class MacroCommand;
        
class TaskDescriptionPanelImpl : public QWidget, public Ui_TaskDescriptionPanelBase
{
    Q_OBJECT
public:
    TaskDescriptionPanelImpl(Node &node, QWidget *parent);
    ~TaskDescriptionPanelImpl() override;
        
public Q_SLOTS:
    virtual void slotChanged();

Q_SIGNALS:
    void textChanged(bool);

protected:
    Node &m_node;

private:
    OBJECTCONNECTIONS;
};

class TaskDescriptionPanel : public TaskDescriptionPanelImpl
{
    Q_OBJECT
public:
    explicit TaskDescriptionPanel(Node &node, QWidget *parent = nullptr, bool readOnly = false);

    MacroCommand *buildCommand();

    bool ok();

    void setStartValues(Node &node);

protected:
    void initDescription(bool readOnly);
};

class PLANUI_EXPORT TaskDescriptionDialog : public KoDialog
{
    Q_OBJECT
public:
    /**
     * The constructor for the task description dialog.
     * @param task the node to show
     * @param parent parent widget
     * @param readOnly determines whether the data are read-only
     */
    explicit TaskDescriptionDialog(Node &node, QWidget *parent = nullptr, bool readOnly = false  );

    MacroCommand *buildCommand();

protected Q_SLOTS:
    void slotButtonClicked(int button) override;

protected:
    TaskDescriptionPanel *m_descriptionTab;
};

} //KPlato namespace

#endif // KPTTASKDESCRIPTIONDIALOG_H
