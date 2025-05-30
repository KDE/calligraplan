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

#ifdef USE_KCalendarCore
#include <KCalendarCore/Attendee>
#include <KCalendarCore/Attachment>
#include <KCalendarCore/Calformat>
#include <KCalendarCore/MemoryCalendar.h>
#include <kcalendarcore_version.h>
#endif

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

#ifndef USE_KCALCORE
QString beginCalendar()
{
    QString s;
    s += QString("BEGIN:VCALENDAR") + "\r\n";
    s += QString("PRODID:-//K Desktop Environment//NONSGML Calligra Plan %1//EN").arg(PLAN_VERSION_STRING) + "\r\n";
    s += QString("VERSION:2.0") + "\r\n";
    s += QString("CALSCALE:GREGORIAN") + "\r\n";
    s += QString("METHOD:PUBLISH") + "\r\n";
    return s;
}
QString endCalendar()
{
    return QString() + QString("END:VCALENDAR") + "\r\n";
}

QString dtToString(const QDateTime &dt)
{
    return dt.toUTC().toString("yyyyMMddTHHmmssZ"); // 20160707T010000Z
}

QString doAttendees(const Node &node, long sid)
{
    QString s;
    Schedule *schedule = node.schedule(sid);
    if (schedule) {
        const auto resources = schedule->resources();
        for (const Resource *r : resources) {
            if (r->type() == Resource::Type_Work) {
                s += QString("ATTENDEE;CN=") + r->name() + "\r\n\t";
                s += QString(";RSVP=FALSE;PARTSTAT=NEEDS-ACTION;ROLE=REQ-PARTICIPANT;") + "\r\n\t";
                s += QString("CUTYPE=INDIVIDUAL;") + "\r\n\t";
                s += QString("X-UID=") + r->id();
                s += ":MAILTO:" + r->email() + "\r\n";
            }
        }
    } else {
        const QList<Resource*> lst = static_cast<const Task&>(node).requestedResources();
        for (const Resource *r :lst) {
            if (r->type() == Resource::Type_Work) {
                s += QString("ATTENDEE;CN=") + r->name() + "\r\n\t";
                s += QString(";RSVP=FALSE;PARTSTAT=NEEDS-ACTION;ROLE=REQ-PARTICIPANT;") + "\r\n\t";
                s += QString("CUTYPE=INDIVIDUAL;") + "\r\n\t";
                s += QString("X-UID=") + r->id();
                s += ":MAILTO:" + r->email() + "\r\n";
            }
        }
    }
    return s;
}

QString doAttachment(const Documents &docs)
{
    QString s;
    const auto documents = docs.documents();
    for (const Document *doc : documents) {
        s += QString("ATTACH:") + doc->url().url() + "\r\n";
    }
    return s;
}

void escape(QString &txt)
{
    txt.replace('\\', "\\\\");
    txt.replace('\n', "\\n");
    txt.replace(',', "\\,");
    txt.replace(':', "\\:");
    txt.replace(';', "\\;");
}

QString ICalendarExport::doDescription(const QString &description)
{
    QTextEdit te;
    te.setHtml(description);
    QString txt = te.toPlainText().trimmed();
    QString s;
    if (!txt.isEmpty()) {
        s = QString("DESCRIPTION") + QString::number(m_descriptions.count()) + ':' + "\r\n";
        escape(txt);
        m_descriptions << txt;
        txt = description;
        txt.remove('\n');
        txt.remove('\r'); // in case...
        escape(txt);
        s += QString("X-ALT-DESC;FMTTYPE=text/html:") + txt + "\r\n";
    }
    return s;
}

QString ICalendarExport::createTodo(const Node &node, long sid)
{
    QString s;
    s += QString("BEGIN:VTODO") + "\r\n";
    QString txt = node.name();
    escape(txt);
    s += QString("SUMMARY:") + txt + "\r\n";
    s += doDescription(node.description());
    s += QString("UID:") + node.id() + "\r\n";
    s += QString("DTSTAMP:") + dtToString(QDateTime::currentDateTime()) + "\r\n";
    s += QString("CREATED:") + dtToString(QDateTime::currentDateTime()) + "\r\n";
    s += QString("LAST-MODIFIED:") + dtToString(QDateTime::currentDateTime()) + "\r\n";
    s += QString("CATEGORIES:Plan") + "\r\n";
    DateTime dt = node.startTime(sid);
    if (dt.isValid()) {
        s += QString("DTSTART:") + dtToString(dt) + "\r\n";
    }
    dt = node.endTime(sid);
    if (dt.isValid()) {
        s += QString("DUE:") + dtToString(dt) + "\r\n";
    }
    if (node.parentNode()) {
        if (m_includeSummarytasks && node.parentNode()->type() == Node::Type_Summarytask) {
            s += QString("RELATED-TO:") + node.parentNode()->id() + "\r\n";
        } else if (m_includeProject) {
            s += QString("RELATED-TO:") + node.projectNode()->id() + "\r\n";
        }
    }
    if (node.type() == Node::Type_Task) {
        s += QString("PERCENT-COMPLETE:") + QString::number(static_cast<const Task&>(node).completion().percentFinished()) + "\r\n";
        s += doAttendees(node, sid);
    } else if (node.type() == Node::Type_Milestone) {
        s += QString("PERCENT-COMPLETE:") + QString::number(static_cast<const Task&>(node).completion().percentFinished()) + "\r\n";
    } else if (node.type() == Node::Type_Project) {
        if (!node.leader().isEmpty()) {
            s += QString("ORGANIZER:") + node.leader() + "\r\n";
        }
    }
    s += doAttachment(node.documents());
    s += QString("END:VTODO") + "\r\n";
    return s;
}

QString ICalendarExport::doNode(const Node *node, long sid)
{
    QString s;
    bool create = true;
    if (node->type() == Node::Type_Project) {
        create = m_includeProject;
    } else if (node->type() == Node::Type_Summarytask) {
        create = m_includeSummarytasks;
    }
    debugPlanICalExport<<node<<"create:"<<create;
    if (create) {
         s = createTodo(*node, sid);
    }
    for (int i = 0; i < node->numChildren(); ++i) {
        s += doNode(node->childNode(i), sid);
    }
    return s;
}

void foldData(QString &data)
{
    int count = 0; // bytecount
    for (int i = 0; i < data.length() - 6; ++i) {
        if (data.at(i) == '\r' && data.at(i+1) == '\n') {
            count = 0;
            ++i; // skip past LF
            continue;
        }
        if (count >= 70) {
            data.insert(i, "\r\n\t");
            count = 0;
            i += 2; // skip past CRLFTAB
            continue;
        }
        // we count bytes, so need to know bytesize of character
        count += QByteArray::fromStdString(QString(data.at(i)).toStdString()).size();
    }
    // remove any empty lines (not allowed)
    while (data.contains("\r\n\r\n")) {
        data.replace("\r\n\r\n", "\r\n");
    }
}

KoFilter::ConversionStatus ICalendarExport::convert(const Project &project, QFile &file)
{
    long sid = m_scheduleId;
    QString data = beginCalendar();
    data += doNode(&project, sid);
    data += endCalendar();

    foldData(data);
    for (int i = 0; i < m_descriptions.count(); ++i) {
        QString rs = QString("DESCRIPTION") + QString::number(i) + ':';
        QString s = QString("DESCRIPTION:") + m_descriptions.at(i);
        foldData(s);
        data.replace(rs, s);
    }
    qint64 n = file.write(data.toUtf8());
    if (n < 0) {
        return KoFilter::InternalError;
    }
    return KoFilter::OK;
}

#else
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
#if KCALCORE_VERSION >= QT_VERSION_CHECK(5, 11, 80)
        KCalendarCore::Person p = KCalendarCore::Person::fromFullName(node->leader());
        KCalendarCore::Attendee a(p.name(), p.email());
        a.setRole(KCalendarCore::Attendee::NonParticipant);
#else
        KCalendarCore::Person::Ptr p = KCalendarCore::Person::fromFullName(node->leader());
        KCalendarCore::Attendee::Ptr a(new KCalendarCore::Attendee(p->name(), p->email()));
        a->setRole(KCalendarCore::Attendee::NonParticipant);
#endif
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
#if KCALCORE_VERSION >= QT_VERSION_CHECK(5, 11, 80)
                    todo->addAttendee(KCalendarCore::Attendee(r->name(), r->email()));
#else
                    todo->addAttendee(KCalendarCore::Attendee::Ptr(new KCalendarCore::Attendee(r->name(), r->email())));
#endif
                }
            }
        } else {
            const auto resources = s->resources();
            for (const Resource *r : resources) {
                if (r->type() == Resource::Type_Work) {
#if KCALCORE_VERSION >= QT_VERSION_CHECK(5, 11, 80)
                    todo->addAttendee(KCalendarCore::Attendee(r->name(), r->email()));
#else
                    todo->addAttendee(KCalendarCore::Attendee::Ptr(new KCalendarCore::Attendee(r->name(), r->email())));
#endif
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
#if KCALCORE_VERSION >= QT_VERSION_CHECK(5, 11, 80)
        todo->addAttachment(KCalendarCore::Attachment(doc->url().url()));
#else
        todo->addAttachment(KCalendarCore::Attachment::Ptr(new KCalendarCore::Attachment(doc->url().url())));
#endif
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
#endif
#include "icalendarexport.moc"
