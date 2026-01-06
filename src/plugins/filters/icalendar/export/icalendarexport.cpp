/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2009, 2011, 2012 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "icalendarexport.h"
#include "ICalExportDialog.h"
#include "config.h"

#include <kptproject.h>
#include <kpttask.h>
#include <kptnode.h>
#include <kptresource.h>
#include <kptdocuments.h>
#include "kptdebug.h"

#include <KCalendarCore/Attendee>
#include <KCalendarCore/Attachment>
#include <KCalendarCore/CalFormat>
#include <KCalendarCore/ICalFormat>
#include <KCalendarCore/MemoryCalendar>
#include <kcalendarcore_version.h>

#include <QByteArray>
#include <QString>
#include <QFile>
#include <QTextEdit>

#include <KPluginFactory>

#include <KoFilterChain.h>
#include <KoFilterManager.h>
#include <KoDocument.h>


using namespace KPlato;

K_PLUGIN_FACTORY_WITH_JSON(ICalendarExportFactory, "plan_icalendar_export.json",
                           registerPlugin<ICalendarExport>();)


const QLoggingCategory &PLAN_ICAL_EXPORT_LOG()
{
    static const QLoggingCategory category("calligra.plan.ical.export");
    return category;
}
#define debugPlanICalExport qCDebug(PLAN_ICAL_EXPORT_LOG)<<Q_FUNC_INFO

ICalendarExport::ICalendarExport(QObject* parent, const QVariantList &)
        : KoFilter(parent)
{
}

KoFilter::ConversionStatus ICalendarExport::convert(const QByteArray& from, const QByteArray& to)
{
    debugPlanICalExport << from << to;
    if ((from != "application/x-vnd.kde.plan") || (to != "text/calendar")) {
        return KoFilter::NotImplemented;
    }
    bool batch = false;
    if (m_chain->manager()) {
        batch = m_chain->manager()->getBatchMode();
    }
    if (batch) {
        //TODO
        debugPlanICalExport<<"batch";
        return KoFilter::UsageError;
    }
    debugPlanICalExport<<"online:"<<m_chain->inputDocument();
    KoDocument *doc = m_chain->inputDocument();
    if (!doc || !doc->project()) {
        errorPlan << "Cannot open Plan document";
        return KoFilter::InternalError;
    }
    if (m_chain->outputFile().isEmpty()) {
        errorPlan << "Output filename is empty";
        return KoFilter::InternalError;
    }
    QFile file(m_chain->outputFile());
    if (! file.open(QIODevice::WriteOnly)) {
        errorPlan << "Failed to open output file:" << file.fileName();
        return KoFilter::StorageCreationError;
    }
    QApplication::restoreOverrideCursor();
    ICalExportDialog dlg(*doc->project());
    if (dlg.exec() != QDialog::Accepted) {
        QApplication::setOverrideCursor(Qt::WaitCursor);
        return KoFilter::UserCancelled;
    }
    QApplication::setOverrideCursor(Qt::WaitCursor);
    m_scheduleId = dlg.scheduleId();
    m_includeProject = dlg.includeProject();
    m_includeSummarytasks = dlg.includeSummarytasks();
    KoFilter::ConversionStatus status = convert(*doc->project(), file);
    file.close();
    debugPlanICalExport<<"Finished with status:"<<status;
    return status;
}

KoFilter::ConversionStatus ICalendarExport::convert(const Project &project, QFile &file)
{
    KCalendarCore::Calendar::Ptr cal(new KCalendarCore::MemoryCalendar("UTC"));

    //TODO: schedule selection dialog
    long id = ANYSCHEDULED;
    bool baselined = project.isBaselined(id);
    const QList<ScheduleManager*> lst = project.allScheduleManagers();
    for (const ScheduleManager *m : lst) {
        if (! baselined) {
            id = lst.last()->scheduleId();
            //debugPlanICalExport<<"last:"<<id;
            break;
        }
        if (m->isBaselined()) {
            id = m->scheduleId();
            //debugPlanICalExport<<"baselined:"<<id;
            break;
        }
    }
    //debugPlanICalExport<<id;
    createTodos(cal, &project, id);

    KCalendarCore::ICalFormat format;
    qint64 n = file.write(format.toString(cal).toUtf8());
    if (n < 0) {
        return KoFilter::InternalError;
    }
    return KoFilter::OK;
}

void ICalendarExport::createTodos(KCalendarCore::Calendar::Ptr cal, const Node *node, long id, KCalendarCore::Todo::Ptr parent)
{
    KCalendarCore::Todo::Ptr todo(new KCalendarCore::Todo());
    todo->setUid(node->id());
    todo->setSummary(node->name());
    todo->setDescription(node->description());
    todo->setCategories(QStringLiteral("Plan"));
    if (! node->projectNode()->leader().isEmpty()) {
        todo->setOrganizer(node->projectNode()->leader());
    }
    if (node->type() != Node::Type_Project && ! node->leader().isEmpty()) {
        KCalendarCore::Person p = KCalendarCore::Person::fromFullName(node->leader());
        KCalendarCore::Attendee a(p.name(), p.email());
        a.setRole(KCalendarCore::Attendee::NonParticipant);
        todo->addAttendee(a);
    }
    DateTime st = node->startTime(id);
    DateTime et = node->endTime(id);
    if (st.isValid()) {
        todo->setDtStart(QDateTime(st));
    }
    if (et.isValid()) {
        todo->setDtDue(QDateTime(et));
    }
    if (node->type() == Node::Type_Task) {
        const Task *task = qobject_cast<Task*>(const_cast<Node*>(node));
        Schedule *s = task->schedule(id);
        if (id < 0 || s == nullptr) {
            // Not scheduled, use requests
            const QList<Resource*> lst = task->requestedResources();
            for (const Resource *r : lst) {
                if (r->type() == Resource::Type_Work) {
                    todo->addAttendee(KCalendarCore::Attendee(r->name(), r->email()));
                }
            }
        } else {
            const auto resources = s->resources();
            for (const Resource *r : resources) {
                if (r->type() == Resource::Type_Work) {
                    todo->addAttendee(KCalendarCore::Attendee(r->name(), r->email()));
                }
            }

        }
    } else if (node->type() == Node::Type_Milestone) {
        const Task *task = qobject_cast<Task*>(const_cast<Node*>(node));
        todo->setDtStart(QDateTime());
        todo->setPercentComplete(task->completion().percentFinished());
    }
    const auto documents = node->documents().documents();
    for (const Document *doc : documents) {
        todo->addAttachment(KCalendarCore::Attachment(doc->url().url()));
    }
    if (! parent.isNull()) {
        todo->setRelatedTo(parent->uid(), KCalendarCore::Incidence::RelTypeParent);
    }
    cal->addTodo(todo);
    const auto nodes = node->childNodeIterator();
    for (const Node *n : nodes) {
        createTodos(cal, n, id, todo);
    }
}
#include "icalendarexport.moc"
