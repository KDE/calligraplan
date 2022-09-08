/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "kpttask.h"
#include "kptappointment.h"
#include "kptproject.h"
#include "kptduration.h"
#include "kptrelation.h"
#include "kptdatetime.h"
#include "kptcalendar.h"
#include "kpteffortcostmap.h"
#include "kptschedule.h"
#include "kptxmlloaderobject.h"
#include "XmlSaveContext.h"
#include <kptdebug.h>

#include <KoXmlReader.h>

#include <KLocalizedString>

#include <QElapsedTimer>

using namespace KPlato;

WorkPackage::WorkPackage(Task *task)
    : m_task(task),
    m_manager(nullptr),
    m_transmitionStatus(TS_None)
{
    m_completion.setNode(task);
}

WorkPackage::WorkPackage(const WorkPackage &wp)
    : m_task(nullptr),
    m_manager(nullptr),
    m_completion(wp.m_completion),
    m_ownerName(wp.m_ownerName),
    m_ownerId(wp.m_ownerId),
    m_transmitionStatus(wp.m_transmitionStatus),
    m_transmitionTime(wp.m_transmitionTime)
{
}

WorkPackage::~WorkPackage()
{
}

bool WorkPackage::loadXML(KoXmlElement &element, XMLLoaderObject &status)
{
    Q_UNUSED(status);
    m_ownerName = element.attribute(QStringLiteral("owner"));
    m_ownerId = element.attribute(QStringLiteral("owner-id"));
    return true;
}

void WorkPackage::saveXML(QDomElement &element) const
{
    QDomElement el = element.ownerDocument().createElement(QStringLiteral("workpackage"));
    element.appendChild(el);
    el.setAttribute(QStringLiteral("owner"), m_ownerName);
    el.setAttribute(QStringLiteral("owner-id"), m_ownerId);
}

bool WorkPackage::loadLoggedXML(KoXmlElement &element, XMLLoaderObject &status)
{
    m_ownerName = element.attribute(QStringLiteral("owner"));
    m_ownerId = element.attribute(QStringLiteral("owner-id"));
    m_transmitionStatus = transmitionStatusFromString(element.attribute(QStringLiteral("status")));
    m_transmitionTime = DateTime(QDateTime::fromString(element.attribute(QStringLiteral("time")), Qt::ISODate));
    return m_completion.loadXML(element, status);
}

void WorkPackage::saveLoggedXML(QDomElement &element) const
{
    QDomElement el = element.ownerDocument().createElement(QStringLiteral("workpackage"));
    element.appendChild(el);
    el.setAttribute(QStringLiteral("owner"), m_ownerName);
    el.setAttribute(QStringLiteral("owner-id"), m_ownerId);
    el.setAttribute(QStringLiteral("status"), transmitionStatusToString(m_transmitionStatus));
    el.setAttribute(QStringLiteral("time"), m_transmitionTime.toString(Qt::ISODate));
    m_completion.saveXML(el);
}

QList<Resource*> WorkPackage::fetchResources()
{
    return fetchResources(id());
}

QList<Resource*> WorkPackage::fetchResources(long id)
{
    //debugPlan<<m_task.name();
    QList<Resource*> lst;
    if (id == NOTSCHEDULED) {
        if (m_task) {
            lst << m_task->requestedResources();
        }
    } else {
        if (m_task) lst = m_task->assignedResources(id);
        const auto resources = m_completion.resources();
        for (const Resource *r : resources) {
            if (! lst.contains(const_cast<Resource*>(r))) {
                lst << const_cast<Resource*>(r);
            }
        }
    }
    return lst;
}

Completion &WorkPackage::completion()
{
    return m_completion;
}

const Completion &WorkPackage::completion() const
{
    return m_completion;
}


void WorkPackage::setScheduleManager(ScheduleManager *sm)
{
    m_manager = sm;
}


QString WorkPackage::transmitionStatusToString(WorkPackage::WPTransmitionStatus sts, bool trans)
{
    QString s = trans ? i18n("None") : QStringLiteral("None");
    switch (sts) {
        case TS_Send:
            s = trans ? i18n("Send") : QStringLiteral("Send");
            break;
        case TS_Receive:
            s = trans ? i18n("Receive") : QStringLiteral("Receive");
            break;
        case TS_Rejected:
            s = trans ? i18n("Rejected") : QStringLiteral("Rejected");
            break;
        default:
            break;
    }
    return s;
}

WorkPackage::WPTransmitionStatus WorkPackage::transmitionStatusFromString(const QString &sts)
{
    QStringList lst;
    lst << QStringLiteral("None") << QStringLiteral("Send") << QStringLiteral("Receive");
    int s = lst.indexOf(sts);
    return s < 0 ? TS_None : static_cast<WPTransmitionStatus>(s);
}

void WorkPackage::clear()
{
    //m_task = 0;
    m_manager = nullptr;
    m_ownerName.clear();
    m_ownerId.clear();
    m_transmitionStatus = TS_None;
    m_transmitionTime = DateTime();
    m_log.clear();

    m_completion = Completion();
    m_completion.setNode(m_task);

}

//--------------------------------
WorkPackageSettings::WorkPackageSettings()
    : usedEffort(true),
    progress(true),
    documents(true),
    remainingEffort(true)
{
}

void WorkPackageSettings::saveXML(QDomElement &element) const
{
    QDomElement el = element.ownerDocument().createElement(QStringLiteral("settings"));
    element.appendChild(el);
    el.setAttribute(QStringLiteral("used-effort"), QString::number((int)usedEffort));
    el.setAttribute(QStringLiteral("progress"), QString::number((int)progress));
    el.setAttribute(QStringLiteral("documents"), QString::number((int)documents));
    el.setAttribute(QStringLiteral("remaining-effort"), QString::number((int)remainingEffort));
}

bool WorkPackageSettings::loadXML(const KoXmlElement &element)
{
    usedEffort = (bool)element.attribute(QStringLiteral("used-effort")).toInt();
    progress = (bool)element.attribute(QStringLiteral("progress")).toInt();
    documents = (bool)element.attribute(QStringLiteral("documents")).toInt();
    remainingEffort = (bool)element.attribute(QStringLiteral("remaining-effort")).toInt();
    return true;
}

bool WorkPackageSettings::operator==(KPlato::WorkPackageSettings s) const
{
    return usedEffort == s.usedEffort &&
            progress == s.progress &&
            documents == s.documents &&
            remainingEffort == s.remainingEffort;
}

bool WorkPackageSettings::operator!=(KPlato::WorkPackageSettings s) const
{
    return ! operator==(s);
}
