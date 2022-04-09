/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2009, 2010 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTSCHEDULERPLUGIN_H
#define KPTSCHEDULERPLUGIN_H

#include "plankernel_export.h"

#include "kptschedule.h"
#include "SchedulingContext.h"

#include <KoXmlReader.h>

#include <QObject>
#include <QString>
#include <QMutex>
#include <QThread>
#include <QTimer>
#include <QEventLoopLocker>
#include <QMultiMap>

namespace KPlato
{

class SchedulerThread;
class Project;
class ScheduleManager;
class Node;
class XMLLoaderObject;

/**
 SchedulerPlugin is the base class for project calculation plugins.

 Sub-class SchedulerThread to do the actual calculation, then re-implement calculate()
 to calculate the project, and slotFinished() to fetch the result into your project.
 
 There is two ways to show progress:
 <ul>
 <li> Connect the SchedulerThread::maxProgressChanged() to ScheduleManager::setMaxProgress() and
      and SchedulerThread::progressChanged() to ScheduleManager::setProgress().
      Note that too many progress signals too often may choke the ui thread.
 <li> Start the m_synctimer. This will fetch progress and log messages every 500 ms (by default).
 </ul>

 When the thread has finished scheduling, data can be fetched from its temporary project
 into the real project by calling the updateProject() method.
*/
class PLANKERNEL_EXPORT SchedulerPlugin : public QObject
{
    Q_OBJECT
public:
    explicit SchedulerPlugin(QObject *parent);
    ~SchedulerPlugin() override;

    /// Localized name
    QString name() const;
    /// Name is normally set by the plugin loader, from Name in the desktop file
    void setName(const QString &name);
    /// Localized comment
    QString comment() const;
    /// Comment is normally set by the plugin loader, from Comment in the desktop file
    void setComment(const QString &name);
    /// A more elaborate description suitable for use in what's this
    virtual QString description() const { return QString(); }
    /// The schedulers capabilities
    enum Capabilities {
        AvoidOverbooking = 1,
        AllowOverbooking = 2,
        ScheduleForward = 4,
        ScheduleBackward = 8,
        ScheduleInParallel = 16
    };
    /// Return the schedulers capabilities.
    /// By default returns all capabilities
    virtual int capabilities() const;
    /// Stop calculation of the schedule @p sm. Current result may be used.
    void stopCalculation(ScheduleManager *sm);
    /// Terminate calculation of the schedule @p sm. No results will be available.
    void haltCalculation(ScheduleManager *sm);
    
    /// Stop calculation of the scheduling @p job. Current result may be used.
    virtual void stopCalculation(SchedulerThread *job);
    /// Terminate calculation of the scheduling @p job. No results will be available.
    virtual void haltCalculation(SchedulerThread *job);
    
    /// Calculate the project
    virtual void calculate(Project &project, ScheduleManager *sm, bool nothread = false) { Q_UNUSED(project) Q_UNUSED(sm) Q_UNUSED(nothread)  };

    /// Return the list of supported granularities
    /// An empty list means granularityIndex is not supported (the default)
    QList<long unsigned int> granularities() const;
    /// Return current index of supported granularities
    int granularityIndex() const;
    /// Set current index of supported granularities
    void setGranularityIndex(int index);

    /// return the current granularity in millesonds
    ulong granularity() const;

    /// Set parallel scheduling option to @p value.
    void setScheduleInParallel(bool value);
    /// Return true if multiple projects shall be scheduled in parallel.
    /// Returns false if the plugin does not support this option.
    bool scheduleInParallel() const;

    /// Schedule all projects
    virtual void schedule(SchedulingContext &context);

protected Q_SLOTS:
    virtual void slotSyncData();

protected:
    void updateProject(const Project *tp, const ScheduleManager *tm, Project *mp, ScheduleManager *sm) const;

    void updateProgress();
    void updateLog();
    void updateLog(SchedulerThread *job);

private:
    class Private;
    Private *d;

protected:
    QTimer m_synctimer;
    QList<SchedulerThread*> m_jobs;

    int m_granularityIndex;
    QList<long unsigned int> m_granularities;
};

/**
 SchedulerThread is a basic class used to implement project calculation in a separate thread.
 The scheduling thread is meant to run on a private copy of the project to avoid that the ui thread
 changes the data while calculations are going on.
 
 The constructor creates a KoXmlDocument m_pdoc of the project that can be used to
 create a private project. This should be done in the reimplemented run() method.
 
 When the calculations are done the signal jobFinished() is emitted. This can be used to
 fetch data from the private calculated project into the actual project.
 
 To track progress, the progress() method should be called from the ui thread with
 an appropriate interval to avoid overload of the ui thread.
 The progressChanged() signal may also be used but note that async signal handling are very slow
 so it may affect the ui threads performance too much.
*/
class PLANKERNEL_EXPORT SchedulerThread : public QThread
{
    Q_OBJECT
public:
    /// Create a scheduler with scheduling @p granularity in milliseconds
    SchedulerThread(Project *project, ScheduleManager *manager, ulong granularity, QObject *parent = nullptr);
    /// Create a scheduler with scheduling @p granularity in milliseconds
    explicit SchedulerThread(ulong granularity = 0, QObject *parent = nullptr);
    ~SchedulerThread() override;

    Project *mainProject() const { return m_mainproject; }
    ScheduleManager *mainManager() const { return m_mainmanager; }
    
    Project *project() const;
    ScheduleManager *manager() const;

    /// Run with no thread
    void doRun();
    
    /// The scheduling is stopping
    bool isStopped() const { return m_stopScheduling; }
    /// The scheduling is halting
    bool isHalted() const { return m_haltScheduling; }

    int maxProgress() const;
    int progress() const;
    QVector<Schedule::Log> takeLog();

    QMap<int, QString> phaseNames() const;

    /// Save the @p project into @p document
    static void saveProject(Project *project, QDomDocument &document);
    /// Load the @p project from @p document
    static bool loadProject(Project *project, const KoXmlDocument &document);

    ///Add a scheduling error log message
    void logError(Node *n, Resource *r, const QString &msg, int phase = -1);
    ///Add a scheduling warning log message
    void logWarning(Node *n, Resource *r, const QString &msg, int phase = -1);
    ///Add a scheduling information log message
    void logInfo(Node *n, Resource *r, const QString &msg, int phase = -1);
    ///Add a scheduling debug log message
    void logDebug(Node *n, Resource *r, const QString &msg, int phase = -1);

    static void updateProject(const Project *tp, const ScheduleManager *tm, Project *mp, ScheduleManager *sm);
    static void updateNode(const Node *tn, Node *mn, long sid, XMLLoaderObject &status);
    static void updateResource(const KPlato::Resource *tr, Resource *r, XMLLoaderObject &status);
    static void updateAppointments(const Project *tp, const ScheduleManager *tm, Project *mp, ScheduleManager *sm, XMLLoaderObject &status);

    /// Schedule all projects
    virtual void schedule(SchedulingContext &context);

Q_SIGNALS:
    /// Job has started
    void jobStarted(KPlato::SchedulerThread *job);
    /// Job is finished
    void jobFinished(KPlato::SchedulerThread *job);

    /// Maximum progress value has changed
    void maxProgressChanged(int value, KPlato::ScheduleManager *sm = nullptr);
    /// Progress has changed
    void progressChanged(int value, KPlato::ScheduleManager *sm = nullptr);

public Q_SLOTS:
    /// Stop scheduling. Result may still be used.
    virtual void stopScheduling();
    /// Halt scheduling. Discard result.
    virtual void haltScheduling();


protected Q_SLOTS:
    virtual void slotStarted();
    virtual void slotFinished();

    void setMaxProgress(int);
    void setProgress(int);

    void slotAddLog(const KPlato::Schedule::Log &log);

protected:
    /// Re-implement to do the job
    void run() override {}

    /// Get a schedulemanager that can be scheduled.
    /// If an existing manager cannot be used, a new one is created.
    ScheduleManager *getScheduleManager(Project *project);

protected:
    /// The actual project to be calculated. Not accessed outside constructor.
    Project *m_mainproject;
    /// The actual schedule manager to be calculated. Not accessed outside constructor.
    ScheduleManager *m_mainmanager;
    /// The schedule manager identity
    QString m_mainmanagerId;

    ulong m_granularity; /// The granularity if supported

    /// The temporary project
    Project *m_project;
    mutable QMutex m_projectMutex;
    /// The temporary schedule manager
    ScheduleManager *m_manager;
    mutable QMutex m_managerMutex;

    bool m_stopScheduling; /// Stop asap, preliminary result may be used
    bool m_haltScheduling; /// Stop and discrad result. Delete yourself.
    
    KoXmlDocument m_pdoc;

    int m_maxprogress;
    mutable QMutex m_maxprogressMutex;
    int m_progress;
    mutable QMutex m_progressMutex;
    QVector<Schedule::Log> m_logs;
    mutable QMutex m_logMutex;
    QEventLoopLocker m_eventLoopLocker; /// to keep locale around, TODO: check if still needed with QLocale
};

} //namespace KPlato

#endif
