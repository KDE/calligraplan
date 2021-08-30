/* This file is part of the KDE project
* SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
*
* SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "SchedulingModel.h"
#include <MainDocument.h>

#include <kptproject.h>
#include <kptschedule.h>
#include <kptnode.h>

#include <QAbstractItemView>
#include <QDebug>

SchedulingModel::SchedulingModel(QObject *parent)
    : KExtraColumnsProxyModel(parent)
{
    appendColumn(xi18nc("@title:column", "Status"));
    appendColumn(xi18nc("@title:column", "Control"));
    appendColumn(xi18nc("@title:column", "Priority"));

    m_controlKeys << "Schedule" << "Include" << "Exclude";
    m_controlDisplay << i18n("Schedule") << i18n("Include") << i18n("Exclude");

    m_baseModel = new ProjectsFilterModel(this);
    // Note: changes might affect methods below
    const QList<int> columns = QList<int>()
    << KPlato::NodeModel::NodeName
    << KPlato::NodeModel::NodeConstraintStart
    << KPlato::NodeModel::NodeConstraintEnd
    << KPlato::NodeModel::NodeDescription
    << KPlato::NodeModel().propertyCount() + ProjectsModel::ScheduleManagerColumn;
    m_baseModel->setAcceptedColumns(columns);
    setSourceModel(m_baseModel);
}

SchedulingModel::~SchedulingModel()
{
}

Qt::ItemFlags SchedulingModel::flags(const QModelIndex &idx) const
{
    Qt::ItemFlags f = KExtraColumnsProxyModel::flags(idx);
    int extraColumn = extraColumnForProxyColumn(idx.column());
    switch (extraColumn) {
        case 0: // Status
            break;
        case 1: { // Control
            f |= Qt::ItemIsEditable;
            break;
        }
        case 2: { // Priority
            f |= Qt::ItemIsEditable;
            break;
        }
        default: break;
    }
    return f;
}

QString SchedulingModel::displayString(const QString &key) const
{
    QString v = m_controlDisplay.value(m_controlKeys.indexOf(key));
    if (v.isEmpty()) {
        v = i18n("Exclude");
    }
    return v;
}

QString SchedulingModel::keyString(const QString &value) const
{
    return m_controlKeys.value(m_controlDisplay.indexOf(value));
}

bool SchedulingModel::setExtraColumnData(const QModelIndex &parent, int row, int extraColumn, const QVariant &value, int role)
{
    if (parent.isValid()) {
        return false;
    }
    KoDocument *doc = portfolio()->documents().at(row);
    if (!doc) {
        return false;
    }
    if (role == Qt::EditRole) {
        switch (extraColumn) {
            case 0: // Status
                break;
            case 1: { // Control
                if (portfolio()->setDocumentProperty(doc, SCHEDULINGCONTROL,  m_controlKeys.value(value.toInt()))) {
                    extraColumnDataChanged(parent, row, extraColumn, QVector<int>()<<Qt::DisplayRole);
                    return true;
                }
                break;
            }
            case 2: { // Priority
                if (portfolio()->setDocumentProperty(doc, SCHEDULINGPRIORITY, value)) {
                    extraColumnDataChanged(parent, row, extraColumn, QVector<int>()<<Qt::DisplayRole);
                    return true;
                }
                break;
            }
            default: {
                break;
            }
        }
    }
    return false;
}

QVariant SchedulingModel::extraColumnData(const QModelIndex &parent, int row, int extraColumn, int role) const
{
    Q_UNUSED(parent)
    KoDocument *doc = portfolio()->documents().at(row);
    if (!doc) {
        return QVariant();
    }
    switch (extraColumn) {
        case 0: { // Status
            switch (role) {
                case Qt::DisplayRole: {
                    auto *project = doc->project();
                    KPlato::ScheduleManager *sm = doc->project()->findScheduleManagerByName(doc->property(SCHEDULEMANAGERNAME).toString());
                    if (sm && (sm->calculationResult() & KPlato::ScheduleManager::CalculationError)) {
                        return i18n("Error");
                    }
                    auto st = project->status(sm);
                    if (st & KPlato::Node::State_Scheduling) {
                        return i18n("Scheduling...");
                    } else if (st & KPlato::Node::State_NotScheduled) {
                        return i18n("Not scheduled");
                    } else if (st & KPlato::Node::State_Finished) {
                        return i18n("Finished");
                    } else if (st & KPlato::Node::State_Started) {
                        return i18n("Started");
                    } else if (st & KPlato::Node::State_Baselined) {
                        return i18n("Baselined");
                    }
                    return i18n("Scheduled");
                }
                default:
                    return QVariant();
            }
            break;
        }
        case 1: { // Control
            switch (role) {
                case Qt::DisplayRole: {
                    return displayString(doc->property(SCHEDULINGCONTROL).toString());
                }
                case KPlato::Role::EnumList: {
                    return m_controlDisplay;
                }
                case KPlato::Role::EnumListValue: {
                    return m_controlKeys.indexOf(doc->property(SCHEDULINGCONTROL).toString());
                }
                case Qt::ToolTipRole: {
                    QString value = doc->property(SCHEDULINGCONTROL).toString();
                    if (value == "Schedule") {
                        return xi18nc("@info:tooltip", "Schedule this project");
                    }
                    if (value == "Include") {
                        return xi18nc("@info:tooltip", "Include resource bookings from this project");
                    }
                    if (value == "Exclude") {
                        return xi18nc("@info:tooltip", "Exclude this project");
                    }
                    if (value.isEmpty()) {
                        return xi18nc("@info:tooltip", "Exclude this project");
                    }
                }
                default:break;
            }
            break;
        }
        case 2: { // Priority
            switch (role) {
                case Qt::DisplayRole: {
                    return doc->property(SCHEDULINGPRIORITY);
                }
                case Qt::ToolTipRole: {
                    return xi18nc("@info:tooltip", "Highest priority is scheduled first");
                }
                case KPlato::Role::Minimum: {
                    return 0;
                }
                case KPlato::Role::Maximum: {
                    return 1000;
                }
                default:break;
            }
            break;
        }
    }
    return QVariant();
}

void SchedulingModel::setDelegates(QAbstractItemView *view) {
    int offset = sourceModel()->columnCount() - 1;
    view->setItemDelegateForColumn(offset, new KPlato::EnumDelegate(view)); // ScheduleManagerColumn
    view->setItemDelegateForColumn(2 + offset, new KPlato::EnumDelegate(view)); // Control
    view->setItemDelegateForColumn(3 + offset, new KPlato::SpinBoxDelegate(view)); // Priority
}

MainDocument *SchedulingModel::portfolio() const
{
    return m_baseModel->portfolio();
}

void SchedulingModel::setPortfolio(MainDocument *portfolio)
{
    m_baseModel->setPortfolio(portfolio);
    Q_EMIT portfolioChanged();
}
