/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2006-2007 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef XMLLOADEROBJECT_H
#define XMLLOADEROBJECT_H

#include "plankernel_export.h"

#include <QElapsedTimer>
#include <QDateTime>
#include <QTimeZone>
#include <QPointer>

#include <KoUpdater.h>

class QString;

class KoXmlDocument;
class KoXmlElement;

namespace KPlato 
{
class Project;
class ProjectLoaderBase;
class Calendar;

class PLANKERNEL_EXPORT XMLLoaderObject
{
public:
    enum Severity { None=0, Errors=1, Warnings=2, Diagnostics=3, Debug=4 };
    XMLLoaderObject();
    ~XMLLoaderObject();

    void setProject(Project *proj);
    Project &project() const;
    
    QString version() const;
    void setVersion(const QString &ver);

    QString mimetype() const;
    void setMimetype(const QString &mime);

    const QTimeZone &projectTimeZone() const;
    void setProjectTimeZone(const QTimeZone &timeZone);

    ProjectLoaderBase *loader();

    void startLoad();
    void stopLoad();
    QDateTime lastLoaded() const;
    int elapsed() const;
    QString formatElapsed();
    
    void setLogLevel(Severity sev);
    const QStringList &log() const;
    void error(const QString &msg);
    void warning(const QString &msg);
    void diagnostic(const QString &msg);
    void debug(const QString &msg);
    void message(const QString &msg);
    void addMsg(int sev, const QString& msg);
    void addMsg(const QString &msg);
    void increment(int sev);
    void incErrors();
    int errors() const;
    bool error() const;
    void incWarnings();
    int warnings() const;
    bool warning() const;

    // help to handle version < 0.6
    void setBaseCalendar(Calendar *cal);
    Calendar *baseCalendar() const;

    void setUpdater(KoUpdater *updater);
    void setProgress(int value);

    void setLoadTaskChildren(bool state);
    bool loadTaskChildren();

    /// Load a project from xml
    bool loadProject(Project *project, const KoXmlDocument &document);
    bool loadWorkIntervalsCache(Project *project, const KoXmlElement &plan);

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
