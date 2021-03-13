/* This file is part of the KDE project
   Copyright (C) 2006 - 2007 Dag Andersen <dag.andersen@kdemail.net>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation;
   version 2 of the License.
   
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef XMLLOADEROBJECT_H
#define XMLLOADEROBJECT_H

#include "plankernel_export.h"

#include "kptproject.h"
#include "kptdatetime.h"

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
    : m_project(nullptr),
      m_errors(0),
      m_warnings(0),
      m_logLevel(Diagnostics),
      m_log(),
      m_baseCalendar(nullptr),
      m_loadTaskChildren(true)
    {}
    ~XMLLoaderObject() {}

    void setProject(Project *proj) { m_project = proj; }
    Project &project() const { return *m_project; }
    
    QString version() const { return m_version; }
    void setVersion(const QString &ver) { m_version = ver; }

    QString workVersion() const { return m_workversion; }
    void setWorkVersion(const QString &ver) { m_workversion = ver; }

    QString mimetype() const { return m_mimetype; }
    void setMimetype(const QString &mime) { m_mimetype = mime; }

    const QTimeZone &projectTimeZone() const { return m_projectTimeZone; }
    void setProjectTimeZone(const QTimeZone &timeZone) { m_projectTimeZone = timeZone; }

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
        if (sev == Errors) s = QLatin1String("ERROR");
        else if (sev == Warnings) s = QLatin1String("WARNING");
        else if (sev == Diagnostics) s = QLatin1String("Diagnostic");
        else if (sev == Debug) s = QLatin1String("Debug");
        else s = QLatin1String("Message");
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
        m_project = project;
        bool result = false;

        if (m_updater) m_updater->setProgress(5);
        startLoad();

        KoXmlElement plan = document.documentElement();
        m_version = plan.attribute("version", PLAN_FILE_SYNTAX_VERSION);
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
                                errorPlan<<n->name()<<s;
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
        stopLoad();
        if (m_updater) m_updater->setProgress(100); // the rest is only processing, not loading

        return result;
    }

    bool loadWorkIntervalsCache(Project *project, const KoXmlElement &plan) {
        if (!project) {
            return false;
        }
        m_project = project;
        KoXmlElement re;
        forEachElement(re, plan) {
            if (re.localName() == "resource") {
                Resource *r = project->resource(re.attribute("id"));
                if (r) {
                    r->loadCalendarIntervalsCache(re, *this);
                } else {
                    warnPlanXml<<"Resource not found:"<<"id:"<<re.attribute("id")<<"name:"<<re.attribute("name");
                }
            }
        }
        return true;
    }

protected:
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
