/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2006-2007 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef XMLLOADEROBJECT_H
#define XMLLOADEROBJECT_H

#include "plankernel_export.h"

#include "kptproject.h"
#include "kptdatetime.h"
#include "ProjectLoaderBase.h"
#include "ProjectLoader_v0.h"

#include <KoUpdater.h>
#include <KoXmlReader.h>

#include <QDateTime>
#include <QString>
#include <QStringList>
#include <QPointer>
#include <QElapsedTimer>

namespace KPlato 
{

class PLANKERNEL_EXPORT XMLLoaderObject {
public:
    enum Severity { None=0, Errors=1, Warnings=2, Diagnostics=3, Debug=4 };
    XMLLoaderObject()
    : m_loader(nullptr),
      m_project(nullptr),
      m_errors(0),
      m_warnings(0),
      m_logLevel(Diagnostics),
      m_log(),
      m_baseCalendar(nullptr),
      m_loadTaskChildren(true)
    {}
    ~XMLLoaderObject() { delete m_loader; }

    void setProject(Project *proj) { m_project = proj; }
    Project &project() const { return *m_project; }
    
    QString version() const { return m_version; }
    void setVersion(const QString &ver) {
        m_version = ver;
        delete m_loader;
        m_loader = nullptr;
        if (m_version.split(QLatin1Char('.')).value(0).toInt() == 0) {
            m_loader = new ProjectLoader_v0();
        } else {
            warnPlanXml<<"Unknown version:"<<ver<<"Failed to create a loader";
        }
    }

    QString workVersion() const { return m_workversion; }
    void setWorkVersion(const QString &ver) { m_workversion = ver; }

    QString mimetype() const { return m_mimetype; }
    void setMimetype(const QString &mime) { m_mimetype = mime; }

    const QTimeZone &projectTimeZone() const { return m_projectTimeZone; }
    void setProjectTimeZone(const QTimeZone &timeZone) { m_projectTimeZone = timeZone; }

    ProjectLoaderBase *loader() { return m_loader; }

    void startLoad() {
        m_timer.start();
        m_starttime = QDateTime::currentDateTime();
        m_errors = m_warnings = 0;
        m_log.clear();
        addMsg(QStringLiteral("Loading started at %1").arg(m_starttime.toString()));
    }
    void stopLoad() { 
        m_elapsed = m_timer.elapsed();
        addMsg(QStringLiteral("Loading finished at %1, took %2").arg(QDateTime::currentDateTime().toString(), formatElapsed()));
    }
    QDateTime lastLoaded() const { return m_starttime; }
    int elapsed() const { return m_elapsed; }
    QString formatElapsed() { return QStringLiteral("%1 seconds").arg((double)m_elapsed/1000); }
    
    void setLogLevel(Severity sev) { m_logLevel = sev; }
    const QStringList &log() const { return m_log; }
    void error(const QString &msg) { addMsg(Errors, msg); }
    void warning(const QString &msg) { addMsg(Errors, msg); }
    void diagnostic(const QString &msg) { addMsg(Diagnostics, msg); }
    void debug(const QString &msg) { addMsg(Debug, msg); }
    void message(const QString &msg) { addMsg(None, msg); }
    void addMsg(int sev, const QString& msg) {
        increment(sev);
        if (m_logLevel < sev) return;
        QString s;
        if (sev == Errors) s = QStringLiteral("ERROR");
        else if (sev == Warnings) s = QStringLiteral("WARNING");
        else if (sev == Diagnostics) s = QStringLiteral("Diagnostic");
        else if (sev == Debug) s = QStringLiteral("Debug");
        else s = QStringLiteral("Message");
        m_log<<QStringLiteral("%1: %2").arg(s, 13).arg(msg);
    }
    void addMsg(const QString &msg) { m_log<<msg; }
    void increment(int sev) {
        if (sev == Errors) { incErrors(); return; }
        if (sev == Warnings) { incWarnings(); return; }
    }
    void incErrors() { ++m_errors; }
    int errors() const { return m_errors; }
    bool error() const { return m_errors > 0; }
    void incWarnings() { ++m_warnings; }
    int warnings() const { return m_warnings; }
    bool warning() const { return m_warnings > 0; }

    // help to handle version < 0.6
    void setBaseCalendar(Calendar *cal) { m_baseCalendar = cal; }
    Calendar *baseCalendar() const { return m_baseCalendar; }

    void setUpdater(KoUpdater *updater) { m_updater = updater; }
    void setProgress(int value) { if (m_updater) m_updater->setProgress(value); }

    void setLoadTaskChildren(bool state) { m_loadTaskChildren = state; }
    bool loadTaskChildren() { return m_loadTaskChildren; }

    /// Load a project from xml
    bool loadProject(Project *project, const KoXmlDocument &document)
    {
        debugPlanXml<<project;
        m_project = project;
        bool result = false;

        if (m_updater) m_updater->setProgress(5);
        startLoad();

        KoXmlElement plan = document.documentElement();
        setVersion(plan.attribute(QStringLiteral("version"), PLAN_FILE_SYNTAX_VERSION));
#if 1
        if (m_loader) {
            result = m_loader->load(*this, document);
        } else {
            errorPlanXml<<"There is no loader for version:"<<m_version;
            result = false;
        }
        if (!result) {
            addMsg(Errors, QStringLiteral("Loading of project failed"));
        }
#else
        KoXmlNode n = plan.firstChild();
        for (; ! n.isNull(); n = n.nextSibling()) {
            if (! n.isElement()) {
                continue;
            }
            KoXmlElement e = n.toElement();
            if (e.tagName() == "project") {
                if (m_project->load(e, *this)) {
                    if (m_project->id().isEmpty()) {
                        m_project->setId(m_project->uniqueNodeId());
                        m_project->registerNodeId(m_project);
                    }
                    // Cleanup after possible bug:
                    // There should *not* be any deleted schedules (or with parent == 0)
                    const QList<Node*> nodes = m_project->nodeDict().values();
                    for (Node *n : nodes) {
                        const QList<Schedule*> schedules = n->schedules().values();
                        for (Schedule *s : schedules) {
                            if (s->isDeleted()) { // true also if parent == 0
                                errorPlanXml<<n->name()<<s;
                                n->takeSchedule(s);
                                delete s;
                            }
                        }
                    }
                    result = true;
                } else {
                    addMsg(Errors, "Loading of project failed");
                }
            }
        }
#endif
        stopLoad();
        if (m_updater) m_updater->setProgress(100); // the rest is only processing, not loading

        debugPlanXml<<project<<result;
        return result;
    }

    bool loadWorkIntervalsCache(Project *project, const KoXmlElement &plan) {
        if (!project) {
            return false;
        }
        m_project = project;
        KoXmlElement re;
        forEachElement(re, plan) {
            if (re.localName() == QStringLiteral("resource")) {
                Resource *r = project->resource(re.attribute(QStringLiteral("id")));
                if (r) {
                    r->loadCalendarIntervalsCache(re, *this);
                } else {
                    warnPlanXml<<"Resource not found:"<<"id:"<<re.attribute(QStringLiteral("id"))<<"name:"<<re.attribute(QStringLiteral("name"));
                }
            }
        }
        return true;
    }

protected:
    ProjectLoaderBase *m_loader;
    Project *m_project;
    int m_errors;
    int m_warnings;
    int m_logLevel;
    QStringList m_log;
    QDateTime m_starttime;
    QElapsedTimer m_timer;
    int m_elapsed;
    QString m_version;
    QString m_workversion;
    QString m_mimetype;
    QTimeZone m_projectTimeZone;

    Calendar *m_baseCalendar; // help to handle version < 0.6

    QPointer<KoUpdater> m_updater;

    bool m_loadTaskChildren;
};

} //namespace KPlato

#endif
