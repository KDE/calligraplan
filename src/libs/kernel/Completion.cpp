/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "kpttask.h"
#include "kptappointment.h"
#include "kptproject.h"
#include "kptdatetime.h"
#include "kpteffortcostmap.h"
#include "kptschedule.h"
#include "kptxmlloaderobject.h"
#include "XmlSaveContext.h"
#include <kptdebug.h>

#include <KoXmlReader.h>

#include <KLocalizedString>

#include <QElapsedTimer>

using namespace KPlato;

Completion::Completion(Node *node)
    : m_node(node),
      m_started(false),
      m_finished(false),
      m_entrymode(EnterEffortPerResource)
{}

Completion::Completion(const Completion &c)
{
    copy(c);
}

Completion::~Completion()
{
    qDeleteAll(m_entries);
    qDeleteAll(m_usedEffort);
}

void Completion::copy(const Completion &p)
{
    m_node = nullptr; //NOTE
    m_started = p.isStarted(); m_finished = p.isFinished();
    m_startTime = p.startTime(); m_finishTime = p.finishTime();
    m_entrymode = p.entrymode();

    qDeleteAll(m_entries);
    m_entries.clear();
    Completion::EntryList::ConstIterator entriesIt = p.entries().constBegin();
    const Completion::EntryList::ConstIterator entriesEnd = p.entries().constEnd();
    for (; entriesIt != entriesEnd; ++entriesIt) {
        addEntry(entriesIt.key(), new Entry(*entriesIt.value()));
    }

    qDeleteAll(m_usedEffort);
    m_usedEffort.clear();
    Completion::ResourceUsedEffortMap::ConstIterator usedEffortMapIt = p.usedEffortMap().constBegin();
    const Completion::ResourceUsedEffortMap::ConstIterator usedEffortMapEnd = p.usedEffortMap().constEnd();
    for (; usedEffortMapIt != usedEffortMapEnd; ++usedEffortMapIt) {
        addUsedEffort(usedEffortMapIt.key(), new UsedEffort(*usedEffortMapIt.value()));
    }
}

bool Completion::operator==(const Completion &p)
{
    return m_started == p.isStarted() && m_finished == p.isFinished() &&
            m_startTime == p.startTime() && m_finishTime == p.finishTime() &&
            m_entries == p.entries() &&
            m_usedEffort == p.usedEffortMap();
}
Completion &Completion::operator=(const Completion &p)
{
    copy(p);
    return *this;
}

void Completion::changed(int property)
{
    if (m_node) {
        m_node->changed(property);
    }
}

void Completion::setStarted(bool on)
{
     m_started = on;
     changed(Node::CompletionStartedProperty);
}

void Completion::setFinished(bool on)
{
     m_finished = on;
     changed(Node::CompletionFinishedProperty);
}

void Completion::setStartTime(const DateTime &dt)
{
     m_startTime = dt;
     changed(Node::CompletionStartTimeProperty);
}

void Completion::setFinishTime(const DateTime &dt)
{
     m_finishTime = dt;
     changed(Node::CompletionFinishTimeProperty);
}

void Completion::setPercentFinished(QDate date, int value)
{
    Entry *e = nullptr;
    if (m_entries.contains(date)) {
        e = m_entries[ date ];
    } else {
        e = new Entry();
        m_entries[ date ] = e;
    }
    e->percentFinished = value;
    changed(Node::CompletionPercentageProperty);
}

void Completion::setRemainingEffort(QDate date, KPlato::Duration value)
{
    Entry *e = nullptr;
    if (m_entries.contains(date)) {
        e = m_entries[ date ];
    } else {
        e = new Entry();
        m_entries[ date ] = e;
    }
    e->remainingEffort = value;
    changed(Node::CompletionRemainingEffortProperty);
}

void Completion::setActualEffort(QDate date, KPlato::Duration value)
{
    Entry *e = nullptr;
    if (m_entries.contains(date)) {
        e = m_entries[ date ];
    } else {
        e = new Entry();
        m_entries[ date ] = e;
    }
    e->totalPerformed = value;
    changed(Node::CompletionActualEffortProperty);
}

void Completion::addEntry(QDate date, Entry *entry)
{
    Q_ASSERT(!m_entries.contains(date));
     m_entries.insert(date, entry);
     //debugPlan<<m_entries.count()<<" added:"<<date;
     changed(Node::CompletionEntryProperty);
}

Completion::Entry *Completion::takeEntry(QDate date)
{
    auto e = m_entries.take(date);
    if (e) {
        changed();
    }
    return e;
}

QDate Completion::entryDate() const
{
    return m_entries.isEmpty() ? QDate() : m_entries.lastKey();
}

int Completion::percentFinished() const
{
    return m_entries.isEmpty() ? 0 : m_entries.last()->percentFinished;
}

int Completion::percentFinished(QDate date) const
{
    int x = 0;
    EntryList::const_iterator it;
    for (it = m_entries.constBegin(); it != m_entries.constEnd() && it.key() <= date; ++it) {
        x = it.value()->percentFinished;
    }
    return x;
}

Duration Completion::remainingEffort() const
{
    return m_entries.isEmpty() ? Duration::zeroDuration : m_entries.last()->remainingEffort;
}

Duration Completion::remainingEffort(QDate date) const
{
    Duration x;
    EntryList::const_iterator it;
    for (it = m_entries.constBegin(); it != m_entries.constEnd() && it.key() <= date; ++it) {
        x = it.value()->remainingEffort;
    }
    return x;
}

Duration Completion::actualEffort() const
{
    Duration eff;
    if (m_entrymode == EnterEffortPerResource) {
        for (const UsedEffort *ue : std::as_const(m_usedEffort)) {
            const QMap<QDate, UsedEffort::ActualEffort> map = ue->actualEffortMap();
            QMap<QDate, UsedEffort::ActualEffort>::const_iterator it;
            for (it = map.constBegin(); it != map.constEnd(); ++it) {
                eff += it.value().effort();
            }
        }
    } else if (! m_entries.isEmpty()) {
        eff = m_entries.last()->totalPerformed;
    }
    return eff;
}

Duration Completion::actualEffort(const Resource *resource, QDate date) const
{
    UsedEffort *ue = usedEffort(resource);
    if (ue == nullptr) {
        return Duration::zeroDuration;
    }
    UsedEffort::ActualEffort ae = ue->effort(date);
    return ae.effort();
}

Duration Completion::actualEffort(QDate date) const
{
    Duration eff;
    if (m_entrymode == EnterEffortPerResource) {
        for (const UsedEffort *ue : std::as_const(m_usedEffort)) {
            if (ue && ue->actualEffortMap().contains(date)) {
                eff += ue->actualEffortMap().value(date).effort();
            }
        }
    } else {
        // Hmmm: How to really know a specific date?
        if (m_entries.contains(date)) {
            eff = m_entries[ date ]->totalPerformed;
        }
    }
    return eff;
}

Duration Completion::actualEffortTo(QDate date) const
{
    //debugPlan<<date;
    Duration eff;
    if (m_entrymode == EnterEffortPerResource) {
        for (const UsedEffort *ue : std::as_const(m_usedEffort)) {
            eff += ue->effortTo(date);
        }
    } else {
        QMap<QDate, Completion::Entry*>::const_iterator it = m_entries.constEnd();
        for (--it; it != m_entries.constBegin(); --it) {
            if (it.key() <= date) {
                eff = it.value()->totalPerformed;
                break;
            }
        }
    }
    return eff;
}

double Completion::averageCostPrHour(QDate date, long id) const
{
    Schedule *s = m_node->schedule(id);
    if (s == nullptr) {
        return 0.0;
    }
    double cost = 0.0;
    double eff = 0.0;
    QList<double> cl;
    const auto appointments = s->appointments();
    for (const Appointment *a : appointments) {
        cl << a->resource()->resource()->normalRate();
        double e = a->plannedEffort(date).toDouble(Duration::Unit_h);
        if (e > 0.0) {
            eff += e;
            cost += e * cl.last();
        }
    }
    if (eff > 0.0) {
        cost /= eff;
    } else {
        for (double c : std::as_const(cl)) {
            cost += c;
        }
        cost /= cl.count();
    }
    return cost;
}

EffortCostMap Completion::effortCostPrDay(QDate start, QDate end, long id) const
{
    //debugPlan<<m_node->name()<<start<<end;
    EffortCostMap ec;
    if (! isStarted()) {
        return ec;
    }
    switch (m_entrymode) {
        case FollowPlan:
            break;
        case EnterCompleted:
        case EnterEffortPerTask: {
            QDate st = start.isValid() ? start : m_startTime.date();
            QDate et = end.isValid() ? end : m_finishTime.date();
            Duration last;
            QMap<QDate, Completion::Entry*>::const_iterator it = m_entries.constBegin();
            for (; it != m_entries.constEnd(); ++it) {
                if (it.key() < st) {
                    continue;
                }
                if (et.isValid() && it.key() > et) {
                    break;
                }
                Duration e = it.value()->totalPerformed;
                if (e != Duration::zeroDuration && e != last) {
                    Duration eff = e - last;
                    ec.insert(it.key(), eff, eff.toDouble(Duration::Unit_h) * averageCostPrHour(it.key(), id));
                    last = e;
                }
            }
            break;
        }
        case EnterEffortPerResource: {
            std::pair<QDate, QDate> dates = actualStartEndDates();
            if (! dates.first.isValid()) {
                // no data, so just break
                break;
            }
            QDate st = start.isValid() ? start : dates.first;
            QDate et = end.isValid() ? end : dates.second;
            for (QDate d = st; d <= et; d = d.addDays(1)) {
                ec.add(d, actualEffort(d), actualCost(d));
            }
            break;
        }
        default:
            break;
    }
    return ec;
}

EffortCostMap Completion::effortCostPrDay(const Resource *resource, QDate start, QDate end, long id) const
{
    Q_UNUSED(id);
    //debugPlan<<m_node->name()<<start<<end;
    EffortCostMap ec;
    if (! isStarted()) {
        return ec;
    }
    switch (m_entrymode) {
        case FollowPlan:
            break;
        case EnterCompleted:
        case EnterEffortPerTask: {
            //TODO but what todo?
            break;
        }
        case EnterEffortPerResource: {
            std::pair<QDate, QDate> dates = actualStartEndDates();
            if (! dates.first.isValid()) {
                // no data, so just break
                break;
            }
            QDate st = start.isValid() ? start : dates.first;
            QDate et = end.isValid() ? end : dates.second;
            for (QDate d = st; d <= et; d = d.addDays(1)) {
                ec.add(d, actualEffort(resource, d), actualCost(resource, d));
            }
            break;
        }
    }
    return ec;
}

void Completion::addUsedEffort(const Resource *resource, Completion::UsedEffort *value)
{
    UsedEffort *v = value == nullptr ? new UsedEffort() : value;
    if (m_usedEffort.contains(resource)) {
        m_usedEffort[ resource ]->mergeEffort(*v);
        if (v != value) {
            delete v;
        }
    } else {
        m_usedEffort.insert(resource, v);
    }
    changed(Node::CompletionUsedEffortProperty);
}

Completion::UsedEffort *Completion::takeUsedEffort(const Resource *r)
{
    auto e = m_usedEffort.take(const_cast<Resource*>(r));
    if (e) {
        changed();
    }
    return e;
}

void Completion::setActualEffort(Resource *resource, const QDate &date, const Completion::UsedEffort::ActualEffort &value)
{
    if (value.isNull()) {
        if (!m_usedEffort.contains(resource)) {
            return;
        }
        UsedEffort *ue = m_usedEffort.value(resource);
        if (!ue) {
            return;
        }
        ue->takeEffort(date);
    } else {
        UsedEffort *ue = m_usedEffort[resource];
        if (!ue) {
            ue = new UsedEffort();
            m_usedEffort.insert(resource, ue);
        }
        ue->setEffort(date, value);
    }
    changed(Node::CompletionActualEffortProperty);
}

Completion::UsedEffort::ActualEffort Completion::getActualEffort(const Resource *resource, const QDate &date) const
{
    UsedEffort::ActualEffort value;
    UsedEffort *ue = m_usedEffort.value(const_cast<Resource*>(resource));
    if (ue) {
        value = ue->effort(date);
    }
    return value;
}

QString Completion::note() const
{
    return m_entries.isEmpty() ? QString() : m_entries.last()->note;
}

void Completion::setNote(const QString &str)
{
    if (! m_entries.isEmpty()) {
        m_entries.last()->note = str;
        changed(Node::CompletionNoteProperty);
    }
}

std::pair<QDate, QDate> Completion::actualStartEndDates() const
{
    std::pair<QDate, QDate> p;
    ResourceUsedEffortMap::const_iterator it;
    for (it = m_usedEffort.constBegin(); it != m_usedEffort.constEnd(); ++it) {
        if (!it.value()->actualEffortMap().isEmpty()) {
            QDate d = it.value()->firstDate();
            if (!p.first.isValid() || d < p.first) {
                p.first = d;
            }
            d = it.value()->lastDate();
            if (!p.second.isValid() || d > p.second) {
                p.second = d;
            }
        }
    }
    return p;
}

double Completion::actualCost(QDate date) const
{
    //debugPlan<<date;
    double c = 0.0;
    ResourceUsedEffortMap::const_iterator it;
    for (it = m_usedEffort.constBegin(); it != m_usedEffort.constEnd(); ++it) {
        double nc = it.key()->normalRate();
        double oc = it.key()->overtimeRate();
        if (it.value()->actualEffortMap().contains(date)) {
            UsedEffort::ActualEffort a = it.value()->effort(date);
            c += a.normalEffort().toDouble(Duration::Unit_h) * nc;
            c += a.overtimeEffort().toDouble(Duration::Unit_h) * oc;
        }
    }
    return c;
}

double Completion::actualCost(const Resource *resource) const
{
    UsedEffort *ue = usedEffort(resource);
    if (ue == nullptr) {
        return 0.0;
    }
    double c = 0.0;
    double nc = resource->normalRate();
    double oc = resource->overtimeRate();
    const auto list = ue->actualEffortMap().values();
    for (const UsedEffort::ActualEffort &a : list) {
        c += a.normalEffort().toDouble(Duration::Unit_h) * nc;
        c += a.overtimeEffort().toDouble(Duration::Unit_h) * oc;
    }
    return c;
}

double Completion::actualCost() const
{
    double c = 0.0;
    ResourceUsedEffortMap::const_iterator it;
    for (it = m_usedEffort.constBegin(); it != m_usedEffort.constEnd(); ++it) {
        c += actualCost(it.key());
    }
    return c;
}

double Completion::actualCost(const Resource *resource, QDate date) const
{
    UsedEffort *ue = usedEffort(resource);
    if (ue == nullptr) {
        return 0.0;
    }
    UsedEffort::ActualEffort a = ue->actualEffortMap().value(date);
    double c = a.normalEffort().toDouble(Duration::Unit_h) * resource->normalRate();
    c += a.overtimeEffort().toDouble(Duration::Unit_h) * resource->overtimeRate();
    return c;
}

EffortCostMap Completion::actualEffortCost(long int id, KPlato::EffortCostCalculationType type) const
{
    //debugPlan;
    EffortCostMap map;
    if (! isStarted()) {
        return map;
    }
    QList< QMap<QDate, UsedEffort::ActualEffort> > lst;
    QList< double > rate;
    QDate start, end;
    ResourceUsedEffortMap::const_iterator it;
    for (it = m_usedEffort.constBegin(); it != m_usedEffort.constEnd(); ++it) {
        const Resource *r = it.key();
        //debugPlan<<m_node->name()<<r->name();
        lst << usedEffort(r)->actualEffortMap();
        if (lst.last().isEmpty()) {
            lst.takeLast();
            continue;
        }
        if (r->type() == Resource::Type_Material) {
            if (type == ECCT_All) {
                rate.append(r->normalRate());
            } else if (type == ECCT_EffortWork) {
                rate.append(0.0);
            } else {
                lst.takeLast();
                continue;
            }
        } else {
            rate.append(r->normalRate());
        }
        if (! start.isValid() || start > lst.last().firstKey()) {
            start = lst.last().firstKey();
        }
        if (! end.isValid() || end < lst.last().lastKey()) {
            end = lst.last().lastKey();
        }
    }
    if (! lst.isEmpty() && start.isValid() && end.isValid()) {
        for (QDate d = start; d <= end; d = d.addDays(1)) {
            EffortCost c;
            for (int i = 0; i < lst.count(); ++i) {
                UsedEffort::ActualEffort a = lst.at(i).value(d);
                double nc = rate.value(i);
                Duration eff = a.normalEffort();
                double cost = eff.toDouble(Duration::Unit_h) * nc;
                c.add(eff, cost);
            }
            if (c.effort() != Duration::zeroDuration || c.cost() != 0.0) {
                map.add(d, c);
            }
        }
    } else if (! m_entries.isEmpty()) {
        QDate st = start.isValid() ? start : m_startTime.date();
        QDate et = end.isValid() ? end : m_finishTime.date();
        Duration last;
        QMap<QDate, Completion::Entry*>::const_iterator it = m_entries.constBegin();
        for (; it != m_entries.constEnd(); ++it) {
            if (it.key() < st) {
                continue;
            }
            Duration e = it.value()->totalPerformed;
            if (e != Duration::zeroDuration && e != last) {
                //debugPlan<<m_node->name()<<d<<(e - last).toDouble(Duration::Unit_h);
                double eff = (e - last).toDouble(Duration::Unit_h);
                map.insert(it.key(), e - last, eff * averageCostPrHour(it.key(), id)); // try to guess cost
                last = e;
            }
            if (et.isValid() && it.key() > et) {
                break;
            }
        }
    }
    return map;
}

EffortCost Completion::actualCostTo(long int id, QDate date) const
{
    //debugPlan<<date;
    EffortCostMap ecm = actualEffortCost(id);
    return EffortCost(ecm.effortTo(date), ecm.costTo(date));
}

QStringList Completion::entrymodeList() const
{
    return QStringList()
            << QStringLiteral("FollowPlan")
            << QStringLiteral("EnterCompleted")
            << QStringLiteral("EnterEffortPerTask")
            << QStringLiteral("EnterEffortPerResource");

}

void Completion::setEntrymode(const QString &mode)
{
    int m = entrymodeList().indexOf(mode);
    if (m == -1) {
        m = EnterCompleted;
    }
    m_entrymode = static_cast<Entrymode>(m);
}
QString Completion::entryModeToString() const
{
    return entrymodeList().value(m_entrymode);
}

bool Completion::loadXML(KoXmlElement &element, XMLLoaderObject &status)
{
    //debugPlan;
    QString s;
    m_started = (bool)element.attribute(QStringLiteral("started"), QStringLiteral("0")).toInt();
    m_finished = (bool)element.attribute(QStringLiteral("finished"), QStringLiteral("0")).toInt();
    s = element.attribute(QStringLiteral("startTime"));
    if (!s.isEmpty()) {
        m_startTime = DateTime::fromString(s, status.projectTimeZone());
    }
    s = element.attribute(QStringLiteral("finishTime"));
    if (!s.isEmpty()) {
        m_finishTime = DateTime::fromString(s, status.projectTimeZone());
    }
    setEntrymode(element.attribute(QStringLiteral("entrymode")));
    if (status.version() < QStringLiteral("0.6")) {
        if (m_started) {
            Entry *entry = new Entry(element.attribute(QStringLiteral("percent-finished"), QStringLiteral("0")).toInt(), Duration::fromString(element.attribute(QStringLiteral("remaining-effort"))),  Duration::fromString(element.attribute(QStringLiteral("performed-effort"))));
            entry->note = element.attribute(QStringLiteral("note"));
            QDate date = m_startTime.date();
            if (m_finished) {
                date = m_finishTime.date();
            }
            // almost the best we can do ;)
            addEntry(date, entry);
        }
    } else {
        KoXmlElement e;
        forEachElement(e, element) {
                if (e.tagName() == QStringLiteral("completion-entry")) {
                    QDate date;
                    s = e.attribute(QStringLiteral("date"));
                    if (!s.isEmpty()) {
                        date = QDate::fromString(s, Qt::ISODate);
                    }
                    if (!date.isValid()) {
                        warnPlan<<"Invalid date: "<<date<<s;
                        continue;
                    }
                    Entry *entry = new Entry(e.attribute(QStringLiteral("percent-finished"), QStringLiteral("0")).toInt(), Duration::fromString(e.attribute(QStringLiteral("remaining-effort"))),  Duration::fromString(e.attribute(QStringLiteral("performed-effort"))));
                    addEntry(date, entry);
                } else if (e.tagName() == QStringLiteral("used-effort")) {
                    KoXmlElement el;
                    forEachElement(el, e) {
                            if (el.tagName() == QStringLiteral("resource")) {
                                QString id = el.attribute(QStringLiteral("id"));
                                Resource *r = status.project().resource(id);
                                if (r == nullptr) {
                                    warnPlan<<"Cannot find resource, id="<<id;
                                    continue;
                                }
                                UsedEffort *ue = new UsedEffort();
                                addUsedEffort(r, ue);
                                ue->loadXML(el, status);
                            }
                    }
                }
        }
    }
    return true;
}

void Completion::saveXML(QDomElement &element)  const
{
    QDomElement el = element.ownerDocument().createElement(QStringLiteral("progress"));
    element.appendChild(el);
    el.setAttribute(QStringLiteral("started"), QString::number((int)m_started));
    el.setAttribute(QStringLiteral("finished"), QString::number((int)m_finished));
    el.setAttribute(QStringLiteral("startTime"), m_startTime.toString(Qt::ISODate));
    el.setAttribute(QStringLiteral("finishTime"), m_finishTime.toString(Qt::ISODate));
    el.setAttribute(QStringLiteral("entrymode"), entryModeToString());
    QMap<QDate, Completion::Entry*>::const_iterator it = m_entries.constBegin();
    for (; it != m_entries.constEnd(); ++it) {
        QDomElement elm = el.ownerDocument().createElement(QStringLiteral("completion-entry"));
        el.appendChild(elm);
        const auto e = m_entries[it.key()];
        elm.setAttribute(QStringLiteral("date"), it.key().toString(Qt::ISODate));
        elm.setAttribute(QStringLiteral("percent-finished"), e->percentFinished);
        elm.setAttribute(QStringLiteral("remaining-effort"), e->remainingEffort.toString());
        elm.setAttribute(QStringLiteral("performed-effort"), e->totalPerformed.toString());
        elm.setAttribute(QStringLiteral("note"), e->note);
    }
    if (! m_usedEffort.isEmpty()) {
        QDomElement elm = el.ownerDocument().createElement(QStringLiteral("used-effort"));
        el.appendChild(elm);
        ResourceUsedEffortMap::ConstIterator i = m_usedEffort.constBegin();
        for (; i != m_usedEffort.constEnd(); ++i) {
            if (i.value() == nullptr) {
                continue;
            }
            QDomElement e = elm.ownerDocument().createElement(QStringLiteral("resource"));
            elm.appendChild(e);
            e.setAttribute(QStringLiteral("id"), i.key()->id());
            i.value()->saveXML(e);
        }
    }
}

//--------------
Completion::UsedEffort::UsedEffort()
{
}

Completion::UsedEffort::UsedEffort(const UsedEffort &e)
{
    mergeEffort(e);
}

Completion::UsedEffort::~UsedEffort()
{
}

void Completion::UsedEffort::mergeEffort(const Completion::UsedEffort &value)
{
    const QMap<QDate, ActualEffort> map = value.actualEffortMap();
    QMap<QDate, ActualEffort>::const_iterator it;
    for (it = map.constBegin(); it != map.constEnd(); ++it) {
        setEffort(it.key(), it.value());
    }
}

void Completion::UsedEffort::setEffort(QDate date, const ActualEffort &value)
{
    m_actual.insert(date, value);
}

Duration Completion::UsedEffort::effortTo(QDate date) const
{
    Duration eff;
    QMap<QDate, ActualEffort>::const_iterator it;
    for (it = m_actual.constBegin(); it != m_actual.constEnd() && it.key() <= date; ++it) {
        eff += it.value().effort();
    }
    return eff;
}

Duration Completion::UsedEffort::effort() const
{
    Duration eff;
    for (const ActualEffort &e : std::as_const(m_actual)) {
        eff += e.effort();
    }
    return eff;
}

bool Completion::UsedEffort::operator==(const Completion::UsedEffort &e) const
{
    return m_actual == e.actualEffortMap();
}

bool Completion::UsedEffort::loadXML(KoXmlElement &element, XMLLoaderObject &)
{
    //debugPlan;
    KoXmlElement e;
    forEachElement(e, element) {
            if (e.tagName() == QStringLiteral("actual-effort")) {
                QDate date = QDate::fromString(e.attribute(QStringLiteral("date")), Qt::ISODate);
                if (date.isValid()) {
                    ActualEffort a;
                    a.setNormalEffort(Duration::fromString(e.attribute(QStringLiteral("normal-effort"))));
                    a.setOvertimeEffort(Duration::fromString(e.attribute(QStringLiteral("overtime-effort"))));
                    setEffort(date, a);
                }
            }
    }
    return true;
}

void Completion::UsedEffort::saveXML(QDomElement &element) const
{
    if (m_actual.isEmpty()) {
        return;
    }
    DateUsedEffortMap::ConstIterator i = m_actual.constBegin();
    for (; i != m_actual.constEnd(); ++i) {
        QDomElement el = element.ownerDocument().createElement(QStringLiteral("actual-effort"));
        element.appendChild(el);
        el.setAttribute(QStringLiteral("overtime-effort"), i.value().overtimeEffort().toString());
        el.setAttribute(QStringLiteral("normal-effort"), i.value().normalEffort().toString());
        el.setAttribute(QStringLiteral("date"), i.key().toString(Qt::ISODate));
    }
}

QHash<Resource*, Appointment> Completion::createAppointments() const
{
    QHash<Resource*, Appointment> apps;

    switch (m_entrymode) {
        case FollowPlan:
            break;
        case EnterCompleted:
            break;
        case EnterEffortPerTask:
            apps = createAppointmentsPerTask();
            break;
        case EnterEffortPerResource:
            apps = createAppointmentsPerResource();
            break;
    }
    return apps;
}

QHash<Resource*, Appointment> Completion::createAppointmentsPerTask() const
{
    QHash<Resource*, Appointment> apps;
    if (!m_started || !m_startTime.isValid()) {
        return apps;
    }
    if (m_node->type() != Node::Type_Task) {
        errorPlan<<Q_FUNC_INFO<<m_node<<"is not a task";
        return apps;
    }
    Project *project = qobject_cast<Project*>(m_node->projectNode());
    if (!project) {
        errorPlan<<Q_FUNC_INFO<<m_node<<"Cannot find project node";
        return apps;
    }
    auto task = static_cast<Task*>(m_node);
    QList<Resource*> resources;
    const auto apps2 = task->currentSchedule()->appointments();
    for (const auto a : apps2) {
        Q_ASSERT(a->resource());
        Q_ASSERT(a->resource()->resource());
        resources << a->resource()->resource();
    }
    if (resources.isEmpty()) {
        errorPlan<<Q_FUNC_INFO<<m_node<<"No resources has been scheduled to work on this task";
        return apps;
    }
    qreal workDay = project->standardWorktime()->durationDay().milliseconds();
    const auto dates = entries().keys();
    for (const QDate &date : dates) {
        if (date < m_startTime.date()) {
            continue;
        }
        DateTime start = DateTime(date, QTime(), project->timeZone());
        qreal day = 86400000;
        if (date == m_startTime.date()) {
            start = m_startTime;
            day -= start.time().msecsSinceStartOfDay();
        }
        qreal factor = workDay / day;
        for (Resource *r : std::as_const(resources)) {
            const qreal actualEffort = this->actualEffort(date).milliseconds() / resources.count();
            const int load = 100 * factor * actualEffort / workDay;
            apps[r].addInterval(start, DateTime(date.addDays(1), QTime(), project->timeZone()), load);
        }
    }
    return apps;
}

QHash<Resource*, Appointment> Completion::createAppointmentsPerResource() const
{
    Q_ASSERT(m_node);
    QHash<Resource*, Appointment> apps;
    if (!m_started || !m_startTime.isValid()) {
        return apps;
    }
    Project *project = qobject_cast<Project*>(m_node->projectNode());
    if (!project) {
        errorPlan<<Q_FUNC_INFO<<m_node<<"Cannot find project node";
        return apps;
    }
    //
    qreal workDay = project->standardWorktime()->durationDay().milliseconds();
    ResourceUsedEffortMap::const_iterator it;
    for (it = usedEffortMap().constBegin(); it != usedEffortMap().constEnd(); ++it) {
        auto r = const_cast<Resource*>(it.key());
        const QMap<QDate, UsedEffort::ActualEffort> actualEffortMap = it.value()->actualEffortMap();
        QMap<QDate, UsedEffort::ActualEffort>::const_iterator it2;
        for (it2 = actualEffortMap.constBegin(); it2 != actualEffortMap.constEnd(); ++it2) {
            const QDate date = it2.key();
            DateTime start = DateTime(date, QTime(), project->timeZone());
            qreal day = 86400000;
            if (date == m_startTime.date()) {
                start = m_startTime;
                day -= start.time().msecsSinceStartOfDay();
            }
            qreal factor = workDay / day;
            const qreal actualEffort = it2.value().effort().milliseconds();
            const int load = 100 * factor * actualEffort / workDay;
            apps[r].addInterval(start, DateTime(date.addDays(1), QTime(), project->timeZone()), load);
        }
    }
    return apps;
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const KPlato::Completion::UsedEffort::ActualEffort &ae)
{
    dbg << QStringLiteral("%1").arg(ae.normalEffort().toDouble(KPlato::Duration::Unit_h), 1);
    return dbg;
}
QDebug operator<<(QDebug dbg, const KPlato::Completion::Entry *e)
{
    return operator<<(dbg, *e);
}
QDebug operator<<(QDebug dbg, const KPlato::Completion::Entry &e)
{
    dbg.noquote().nospace() << "Completion::Entry[";
    dbg << e.percentFinished << '%';
    dbg << " r: " << e.remainingEffort.toDouble(Duration::Unit_h) << 'h';
    dbg << " t: " << e.totalPerformed.toDouble(Duration::Unit_h) << 'h';
    dbg << ']';
    return dbg;
}
#endif
