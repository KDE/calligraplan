/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 2022 Dag Andersen <dag.andersen@kdemail.net>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef SHAREDRESOURCESDIALOG_H
#define SHAREDRESOURCESDIALOG_H

#include <KoDialog.h>
#include "ui_SharedResourcesDialog.h"

#include <Resource.h>
#include <kptcalendar.h>
#include <ResourceGroup.h>
#include <NamedCommand.h>

#include <QStandardItem>
#include <QUndoCommand>

class SharedResourcesDialog : public KoDialog
{
    Q_OBJECT
public:
    SharedResourcesDialog(QList<KPlato::ResourceGroup*> groups,
                          QList<KPlato::Resource*> resources,
                          QList<KPlato::Calendar*> calendars,
                          QWidget *parent = nullptr);

    KUndo2Command *buildCommand() const;

public Q_SLOTS:
    void setDefaultAction(int action);
    void setAction(int action);
    void markAsKeep();
    void markAsRemove();
    void markAsConvert();

private Q_SLOTS:
    void slotSelectionChanged();

private:
    Ui::SharedResourcesDialog ui;
};

class ModelItem : public QStandardItem
{
public:
    ModelItem(const QString &text = QString());
    QStandardItem *clone() const override;
    QVariant data(int role = Qt::UserRole) const override;
    void setData(const QVariant &value, int role = Qt::EditRole) override;

public:
    bool isActionItem = false;
    KPlato::ResourceGroup *group = nullptr;
    KPlato::Resource *resource = nullptr;
    KPlato::Calendar *calendar = nullptr;
    int currentAction = 0;
};

class ModifyResourceOriginCmd : public KPlato::NamedCommand
{
public:
    explicit ModifyResourceOriginCmd(KPlato::Resource *resource, bool origin, const KUndo2MagicString &name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;
private:
    KPlato::Resource *m_resource;
    bool m_neworigin;
    bool m_oldorigin;
};

class ModifyResourceIdCmd : public KPlato::NamedCommand
{
public:
    explicit ModifyResourceIdCmd(KPlato::Resource *resource, const KUndo2MagicString &name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;
private:
    KPlato::Resource *m_resource;
    KPlato::Project *m_project;
    QString m_newid;
    QString m_oldid;
};

class ModifyGroupOriginCmd : public KPlato::NamedCommand
{
public:
    explicit ModifyGroupOriginCmd(KPlato::ResourceGroup *group, bool origin, const KUndo2MagicString &name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;
private:
    KPlato::ResourceGroup *m_group;
    bool m_neworigin;
    bool m_oldorigin;
};

class ModifyGroupIdCmd : public KPlato::NamedCommand
{
public:
    explicit ModifyGroupIdCmd(KPlato::ResourceGroup *group, const KUndo2MagicString &name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;
private:
    KPlato::ResourceGroup *m_group;
    KPlato::Project *m_project;
    QString m_newid;
    QString m_oldid;
};

class ModifyCalendarOriginCmd : public KPlato::NamedCommand
{
public:
    explicit ModifyCalendarOriginCmd(KPlato::Calendar *calendar, bool origin, const KUndo2MagicString &name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;
private:
    KPlato::Calendar *m_calendar;
    bool m_neworigin;
    bool m_oldorigin;
};

class ModifyCalendarIdCmd : public KPlato::NamedCommand
{
public:
    explicit ModifyCalendarIdCmd(KPlato::Calendar *calendar, const KUndo2MagicString &name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;
private:
    KPlato::Calendar *m_calendar;
    KPlato::Project *m_project;
    QString m_newid;
    QString m_oldid;
};

QDebug operator<<(QDebug dbg, const ModelItem *item);

#endif // SHAREDRESOURCESDIALOG_H
