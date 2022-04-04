/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTCOMPLETION_H
#define KPTCOMPLETION_H

#include "plankernel_export.h"

#include "kptglobal.h"
#include "kptdatetime.h"
#include "kptduration.h"
#include "WorkPackage.h"

#include <QList>
#include <QMap>
#include <utility>

/// The main namespace.
namespace KPlato
{

class Node;
class Resource;
class XmlSaveContext;
class Appointment;

/**
 * The Completion class holds information about the tasks progress.
 */
class PLANKERNEL_EXPORT Completion
{
    
public:
    class PLANKERNEL_EXPORT UsedEffort
    {
        public:
            class PLANKERNEL_EXPORT ActualEffort
            {
                public:
                    explicit ActualEffort(KPlato::Duration ne = Duration::zeroDuration, KPlato::Duration oe = Duration::zeroDuration)
                        : m_normalEffort(ne)
                        , m_overtimeEffort(oe)
                    {}
                    ActualEffort(const ActualEffort &e)
                        : m_normalEffort(e.m_normalEffort)
                        , m_overtimeEffort(e.m_overtimeEffort)
                    {}
                    ~ActualEffort() {}
                    bool isNull() const { return (m_normalEffort + m_overtimeEffort) == Duration::zeroDuration; }
                    Duration normalEffort() const { return m_normalEffort; }
                    void setNormalEffort(KPlato::Duration e) { m_normalEffort = e; }
                    Duration overtimeEffort() const { return m_overtimeEffort; }
                    void setOvertimeEffort(KPlato::Duration e) { m_overtimeEffort = e; }
                    /// Returns the sum of normalEffort + overtimeEffort
                    Duration effort() const { return m_normalEffort + m_overtimeEffort; }
                    void setEffort(KPlato::Duration ne, KPlato::Duration oe = Duration::zeroDuration) { m_normalEffort = ne; m_overtimeEffort = oe; }
                    ActualEffort &operator=(const ActualEffort &other) {
                        m_normalEffort = other.m_normalEffort;
                        m_overtimeEffort = other.m_overtimeEffort;
                        return *this;
                    }
                    bool operator==(const ActualEffort &a) const {
                        return m_normalEffort == a.m_normalEffort && m_overtimeEffort == a.m_overtimeEffort;
                    }
                private:
                    Duration m_normalEffort;
                    Duration m_overtimeEffort;
            };
            UsedEffort();
            UsedEffort(const UsedEffort &e);
            ~UsedEffort();
            bool operator==(const UsedEffort &e) const;
            bool operator!=(const UsedEffort &e) const { return !operator==(e); }
            void mergeEffort(const UsedEffort &value);
            void setEffort(QDate date, const ActualEffort &value);
            /// Returns the total effort up to @p date
            Duration effortTo(QDate date) const;
            /// Returns the total effort on @p date
            ActualEffort effort(QDate date) const { return m_actual.value(date); }
            ActualEffort takeEffort(QDate date) { return m_actual.take(date); }
            /// Returns the total effort for all registered dates
            Duration effort() const;
            QDate firstDate() const { return m_actual.firstKey(); }
            QDate lastDate() const { return m_actual.lastKey(); }
            QMap<QDate, ActualEffort> actualEffortMap() const { return m_actual; }
            
            /// Load from document
            bool loadXML(KoXmlElement &element, XMLLoaderObject &status);
            /// Save to document
            void saveXML(QDomElement &element) const;
            bool contains(QDate date) const { return m_actual.contains(date); }

        private:
            QMap<QDate, ActualEffort> m_actual;
    };
    typedef QMap<QDate, UsedEffort::ActualEffort> DateUsedEffortMap;
    
    class PLANKERNEL_EXPORT Entry
    {
        public:
            Entry()
            : percentFinished(0),
              remainingEffort(Duration::zeroDuration),
              totalPerformed(Duration::zeroDuration)
            {}
            Entry(int percent,  Duration remaining, Duration performed)
            : percentFinished(percent),
              remainingEffort(remaining),
              totalPerformed(performed)
            {}
            Entry(const Entry &e) { copy(e); }
            bool operator==(const Entry &e) const {
                return percentFinished == e.percentFinished
                    && remainingEffort == e.remainingEffort
                    && totalPerformed == e.totalPerformed
                    && note == e.note;
            }
            bool operator!=(const Entry &e) const { return ! operator==(e); }
            Entry &operator=(const Entry &e) { copy(e); return *this; }
            
            int percentFinished;
            Duration remainingEffort;
            Duration totalPerformed;
            QString note;
        protected:
            void copy(const Entry &e) {
                percentFinished = e.percentFinished;
                remainingEffort = e.remainingEffort;
                totalPerformed = e.totalPerformed;
                note = e.note;
            }
    };
    typedef QMap<QDate, Entry*> EntryList;

    typedef QHash<const Resource*, UsedEffort*> ResourceUsedEffortMap;
    
    explicit Completion(Node *node = nullptr);  // review * or &, or at all?
    Completion(const Completion &copy);
    virtual ~Completion();
    
    bool operator==(const Completion &p);
    bool operator!=(Completion &p) { return !(*this == p); }
    Completion &operator=(const Completion &p);
    
    /// Load from document
    bool loadXML(KoXmlElement &element, XMLLoaderObject &status);
    /// Save to document
    void saveXML(QDomElement &element) const;
    
    bool startIsValid() const { return m_started && m_startTime.isValid(); }
    bool isStarted() const { return m_started; }
    void setStarted(bool on);
    bool finishIsValid() const { return m_finished && m_finishTime.isValid(); }
    bool isFinished() const { return m_finished; }
    void setFinished(bool on);
    DateTime startTime() const { return m_startTime; }
    void setStartTime(const DateTime &dt);
    DateTime finishTime() const { return m_finishTime; }
    void setFinishTime(const DateTime &dt);
    void setPercentFinished(QDate date, int value);
    void setRemainingEffort(QDate date, Duration value);
    void setActualEffort(QDate date, Duration value);
    
    /// Return a list of the resource that has done any work on this task
    QList<const Resource*> resources() const { return m_usedEffort.keys(); }
    
    const EntryList &entries() const { return m_entries; }
    void addEntry(QDate date, Entry *entry);
    Entry *takeEntry(QDate date) { return m_entries.take(date); changed(); }
    Entry *entry(QDate date) const { return m_entries[ date ]; }
    
    /// Returns the date of the latest entry
    QDate entryDate() const;
    /// Returns the percentFinished of the latest entry
    int percentFinished() const;
    /// Returns the percentFinished on @p date
    int percentFinished(QDate date) const;
    /// Returns the estimated remaining effort
    Duration remainingEffort() const;
    /// Returns the estimated remaining effort on @p date
    Duration remainingEffort(QDate date) const;
    /// Returns the total actual effort
    Duration actualEffort() const;
    /// Returns the total actual effort on @p date
    Duration actualEffort(QDate date) const;
    /// Returns the total actual effort upto and including @p date
    Duration actualEffortTo(QDate date) const;
    /// Returns the actual effort for @p resource on @p date
    Duration actualEffort(const Resource *resource, QDate date) const;
    /// TODO
    QString note() const;
    /// TODO
    void setNote(const QString &str);
    
    /// Returns the total actual cost
    double actualCost() const;
    /// Returns the actual cost for @p resource
    double actualCost(const Resource *resource) const;
    /// Returns the actual cost on @p date
    double actualCost(QDate date) const;
    /// Returns the total actual cost for @p resource on @p date
    double actualCost(const Resource *resource, QDate date) const;
    /// Returns the total actual effort and cost upto and including @p date
    EffortCost actualCostTo(long int id, QDate date) const;
    
    /**
     * Returns a map of all actual effort and cost entered
     */
    virtual EffortCostMap actualEffortCost(long id, EffortCostCalculationType type = ECCT_All) const;

    void addUsedEffort(const Resource *resource, UsedEffort *value = nullptr);
    UsedEffort *takeUsedEffort(const Resource *r) { return m_usedEffort.take(const_cast<Resource*>(r) ); changed(); }
    UsedEffort *usedEffort(const Resource *r) const { return m_usedEffort.value(const_cast<Resource*>(r) ); }
    const ResourceUsedEffortMap &usedEffortMap() const { return m_usedEffort; }

    void setActualEffort(Resource *resource, const QDate &date, const UsedEffort::ActualEffort &value);
    // FIXME name clash
    Completion::UsedEffort::ActualEffort getActualEffort(const Resource *resource, const QDate &date) const;

    void changed(int property = -1);
    Node *node() const { return m_node; }
    void setNode(Node *node) { m_node = node; }
    
    enum Entrymode { FollowPlan, EnterCompleted, EnterEffortPerTask, EnterEffortPerResource };
    void setEntrymode(Entrymode mode) { m_entrymode = mode; }
    Entrymode entrymode() const { return m_entrymode; }
    void setEntrymode(const QString &mode);
    QString entryModeToString() const;
    QStringList entrymodeList() const;
    
    EffortCostMap effortCostPrDay(QDate start, QDate end, long id = -1) const;
    /// Returns the actual effort and cost pr day used by @p resource
    EffortCostMap effortCostPrDay(const Resource *resource, QDate start, QDate end, long id = CURRENTSCHEDULE) const;

    // Convert completion data to appointments
    QHash<Resource*, Appointment> createAppointments() const;

protected:
    void copy(const Completion &copy);
    double averageCostPrHour(QDate date, long id) const;
    std::pair<QDate, QDate> actualStartEndDates() const;

    QHash<Resource*, Appointment> createAppointmentsPerTask() const;
    QHash<Resource*, Appointment> createAppointmentsPerResource() const;

private:
    Node *m_node;
    bool m_started, m_finished;
    DateTime m_startTime, m_finishTime;
    EntryList m_entries;
    ResourceUsedEffortMap m_usedEffort;
    Entrymode m_entrymode;
    
#ifndef NDEBUG
public:
    void printDebug(const QByteArray &ident) const;
#endif
};

}  //KPlato namespace

Q_DECLARE_METATYPE(KPlato::Completion::UsedEffort::ActualEffort)

#ifndef QT_NO_DEBUG_STREAM
PLANKERNEL_EXPORT QDebug operator<<(QDebug dbg, const KPlato::Completion::UsedEffort::ActualEffort &ae);
PLANKERNEL_EXPORT QDebug operator<<(QDebug dbg, const KPlato::Completion::Entry *e);
PLANKERNEL_EXPORT QDebug operator<<(QDebug dbg, const KPlato::Completion::Entry &e);
#endif

#endif
