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
#include <ExtraProperties.h>

#define RESOURCEID_ROLE 100501
#define RESOURCEAVAILABEROLE 100502

ResourceUsageModel::ResourceUsageModel(QObject *parent)
    : QAbstractTableModel(parent)
    , m_portfolio(nullptr)
    , m_minimumStartDate(QDate::currentDate())
{
}

ResourceUsageModel::~ResourceUsageModel()
{
}

void ResourceUsageModel::setDateRange(const QDate &min, const QDate &max)
{
    beginResetModel();
    m_minimumStartDate = min;
    if (max.isValid() && max < min) {
        m_maximumEndDate = min;
    } else {
        m_maximumEndDate = max;
    }
    updateData();
    endResetModel();
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
    return std::max(tasks.count(), qsizetype(1));
}


QVariant ResourceUsageModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == AXISRANGEROLE) {
        return QList<QVariant>() << 0.0 << m_normalMax << 0.0 << m_stackedMax;
    }
    if (m_usage.isEmpty()) {
        return QVariant();
    }
    if (orientation == Qt::Horizontal) {
        switch (role) {
            case Qt::DisplayRole: {
                // legends (task names)
                const auto dates = m_usage.keys();
                const auto tasks = m_usage.value(dates.at(0)).keys();
                const auto taskName = tasks.value(section);
                return taskName;
            }
            case Qt::TextAlignmentRole:
                return Qt::AlignLeft;
            default:
                return QVariant();
        }
    } else if (orientation == Qt::Vertical) {
        switch (role) {
            case Qt::DisplayRole: {
                // x-axis labels (dates)
                const auto dates = m_usage.keys();
                return QLocale().toString(dates.value(section), QLocale::ShortFormat);
            }
            case Qt::ForegroundRole: {
                const auto dates = m_usage.keys();
                const auto date = dates.value(section);
                if (date == QDate::currentDate()) {
                    return QColor::fromRgb(0xFF, 0, 0);
                }
            }
                break;
            default:
                return QVariant();
        }
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
    if (role != Qt::DisplayRole && role != Qt::ToolTipRole) {
        return QVariant();
    }
    switch (role) {
        case Qt::DisplayRole: {
            const auto dates = m_usage.keys();
            const auto date = dates.at(idx.row());
            const auto map = m_usage.value(date);
            const auto values = map.values();
            return values.isEmpty() ? 0.0 : values.at(idx.column()).second;
        }
        case Qt::ToolTipRole: {
            const auto dates = m_usage.keys();
            const auto date = dates.at(idx.row());
            const auto map = m_usage.value(date);
            const auto tasks = map.values();
            const auto task = tasks.value(idx.column()).first;
            if (!task) {
                return i18n("No task");
            }
            return xi18nc("@info:tooltip", "WBS: %1<nl/>Task: %2<nl/>Project: %3", task->wbsCode(), task->name(), task->projectNode()->name());
        }
        default: break;
    }
    return QVariant();
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
    m_usage.clear();
    m_available.clear();
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
            auto e = project->endTime(sid).date();
            if (!e.isValid()) {
                continue;
            }
            if (m_maximumEndDate.isValid()) {
                e = std::min<QDate>(e, m_maximumEndDate);
            }
            if (e < m_minimumStartDate) {
                continue;
            }
            endDate = std::max<QDate>(endDate, e);
            auto s = project->startTime(sid).date();
            if (!s.isValid()) {
                continue;
            }
            s = std::max(s, m_minimumStartDate);
            if (!startDate.isValid() || s < startDate) {
                startDate = s;
            }
        }
        if (startDate.isValid() && endDate.isValid()) {
            size = startDate.daysTo(endDate) + 1;
        }
    }
    if (size == 0) {
        size = 5;
    }
    if (!startDate.isValid()) {
        startDate = m_minimumStartDate;
    }
    for (int i = 0; i < size; ++i) {
        const auto date(startDate.addDays(i));
        m_usage.insert(date, QMap<QString, std::pair<KPlato::Node*, double>>());
    }
}

void ResourceUsageModel::setCurrentResource(const QString &id)
{
    beginResetModel();
    m_currentResourceId = id;
    updateData();
    endResetModel();
}

void ResourceUsageModel::updateData()
{
    m_usage.clear();
    m_available.clear();
    if (m_currentResourceId.isEmpty()) {
        // no resource, just use default data
        initiateEmptyData();
        return;
    }
    m_normalMax = 0.0;
    m_stackedMax = 0.0;
    if (m_portfolio) {
        QList<KPlato::Task*> tasks;
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
            auto e = project->endTime(sid).date();
            if (!e.isValid()) {
                continue;
            }
            if (m_maximumEndDate.isValid()) {
                e = std::min<QDate>(e, m_maximumEndDate);
            }
            if (e < m_minimumStartDate) {
                continue;
            }
            endDate = std::max<QDate>(endDate, e);
            auto s = project->startTime(sid).date();
            if (!s.isValid()) {
                continue;
            }
            s = std::max(s, m_minimumStartDate);
            if (!startDate.isValid() || s < startDate) {
                startDate = s;
            }
            QMap<QString, KPlato::Task*> taskList;
            const auto lst = project->allTasks();
            for (const auto t : lst) {
                if (t->type() == KPlato::Node::Type_Task && t->isScheduled(sid)) {
                    taskList.insert(t->name(), t);
                }
            }
            tasks = taskList.values();
        }
        if (!startDate.isValid() || !endDate.isValid()) {
            return;
        }
        int size = startDate.daysTo(endDate) + 1;
        // initiate m_usage
        for (int i = 0; i < size; ++i) {
            const auto date(startDate.addDays(i));
            for (auto t : std::as_const(tasks)) {
                m_usage[date].insert(t->name(), std::pair<KPlato::Node*, double>(t, (double)0.0));
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
                auto task = static_cast<KPlato::Task*>(a->node()->node());
                const auto intervalList = a->intervals();
                for (int i = 0; i < size; ++i) {
                    const KPlato::DateTime date(startDate.addDays(i));
                    double effort = intervalList.effort(date, date.addDays(1)).toDouble();
                    auto te = std::pair<KPlato::Node*, double>(task, effort);
                    m_usage[date.date()].insert(task->name(), te);
                    if (effort > 0.0  && tasks.contains(task)) {
                        tasks.removeOne(task);
                    }
                }
            }
        }
        // Remove tasks not used by this resource
        QMap<QDate, QMap<QString, std::pair<KPlato::Node*, double>>>::iterator it;
        for (it = m_usage.begin(); it != m_usage.end(); ++it) {
            for (auto task : std::as_const(tasks)) {
                it.value().remove(task->name());
            }
        }
        for (it = m_usage.begin(); it != m_usage.end(); ++it) {
            double total = 0.0;
            const auto tasks = it.value();
            QMap<QString, std::pair<KPlato::Node*, double>>::const_iterator taskIt = tasks.constBegin();
            for (; taskIt != tasks.constEnd(); ++taskIt) {
                total += taskIt.value().second;
                m_normalMax = std::max(m_normalMax, taskIt.value().second);
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
