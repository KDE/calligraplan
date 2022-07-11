/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 2022 Dag Andersen <dag.andersen@kdemail.net>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "SharedResourcesDialog.h"

#include <kptitemmodelbase.h>
#include <kptproject.h>
#include <kptcommand.h>
#include <RemoveResourceCmd.h>

#include <QStandardItemModel>
#include <QHeaderView>

using namespace KPlato;

SharedResourcesDialog::SharedResourcesDialog(QList<ResourceGroup*> groups,
                                             QList<Resource*> resources,
                                             QList<Calendar*> calendars,
                                             QWidget *parent)
    : KoDialog(parent)
{
    setButtons(KoDialog::Ok);
    ui.setupUi(this);
    setMainWidget(ui.mainWidget);

    QStandardItemModel *m = new QStandardItemModel(this);
    const QStringList headers = { i18nc("@title:column", "Name"), i18nc("@title:column", "Action") };
    m->setHorizontalHeaderLabels(headers);
    if (!calendars.isEmpty()) {
        auto item = new ModelItem(i18n("Calendars"));
        item->setEditable(false);
        m->appendRow(item);
        for (const auto c : calendars) {
            auto item1 = new ModelItem();
            item1->calendar = c;
            item1->isActionItem = true;
            item1->setEditable(false);
            auto item2 = new ModelItem(c->name());
            item2->calendar = c;
            item2->setEditable(true);
            m->appendRow(QList<QStandardItem*>()<<item2<<item1);
        }
    }
    for (const auto g : groups) {
        auto item1 = new ModelItem();
        item1->group = g;
        item1->isActionItem = true;
        item1->setEditable(false);
        auto item2 = new ModelItem(g->name());
        item2->group = g;
        item2->setEditable(true);
        m->appendRow(QList<QStandardItem*>()<<item2<<item1);
    }
    if (!resources.isEmpty()) {
        auto item = new ModelItem(i18n("Resources"));
        item->setEditable(false);
        m->appendRow(QList<QStandardItem*>()<<item<<new ModelItem);
        for (const auto r : resources) {
            auto item1 = new ModelItem();
            item1->resource = r;
            item1->isActionItem = true;
            item1->setEditable(false);
            auto item2 = new ModelItem(r->name());
            item2->resource = r;
            item2->setEditable(true);
            item->appendRow(QList<QStandardItem*>()<<item2<<item1);
        }
    }
    ui.resourceView->setModel(m);
    ui.resourceView->setItemDelegateForColumn(1, new EnumDelegate(this));
    ui.resourceView->expandAll();
    slotSelectionChanged();
    connect(ui.resourceView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &SharedResourcesDialog::slotSelectionChanged);
    connect(ui.resourceView->selectionModel(), &QItemSelectionModel::currentChanged, this, &SharedResourcesDialog::slotSelectionChanged);

    connect(ui.markAsConvert, &QPushButton::clicked, this, &SharedResourcesDialog::markAsConvert);
    connect(ui.markAsRemove, &QPushButton::clicked, this, &SharedResourcesDialog::markAsRemove);
    connect(ui.markAsKeep, &QPushButton::clicked, this, &SharedResourcesDialog::markAsKeep);

    setWhatsThis(xi18nc("@info:whatsthis",
                        "<title>Shared Resource Dialog</title>"
                        "<para>"
                        "Enables you to decide how to treat shared resources, shared resource groups and shared calendars"
                        " when they have been deleted from the shared resources file."
                        "</para><para>"
                        "You can select if an item shall be:"
                        "<list>"
                        "<item><emphasis>Removed:</emphasis> Removes the item from your project too.</item>"
                        "<item><emphasis>Converted:</emphasis> Converts the item to local item.</item>"
                        "<item><emphasis>Kept:</emphasis> Keeps the item as a shared item.</item>"
                        "</list>"
                        "<nl/><link url='%1'>More...</link>"
                        "</para>", QStringLiteral("plan:managing-resources#shared-resources")));
}

KUndo2Command *SharedResourcesDialog::buildCommand() const
{
    MacroCommand *cmd = new MacroCommand();

    const auto model = static_cast<QStandardItemModel*>(ui.resourceView->model());
    for (int r = 0; r < model->rowCount(); ++r) {
        const auto parent = static_cast<ModelItem*>(model->item(r));
        debugPlanShared<<"parent:"<<parent;
        for (int row = 0; row < parent->rowCount(); ++row) {
            const auto item = static_cast<ModelItem*>(parent->child(row, 1));
            debugPlanShared<<"item:"<<item;
            if (item->group) {
                auto project = item->group->project();
                Q_ASSERT(project);
                switch (item->data(Role::EnumListValue).toInt()) {
                    case 0: // remove
                        if (item->group->resources().isEmpty()) {
                            cmd->addCommand(new RemoveResourceGroupCmd(project, item->group));
                            break;
                        }
                        // we may have put local resource(s) in this group so we need to keep it
                        // FIXME: Handle in model or warning or...
                        Q_FALLTHROUGH();
                    case 1: // convert
                        cmd->addCommand(new ModifyGroupOriginCmd(item->group, false));
                        cmd->addCommand(new ModifyGroupIdCmd(item->group));
                        break;
                    default: // keep
                        break;
                }
            } else if (item->resource) {
                auto project = item->resource->project();
                Q_ASSERT(project);
                switch (item->data(Role::EnumListValue).toInt()) {
                    case 0: { // remove
                        cmd->addCommand(new RemoveResourceCmd(item->resource));
                        break;
                    }
                    case 1: { // convert
                        cmd->addCommand(new ModifyResourceOriginCmd(item->resource, false));
                        cmd->addCommand(new ModifyResourceIdCmd(item->resource));
                        break;
                    }
                    default: // keep
                        break;
                }
            } else if (item->calendar) {
                auto project = item->calendar->project();
                Q_ASSERT(project);
                switch (item->data(Role::EnumListValue).toInt()) {
                    case 0: { // remove
                        cmd->addCommand(new CalendarRemoveCmd(project, item->calendar));
                        break;
                    }
                    case 1: // convert
                        cmd->addCommand(new ModifyCalendarOriginCmd(item->calendar, false));
                        cmd->addCommand(new ModifyCalendarIdCmd(item->calendar));
                        break;
                    default: // keep
                        break;
                }
            } else {
                warnPlanShared<<"Internal error, no object to remove";
            }
        }
    }
    if (cmd->isEmpty()) {
        delete cmd;
        cmd = nullptr;
    }
    return cmd;
}

void SharedResourcesDialog::slotSelectionChanged()
{
    const auto indexes = ui.resourceView->selectionModel()->selectedRows();
    for (const auto idx : indexes) {
        if (idx.parent().isValid()) {
            ui.markAsRemove->setEnabled(true);
            ui.markAsConvert->setEnabled(true);
            ui.markAsKeep->setEnabled(true);
            return;
        }
    }
    ui.markAsRemove->setEnabled(false);
    ui.markAsConvert->setEnabled(false);
    ui.markAsKeep->setEnabled(false);
}

void SharedResourcesDialog::setDefaultAction(int action)
{
    auto model = qobject_cast<QStandardItemModel*>(ui.resourceView->model());
    for (int row = 0; row < model->rowCount(); ++row) {
        const auto item = static_cast<ModelItem*>(model->itemFromIndex(model->index(row, 1)));
        if (item->isActionItem) {
            item->setData(action);
        }
        const auto parent = static_cast<ModelItem*>(model->itemFromIndex(model->index(row, 0)));
        for (int row = 0; row < parent->rowCount(); ++row) {
            auto i = static_cast<ModelItem*>(parent->child(row, 1));
            if (i->isActionItem) {
                i->setData(action);
            }
        }
    }
}

void SharedResourcesDialog::setAction(int action)
{
    const auto model = qobject_cast<QStandardItemModel*>(ui.resourceView->model());
    const auto indexes = ui.resourceView->selectionModel()->selectedRows(1);
    for (const auto idx : indexes) {
        auto item = static_cast<ModelItem*>(model->itemFromIndex(idx));
        if (item->isActionItem) {
            item->setData(action);
        }
    }
}

void SharedResourcesDialog::markAsRemove()
{
    setAction(0);
}

void SharedResourcesDialog::markAsConvert()
{
    setAction(1);
}

void SharedResourcesDialog::markAsKeep()
{
    setAction(2);
}

// The item
ModelItem::ModelItem(const QString &text)
    : QStandardItem(text)
{

}

QStandardItem *ModelItem::clone() const
{
    return new ModelItem();
}

QVariant ModelItem::data(int role) const
{
    if (isActionItem) {
        static QStringList actions = { i18n("Remove"), i18n("Convert"), i18n("Keep") };
        switch (role) {
            case Qt::DisplayRole: {
                auto value = actions.value(currentAction);
                return value;
            }
            case Role::EnumList: {
                return actions;
            }
            case Qt::EditRole:
            case Role::EnumListValue:
                return currentAction;
            default:
                break;
        }
    }
    return QStandardItem::data(role);
}

void ModelItem::setData(const QVariant &value, int role)
{
    switch (role) {
        case Qt::EditRole:
            currentAction = value.toInt();
            emitDataChanged();
            break;
        default:
            break;
    }
}

ModifyResourceOriginCmd::ModifyResourceOriginCmd(Resource *resource, bool origin, const KUndo2MagicString &name)
    : NamedCommand(name)
    , m_resource(resource)
    , m_neworigin(origin)
{
    m_oldorigin = resource->isShared();
}

void ModifyResourceOriginCmd::execute()
{
    m_resource->setShared(m_neworigin);
}

void ModifyResourceOriginCmd::unexecute()
{
    m_resource->setShared(m_oldorigin);
}

ModifyResourceIdCmd::ModifyResourceIdCmd(Resource *resource, const KUndo2MagicString &name)
    : NamedCommand(name)
    , m_resource(resource)
    , m_project(resource->project())
{
    m_newid = m_project->uniqueResourceId();
    m_oldid = resource->id();
}

void ModifyResourceIdCmd::execute()
{
    m_project->removeResourceId(m_oldid);
    m_resource->setId(m_newid);
    m_project->insertResourceId(m_newid, m_resource);
    const auto resources = m_project->resourceList();
    // update required resources
    for (auto res : resources) {
        res->refreshRequiredIds();
    }
    // update teams
    for (auto res : resources) {
        res->refreshTeamMemberIds();
    }
}

void ModifyResourceIdCmd::unexecute()
{
    m_project->removeResourceId(m_newid);
    m_resource->setId(m_oldid);
    m_project->insertResourceId(m_oldid, m_resource);
    const auto resources = m_project->resourceList();
    // update required resources
    for (auto res : resources) {
        res->refreshRequiredIds();
    }
    // update teams
    for (auto res : resources) {
        res->refreshTeamMemberIds();
    }
}

ModifyGroupOriginCmd::ModifyGroupOriginCmd(ResourceGroup *group, bool origin, const KUndo2MagicString &name)
    : NamedCommand(name)
    , m_group(group)
    , m_neworigin(origin)
{
    m_oldorigin = group->isShared();
}

void ModifyGroupOriginCmd::execute()
{
    m_group->setShared(m_neworigin);
}

void ModifyGroupOriginCmd::unexecute()
{
    m_group->setShared(m_oldorigin);
}

ModifyGroupIdCmd::ModifyGroupIdCmd(ResourceGroup *group, const KUndo2MagicString &name)
    : NamedCommand(name)
    , m_group(group)
    , m_project(group->project())
{
    m_newid = m_project->uniqueResourceGroupId();
    m_oldid = group->id();
}

void ModifyGroupIdCmd::execute()
{
    m_project->removeResourceGroupId(m_oldid);
    m_group->setId(m_newid);
    m_project->insertResourceGroupId(m_newid, m_group);
}

void ModifyGroupIdCmd::unexecute()
{
    m_project->removeResourceGroupId(m_newid);
    m_group->setId(m_oldid);
    m_project->insertResourceGroupId(m_oldid, m_group);
}

ModifyCalendarOriginCmd::ModifyCalendarOriginCmd(Calendar *calendar, bool origin, const KUndo2MagicString &name)
    : NamedCommand(name)
    , m_calendar(calendar)
    , m_neworigin(origin)
{
    m_oldorigin = calendar->isShared();
}

void ModifyCalendarOriginCmd::execute()
{
    m_calendar->setShared(m_neworigin);
}

void ModifyCalendarOriginCmd::unexecute()
{
    m_calendar->setShared(m_oldorigin);
}

ModifyCalendarIdCmd::ModifyCalendarIdCmd(Calendar *calendar, const KUndo2MagicString &name)
    : NamedCommand(name)
    , m_calendar(calendar)
    , m_project(calendar->project())
{
    m_newid = m_project->uniqueCalendarId();
    m_oldid = calendar->id();
}

void ModifyCalendarIdCmd::execute()
{
    m_project->removeCalendarId(m_oldid);
    m_calendar->setId(m_newid);
    m_project->insertCalendarId(m_newid, m_calendar);
}

void ModifyCalendarIdCmd::unexecute()
{
    m_project->removeCalendarId(m_newid);
    m_calendar->setId(m_oldid);
    m_project->insertCalendarId(m_oldid, m_calendar);
}

QDebug operator<<(QDebug dbg, const ModelItem *item)
{
    dbg.noquote().nospace()<<"ModelItem["<<item->data(Qt::DisplayRole).toString();
    if (item->isActionItem) {
        dbg<<" A";
        if (item->calendar) dbg<<"C: "<<item->calendar->name();
        if (item->group) dbg<<"G: "<<item->group->name();
        if (item->resource) dbg<<"R: "<<item->resource->name();
    }
    dbg<<']';
    dbg.space().quote();
    return dbg;
}
