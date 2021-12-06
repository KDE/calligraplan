/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTWORKPACKAGE_H
#define KPTWORKPACKAGE_H

#include "plankernel_export.h"

#include "kptnode.h"
#include "kptglobal.h"
#include "kptdatetime.h"
#include "kptduration.h"
#include "Completion.h"

#include <QList>
#include <QMap>
#include <utility>

/// The main namespace.
namespace KPlato
{

class Completion;
class XmlSaveContext;
class Appointment;
class Resource;
class Task;

class PLANKERNEL_EXPORT WorkPackageSettings
{
public:
    WorkPackageSettings();
    bool loadXML(const KoXmlElement &element);
    void saveXML(QDomElement &element) const;
    bool operator==(WorkPackageSettings settings) const;
    bool operator!=(WorkPackageSettings settings) const;
    bool usedEffort;
    bool progress;
    bool documents;
    bool remainingEffort;
};

/**
 * The WorkPackage class controls work flow for a task
 */
class PLANKERNEL_EXPORT WorkPackage
{
public:

    /// @enum WPTransmitionStatus describes if this package was sent or received
    enum WPTransmitionStatus {
        TS_None,        /// Not sent nor received
        TS_Send,        /// Package was sent to resource
        TS_Receive,     /// Package was received from resource
        TS_Rejected     /// Received package was rejected by project manager
    };

    explicit WorkPackage(Task *task = nullptr);
    explicit WorkPackage(const WorkPackage &wp);
    virtual ~WorkPackage();

    Task *parentTask() const { return m_task; }
    void setParentTask(Task *task) { m_task = task; }

    /// Returns the transmission status of this package
    WPTransmitionStatus transmitionStatus() const { return m_transmitionStatus; }
    void setTransmitionStatus(WPTransmitionStatus sts) { m_transmitionStatus = sts; }
    static QString transmitionStatusToString(WPTransmitionStatus sts, bool trans = false);
    static WPTransmitionStatus transmitionStatusFromString(const QString &sts);
    
    /// Load from document
    virtual bool loadXML(KoXmlElement &element, XMLLoaderObject &status);
    /// Save the full workpackage
    virtual void saveXML(QDomElement &element) const;

    /// Load from document
    virtual bool loadLoggedXML(KoXmlElement &element, XMLLoaderObject &status);
    /// Save the full workpackage
    virtual void saveLoggedXML(QDomElement &element) const;

    /// Set schedule manager
    void setScheduleManager(ScheduleManager *sm);
    /// Return schedule manager
    ScheduleManager *scheduleManager() const { return m_manager; }
    /// Return the schedule id, or NOTSCHEDULED if no schedule manager is set
    long id() const { return m_manager ? m_manager->scheduleId() : NOTSCHEDULED; }

    Completion &completion();
    const Completion &completion() const;

    void addLogEntry(DateTime &dt, const QString &str);
    QMap<DateTime, QString> log() const;
    QStringList log();

    /// Return a list of resources fetched from the appointments or requests
    /// merged with resources added to completion
    QList<Resource*> fetchResources();

    /// Return a list of resources fetched from the appointments or requests
    /// merged with resources added to completion
    QList<Resource*> fetchResources(long id);

    /// Returns id of the resource that owns this package. If empty, task leader owns it.
    QString ownerId() const { return m_ownerId; }
    /// Set the resource that owns this package to @p owner. If empty, task leader owns it.
    void setOwnerId(const QString &id) { m_ownerId = id; }

    /// Returns the name of the resource that owns this package.
    QString ownerName() const { return m_ownerName; }
    /// Set the name of the resource that owns this package.
    void setOwnerName(const QString &name) { m_ownerName = name; }

    DateTime transmitionTime() const { return m_transmitionTime; }
    void setTransmitionTime(const DateTime &dt) { m_transmitionTime = dt; }

    /// Clear workpackage data
    void clear();

private:
    Task *m_task;
    ScheduleManager *m_manager;
    Completion m_completion;
    QString m_ownerName;
    QString m_ownerId;
    WPTransmitionStatus m_transmitionStatus;
    DateTime m_transmitionTime;

    QMap<DateTime, QString> m_log;
};

}

#endif
