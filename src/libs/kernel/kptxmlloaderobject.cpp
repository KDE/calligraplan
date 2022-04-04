/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2006-2007 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kptxmlloaderobject.h"

#include "kptproject.h"
#include "kptdatetime.h"
#include "ProjectLoaderBase.h"
#include "ProjectLoader_v0.h"

#include <MimeTypes.h>
#include <KoXmlReader.h>

#include <QString>
#include <QStringList>

using namespace KPlato ;

XMLLoaderObject::XMLLoaderObject()
    : m_loader(nullptr)
    , m_project(nullptr)
    , m_errors(0)
    , m_warnings(0)
    , m_logLevel(Diagnostics)
    , m_log()
    , m_baseCalendar(nullptr)
    , m_loadTaskChildren(true)
{}

XMLLoaderObject::~XMLLoaderObject()
{
    delete m_loader;
}

void XMLLoaderObject::setProject(Project *proj)
{
    m_project = proj;
}

Project &XMLLoaderObject::project() const
{
    return *m_project;
}

QString XMLLoaderObject::version() const
{
    return m_version;
}

void XMLLoaderObject::setVersion(const QString &ver)
{
    m_version = ver;
    delete m_loader;
    m_loader = nullptr;
    if (m_mimetype == PLAN_MIME_TYPE || m_mimetype == PLANWORK_MIME_TYPE) {
        if (m_version.split(QLatin1Char('.')).value(0).toInt() == 0) {
            m_loader = new ProjectLoader_v0();
        } else {
            warnPlanXml<<"Unknown version:"<<ver<<"Failed to create a loader";
        }
    } else {
        debugPlanXml<<"Unknown mimetype:"<<m_mimetype<<"Failed to create a loader";
    }
}

QString XMLLoaderObject::mimetype() const
{
    return m_mimetype;
}

void XMLLoaderObject::setMimetype(const QString &mime)
{
    m_mimetype = mime;
}

const QTimeZone &XMLLoaderObject::projectTimeZone() const
{
    return m_projectTimeZone;
}

void XMLLoaderObject::setProjectTimeZone(const QTimeZone &timeZone)
{
    m_projectTimeZone = timeZone;
}

ProjectLoaderBase *XMLLoaderObject::loader()
{
    return m_loader;
}

void XMLLoaderObject::startLoad()
{
    m_timer.start();
    m_starttime = QDateTime::currentDateTime();
    m_errors = m_warnings = 0;
    m_log.clear();
    addMsg(QStringLiteral("Loading started at %1").arg(m_starttime.toString()));
}

void XMLLoaderObject::stopLoad()
{
    m_elapsed = m_timer.elapsed();
    addMsg(QStringLiteral("Loading finished at %1, took %2").arg(QDateTime::currentDateTime().toString(), formatElapsed()));
}

QDateTime XMLLoaderObject::lastLoaded() const
{
    return m_starttime;
}

int XMLLoaderObject::elapsed() const
{
    return m_elapsed;
}

QString XMLLoaderObject::formatElapsed()
{
    return QStringLiteral("%1 seconds").arg((double)m_elapsed/1000);
}

void XMLLoaderObject::setLogLevel(Severity sev)
{
    m_logLevel = sev;
}

const QStringList &XMLLoaderObject::log() const
{
    return m_log;
}

void XMLLoaderObject::error(const QString &msg)
{
    addMsg(Errors, msg);
}

void XMLLoaderObject::warning(const QString &msg)
{
    addMsg(Errors, msg);
}

void XMLLoaderObject::diagnostic(const QString &msg)
{
    addMsg(Diagnostics, msg);
}

void XMLLoaderObject::debug(const QString &msg)
{
    addMsg(Debug, msg);
}

void XMLLoaderObject::message(const QString &msg)
{
    addMsg(None, msg);
}

void XMLLoaderObject::addMsg(int sev, const QString& msg)
{
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

void XMLLoaderObject::addMsg(const QString &msg)
{
    m_log<<msg;
}

void XMLLoaderObject::increment(int sev)
{
    if (sev == Errors) { incErrors(); return; }
    if (sev == Warnings) { incWarnings(); return; }
}

void XMLLoaderObject::incErrors()
{
    ++m_errors;
}

int XMLLoaderObject::errors() const
{
    return m_errors;
}

bool XMLLoaderObject::error() const
{
    return m_errors > 0;
}

void XMLLoaderObject::incWarnings()
{
    ++m_warnings;
}

int XMLLoaderObject::warnings() const
{
    return m_warnings;
}

bool XMLLoaderObject::warning() const
{
    return m_warnings > 0;
}

// help to handle version < 0.6
void XMLLoaderObject::setBaseCalendar(Calendar *cal)
{
    m_baseCalendar = cal;
}

Calendar *XMLLoaderObject::baseCalendar() const
{
    return m_baseCalendar;
}

void XMLLoaderObject::setUpdater(KoUpdater *updater)
{
    m_updater = updater;
}

void XMLLoaderObject::setProgress(int value)
{
    if (m_updater) m_updater->setProgress(value);
}

void XMLLoaderObject::setLoadTaskChildren(bool state)
{
    m_loadTaskChildren = state;
}

bool XMLLoaderObject::loadTaskChildren()
{
    return m_loadTaskChildren;
}

/// Load a project from xml
bool XMLLoaderObject::loadProject(Project *project, const KoXmlDocument &document)
{
    debugPlanXml<<project;
    m_project = project;
    bool result = false;

    if (m_updater) m_updater->setProgress(5);
    startLoad();

    KoXmlElement plan = document.documentElement();
    setMimetype(plan.attribute(QStringLiteral("mime")));
    setVersion(plan.attribute(QStringLiteral("version"), PLAN_FILE_SYNTAX_VERSION));
    if (m_loader) {
        result = m_loader->load(*this, document);
    } else {
        errorPlanXml<<"There is no loader for version:"<<m_version;
        result = false;
    }
    if (!result) {
        addMsg(Errors, QStringLiteral("Loading of project failed"));
    }
    stopLoad();
    if (m_updater) m_updater->setProgress(100); // the rest is only processing, not loading

    debugPlanXml<<project<<result;
    return result;
}

bool XMLLoaderObject::loadWorkIntervalsCache(Project *project, const KoXmlElement &plan)
{
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
