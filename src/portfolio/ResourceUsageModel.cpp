/* This file is part of the KDE project
* SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
*
* SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ResourceUsageModel.h"
#include "PlanGroupDebug.h"

#include <MainDocument.h>

#include <ScrollableChart.h>

#include <kptproject.h>

#include <KoIcon.h>

#define RESOURCEID_ROLE 100501
#define RESOURCEAVAILABEROLE 100502

ResourceUsageModel::ResourceUsageModel(QObject *parent)
    : QAbstractTableModel(parent)
    , m_portfolio(nullptr)
{
}

ResourceUsageModel::~ResourceUsageModel()
{
}

int ResourceUsageModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_usage.count();
}

int ResourceUsageModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    if (m_usage.isEmpty()) {
        return 0;
    }
    const auto dates = m_usage.keys();
    const auto tasks = m_usage.value(dates.at(0)).keys();
    return std::max(tasks.count(), 1);
}


QVariant ResourceUsageModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == AXISRANGEROLE) {
        return QList<QVariant>() << 0.0 << m_normalMax << 0.0 << m_stackedMax;
    }
    if (role != Qt::DisplayRole) {
        return QVariant();
    }
    if (m_usage.isEmpty()) {
        return QVariant();
    }
    if (orientation == Qt::Vertical) {
        // x-axis labels (dates)
        const auto dates = m_usage.keys();
        return QLocale().toString(dates.value(section), QLocale::ShortFormat);
    } else if (orientation == Qt::Horizontal) {
        // legends (task names)
        const auto dates = m_usage.keys();
        const auto tasks = m_usage.value(dates.at(0)).keys();
        if (section >= tasks.count()) {
            return i18n("No task");
        }
        const auto task = tasks.value(section);
        return task->name();
    }
    return QVariant();
}

QVariant ResourceUsageModel::data(const QModelIndex &idx, int role) const
{
    if (role == RESOURCEAVAILABEROLE) {
        const auto dates = m_available.keys();
        const auto date = dates.value(idx.row());
        auto v = m_available.value(date);
        return v;
    }
    if (role != Qt::DisplayRole) {
        return QVariant();
    }
    const auto dates = m_usage.keys();
    const auto date = dates.at(idx.row());
    const auto map = m_usage.value(date);
    const auto values = map.values();
    if (values.isEmpty()) {
        return 0.0;
    }
    return values.at(idx.column());
}

MainDocument *ResourceUsageModel::portfolio() const
{
    return m_portfolio;
}

void ResourceUsageModel::setPortfolio(MainDocument *portfolio)
{
    beginResetModel();
    if (m_portfolio) {
        disconnect(m_portfolio, &MainDocument::documentAboutToBeInserted, this, &ResourceUsageModel::documentAboutToBeInserted);
        disconnect(m_portfolio, &MainDocument::documentInserted, this, &ResourceUsageModel::documentInserted);
        disconnect(m_portfolio, &MainDocument::documentAboutToBeRemoved, this, &ResourceUsageModel::documentAboutToBeRemoved);
        disconnect(m_portfolio, &MainDocument::documentRemoved, this, &ResourceUsageModel::documentRemoved);

        disconnect(m_portfolio, &MainDocument::documentChanged, this, &ResourceUsageModel::documentChanged);
        disconnect(m_portfolio, &MainDocument::projectChanged, this, &ResourceUsageModel::projectChanged);
    }
    m_portfolio = portfolio;
    if (m_portfolio) {
        connect(m_portfolio, &MainDocument::documentAboutToBeInserted, this, &ResourceUsageModel::documentAboutToBeInserted);
        connect(m_portfolio, &MainDocument::documentInserted, this, &ResourceUsageModel::documentInserted);
        connect(m_portfolio, &MainDocument::documentAboutToBeRemoved, this, &ResourceUsageModel::documentAboutToBeRemoved);
        connect(m_portfolio, &MainDocument::documentRemoved, this, &ResourceUsageModel::documentRemoved);

        connect(m_portfolio, &MainDocument::documentChanged, this, &ResourceUsageModel::documentChanged);
        connect(m_portfolio, &MainDocument::projectChanged, this, &ResourceUsageModel::projectChanged);
    }
    initiateEmptyData();
    endResetModel();
    Q_EMIT portfolioChanged();
}

void ResourceUsageModel::reset()
{
    Q_EMIT portfolioChanged();
}

void ResourceUsageModel::documentAboutToBeInserted(int row)
{
    Q_UNUSED(row)
    beginResetModel();
}

void ResourceUsageModel::documentInserted()
{
    updateData();
    endResetModel();
}

void ResourceUsageModel::documentAboutToBeRemoved(int row)
{
    Q_UNUSED(row)
    beginResetModel();
}

void ResourceUsageModel::documentRemoved()
{
    updateData();
    endResetModel();
}

void ResourceUsageModel::documentChanged(KoDocument *doc, int row)
{
    Q_UNUSED(doc)
    Q_UNUSED(row)
    setCurrentResource(m_currentResourceId);
}

void ResourceUsageModel::projectChanged(KoDocument *doc)
{
    documentChanged(doc, m_portfolio->documents().indexOf(doc));
}

void ResourceUsageModel::initiateEmptyData()
{
    m_normalMax = 0.0;
    m_stackedMax = 0.0;
    int size = 0;
    QDate startDate;
    if (m_portfolio) {
        QDate endDate;
        const auto docs = m_portfolio->documents();
        for (const auto doc : docs) {
            const auto project = doc->project();
            const auto sm = project->findScheduleManagerByName(doc->property(SCHEDULEMANAGERNAME).toString());
            if (!sm) {
                continue;
            }
            const auto sid = sm->scheduleId();
            const auto s = project->startTime(sid).date();
            if (!startDate.isValid() || (s.isValid() && startDate > s)) {
                startDate = project->startTime(sid).date();
            }
            endDate = std::max<QDate>(endDate, project->endTime(sid).date());
        }
        if (startDate.isValid() && endDate.isValid()) {
            size = startDate.daysTo(endDate) + 1;
        }
    }
    if (size == 0) {
        size = 5;
    }
    if (!startDate.isValid()) {
        startDate = QDate::currentDate();
    }
    for (int i = 0; i < size; ++i) {
        const auto date(startDate.addDays(i));
        m_usage.insert(date, QHash<KPlato::Node*, double>());
    }
}

void ResourceUsageModel::setCurrentResource(const QString &id)
{
    m_currentResourceId = id;
    if (id.isEmpty()) {
        // no resource, just use default data
        beginResetModel();
        initiateEmptyData();
        endResetModel();
        return;
    }
    beginResetModel();
    updateData();
    endResetModel();
}

void ResourceUsageModel::updateData()
{
    if (m_currentResourceId.isEmpty()) {
        // no resource, just use default data
        initiateEmptyData();
        return;
    }
    m_normalMax = 0.0;
    m_stackedMax = 0.0;
    m_available.clear();
    if (m_portfolio) {
        QList<KPlato::Node*> tasks;
        QDate startDate;
        QDate endDate;
        // get a list of all tasks in all projects
        // and calculate start and end time
        const auto docs = m_portfolio->documents();
        for (const auto doc : docs) {
            const auto project = doc->project();
            const auto sm = project->findScheduleManagerByName(doc->property(SCHEDULEMANAGERNAME).toString());
            if (!sm) {
                continue;
            }
            const auto sid = sm->scheduleId();
            const auto tasklist = project->allTasks();
            for (const auto t : tasklist) {
                if (t->type() == KPlato::Node::Type_Task && t->isScheduled(sid)) {
                    tasks << t;
                }
            }
            const auto s = project->startTime(sid).date();
            if (!startDate.isValid() || (s.isValid() && startDate > s)) {
                startDate = project->startTime(sid).date();
            }
            endDate = std::max<QDate>(endDate, project->endTime(sid).date());
        }
        if (!startDate.isValid() || !endDate.isValid()) {
            return;
        }
        int size = startDate.daysTo(endDate) + 1;
        // initiate m_usage
        for (int i = 0; i < size; ++i) {
            const auto date(startDate.addDays(i));
            for (auto t : qAsConst(tasks)) {
                m_usage[date].insert(t, 0.0);
            }
        }
        KPlato::Resource *currentResource =nullptr;
        for (const auto doc : docs) {
            const auto project = doc->project();
            const auto resource = project->resource(m_currentResourceId);
            if (!resource) {
                debugPortfolio<<project<<"No resource"<<m_currentResourceId;
                continue;
            }
            const auto sm = project->findScheduleManagerByName(doc->property(SCHEDULEMANAGERNAME).toString());
            if (!sm) {
                debugPortfolio<<project<<"No scheduleManager"<<doc->property(SCHEDULEMANAGERNAME).toString();
                continue;
            }
            const auto schedule = resource->schedule(sm->scheduleId());
            if (!schedule) {
                debugPortfolio<<project<<"No resource schedule"<<sm->scheduleId();
                continue;
            }
            if (!currentResource) {
                currentResource = resource;
            }
            const auto appointments = schedule->appointments();
            for (const auto a : appointments) {
                auto task = a->node()->node();
                const auto intervalList = a->intervals();
                for (int i = 0; i < size; ++i) {
                    KPlato::DateTime date(startDate.addDays(i));
                    double effort = intervalList.effort(date, date.addDays(1)).toDouble();
                    m_usage[date.date()].insert(task, effort);
                    if (effort > 0.0  && tasks.contains(task)) {
                        tasks.removeOne(task);
                    }
                }
            }
        }
        // Remove tasks not used by this resource
        QMap<QDate, QHash<KPlato::Node*, double> >::iterator it;
        for (it = m_usage.begin(); it != m_usage.end(); ++it) {
            for (auto task : qAsConst(tasks)) {
                it.value().remove(task);
            }
        }
        for (it = m_usage.begin(); it != m_usage.end(); ++it) {
            double total = 0.0;
            const auto tasks = it.value();
            QHash<KPlato::Node*, double>::const_iterator taskIt = tasks.constBegin();
            for (; taskIt != tasks.constEnd(); ++taskIt) {
                total += taskIt.value();
                m_normalMax = std::max(m_normalMax, taskIt.value());
            }
            m_stackedMax = std::max(m_stackedMax, total);
        }
        if (currentResource) {
            const auto cal = currentResource->calendar();
            if (!cal && currentResource->type() == KPlato::Resource::Type_Material) {
                m_normalMax = std::max(m_normalMax, 24.0);
                m_stackedMax = std::max(m_stackedMax, 24.0);
                return;
            }
            if (!cal) {
                return;
            }
            for (it = m_usage.begin(); it != m_usage.end(); ++it) {
                const auto time = KPlato::DateTime(it.key(), QTime());
                const KPlato::DateTime end = time.addDays(1);
                auto effort = cal->effort(time, end).toDouble();
                m_available.insert(it.key(), effort);
                if (effort > 0) {
                    //qInfo()<<Q_FUNC_INFO<<it.key()<<end<<effort<<m_available;
                }
                m_normalMax = std::max(m_normalMax, effort);
                m_stackedMax = std::max(m_stackedMax, effort);
            }
        }
    }
}

ResourceAvailableModel::ResourceAvailableModel(QObject* parent)
    : QSortFilterProxyModel(parent)
{
}
#if 0
QModelIndex ResourceAvailableModel::mapFromSource(const QModelIndex& sourceIndex) const
{
    return createIndex(sourceIndex.row(), sourceIndex.column());
}

QModelIndex ResourceAvailableModel::mapToSource(const QModelIndex& proxyIndex) const
{
    if (!sourceModel()) {
        return QModelIndex();
    }
    return proxyIndex.isValid() ? sourceModel()->index(proxyIndex.row(), proxyIndex.column()) : proxyIndex;
}

QModelIndex ResourceAvailableModel::parent(const QModelIndex& idx) const
{
    Q_UNUSED(idx)
    return QModelIndex();
}

QModelIndex ResourceAvailableModel::index(int row, int column, const QModelIndex& parent) const
{
    return parent.isValid() ? QModelIndex() : createIndex(row, column);
}

int ResourceAvailableModel::rowCount(const QModelIndex& parent) const
{
    return sourceModel() && !parent.isValid() ? sourceModel()->rowCount() : 0;
}
#endif
int ResourceAvailableModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return 1;
}

QVariant ResourceAvailableModel::data(const QModelIndex& index, int role) const
{
    if (role == Qt::DisplayRole) {
        role = RESOURCEAVAILABEROLE;
    }
    const auto v = QSortFilterProxyModel::data(index, role);
    return v;
}

void ResourceAvailableModel::sourceReset()
{
    beginResetModel();
    endResetModel();
}
