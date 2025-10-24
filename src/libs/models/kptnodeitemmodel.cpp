/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2007-2009, 2012 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2016 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "kptnodeitemmodel.h"

#include "kptglobal.h"
#include "kptlocale.h"
#include "kptcommonstrings.h"
#include "kptcommand.h"
#include <AddResourceCmd.h>
#include <AddParentGroupCmd.h>
#include <InsertProjectCmd.h>
#include "kptduration.h"
#include "kptproject.h"
#include "kptnode.h"
#include "kpttaskcompletedelegate.h"
#include "kptxmlloaderobject.h"
#include "XmlSaveContext.h"
#include "InsertProjectXmlCommand.h"
#include "kptdebug.h"

#include <KoXmlReader.h>
#include <KoXmlWriter.h>
#include <KoOdf.h>
#include <KoOdfWriteStore.h>
#include <KoStore.h>
#include <KoStoreDevice.h>
#include <KoXmlNS.h>
#include <KoIcon.h>
#include <ParameterSubstitutionDialog.h>

#include <QMimeData>
#include <QMimeDatabase>
#include <QModelIndex>
#include <QByteArray>
#include <QHash>

#include <krichtextwidget.h>

#include <KGanttGlobal>

#include <math.h>


namespace KPlato
{


//--------------------------------------
NodeModel::NodeModel()
    : QObject(),
    m_project(nullptr),
    m_manager(nullptr),
    m_now(QDate::currentDate()),
    m_prec(1)
{
}

const QMetaEnum NodeModel::columnMap() const
{
    return metaObject()->enumerator(metaObject()->indexOfEnumerator("Properties"));
}

void NodeModel::setProject(Project *project)
{
    //debugPlan<<m_project<<"->"<<project;
    m_project = project;
}

void NodeModel::setManager(ScheduleManager *sm)
{
    //debugPlan<<m_manager<<"->"<<sm;
    m_manager = sm;
}

QVariant NodeModel::name(const Node *node, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            return node->name();
        case Qt::ToolTipRole: {
            QTextEdit w(node->description(), nullptr);
            QString description = w.toPlainText();
            if (description.length() > 200) {
                description = description.left(200) + QStringLiteral(" ...");
                description.replace(QLatin1Char('\n'), QStringLiteral("<br/>"));
            } else {
                description = node->description();
            }
            w.setHtml(i18n("<p><strong>%1: %2</strong></p><p>%3</p>", node->wbsCode(), node->name(), description));
            return w.toHtml();
        }
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
        case Qt::DecorationRole:
            if (node->type() == Node::Type_Project) {
                uint st = static_cast<const Project*>(node)->status();
                if (st &= Node::State_Finished) {
                    return koIcon("end-of-life");
                }
            }
            if (node->isBaselined()) {
                return koIcon("view-time-schedule-baselined");
            }
            break;
        case Role::Foreground: {
            if (! m_project) {
                break;
            }
            switch (node->type()) {
                case Node::Type_Task:
                    return static_cast<const Task*>(node)->completion().isFinished() ? m_project->config().taskFinishedColor() : m_project->config().taskNormalColor();
                case Node::Type_Milestone:
                    return static_cast<const Task*>(node)->completion().isFinished() ? m_project->config().milestoneFinishedColor() : m_project->config().milestoneNormalColor();
                case Node::Type_Summarytask:
                    return m_project->config().summaryTaskLevelColor(node->level());
                default:
                    break;
            }
            break;
        }
    }
    return QVariant();
}

QVariant NodeModel::leader(const Node *node, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
        case Qt::ToolTipRole:
            return node->leader();
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::allocation(const Node *node, int role) const
{
    if (node->type() == Node::Type_Task) {
        switch (role) {
            case Qt::DisplayRole:
            case Qt::EditRole:
                return node->requests().requestNameList().join(QStringLiteral(","));
            case Qt::ToolTipRole: {
                QMap<QString, std::pair<QStringList, QStringList> > lst;
                const QList<ResourceRequest*> requests = node->requests().resourceRequests(false);
                for (ResourceRequest *rr : requests) {
                    const auto name = rr->resource()->name();
                    lst.insert(name, std::pair<QStringList, QStringList>());
                    const auto alternatives = rr->alternativeRequests();
                    for (ResourceRequest *r : alternatives) {
                        lst[name].first << r->resource()->name();
                    }
                    const QList<Resource*> required = rr->requiredResources();
                    for (Resource *r : required) {
                        lst[name].second << r->name();
                    }
                }
                if (lst.isEmpty()) {
                    return xi18nc("@info:tooltip", "No resources has been allocated");
                }
                QStringList sl;
                for (QMap<QString, std::pair<QStringList, QStringList> >::ConstIterator it = lst.constBegin(); it != lst.constEnd(); ++it) {
                    const auto name = it.key();
                    const auto alternatives = lst.value(name).first;
                    const auto required = lst.value(name).second;
                    QString alts;
                    QString reqs;
                    if (!alternatives.isEmpty()) {
                        alts = i18nc("list of alternative resources", " [%1]", alternatives.join(QStringLiteral(", ")));
                    }
                    if (!required.isEmpty()) {
                        reqs = i18nc("list of required resources", " (%1)", required.join(QStringLiteral(", ")));
                    }
                    sl << i18nc("1=resource name 2=list of alternatives 3=list of required", "%1%2%3", name, alts, reqs);
                }
                if (sl.count() == 1) {
                    return xi18nc("@info:tooltip 1=resource name", "Allocated resource:<nl/>%1", sl.first());
                }
                KLocalizedString ks = kxi18nc("@info:tooltip 1=list of resources", "Allocated resources:<nl/>%1");
                // Hack to get around ks escaping '<' and '>'
                QString s = ks.subs(sl.join(QStringLiteral("#¤#"))).toString();
                return s.replace(QStringLiteral("#¤#"), QStringLiteral("<br/>"));
            }
            case Qt::StatusTipRole:
            case Qt::WhatsThisRole:
                return QVariant();
        }
    }
    return QVariant();
}

QVariant NodeModel::description(const Node *node, int role) const
{
    switch (role) {
        case Qt::DisplayRole: {
            KRichTextWidget w(node->description(), nullptr);
            w.switchToPlainText();
            QString s = w.textOrHtml();
            s.remove(QLatin1Char('\r'));
            return s.replace(QLatin1Char('\n'), QLatin1Char(' '));
        }
        case Qt::ToolTipRole: {
            KRichTextWidget w(node->description(), nullptr);
            w.switchToPlainText();
            if (w.textOrHtml().isEmpty()) {
                return QVariant();
            }
            return node->description();
        }
        case Qt::EditRole:
            return node->description();
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::type(const Node *node, int role) const
{
    //debugPlan<<node->name()<<", "<<role;
    switch (role) {
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
            return node->typeToString(true);
        case Qt::EditRole:
            return node->type();
        case Qt::TextAlignmentRole:
            return (int)(Qt::AlignLeft|Qt::AlignVCenter);
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::constraint(const Node *node, int role) const
{
    if (node->type() == Node::Type_Project) {
        switch (role) {
            case Qt::DisplayRole:
                return i18n("Target times");
            case Qt::ToolTipRole:
                return xi18nc("@info:tooltip", "Earliest start and latest finish");
            case Role::EnumList:
            case Qt::EditRole:
            case Role::EnumListValue:
                return QVariant();
            case Qt::TextAlignmentRole:
                return Qt::AlignCenter;
            case Qt::StatusTipRole:
            case Qt::WhatsThisRole:
                return QVariant();
        }
    } else if (node->type() != Node::Type_Summarytask) {
        switch (role) {
            case Qt::DisplayRole:
            case Qt::ToolTipRole:
                return node->constraintToString(true);
            case Role::EnumList:
                return Node::constraintList(true);
            case Qt::EditRole:
                return node->constraint();
            case Role::EnumListValue:
                return (int)node->constraint();
            case Qt::TextAlignmentRole:
                return Qt::AlignCenter;
            case Qt::StatusTipRole:
            case Qt::WhatsThisRole:
                return QVariant();
        }
    }
    return QVariant();
}

QVariant NodeModel::constraintStartTime(const Node *node, int role) const
{
    if (node->type() == Node::Type_Project) {
        switch (role) {
            case Qt::DisplayRole: {
                return QLocale().toString(node->constraintStartTime(), QLocale::ShortFormat);
            }
            case Qt::ToolTipRole: {
                return QLocale().toString(node->constraintStartTime(), QLocale::LongFormat);
            }
            case Qt::EditRole:
                return node->constraintStartTime();
            case Qt::StatusTipRole:
            case Qt::WhatsThisRole:
                return QVariant();
        }
        return QVariant();
    } else if (node->type() != Node::Type_Summarytask) {
        switch (role) {
            case Qt::DisplayRole: {
                QString s = QLocale().toString(node->constraintStartTime(), QLocale::ShortFormat);
                switch (node->constraint()) {
                    case Node::StartNotEarlier:
                    case Node::MustStartOn:
                    case Node::FixedInterval:
                        return s;
                    default:
                        break;
                }
                return QStringLiteral("(%1)").arg(s);
        }
            case Qt::ToolTipRole: {
                int c = node->constraint();
                if (c == Node::MustStartOn || c == Node::StartNotEarlier || c == Node::FixedInterval) {
                    return QLocale().toString(node->constraintStartTime(), QLocale::LongFormat);
                }
                break;
            }
            case Qt::EditRole:
                return node->constraintStartTime();
            case Qt::StatusTipRole:
            case Qt::WhatsThisRole:
                return QVariant();
        }
    }
    return QVariant();
}

QVariant NodeModel::constraintEndTime(const Node *node, int role) const
{
    if (node->type() == Node::Type_Project) {
        switch (role) {
            case Qt::DisplayRole: {
                return QLocale().toString(node->constraintEndTime(), QLocale::ShortFormat);
            }
            case Qt::ToolTipRole: {
                return QLocale().toString(node->constraintEndTime(), QLocale::LongFormat);
            }
            case Qt::EditRole:
                return node->constraintEndTime();
            case Qt::StatusTipRole:
            case Qt::WhatsThisRole:
                return QVariant();
        }
        return QVariant();
    } else if (node->type() != Node::Type_Summarytask) {
        switch (role) {
            case Qt::DisplayRole: {
                QString s = QLocale().toString(node->constraintEndTime(), QLocale::ShortFormat);
                switch (node->constraint()) {
                    case Node::FinishNotLater:
                    case Node::MustFinishOn:
                    case Node::FixedInterval:
                        return s;
                    default:
                        break;
                }
                return QStringLiteral("(%1)").arg(s);
            }
            case Qt::ToolTipRole: {
                int c = node->constraint();
                if (c == Node::FinishNotLater || c == Node::MustFinishOn || c == Node::FixedInterval) {
                    return QLocale().toString(node->constraintEndTime(), QLocale::LongFormat);
                }
                break;
            }
            case Qt::EditRole:
                return node->constraintEndTime();
            case Qt::StatusTipRole:
            case Qt::WhatsThisRole:
                return QVariant();
        }
    }
    return QVariant();
}

QVariant NodeModel::estimateType(const Node *node, int role) const
{
    if (node->estimate() == nullptr) {
        return QVariant();
    }
    switch (role) {
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
            if (node->type() == Node::Type_Task) {
                return node->estimate()->typeToString(true);
            }
            return QString();
        case Role::EnumList:
            return Estimate::typeToStringList(true);
        case Qt::EditRole:
            if (node->type() == Node::Type_Task) {
                return node->estimate()->typeToString();
            }
            return QString();
        case Role::EnumListValue:
            return (int)node->estimate()->type();
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::estimateCalendar(const Node *node, int role) const
{
    if (node->estimate() == nullptr) {
        return QVariant();
    }
    switch (role) {
        case Qt::DisplayRole:
            if (node->type() == Node::Type_Task) {
                if (node->estimate()->calendar()) {
                    return node->estimate()->calendar()->name();
                }
                return i18n("None");
            }
            return QString();
        case Qt::ToolTipRole:
            if (node->type() == Node::Type_Task) {
                if (node->estimate()->type() == Estimate::Type_Effort) {
                    return xi18nc("@info:tooltip", "Not applicable, estimate type is Effort");
                }
                if (node->estimate()->calendar()) {
                    return node->estimate()->calendar()->name();
                }
                return QVariant();
            }
            return QString();
        case Role::EnumList:
        {
            QStringList lst; lst << i18n("None");
            const Node *n = const_cast<Node*>(node)->projectNode();
            if (n) {
                lst += static_cast<const Project*>(n)->calendarNames();
            }
            return lst;
        }
        case Qt::EditRole:
            if (node->type() == Node::Type_Task) {
                if (node->estimate()->calendar() == nullptr) {
                    return i18n("None");
                }
                return node->estimate()->calendar()->name();
            }
            return QString();
        case Role::EnumListValue:
        {
            if (node->estimate()->calendar() == nullptr) {
                return 0;
            }
            QStringList lst;
            const Node *n = const_cast<Node*>(node)->projectNode();
            if (n) {
                lst = static_cast<const Project*>(n)->calendarNames();
            }
            return lst.indexOf(node->estimate()->calendar()->name()) + 1;
        }
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::estimate(const Node *node, int role) const
{
    if (node == nullptr || node->estimate() == nullptr) {
        return QVariant();
    }
    switch (role) {
        case Qt::DisplayRole:
            if (node->type() == Node::Type_Task  || node->type() == Node::Type_Milestone) {
                Duration::Unit unit = node->estimate()->unit();
                QString s = QLocale().toString(node->estimate()->expectedEstimate(), 'f', m_prec) +  Duration::unitToString(unit, true);
                if (node->constraint() == Node::FixedInterval && node->estimate()->type() == Estimate::Type_Duration) {
                    s = QLatin1Char('(') + s + QLatin1Char(')');
                }
                return s;
            }
            break;
        case Qt::ToolTipRole:
            if (node->type() == Node::Type_Task) {
                Estimate::Type t = node->estimate()->type();
                if (node->constraint() == Node::FixedInterval && t == Estimate::Type_Duration) {
                    return xi18nc("@info:tooltip", "Not applicable, constraint is Fixed Interval");
                }
                auto s = estimate(node, Qt::DisplayRole).toString();
                if (node->estimate()->risktype() == Estimate::Risk_None) {
                    if (t == Estimate::Type_Effort) {
                        return xi18nc("@info:tooltip", "Estimated effort: %1", s);
                    }
                    return xi18nc("@info:tooltip", "Estimated duration: %1", s);
                }
                if (t == Estimate::Type_Effort) {
                    auto expected = pertExpected(node->estimate(), Qt::DisplayRole).toString();
                    return xi18nc("@info:tooltip", "Estimated effort: %1<nl/>Expected effort: %2", s, expected);
                }
                auto expected = pertExpected(node->estimate(), Qt::DisplayRole).toString();
                return xi18nc("@info:tooltip", "Estimated duration: %1<nl/>Expected duration: %2", s, expected);
            }
            break;
        case Qt::EditRole:
            return node->estimate()->expectedEstimate();
        case Role::DurationUnit:
            return static_cast<int>(node->estimate()->unit());
        case Role::Minimum:
            return m_project->config().minimumDurationUnit();
        case Role::Maximum:
            return m_project->config().maximumDurationUnit();
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::optimisticRatio(const Node *node, int role) const
{
    if (node->estimate() == nullptr || node->type() == Node::Type_Summarytask || node->type() == Node::Type_Milestone) {
        return QVariant();
    }
    switch (role) {
        case Qt::DisplayRole:
            if (node->type() == Node::Type_Task && node->constraint() == Node::FixedInterval && node->estimate()->type() == Estimate::Type_Duration) {
                QString s = QString::number(node->estimate()->optimisticRatio());
                s = QLatin1Char('(') + s + QLatin1Char(')');
                return s;
            }
            if (node->estimate()) {
                return node->estimate()->optimisticRatio();
            }
            break;
        case Qt::EditRole:
            if (node->estimate()) {
                return node->estimate()->optimisticRatio();
            }
            break;
        case Qt::ToolTipRole:
            if (node->type() == Node::Type_Task) {
                Duration::Unit unit = node->estimate()->unit();
                QString s = QLocale().toString(node->estimate()->optimisticEstimate(), 'f', m_prec) +  Duration::unitToString(unit, true);
                Estimate::Type t = node->estimate()->type();
                if (node->constraint() == Node::FixedInterval && t == Estimate::Type_Duration) {
                    s = xi18nc("@info:tooltip", "Not applicable, constraint is Fixed Interval");
                } else if (t == Estimate::Type_Effort) {
                    s = xi18nc("@info:tooltip", "Optimistic effort: %1", s);
                } else {
                    s = xi18nc("@info:tooltip", "Optimistic duration: %1", s);
                }
                return s;
            }
            break;
        case Role::Minimum:
            return -99;
        case Role::Maximum:
            return 0;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::pessimisticRatio(const Node *node, int role) const
{
    if (node->estimate() == nullptr || node->type() == Node::Type_Summarytask || node->type() == Node::Type_Milestone) {
        return QVariant();
    }
    switch (role) {
        case Qt::DisplayRole:
            if (node->type() == Node::Type_Task && node->constraint() == Node::FixedInterval && node->estimate()->type() == Estimate::Type_Duration) {
                QString s = QString::number(node->estimate()->pessimisticRatio());
                s = QLatin1Char('(') + s + QLatin1Char(')');
                return s;
            }
            if (node->estimate()) {
                return node->estimate()->pessimisticRatio();
            }
            break;
        case Qt::EditRole:
            if (node->estimate()) {
                return node->estimate()->pessimisticRatio();
            }
            break;
        case Qt::ToolTipRole:
            if (node->type() == Node::Type_Task) {
                Duration::Unit unit = node->estimate()->unit();
                QString s = QLocale().toString(node->estimate()->pessimisticEstimate(), 'f', m_prec) +  Duration::unitToString(unit, true);
                Estimate::Type t = node->estimate()->type();
                if (node->constraint() == Node::FixedInterval && t == Estimate::Type_Duration) {
                    s = xi18nc("@info:tooltip", "Not applicable, constraint is Fixed Interval");
                } else if (t == Estimate::Type_Effort) {
                    s = xi18nc("@info:tooltip", "Pessimistic effort: %1", s);
                } else {
                    s = xi18nc("@info:tooltip", "Pessimistic duration: %1", s);
                }
                return s;
            }
            break;
        case Role::Minimum:
            return 0;
        case Role::Maximum:
            return INT_MAX;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::riskType(const Node *node, int role) const
{
    if (node->estimate() == nullptr) {
        return QVariant();
    }
    switch (role) {
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
            if (node->type() == Node::Type_Task) {
                return node->estimate()->risktypeToString(true);
            }
            return QString();
        case Role::EnumList:
            return Estimate::risktypeToStringList(true);
        case Qt::EditRole:
            if (node->type() == Node::Type_Task) {
                return node->estimate()->risktypeToString();
            }
            return QString();
        case Role::EnumListValue:
            return (int)node->estimate()->risktype();
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::priority(const Node *node, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            if (node->type() == Node::Type_Task || node->type() == Node::Type_Milestone) {
                return node->priority();
            }
            break;
    case Qt::ToolTipRole:
            break;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
        case Role::Minimum:
            return 0;
        case Role::Maximum:
            return QVariant(); // No limit
        default:
            break;

    }
    return QVariant();
}

QVariant NodeModel::runningAccount(const Node *node, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            if (node->type() == Node::Type_Task) {
                Account *a = node->runningAccount();
                return a == nullptr ? i18n("None") : a->name();
            }
            break;
        case Qt::ToolTipRole:
            if (node->type() == Node::Type_Task) {
                Account *a = node->runningAccount();
                return a ? xi18nc("@info:tooltip", "Account for resource cost: %1", a->name())
                         : xi18nc("@info:tooltip", "Account for resource cost");
            }
            break;
        case Role::EnumListValue:
        case Qt::EditRole: {
            Account *a = node->runningAccount();
            return a == nullptr ? 0 : (m_project->accounts().costElements().indexOf(a->name()) + 1);
        }
        case Role::EnumList: {
            QStringList lst;
            lst << i18n("None");
            lst += m_project->accounts().costElements();
            return lst;
        }
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::startupAccount(const Node *node, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            if (node->type() == Node::Type_Task  || node->type() == Node::Type_Milestone) {
                Account *a = node->startupAccount();
                //debugPlan<<node->name()<<": "<<a;
                return a == nullptr ? i18n("None") : a->name();
            }
            break;
        case Qt::ToolTipRole:
            if (node->type() == Node::Type_Task  || node->type() == Node::Type_Milestone) {
                Account *a = node->startupAccount();
                //debugPlan<<node->name()<<": "<<a;
                return a ? xi18nc("@info:tooltip", "Account for task startup cost: %1", a->name())
                         : xi18nc("@info:tooltip", "Account for task startup cost");
            }
            break;
        case Role::EnumListValue:
        case Qt::EditRole: {
            Account *a = node->startupAccount();
            return a == nullptr ? 0 : (m_project->accounts().costElements().indexOf(a->name()) + 1);
        }
        case Role::EnumList: {
            QStringList lst;
            lst << i18n("None");
            lst += m_project->accounts().costElements();
            return lst;
        }
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::startupCost(const Node *node, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
            if (node->type() == Node::Type_Task || node->type() == Node::Type_Milestone) {
                return m_project->locale()->formatMoney(node->startupCost());
            }
            break;
        case Qt::EditRole:
            return node->startupCost();
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::shutdownAccount(const Node *node, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            if (node->type() == Node::Type_Task || node->type() == Node::Type_Milestone) {
                Account *a = node->shutdownAccount();
                return a == nullptr ? i18n("None") : a->name();
            }
            break;
        case Qt::ToolTipRole:
            if (node->type() == Node::Type_Task || node->type() == Node::Type_Milestone) {
                Account *a = node->shutdownAccount();
                return a ? xi18nc("@info:tooltip", "Account for task shutdown cost: %1", a->name())
                         : xi18nc("@info:tooltip", "Account for task shutdown cost");
            }
            break;
        case Role::EnumListValue:
        case Qt::EditRole: {
            Account *a = node->shutdownAccount();
            return a == nullptr ? 0 : (m_project->accounts().costElements().indexOf(a->name()) + 1);
        }
        case Role::EnumList: {
            QStringList lst;
            lst << i18n("None");
            lst += m_project->accounts().costElements();
            return lst;
        }
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::shutdownCost(const Node *node, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
            if (node->type() == Node::Type_Task || node->type() == Node::Type_Milestone) {
                return m_project->locale()->formatMoney(node->shutdownCost());
            }
            break;
        case Qt::EditRole:
            return node->shutdownCost();
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::startTime(const Node *node, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            return QLocale().toString(node->startTime(id()), QLocale::ShortFormat);
        case Qt::ToolTipRole:
            //debugPlan<<node->name()<<", "<<role;
            return xi18nc("@info:tooltip", "Scheduled start: %1", QLocale().toString(node->startTime(id()), QLocale::LongFormat));
        case Qt::EditRole:
            return node->startTime(id());
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::endTime(const Node *node, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            return QLocale().toString(node->endTime(id()), QLocale::ShortFormat);
        case Qt::ToolTipRole:
            //debugPlan<<node->name()<<", "<<role;
            return xi18nc("@info:tooltip", "Scheduled finish: %1", QLocale().toString(node->endTime(id()), QLocale::LongFormat));
        case Qt::EditRole:
            return node->endTime(id());
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::duration(const Node *node, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            if (node->type() == Node::Type_Task) {
                Duration::Unit unit = node->estimate()->unit();
                double v = node->duration(id()).toDouble(unit);
                return QVariant(QLocale().toString(v, 'f', m_prec) +  Duration::unitToString(unit, true));
            } else if (node->type() == Node::Type_Project) {
                Duration::Unit unit = Duration::Unit_d;
                double v = node->duration(id()).toDouble(unit);
                return QVariant(QLocale().toString(v, 'f', m_prec) +  Duration::unitToString(unit, true));
            }
            break;
        case Qt::ToolTipRole:
            if (node->type() == Node::Type_Task) {
                Duration::Unit unit = node->estimate()->unit();
                double v = node->duration(id()).toDouble(unit);
                return xi18nc("@info:tooltip", "Scheduled duration: %1", QLocale().toString(v, 'f', m_prec) +  Duration::unitToString(unit, true));
            } else if (node->type() == Node::Type_Project) {
                Duration::Unit unit = Duration::Unit_d;
                double v = node->duration(id()).toDouble(unit);
                return xi18nc("@info:tooltip", "Scheduled duration: %1", QLocale().toString(v, 'f', m_prec) +  Duration::unitToString(unit, true));
            }
            break;
        case Qt::EditRole: {
            return node->duration(id()).toDouble(Duration::Unit_h);
        }
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::varianceDuration(const Node *node, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            if (node->type() == Node::Type_Task) {
                Duration::Unit unit = node->estimate()->unit();
                double v = node->variance(id(), unit);
                return QLocale().toString(v, 'f', 2);
            }
            break;
        case Qt::EditRole:
            if (node->type() == Node::Type_Task) {
                Duration::Unit unit = node->estimate()->unit();
                return node->variance(id(), unit);
            }
            return 0.0;
        case Qt::ToolTipRole:
            if (node->type() == Node::Type_Task) {
                Duration::Unit unit = node->estimate()->unit();
                double v = node->variance(id(), unit);
                return xi18nc("@info:tooltip", "PERT duration variance: %1", QLocale().toString(v ,'f', 2));
            }
            break;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::varianceEstimate(const Estimate *est, int role) const
{
    switch (role) {
        case Qt::DisplayRole: {
            if (est == nullptr) {
                return QVariant();
            }
            Duration::Unit unit = est->unit();
            double v = est->variance(unit);
            //debugPlan<<node->name()<<": "<<v<<" "<<unit<<" : "<<scales;
            return QLocale().toString(v, 'f', 2);
        }
        case Qt::EditRole: {
            if (est == nullptr) {
                return 0.0;
            }
            return est->variance(est->unit());
        }
        case Qt::ToolTipRole: {
            if (est == nullptr) {
                return QVariant();
            }
            Duration::Unit unit = est->unit();
            double v = est->variance(unit);
            return xi18nc("@info:tooltip", "PERT estimate variance: %1", QLocale().toString(v, 'f', 2) + Duration::unitToString(unit, true));
        }
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::optimisticDuration(const Node *node, int role) const
{
    switch (role) {
        case Qt::DisplayRole: {
            if (node->type() != Node::Type_Task) {
                return QVariant();
            }
            Duration d = node->duration(id());
            d = (d * (100 + node->estimate()->optimisticRatio())) / 100;
            Duration::Unit unit = node->estimate()->unit();
            double v = d.toDouble(unit);
                //debugPlan<<node->name()<<": "<<v<<" "<<unit<<" : "<<scales;
            return QVariant(QLocale().toString(v,'f', m_prec) +  Duration::unitToString(unit, true));
            break;
        }
        case Qt::EditRole: {
            if (node->type() != Node::Type_Task) {
                return 0.0;
            }
            Duration d = node->duration(id());
            d = (d * (100 + node->estimate()->optimisticRatio())) / 100;
            Duration::Unit unit = node->estimate()->unit();
            return d.toDouble(unit);
        }
        case Qt::ToolTipRole: {
            if (node->type() != Node::Type_Task) {
                return QVariant();
            }
            Duration d = node->duration(id());
            d = (d * (100 + node->estimate()->optimisticRatio())) / 100;
            Duration::Unit unit = node->estimate()->unit();
            double v = d.toDouble(unit);
            //debugPlan<<node->name()<<": "<<v<<" "<<unit<<" : "<<scales;
            return xi18nc("@info:tooltip", "PERT optimistic duration: %1", QLocale().toString(v, 'f', m_prec) +  Duration::unitToString(unit, true));
            break;
        }
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::optimisticEstimate(const Estimate *est, int role) const
{
    switch (role) {
        case Qt::DisplayRole: {
            if (est == nullptr) {
                return QVariant();
            }
            Duration::Unit unit = est->unit();
            return QVariant(QLocale().toString(est->optimisticEstimate(), 'f', m_prec) +  Duration::unitToString(unit, true));
            break;
        }
        case Qt::EditRole: {
            if (est == nullptr) {
                return 0.0;
            }
            return est->optimisticEstimate();
        }
        case Qt::ToolTipRole: {
            if (est == nullptr) {
                return QVariant();
            }
            Duration::Unit unit = est->unit();
            return xi18nc("@info:tooltip", "Optimistic estimate: %1", QLocale().toString(est->optimisticEstimate(), 'f', m_prec) +  Duration::unitToString(unit, true));
            break;
        }
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::pertExpected(const Estimate *est, int role) const
{
    switch (role) {
        case Qt::DisplayRole: {
            if (est == nullptr) {
                return QVariant();
            }
            Duration::Unit unit = est->unit();
            double v = Estimate::scale(est->pertExpected(), unit, est->scales());
            return QVariant(QLocale().toString(v, 'f', m_prec) +  Duration::unitToString(unit, true));
        }
        case Qt::EditRole: {
            if (est == nullptr) {
                return 0.0;
            }
            return Estimate::scale(est->pertExpected(), est->unit(), est->scales());
        }
        case Qt::ToolTipRole: {
            if (est == nullptr) {
                return QVariant();
            }
            Duration::Unit unit = est->unit();
            double v = Estimate::scale(est->pertExpected(), unit, est->scales());
            return xi18nc("@info:tooltip", "PERT expected estimate: %1", QLocale().toString(v, 'f', m_prec) +  Duration::unitToString(unit, true));
        }
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::pessimisticDuration(const Node *node, int role) const
{
    switch (role) {
        case Qt::DisplayRole: {
            if (node->type() != Node::Type_Task) {
                return QVariant();
            }
            Duration d = node->duration(id());
            d = (d * (100 + node->estimate()->pessimisticRatio())) / 100;
            Duration::Unit unit = node->estimate()->unit();
            double v = d.toDouble(unit);
            //debugPlan<<node->name()<<": "<<v<<" "<<unit<<" : "<<scales;
            return QVariant(QLocale().toString(v, 'f', m_prec) +  Duration::unitToString(unit, true));
            break;
        }
        case Qt::EditRole: {
            if (node->type() != Node::Type_Task) {
                return 0.0;
            }
            Duration d = node->duration(id());
            d = (d * (100 + node->estimate()->pessimisticRatio())) / 100;
            return d.toDouble(node->estimate()->unit());
        }
        case Qt::ToolTipRole: {
            if (node->type() != Node::Type_Task) {
                return QVariant();
            }
            Duration d = node->duration(id());
            d = (d * (100 + node->estimate()->pessimisticRatio())) / 100;
            Duration::Unit unit = node->estimate()->unit();
            double v = d.toDouble(unit);
            //debugPlan<<node->name()<<": "<<v<<" "<<unit<<" : "<<scales;
            return xi18nc("@info:tooltip", "PERT pessimistic duration: %1", QLocale().toString(v, 'f', m_prec) +  Duration::unitToString(unit, true));
            break;
        }
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::pessimisticEstimate(const Estimate *est, int role) const
{
    switch (role) {
        case Qt::DisplayRole: {
            if (est == nullptr) {
                return QVariant();
            }
            Duration::Unit unit = est->unit();
            return QVariant(QLocale().toString(est->pessimisticEstimate(), 'f', m_prec) +  Duration::unitToString(unit, true));
            break;
        }
        case Qt::EditRole: {
            if (est == nullptr) {
                return 0.0;
            }
            return est->pessimisticEstimate();
        }
        case Qt::ToolTipRole: {
            if (est == nullptr) {
                return QVariant();
            }
            Duration::Unit unit = est->unit();
            return xi18nc("@info:tooltip", "Pessimistic estimate: %1", QLocale().toString(est->pessimisticEstimate(), 'f', m_prec) +  Duration::unitToString(unit, true));
            break;
        }
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::earlyStart(const Node *node, int role) const
{
    if (! (node->type() == Node::Type_Task || node->type() == Node::Type_Milestone)) {
        return QVariant();
    }
    const Task *t = static_cast<const Task*>(node);
    switch (role) {
        case Qt::DisplayRole:
            return QLocale().toString(t->earlyStart(id()), QLocale::ShortFormat);
        case Qt::ToolTipRole:
            return QLocale().toString(t->earlyStart(id()).date(), QLocale::ShortFormat);
        case Qt::EditRole:
            return t->earlyStart(id());
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::earlyFinish(const Node *node, int role) const
{
    if (! (node->type() == Node::Type_Task || node->type() == Node::Type_Milestone)) {
        return QVariant();
    }
    const Task *t = static_cast<const Task*>(node);
    switch (role) {
        case Qt::DisplayRole:
            return QLocale().toString(t->earlyFinish(id()), QLocale::ShortFormat);
        case Qt::ToolTipRole:
            return QLocale().toString(t->earlyFinish(id()).date(), QLocale::ShortFormat);
        case Qt::EditRole:
            return t->earlyFinish(id());
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::lateStart(const Node *node, int role) const
{
    if (! (node->type() == Node::Type_Task || node->type() == Node::Type_Milestone)) {
        return QVariant();
    }
    const Task *t = static_cast<const Task*>(node);
    switch (role) {
        case Qt::DisplayRole:
            return QLocale().toString(t->lateStart(id()), QLocale::ShortFormat);
        case Qt::ToolTipRole:
            return QLocale().toString(t->lateStart(id()).date(), QLocale::ShortFormat);
        case Qt::EditRole:
            return t->lateStart(id());
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::lateFinish(const Node *node, int role) const
{
    if (! (node->type() == Node::Type_Task || node->type() == Node::Type_Milestone)) {
        return QVariant();
    }
    const Task *t = static_cast<const Task*>(node);
    switch (role) {
        case Qt::DisplayRole:
            return QLocale().toString(t->lateFinish(id()), QLocale::ShortFormat);
        case Qt::ToolTipRole:
            return QLocale().toString(t->lateFinish(id()).date(), QLocale::ShortFormat);
        case Qt::EditRole:
            return t->lateFinish(id());
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::positiveFloat(const Node *node, int role) const
{
    if (! (node->type() == Node::Type_Task || node->type() == Node::Type_Milestone)) {
        return QVariant();
    }
    const Task *t = static_cast<const Task*>(node);
    switch (role) {
        case Qt::DisplayRole:
            return t->positiveFloat(id()).toString(Duration::Format_i18nHourFraction);
        case Qt::ToolTipRole:
            return t->positiveFloat(id()).toString(Duration::Format_i18nDayTime);
        case Qt::EditRole:
            return t->positiveFloat(id()).toDouble(Duration::Unit_h);
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::freeFloat(const Node *node, int role) const
{
    if (! (node->type() == Node::Type_Task || node->type() == Node::Type_Milestone)) {
        return QVariant();
    }
    const Task *t = static_cast<const Task*>(node);
    switch (role) {
        case Qt::DisplayRole:
            return t->freeFloat(id()).toString(Duration::Format_i18nHourFraction);
        case Qt::ToolTipRole:
            return t->freeFloat(id()).toString(Duration::Format_i18nDayTime);
        case Qt::EditRole:
            return t->freeFloat(id()).toDouble(Duration::Unit_h);
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::negativeFloat(const Node *node, int role) const
{
    if (! (node->type() == Node::Type_Task || node->type() == Node::Type_Milestone)) {
        return QVariant();
    }
    const Task *t = static_cast<const Task*>(node);
    switch (role) {
        case Qt::DisplayRole:
            return t->negativeFloat(id()).toString(Duration::Format_i18nHourFraction);
        case Qt::ToolTipRole:
            return t->negativeFloat(id()).toString(Duration::Format_i18nDayTime);
        case Qt::EditRole:
            return t->negativeFloat(id()).toDouble(Duration::Unit_h);
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::startFloat(const Node *node, int role) const
{
    if (! (node->type() == Node::Type_Task || node->type() == Node::Type_Milestone)) {
        return QVariant();
    }
    const Task *t = static_cast<const Task*>(node);
    switch (role) {
        case Qt::DisplayRole:
            return t->startFloat(id()).toString(Duration::Format_i18nHourFraction);
        case Qt::ToolTipRole:
            return t->startFloat(id()).toString(Duration::Format_i18nDayTime);
        case Qt::EditRole:
            return t->startFloat(id()).toDouble(Duration::Unit_h);
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::finishFloat(const Node *node, int role) const
{
    if (! (node->type() == Node::Type_Task || node->type() == Node::Type_Milestone)) {
        return QVariant();
    }
    const Task *t = static_cast<const Task*>(node);
    switch (role) {
        case Qt::DisplayRole:
            return t->finishFloat(id()).toString(Duration::Format_i18nHourFraction);
        case Qt::ToolTipRole:
            return t->finishFloat(id()).toString(Duration::Format_i18nDayTime);
        case Qt::EditRole:
            return t->finishFloat(id()).toDouble(Duration::Unit_h);
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::assignedResources(const Node *node, int role) const
{
    if (node->type() != Node::Type_Task) {
        return QVariant();
    }
    switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            return node->assignedNameList(id()).join(QStringLiteral(","));
        case Qt::ToolTipRole: {
            QStringList lst = node->assignedNameList(id());
            if (! lst.isEmpty()) {
                return xi18nc("@info:tooltip 1=list of resources", "Assigned resources:<nl/>%1", node->assignedNameList(id()).join(QStringLiteral(", ")));
            }
            break;
        }
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}


QVariant NodeModel::completed(const Node *node, int role) const
{
    if (! (node->type() == Node::Type_Task || node->type() == Node::Type_Milestone)) {
        return QVariant();
    }
    const Task *t = static_cast<const Task*>(node);
    switch (role) {
        case Qt::DisplayRole:
            return t->completion().percentFinished();
        case Qt::EditRole:
            return t->completion().percentFinished();
        case Qt::ToolTipRole:
            if (t->type() == Node::Type_Task) {
                if (t->completion().isFinished()) {
                    return xi18nc("@info:tooltip", "Task finished at %1", QLocale().system().toString(t->completion().finishTime()));
                }
                return xi18nc("@info:tooltip", "Task is %1% completed", t->completion().percentFinished());
            }
            if (t->type() == Node::Type_Milestone && t->completion().isFinished()) {
                return xi18nc("@info:tooltip", "Milestone finished at %1", QLocale().system().toString(t->completion().finishTime()));
            }
            break;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::status(const Node *node, int role) const
{
    if (! (node->type() == Node::Type_Task || node->type() == Node::Type_Milestone)) {
        return QVariant();
    }
    const Task *t = static_cast<const Task*>(node);
    switch (role) {
        case Qt::DisplayRole: {
            int st = t->state(id());
            if (st & Node::State_NotScheduled) {
                return SchedulingState::notScheduled();
            }
            if (st & Node::State_Finished) {
                if (st & Node::State_FinishedLate) {
                    return i18n("Finished late");
                }
                if (st & Node::State_FinishedEarly) {
                    return i18n("Finished early");
                }
                return i18n("Finished");
            }
            if (st & Node::State_Running) {
                if (st & Node::State_Late) {
                    return i18n("Running late");
                }
                return i18n("Running");
            }
            if (st & Node::State_Started) {
                if (st & Node::State_StartedLate) {
                    return i18n("Started late");
                }
                if (st & Node::State_StartedEarly) {
                    return i18n("Started early");
                }
                if (st & Node::State_Late) {
                    return i18n("Running late");
                }
                return i18n("Started");
            }
            if (st & Node::State_ReadyToStart) {
                if (st & Node::State_Late) {
                    return i18n("Not started");
                }
                return i18n("Can start");
            }
            if (st & Node::State_NotReadyToStart) {
                if (st & Node::State_Late) {
                    return i18n("Delayed");
                }
                return i18n("Cannot start");
            }
            return i18n("Not started");
            break;
        }
        case Qt::ToolTipRole: {
            int st = t->state(id());
            if (st & Node::State_NotScheduled) {
                return SchedulingState::notScheduled();
            }
            if (st & Node::State_Finished) {
                if (st & Node::State_FinishedLate) {
                    Duration d = t->completion().finishTime() - t->endTime(id());
                    return xi18nc("@info:tooltip", "Finished %1 late", d.toString(Duration::Format_i18nDay));
                }
                if (st & Node::State_FinishedEarly) {
                    Duration d = t->endTime(id()) - t->completion().finishTime();
                    return xi18nc("@info:tooltip", "Finished %1 early", d.toString(Duration::Format_i18nDay));
                }
                return xi18nc("@info:tooltip", "Finished");
            }
            if (st & Node::State_Started) {
                if (st & Node::State_StartedLate) {
                    Duration d = t->completion().startTime() - t->startTime(id());
                    return xi18nc("@info:tooltip", "Started %1 late", d.toString(Duration::Format_i18nDay));
                }
                if (st & Node::State_StartedEarly) {
                    Duration d = t->startTime(id()) - t->completion().startTime();
                    return xi18nc("@info:tooltip", "Started %1 early", d.toString(Duration::Format_i18nDay));
                }
                return xi18nc("@info:tooltip", "Started");
            }
            if (st & Node::State_Running) {
                return xi18nc("@info:tooltip", "Running");
            }
            if (st & Node::State_ReadyToStart) {
                return xi18nc("@info:tooltip", "Can start");
            }
            if (st & Node::State_NotReadyToStart) {
                return xi18nc("@info:tooltip", "Cannot start");
            }
            return xi18nc("@info:tooltip", "Not started");
            break;
        }
        case Qt::EditRole:
            return t->state(id());
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::startedTime(const Node *node, int role) const
{
    if (! (node->type() == Node::Type_Task || node->type() == Node::Type_Milestone)) {
        return QVariant();
    }
    const Task *t = static_cast<const Task*>(node);
    switch (role) {
        case Qt::DisplayRole:
            if (t->completion().isStarted()) {
                return QLocale().toString(t->completion().startTime(), QLocale::ShortFormat);
            }
            break;
        case Qt::ToolTipRole:
            if (t->completion().isStarted()) {
                return xi18nc("@info:tooltip", "Actual start: %1", QLocale().toString(t->completion().startTime().date(), QLocale::LongFormat));
            }
            break;
        case Qt::EditRole:
            if (t->completion().isStarted()) {
                return t->completion().startTime();
            }
            return QDateTime::currentDateTime();
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::isStarted(const Node *node, int role) const
{
    if (! (node->type() == Node::Type_Task || node->type() == Node::Type_Milestone)) {
        return QVariant();
    }
    const Task *t = static_cast<const Task*>(node);
    switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            return t->completion().isStarted();
        case Qt::ToolTipRole:
            if (t->completion().isStarted()) {
                return xi18nc("@info:tooltip", "The task started at: %1", QLocale().toString(t->completion().startTime().date(), QLocale::LongFormat));
            }
            return xi18nc("@info:tooltip", "The task is not started");
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::finishedTime(const Node *node, int role) const
{
    if (! (node->type() == Node::Type_Task || node->type() == Node::Type_Milestone)) {
        return QVariant();
    }
    const Task *t = static_cast<const Task*>(node);
    switch (role) {
        case Qt::DisplayRole:
            if (t->completion().isFinished()) {
                return QLocale().toString(t->completion().finishTime(), QLocale::ShortFormat);
            }
            break;
        case Qt::ToolTipRole:
            if (t->completion().isFinished()) {
                return xi18nc("@info:tooltip", "Actual finish: %1", QLocale().toString(t->completion().finishTime(), QLocale::LongFormat));
            }
            break;
        case Qt::EditRole:
            if (t->completion().isFinished()) {
                return t->completion().finishTime();
            }
            break;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::isFinished(const Node *node, int role) const
{
    if (! (node->type() == Node::Type_Task || node->type() == Node::Type_Milestone)) {
        return QVariant();
    }
    const Task *t = static_cast<const Task*>(node);
    switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            return t->completion().isFinished();
        case Qt::ToolTipRole:
            if (t->completion().isFinished()) {
                return xi18nc("@info:tooltip", "The task finished at: %1", QLocale().toString(t->completion().finishTime().date(), QLocale::LongFormat));
            }
            return xi18nc("@info:tooltip", "The task is not finished");
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::plannedEffortTo(const Node *node, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            return node->plannedEffortTo(m_now, id()).format();
        case Qt::ToolTipRole:
            return xi18nc("@info:tooltip", "Planned effort until %1: %2", QLocale().toString(m_now, QLocale::ShortFormat), node->plannedEffortTo(m_now, id()).toString(Duration::Format_i18nHour));
        case Qt::EditRole:
            return node->plannedEffortTo(m_now, id()).toDouble(Duration::Unit_h);
        case Role::DurationUnit:
            return static_cast<int>(Duration::Unit_h);
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::actualEffortTo(const Node *node, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            return node->actualEffortTo(m_now).format();
        case Qt::ToolTipRole:
            //debugPlan<<m_now<<node;
            return xi18nc("@info:tooltip", "Actual effort used up to %1: %2", QLocale().toString(m_now, QLocale::ShortFormat), node->actualEffortTo(m_now).toString(Duration::Format_i18nHour));
        case Qt::EditRole:
            return node->actualEffortTo(m_now).toDouble(Duration::Unit_h);
        case Role::DurationUnit:
            return static_cast<int>(Duration::Unit_h);
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::remainingEffort(const Node *node, int role) const
{
    switch (role) {
        case Qt::DisplayRole: {
            const Task *t = dynamic_cast<const Task*>(node);
            if (t) {
                return t->completion().remainingEffort().format();
            }
            break;
        }
        case Qt::ToolTipRole: {
            const Task *t = dynamic_cast<const Task*>(node);
            if (t) {
                return xi18nc("@info:tooltip", "Remaining effort: %1", t->completion().remainingEffort().toString(Duration::Format_i18nHour));
            }
            break;
        }
        case Qt::EditRole: {
            const Task *t = dynamic_cast<const Task*>(node);
            if (t == nullptr) {
                return QVariant();
            }
            return t->completion().remainingEffort().toDouble(Duration::Unit_h);
        }
        case Role::DurationUnit:
            return static_cast<int>(Duration::Unit_h);
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::plannedCostTo(const Node *node, int role) const
{
    Locale *l = m_project->locale();
    switch (role) {
        case Qt::DisplayRole:
            return l->formatMoney(node->plannedCostTo(m_now, id()));
        case Qt::ToolTipRole:
            return xi18nc("@info:tooltip", "Planned cost until %1: %2", QLocale().toString(m_now, QLocale::ShortFormat), l->formatMoney(node->plannedCostTo(m_now, id())));
        case Qt::EditRole:
            return node->plannedCostTo(m_now);
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::actualCostTo(const Node *node, int role) const
{
    Locale *l = m_project->locale();
    switch (role) {
        case Qt::DisplayRole:
            return l->formatMoney(node->actualCostTo(id(), m_now).cost());
        case Qt::ToolTipRole:
            return xi18nc("@info:tooltip", "Actual cost until %1: %2", QLocale().toString(m_now, QLocale::ShortFormat), l->formatMoney(node->actualCostTo(id(), m_now).cost()));
        case Qt::EditRole:
            return node->actualCostTo(id(), m_now).cost();
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::note(const Node *node, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
        case Qt::ToolTipRole:
            if (node->type() == Node::Type_Task) {
                Node *n = const_cast<Node*>(node);
                return static_cast<Task*>(n)->completion().note();
            }
            break;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::nodeSchedulingStatus(const Node *node, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            return node->schedulingStatus(id(), true).value(0);
        case Qt::EditRole:
            return node->schedulingStatus(id(), false).value(0);
        case Qt::ToolTipRole:
            return node->schedulingStatus(id(), true).join(QStringLiteral("\n"));
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::resourceIsMissing(const Node *node, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            if (node->resourceError(id())) {
                return i18n("Error");
            }
            break;
        case Qt::EditRole:
            return node->resourceError(id());
        case Qt::ToolTipRole:
            if (node->resourceError(id())) {
                return xi18nc("@info:tooltip", "Resource allocation is expected when the task estimate type is  <emphasis>Effort</emphasis>");
            }
            break;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
        case Role::Foreground: {
            if (! m_project) {
                break;
            }
            switch (node->type()) {
                case Node::Type_Task: return m_project->config().taskErrorColor();
                case Node::Type_Milestone: return m_project->config().milestoneErrorColor();
                default:
                    break;
            }
        }
    }
    return QVariant();
}

QVariant NodeModel::resourceIsOverbooked(const Node *node, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            if (node->resourceOverbooked(id())) {
                return i18n("Error");
            }
            break;
        case Qt::EditRole:
            return node->resourceOverbooked(id());
        case Qt::ToolTipRole:
            if (node->resourceOverbooked(id())) {
                return xi18nc("@info:tooltip", "A resource has been overbooked");
            }
            break;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
        case Role::Foreground: {
            if (! m_project) {
                break;
            }
            switch (node->type()) {
                case Node::Type_Task: return m_project->config().taskErrorColor();
                case Node::Type_Milestone: return m_project->config().milestoneErrorColor();
                default:
                    break;
            }
        }
    }
    return QVariant();
}

QVariant NodeModel::resourceIsNotAvailable(const Node *node, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            if (node->resourceNotAvailable(id())) {
                return i18n("Error");
            }
            break;
        case Qt::EditRole:
            return node->resourceNotAvailable(id());
        case Qt::ToolTipRole:
            if (node->resourceNotAvailable(id())) {
                return xi18nc("@info:tooltip", "No resource is available for this task");
            }
            break;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
        case Role::Foreground: {
            if (! m_project) {
                break;
            }
            switch (node->type()) {
                case Node::Type_Task: return m_project->config().taskErrorColor();
                case Node::Type_Milestone: return m_project->config().milestoneErrorColor();
                default:
                    break;
            }
        }
    }
    return QVariant();
}

QVariant NodeModel::schedulingConstraintsError(const Node *node, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            if (node->constraintError(id())) {
                return i18n("Error");
            }
            break;
        case Qt::EditRole:
            return node->constraintError(id());
        case Qt::ToolTipRole:
            if (node->constraintError(id())) {
                return xi18nc("@info:tooltip", "Failed to comply with a timing constraint");
            }
            break;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
        case Role::Foreground: {
            if (! m_project) {
                break;
            }
            switch (node->type()) {
                case Node::Type_Task: return m_project->config().taskErrorColor();
                case Node::Type_Milestone: return m_project->config().milestoneErrorColor();
                default:
                    break;
            }
        }
    }
    return QVariant();
}

QVariant NodeModel::nodeIsNotScheduled(const Node *node, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            if (node->notScheduled(id())) {
                return i18n("Error");
            }
            break;
        case Qt::EditRole:
            return node->notScheduled(id());
        case Qt::ToolTipRole:
            if (node->notScheduled(id())) {
                return xi18nc("@info:tooltip", "This task has not been scheduled");
            }
            break;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
        case Role::Foreground: {
            if (! m_project) {
                break;
            }
            switch (node->type()) {
                case Node::Type_Task: return m_project->config().taskErrorColor();
                case Node::Type_Milestone: return m_project->config().milestoneErrorColor();
                default:
                    break;
            }
        }
    }
    return QVariant();
}

QVariant NodeModel::effortNotMet(const Node *node, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            if (node->effortMetError(id())) {
                return i18n("Error");
            }
            break;
        case Qt::EditRole:
            return node->effortMetError(id());
        case Qt::ToolTipRole:
            if (node->effortMetError(id())) {
                return xi18nc("@info:tooltip", "The assigned resources cannot deliver the required estimated effort");
            }
            break;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
        case Role::Foreground: {
            if (! m_project) {
                break;
            }
            switch (node->type()) {
                case Node::Type_Task: return m_project->config().taskErrorColor();
                case Node::Type_Milestone: return m_project->config().milestoneErrorColor();
                default:
                    break;
            }
        }
    }
    return QVariant();
}

QVariant NodeModel::schedulingError(const Node *node, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            if (node->schedulingError(id())) {
                return i18n("Error");
            }
            break;
        case Qt::EditRole:
            return node->schedulingError(id());
        case Qt::ToolTipRole:
            if (node->schedulingError(id())) {
                return xi18nc("@info:tooltip", "Scheduling error");
            }
            break;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
        case Role::Foreground: {
            if (! m_project) {
                break;
            }
            switch (node->type()) {
                case Node::Type_Task: return m_project->config().taskErrorColor();
                case Node::Type_Milestone: return m_project->config().milestoneErrorColor();
                default:
                    break;
            }
        }
    }
    return QVariant();
}

QVariant NodeModel::wbsCode(const Node *node, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            return node->wbsCode();
        case Qt::ToolTipRole:
            return xi18nc("@info:tooltip", "Work breakdown structure code: %1", node->wbsCode());
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
        case SortableRole:
            return node->wbsCode(true);
    }
    return QVariant();
}

QVariant NodeModel::nodeLevel(const Node *node, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            return node->level();
        case Qt::ToolTipRole:
            return xi18nc("@info:tooltip", "Task level: %1", node->level());
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::nodeBCWS(const Node *node, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            return m_project->locale()->formatMoney(node->bcws(m_now, id()), QString(), 0);
        case Qt::EditRole:
            return node->bcws(m_now, id());
        case Qt::ToolTipRole:
            return xi18nc("@info:tooltip", "Budgeted Cost of Work Scheduled at %1: %2", QLocale().toString(m_now, QLocale::ShortFormat), m_project->locale()->formatMoney(node->bcws(m_now, id()), QString(), 0));
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::nodeBCWP(const Node *node, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            return m_project->locale()->formatMoney(node->bcwp(id()), QString(), 0);
        case Qt::EditRole:
            return node->bcwp(id());
        case Qt::ToolTipRole:
            return xi18nc("@info:tooltip", "Budgeted Cost of Work Performed at %1: %2", QLocale().toString(m_now, QLocale::ShortFormat), m_project->locale()->formatMoney(node->bcwp(id()), QString(), 0));
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::nodeACWP(const Node *node, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            return m_project->locale()->formatMoney(node->acwp(m_now, id()).cost(), QString(), 0);
        case Qt::EditRole:
            return node->acwp(m_now, id()).cost();
        case Qt::ToolTipRole:
            return xi18nc("@info:tooltip", "Actual Cost of Work Performed at %1: %2", QLocale().toString(m_now, QLocale::ShortFormat), m_project->locale()->formatMoney(node->acwp(m_now, id()).cost()));
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::nodePerformanceIndex(const Node *node, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            return QLocale().toString(node->schedulePerformanceIndex(m_now, id()), 'f', 2);
        case Qt::EditRole:
            return node->schedulePerformanceIndex(m_now, id());
        case Qt::ToolTipRole:
            return xi18nc("@info:tooltip", "Schedule Performance Index at %1: %2", m_now.toString(), QLocale().toString(node->schedulePerformanceIndex(m_now, id()), 'f', 2));
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
        case Qt::ForegroundRole:
            return QColor(node->schedulePerformanceIndex(m_now, id()) < 1.0 ? Qt::red : Qt::black);
    }
    return QVariant();
}

QVariant NodeModel::nodeIsCritical(const Node *node, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            return node->isCritical(id());
        case Qt::ToolTipRole:
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
        case Role::Foreground: {
            if (! m_project) {
                break;
            }
            switch (node->type()) {
                case Node::Type_Task: return m_project->config().taskNormalColor();
                case Node::Type_Milestone: return m_project->config().milestoneNormalColor();
                default:
                    break;
            }
        }
    }
    return QVariant();
}

QVariant NodeModel::nodeInCriticalPath(const Node *node, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            return node->inCriticalPath(id());
        case Qt::ToolTipRole:
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
        case Role::Foreground: {
            if (! m_project) {
                break;
            }
            switch (node->type()) {
                case Node::Type_Task: return m_project->config().taskNormalColor();
                case Node::Type_Milestone: return m_project->config().milestoneNormalColor();
                default:
                    break;
            }
        }
    }
    return QVariant();
}

QVariant NodeModel::wpOwnerName(const Node *node, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole: {
            const Task *t = dynamic_cast<const Task*>(node);
            if (t == nullptr) {
                return QVariant();
            }
            if (t->wpTransmitionStatus() == WorkPackage::TS_None) {
                return xi18nc("Not available", "NA");
            }
            return t->wpOwnerName();
        }
        case Qt::ToolTipRole: {
            const Task *task = dynamic_cast<const Task*>(node);
            if (task == nullptr) {
                return QVariant();
            }
            int sts = task->wpTransmitionStatus();
            QString t = wpTransmitionTime(node, Qt::DisplayRole).toString();
            if (sts == WorkPackage::TS_Send) {
                return xi18nc("@info:tooltip", "Latest work package sent to %1 at %2", static_cast<const Task*>(node)->wpOwnerName(), t);
            }
            if (sts == WorkPackage::TS_Receive) {
                return xi18nc("@info:tooltip", "Latest work package received from %1 at %2", static_cast<const Task*>(node)->wpOwnerName(), t);
            }
            return xi18nc("@info:tooltip", "Not available");
        }
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::wpTransmitionStatus(const Node *node, int role) const
{
    switch (role) {
        case Qt::DisplayRole: {
            const Task *t = dynamic_cast<const Task*>(node);
            if (t == nullptr) {
                return QVariant();
            }
            if (t->wpTransmitionStatus() == WorkPackage::TS_None) {
                return xi18nc("Not available", "NA");
            }
            return WorkPackage::transmitionStatusToString(t->wpTransmitionStatus(), true);
        }
        case Qt::EditRole: {
            const Task *t = dynamic_cast<const Task*>(node);
            if (t == nullptr) {
                return QVariant();
            }
            return WorkPackage::transmitionStatusToString(t->wpTransmitionStatus(), false);
        }
        case Qt::ToolTipRole:
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::wpTransmitionTime(const Node *node, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole: {
            const Task *t = dynamic_cast<const Task*>(node);
            if (t == nullptr) {
                return QVariant();
            }
            if (t->wpTransmitionStatus() == WorkPackage::TS_None) {
                return xi18nc("Not available", "NA");
            }
            return QLocale().toString(t->wpTransmitionTime(), QLocale::ShortFormat);
        }
        case Qt::ToolTipRole: {
            const Task *task = dynamic_cast<const Task*>(node);
            if (task == nullptr) {
                return QVariant();
            }
            int sts = task->wpTransmitionStatus();
            QString t = wpTransmitionTime(node, Qt::DisplayRole).toString();
            if (sts == WorkPackage::TS_Send) {
                return xi18nc("@info:tooltip", "Latest work package sent: %1", t);
            }
            if (sts == WorkPackage::TS_Receive) {
                return xi18nc("@info:tooltip", "Latest work package received: %1", t);
            }
            return xi18nc("@info:tooltip", "Not available");
        }
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant NodeModel::data(const Node *n, int property, int role) const
{
    QVariant result;
    switch (property) {
        // Edited by user
        case NodeName: result = name(n, role); break;
        case NodeType: result = type(n, role); break;
        case NodeResponsible: result = leader(n, role); break;
        case NodeAllocation: result = allocation(n, role); break;
        case NodeEstimateType: result = estimateType(n, role); break;
        case NodeEstimateCalendar: result = estimateCalendar(n, role); break;
        case NodeEstimate: result = estimate(n, role); break;
        case NodeOptimisticRatio: result = optimisticRatio(n, role); break;
        case NodePessimisticRatio: result = pessimisticRatio(n, role); break;
        case NodeRisk: result = riskType(n, role); break;
        case NodePriority: result = priority(n, role); break;
        case NodeConstraint: result = constraint(n, role); break;
        case NodeConstraintStart: result = constraintStartTime(n, role); break;
        case NodeConstraintEnd: result = constraintEndTime(n, role); break;
        case NodeRunningAccount: result = runningAccount(n, role); break;
        case NodeStartupAccount: result = startupAccount(n, role); break;
        case NodeStartupCost: result = startupCost(n, role); break;
        case NodeShutdownAccount: result = shutdownAccount(n, role); break;
        case NodeShutdownCost: result = shutdownCost(n, role); break;
        case NodeDescription: result = description(n, role); break;

        // Based on edited values
        case NodeExpected: result = pertExpected(n->estimate(), role); break;
        case NodeVarianceEstimate: result = varianceEstimate(n->estimate(), role); break;
        case NodeOptimistic: result = optimisticEstimate(n->estimate(), role); break;
        case NodePessimistic: result = pessimisticEstimate(n->estimate(), role); break;

        // After scheduling
        case NodeStartTime: result = startTime(n, role); break;
        case NodeEndTime: result = endTime(n, role); break;
        case NodeEarlyStart: result = earlyStart(n, role); break;
        case NodeEarlyFinish: result = earlyFinish(n, role); break;
        case NodeLateStart: result = lateStart(n, role); break;
        case NodeLateFinish: result = lateFinish(n, role); break;
        case NodePositiveFloat: result = positiveFloat(n, role); break;
        case NodeFreeFloat: result = freeFloat(n, role); break;
        case NodeNegativeFloat: result = negativeFloat(n, role); break;
        case NodeStartFloat: result = startFloat(n, role); break;
        case NodeFinishFloat: result = finishFloat(n, role); break;
        case NodeAssignments: result = assignedResources(n, role); break;

        // Based on scheduled values
        case NodeDuration: result = duration(n, role); break;
        case NodeVarianceDuration: result = varianceDuration(n, role); break;
        case NodeOptimisticDuration: result = optimisticDuration(n, role); break;
        case NodePessimisticDuration: result = pessimisticDuration(n, role); break;

        // Completion
        case NodeStatus: result = status(n, role); break;
        case NodeCompleted: result = completed(n, role); break;
        case NodePlannedEffort: result = plannedEffortTo(n, role); break;
        case NodeActualEffort: result = actualEffortTo(n, role); break;
        case NodeRemainingEffort: result = remainingEffort(n, role); break;
        case NodePlannedCost: result = plannedCostTo(n, role); break;
        case NodeActualCost: result = actualCostTo(n, role); break;
        case NodeActualStart: result = startedTime(n, role); break;
        case NodeStarted: result = isStarted(n, role); break;
        case NodeActualFinish: result = finishedTime(n, role); break;
        case NodeFinished: result = isFinished(n, role); break;
        case NodeStatusNote: result = note(n, role); break;

        // Scheduling errors
        case NodeSchedulingStatus: result = nodeSchedulingStatus(n, role); break;
        case NodeNotScheduled: result = nodeIsNotScheduled(n, role); break;
        case NodeAssignmentMissing: result = resourceIsMissing(n, role); break;
        case NodeResourceOverbooked: result = resourceIsOverbooked(n, role); break;
        case NodeResourceUnavailable: result = resourceIsNotAvailable(n, role); break;
        case NodeConstraintsError: result = schedulingConstraintsError(n, role); break;
        case NodeEffortNotMet: result = effortNotMet(n, role); break;
        case NodeSchedulingError: result = schedulingError(n, role); break;

        case NodeWBSCode: result = wbsCode(n, role); break;
        case NodeLevel: result = nodeLevel(n, role); break;

        // Performance
        case NodeBCWS: result = nodeBCWS(n, role); break;
        case NodeBCWP: result = nodeBCWP(n, role); break;
        case NodeACWP: result = nodeACWP(n, role); break;
        case NodePerformanceIndex: result = nodePerformanceIndex(n, role); break;
        case NodeCritical: result = nodeIsCritical(n, role); break;
        case NodeCriticalPath: result = nodeInCriticalPath(n, role); break;

        case WPOwnerName: result = wpOwnerName(n, role); break;
        case WPTransmitionStatus: result = wpTransmitionStatus(n, role); break;
        case WPTransmitionTime: result = wpTransmitionTime(n, role); break;

        default:
            //debugPlan<<"Invalid property number: "<<property;
            return result;
    }
    return result;
}

int NodeModel::propertyCount() const
{
    return columnMap().keyCount();
}

KUndo2Command *NodeModel::setData(Node *node, int property, const QVariant & value, int role)
{
    switch (property) {
        case NodeModel::NodeName: return setName(node, value, role);
        case NodeModel::NodeType: return setType(node, value, role);
        case NodeModel::NodeResponsible: return setLeader(node, value, role);
        case NodeModel::NodeAllocation: return setAllocation(node, value, role);
        case NodeModel::NodeEstimateType: return setEstimateType(node, value, role);
        case NodeModel::NodeEstimateCalendar: return setEstimateCalendar(node, value, role);
        case NodeModel::NodeEstimate: return setEstimate(node, value, role);
        case NodeModel::NodeOptimisticRatio: return setOptimisticRatio(node, value, role);
        case NodeModel::NodePessimisticRatio: return setPessimisticRatio(node, value, role);
        case NodeModel::NodeRisk: return setRiskType(node, value, role);
        case NodeModel::NodePriority: return setPriority(node, value, role);
        case NodeModel::NodeConstraint: return setConstraint(node, value, role);
        case NodeModel::NodeConstraintStart: return setConstraintStartTime(node, value, role);
        case NodeModel::NodeConstraintEnd: return setConstraintEndTime(node, value, role);
        case NodeModel::NodeRunningAccount: return setRunningAccount(node, value, role);
        case NodeModel::NodeStartupAccount: return setStartupAccount(node, value, role);
        case NodeModel::NodeStartupCost: return setStartupCost(node, value, role);
        case NodeModel::NodeShutdownAccount: return setShutdownAccount(node, value, role);
        case NodeModel::NodeShutdownCost: return setShutdownCost(node, value, role);
        case NodeModel::NodeDescription: return setDescription(node, value, role);
        case NodeModel::NodeCompleted: return setCompletion(node, value, role);
        case NodeModel::NodeActualEffort: return setActualEffort(node, value, role);
        case NodeModel::NodeRemainingEffort: return setRemainingEffort(node, value, role);
        case NodeModel::NodeActualStart: return setStartedTime(node, value, role);
        case NodeModel::NodeActualFinish: return setFinishedTime(node, value, role);
        default:
            qWarning("data: invalid display value column %d", property);
            return nullptr;
    }
    return nullptr;
}

QVariant NodeModel::headerData(int section, int role)
{
    if (role == Qt::DisplayRole) {
        switch (section) {
            case NodeName: return xi18nc("@title:column", "Name");
            case NodeType: return xi18nc("@title:column", "Type");
            case NodeResponsible: return xi18nc("@title:column", "Responsible");
            case NodeAllocation: return xi18nc("@title:column", "Allocation");
            case NodeEstimateType: return xi18nc("@title:column", "Estimate Type");
            case NodeEstimateCalendar: return xi18nc("@title:column", "Calendar");
            case NodeEstimate: return xi18nc("@title:column", "Estimate");
            case NodeOptimisticRatio: return xi18nc("@title:column", "Optimistic (%)"); // Ratio
            case NodePessimisticRatio: return xi18nc("@title:column", "Pessimistic (%)"); // Ratio
            case NodeRisk: return xi18nc("@title:column", "Risk");
            case NodePriority: return xi18nc("@title:column", "Priority");
            case NodeConstraint: return xi18nc("@title:column", "Constraint");
            case NodeConstraintStart: return xi18nc("@title:column", "Constraint Start");
            case NodeConstraintEnd: return xi18nc("@title:column", "Constraint End");
            case NodeRunningAccount: return xi18nc("@title:column", "Running Account");
            case NodeStartupAccount: return xi18nc("@title:column", "Startup Account");
            case NodeStartupCost: return xi18nc("@title:column", "Startup Cost");
            case NodeShutdownAccount: return xi18nc("@title:column", "Shutdown Account");
            case NodeShutdownCost: return xi18nc("@title:column", "Shutdown Cost");
            case NodeDescription: return xi18nc("@title:column", "Description");

            // Based on edited values
            case NodeExpected: return xi18nc("@title:column", "Expected");
            case NodeVarianceEstimate: return xi18nc("@title:column", "Variance (Est)");
            case NodeOptimistic: return xi18nc("@title:column", "Optimistic");
            case NodePessimistic: return xi18nc("@title:column", "Pessimistic");

            // After scheduling
            case NodeStartTime: return xi18nc("@title:column", "Start Time");
            case NodeEndTime: return xi18nc("@title:column", "End Time");
            case NodeEarlyStart: return xi18nc("@title:column", "Early Start");
            case NodeEarlyFinish: return xi18nc("@title:column", "Early Finish");
            case NodeLateStart: return xi18nc("@title:column", "Late Start");
            case NodeLateFinish: return xi18nc("@title:column", "Late Finish");
            case NodePositiveFloat: return xi18nc("@title:column", "Positive Float");
            case NodeFreeFloat: return xi18nc("@title:column", "Free Float");
            case NodeNegativeFloat: return xi18nc("@title:column", "Negative Float");
            case NodeStartFloat: return xi18nc("@title:column", "Start Float");
            case NodeFinishFloat: return xi18nc("@title:column", "Finish Float");
            case NodeAssignments: return xi18nc("@title:column", "Assignments");

            // Based on scheduled values
            case NodeDuration: return xi18nc("@title:column", "Duration");
            case NodeVarianceDuration: return xi18nc("@title:column", "Variance (Dur)");
            case NodeOptimisticDuration: return xi18nc("@title:column", "Optimistic (Dur)");
            case NodePessimisticDuration: return xi18nc("@title:column", "Pessimistic (Dur)");

            // Completion
            case NodeStatus: return xi18nc("@title:column", "Status");
            // xgettext: no-c-format
            case NodeCompleted: return xi18nc("@title:column", "% Completed");
            case NodePlannedEffort: return xi18nc("@title:column", "Planned Effort");
            case NodeActualEffort: return xi18nc("@title:column", "Actual Effort");
            case NodeRemainingEffort: return xi18nc("@title:column", "Remaining Effort");
            case NodePlannedCost: return xi18nc("@title:column", "Planned Cost");
            case NodeActualCost: return xi18nc("@title:column", "Actual Cost");
            case NodeActualStart: return xi18nc("@title:column", "Actual Start");
            case NodeStarted: return xi18nc("@title:column", "Started");
            case NodeActualFinish: return xi18nc("@title:column", "Actual Finish");
            case NodeFinished: return xi18nc("@title:column", "Finished");
            case NodeStatusNote: return xi18nc("@title:column", "Status Note");

            // Scheduling errors
            case NodeSchedulingStatus: return xi18nc("@title:column", "Scheduling Status");
            case NodeNotScheduled: return xi18nc("@title:column", "Not Scheduled");
            case NodeAssignmentMissing: return xi18nc("@title:column", "Assignment Missing");
            case NodeResourceOverbooked: return xi18nc("@title:column", "Resource Overbooked");
            case NodeResourceUnavailable: return xi18nc("@title:column", "Resource Unavailable");
            case NodeConstraintsError: return xi18nc("@title:column", "Constraints Error");
            case NodeEffortNotMet: return xi18nc("@title:column", "Effort Not Met");
            case NodeSchedulingError: return xi18nc("@title:column", "Scheduling Error");

            case NodeWBSCode: return xi18nc("@title:column", "WBS Code");
            case NodeLevel: return xi18nc("@title:column Node level", "Level");

            // Performance
            case NodeBCWS: return xi18nc("@title:column Budgeted Cost of Work Scheduled", "BCWS");
            case NodeBCWP: return xi18nc("@title:column Budgeted Cost of Work Performed", "BCWP");
            case NodeACWP: return xi18nc("@title:column Actual Cost of Work Performed", "ACWP");
            case NodePerformanceIndex: return xi18nc("@title:column Schedule Performance Index", "SPI");
            case NodeCritical: return xi18nc("@title:column", "Critical");
            case NodeCriticalPath: return xi18nc("@title:column", "Critical Path");

            // Work package handling
            case WPOwnerName: return xi18nc("@title:column", "Owner");
            case WPTransmitionStatus: return xi18nc("@title:column", "Status");
            case WPTransmitionTime: return xi18nc("@title:column", "Time");

            default: return QVariant();
        }
    }
    if (role == Qt::EditRole) {
        switch (section) {
            case NodeName: return QStringLiteral("Name");
            case NodeType: return QStringLiteral("Type");
            case NodeResponsible: return QStringLiteral("Responsible");
            case NodeAllocation: return QStringLiteral("Allocation");
            case NodeEstimateType: return QStringLiteral("Estimate Type");
            case NodeEstimateCalendar: return QStringLiteral("Calendar");
            case NodeEstimate: return QStringLiteral("Estimate");
            case NodeOptimisticRatio: return QStringLiteral("Optimistic (%)"); // Ratio
            case NodePessimisticRatio: return QStringLiteral("Pessimistic (%)"); // Ratio
            case NodeRisk: return QStringLiteral("Risk");
            case NodePriority: return QStringLiteral("Priority");
            case NodeConstraint: return QStringLiteral("Constraint");
            case NodeConstraintStart: return QStringLiteral("Constraint Start");
            case NodeConstraintEnd: return QStringLiteral("Constraint End");
            case NodeRunningAccount: return QStringLiteral("Running Account");
            case NodeStartupAccount: return QStringLiteral("Startup Account");
            case NodeStartupCost: return QStringLiteral("Startup Cost");
            case NodeShutdownAccount: return QStringLiteral("Shutdown Account");
            case NodeShutdownCost: return QStringLiteral("Shutdown Cost");
            case NodeDescription: return QStringLiteral("Description");

            // Based on edited values
            case NodeExpected: return QStringLiteral("Expected");
            case NodeVarianceEstimate: return QStringLiteral("Variance (Est)");
            case NodeOptimistic: return QStringLiteral("Optimistic");
            case NodePessimistic: return QStringLiteral("Pessimistic");

            // After scheduling
            case NodeStartTime: return QStringLiteral("Start Time");
            case NodeEndTime: return QStringLiteral("End Time");
            case NodeEarlyStart: return QStringLiteral("Early Start");
            case NodeEarlyFinish: return QStringLiteral("Early Finish");
            case NodeLateStart: return QStringLiteral("Late Start");
            case NodeLateFinish: return QStringLiteral("Late Finish");
            case NodePositiveFloat: return QStringLiteral("Positive Float");
            case NodeFreeFloat: return QStringLiteral("Free Float");
            case NodeNegativeFloat: return QStringLiteral("Negative Float");
            case NodeStartFloat: return QStringLiteral("Start Float");
            case NodeFinishFloat: return QStringLiteral("Finish Float");
            case NodeAssignments: return QStringLiteral("Assignments");

            // Based on scheduled values
            case NodeDuration: return QStringLiteral("Duration");
            case NodeVarianceDuration: return QStringLiteral("Variance (Dur)");
            case NodeOptimisticDuration: return QStringLiteral("Optimistic (Dur)");
            case NodePessimisticDuration: return QStringLiteral("Pessimistic (Dur)");

            // Completion
            case NodeStatus: return QStringLiteral("Status");
            // xgettext: no-c-format
            case NodeCompleted: return QStringLiteral("% Completed");
            case NodePlannedEffort: return QStringLiteral("Planned Effort");
            case NodeActualEffort: return QStringLiteral("Actual Effort");
            case NodeRemainingEffort: return QStringLiteral("Remaining Effort");
            case NodePlannedCost: return QStringLiteral("Planned Cost");
            case NodeActualCost: return QStringLiteral("Actual Cost");
            case NodeActualStart: return QStringLiteral("Actual Start");
            case NodeStarted: return QStringLiteral("Started");
            case NodeActualFinish: return QStringLiteral("Actual Finish");
            case NodeFinished: return QStringLiteral("Finished");
            case NodeStatusNote: return QStringLiteral("Status Note");

            // Scheduling errors
            case NodeSchedulingStatus: return QStringLiteral("Scheduling Status");
            case NodeNotScheduled: return QStringLiteral("Not Scheduled");
            case NodeAssignmentMissing: return QStringLiteral("Assignment Missing");
            case NodeResourceOverbooked: return QStringLiteral("Resource Overbooked");
            case NodeResourceUnavailable: return QStringLiteral("Resource Unavailable");
            case NodeConstraintsError: return QStringLiteral("Constraints Error");
            case NodeEffortNotMet: return QStringLiteral("Effort Not Met");
            case NodeSchedulingError: return QStringLiteral("Scheduling Error");

            case NodeWBSCode: return QStringLiteral("WBS Code");
            case NodeLevel: return QStringLiteral("Level");

            // Performance
            case NodeBCWS: return QStringLiteral("BCWS");
            case NodeBCWP: return QStringLiteral("BCWP");
            case NodeACWP: return QStringLiteral("ACWP");
            case NodePerformanceIndex: return QStringLiteral("SPI");
            case NodeCritical: return QStringLiteral("Critical");
            case NodeCriticalPath: return QStringLiteral("Critical Path");

            // Work package handling
            case WPOwnerName: return QStringLiteral("Owner");
            case WPTransmitionStatus: return QStringLiteral("Status");
            case WPTransmitionTime: return QStringLiteral("Time");

            default: return QVariant();
        }
    }
    if (role == Qt::ToolTipRole) {
        switch (section) {
            case NodeName: return ToolTip::nodeName();
            case NodeType: return ToolTip::nodeType();
            case NodeResponsible: return ToolTip::nodeResponsible();
            case NodeAllocation: return ToolTip::allocation();
            case NodeEstimateType: return ToolTip::estimateType();
            case NodeEstimateCalendar: return ToolTip::estimateCalendar();
            case NodeEstimate: return ToolTip::estimate();
            case NodeOptimisticRatio: return ToolTip::optimisticRatio();
            case NodePessimisticRatio: return ToolTip::pessimisticRatio();
            case NodeRisk: return ToolTip::riskType();
            case NodePriority: return ToolTip::nodePriority();
            case NodeConstraint: return ToolTip::nodeConstraint();
            case NodeConstraintStart: return ToolTip::nodeConstraintStart();
            case NodeConstraintEnd: return ToolTip::nodeConstraintEnd();
            case NodeRunningAccount: return ToolTip::nodeRunningAccount();
            case NodeStartupAccount: return ToolTip::nodeStartupAccount();
            case NodeStartupCost: return ToolTip::nodeStartupCost();
            case NodeShutdownAccount: return ToolTip::nodeShutdownAccount();
            case NodeShutdownCost: return ToolTip::nodeShutdownCost();
            case NodeDescription: return ToolTip::nodeDescription();

            // Based on edited values
            case NodeExpected: return ToolTip::estimateExpected();
            case NodeVarianceEstimate: return ToolTip::estimateVariance();
            case NodeOptimistic: return ToolTip::estimateOptimistic();
            case NodePessimistic: return ToolTip::estimatePessimistic();

            // After scheduling
            case NodeStartTime: return ToolTip::nodeStartTime();
            case NodeEndTime: return ToolTip::nodeEndTime();
            case NodeEarlyStart: return ToolTip::nodeEarlyStart();
            case NodeEarlyFinish: return ToolTip::nodeEarlyFinish();
            case NodeLateStart: return ToolTip::nodeLateStart();
            case NodeLateFinish: return ToolTip::nodeLateFinish();
            case NodePositiveFloat: return ToolTip::nodePositiveFloat();
            case NodeFreeFloat: return ToolTip::nodeFreeFloat();
            case NodeNegativeFloat: return ToolTip::nodeNegativeFloat();
            case NodeStartFloat: return ToolTip::nodeStartFloat();
            case NodeFinishFloat: return ToolTip::nodeFinishFloat();
            case NodeAssignments: return ToolTip::nodeAssignment();

            // Based on scheduled values
            case NodeDuration: return ToolTip::nodeDuration();
            case NodeVarianceDuration: return ToolTip::nodeVarianceDuration();
            case NodeOptimisticDuration: return ToolTip::nodeOptimisticDuration();
            case NodePessimisticDuration: return ToolTip::nodePessimisticDuration();

            // Completion
            case NodeStatus: return ToolTip::nodeStatus();
            case NodeCompleted: return ToolTip::nodeCompletion();
            case NodePlannedEffort: return ToolTip::nodePlannedEffortTo();
            case NodeActualEffort: return ToolTip::nodeActualEffortTo();
            case NodeRemainingEffort: return ToolTip::nodeRemainingEffort();
            case NodePlannedCost: return ToolTip::nodePlannedCostTo();
            case NodeActualCost: return ToolTip::nodeActualCostTo();
            case NodeActualStart: return ToolTip::completionStartedTime();
            case NodeStarted: return ToolTip::completionStarted();
            case NodeActualFinish: return ToolTip::completionFinishedTime();
            case NodeFinished: return ToolTip::completionFinished();
            case NodeStatusNote: return ToolTip::completionStatusNote();

            // Scheduling errors
            case NodeSchedulingStatus: return ToolTip::nodeSchedulingStatus();
            case NodeNotScheduled: return ToolTip::nodeNotScheduled();
            case NodeAssignmentMissing: return ToolTip::nodeAssignmentMissing();
            case NodeResourceOverbooked: return ToolTip::nodeResourceOverbooked();
            case NodeResourceUnavailable: return ToolTip::nodeResourceUnavailable();
            case NodeConstraintsError: return ToolTip::nodeConstraintsError();
            case NodeEffortNotMet: return ToolTip::nodeEffortNotMet();
            case NodeSchedulingError: return ToolTip::nodeSchedulingError();

            case NodeWBSCode: return ToolTip::nodeWBS();
            case NodeLevel: return ToolTip::nodeLevel();

            // Performance
            case NodeBCWS: return ToolTip::nodeBCWS();
            case NodeBCWP: return ToolTip::nodeBCWP();
            case NodeACWP: return ToolTip::nodeACWP();
            case NodePerformanceIndex: return ToolTip::nodePerformanceIndex();

            // Work package handling FIXME
            case WPOwnerName: return xi18nc("@info:tooltip", "Work package owner");
            case WPTransmitionStatus: return xi18nc("@info:tooltip", "Work package status");
            case WPTransmitionTime: return xi18nc("@info:tooltip", "Work package send/receive time");

            default: return QVariant();
        }
    }
    if (role == Qt::TextAlignmentRole) {
        switch (section) {
            case NodeName:
            case NodeType:
            case NodeResponsible:
            case NodeAllocation:
            case NodeEstimateType:
            case NodeEstimateCalendar:
                return (int)(Qt::AlignLeft|Qt::AlignVCenter);
            case NodeEstimate:
            case NodeOptimisticRatio:
            case NodePessimisticRatio:
            case NodePriority:
                return (int)(Qt::AlignRight|Qt::AlignVCenter); // number
            case NodeRisk:
            case NodeConstraint:
                return (int)(Qt::AlignLeft|Qt::AlignVCenter);
            case NodeConstraintStart:
            case NodeConstraintEnd:
            case NodeRunningAccount:
            case NodeStartupAccount:
                return (int)(Qt::AlignLeft|Qt::AlignVCenter);
            case NodeStartupCost:
                return (int)(Qt::AlignRight|Qt::AlignVCenter); // number
            case NodeShutdownAccount:
                return (int)(Qt::AlignLeft|Qt::AlignVCenter);
            case NodeShutdownCost:
                return (int)(Qt::AlignRight|Qt::AlignVCenter); // number
            case NodeDescription:
                return (int)(Qt::AlignLeft|Qt::AlignVCenter);

            // Based on edited values
            case NodeExpected:
            case NodeVarianceEstimate:
            case NodeOptimistic:
            case NodePessimistic:
                return (int)(Qt::AlignRight|Qt::AlignVCenter); // number

            // After scheduling
            case NodeStartTime:
            case NodeEndTime:
            case NodeEarlyStart:
            case NodeEarlyFinish:
            case NodeLateStart:
            case NodeLateFinish:
                return (int)(Qt::AlignLeft|Qt::AlignVCenter);
            case NodePositiveFloat:
            case NodeFreeFloat:
            case NodeNegativeFloat:
            case NodeStartFloat:
            case NodeFinishFloat:
                return (int)(Qt::AlignRight|Qt::AlignVCenter); // number
            case NodeAssignments:
                return (int)(Qt::AlignLeft|Qt::AlignVCenter);

            // Based on scheduled values
            case NodeDuration:
            case NodeVarianceDuration:
            case NodeOptimisticDuration:
            case NodePessimisticDuration:
                return (int)(Qt::AlignRight|Qt::AlignVCenter); // number

            // Completion
            case NodeStatus:
                return (int)(Qt::AlignLeft|Qt::AlignVCenter);
            case NodeCompleted:
                return (int)(Qt::AlignLeft|Qt::AlignVCenter); // special, presented as a bar
            case NodePlannedEffort:
            case NodeActualEffort:
            case NodeRemainingEffort:
            case NodePlannedCost:
            case NodeActualCost:
                return (int)(Qt::AlignRight|Qt::AlignVCenter); // number
            case NodeActualStart:
            case NodeStarted:
            case NodeActualFinish:
            case NodeFinished:
            case NodeStatusNote:
                return (int)(Qt::AlignLeft|Qt::AlignVCenter);

            // Scheduling errors
            case NodeSchedulingStatus:
            case NodeNotScheduled:
            case NodeAssignmentMissing:
            case NodeResourceOverbooked:
            case NodeResourceUnavailable:
            case NodeConstraintsError:
            case NodeEffortNotMet:
            case NodeSchedulingError:
                return (int)(Qt::AlignLeft|Qt::AlignVCenter);

            case NodeWBSCode:
                return (int)(Qt::AlignLeft|Qt::AlignVCenter);
            case NodeLevel:
                return (int)(Qt::AlignRight|Qt::AlignVCenter); // number

            // Performance
            case NodeBCWS:
            case NodeBCWP:
            case NodeACWP:
            case NodePerformanceIndex:
                return (int)(Qt::AlignRight|Qt::AlignVCenter); // number
            case NodeCritical:
            case NodeCriticalPath:
                return (int)(Qt::AlignLeft|Qt::AlignVCenter);

            case WPOwnerName:
            case WPTransmitionStatus:
            case WPTransmitionTime:
                return (int)(Qt::AlignLeft|Qt::AlignVCenter);
            default:
                return QVariant();
        }
    }
    if (role == Qt::WhatsThisRole) {
        switch (section) {
            case NodeNegativeFloat: return WhatsThis::nodeNegativeFloat();

            default: return QVariant();
        }
    }
    return QVariant();
}

KUndo2Command *NodeModel::setName(Node *node, const QVariant &value, int role)
{
    switch (role) {
        case Qt::EditRole: {
            if (value.toString() == node->name()) {
                return nullptr;
            }
            KUndo2MagicString s = kundo2_i18n("Modify name");
            switch (node->type()) {
                case Node::Type_Task: s = kundo2_i18n("Modify task name"); break;
                case Node::Type_Milestone: s = kundo2_i18n("Modify milestone name"); break;
                case Node::Type_Summarytask: s = kundo2_i18n("Modify summarytask name"); break;
                case Node::Type_Project: s = kundo2_i18n("Modify project name"); break;
            }
            return new NodeModifyNameCmd(*node, value.toString(), s);
        }
    }
    return nullptr;
}

KUndo2Command *NodeModel::setLeader(Node *node, const QVariant &value, int role)
{
    switch (role) {
        case Qt::EditRole: {
            if (value.toString() != node->leader()) {
                return new NodeModifyLeaderCmd(*node, value.toString(), kundo2_i18n("Modify responsible"));
            }
            break;
        }
        default:
            break;
    }
    return nullptr;
}

KUndo2Command *NodeModel::setAllocation(Node *node, const QVariant &value, int role)
{
    Q_UNUSED(node)
    Q_UNUSED(value)
    Q_UNUSED(role)
    return nullptr;
}

KUndo2Command *NodeModel::setDescription(Node *node, const QVariant &value, int role)
{
    switch (role) {
        case Qt::EditRole:
            if (value.toString() == node->description()) {
                return nullptr;
            }
            return new NodeModifyDescriptionCmd(*node, value.toString(), kundo2_i18n("Modify task description"));
    }
    return nullptr;
}

KUndo2Command *NodeModel::setType(Node *, const QVariant &, int)
{
    return nullptr;
}

KUndo2Command *NodeModel::setConstraint(Node *node, const QVariant &value, int role)
{
    switch (role) {
        case Qt::EditRole: {
            Node::ConstraintType v;
            QStringList lst = node->constraintList(false);
            if (lst.contains(value.toString())) {
                v = Node::ConstraintType(lst.indexOf(value.toString()));
            } else {
                v = Node::ConstraintType(value.toInt());
            }
            //debugPlan<<v;
            if (v != node->constraint()) {
                return new NodeModifyConstraintCmd(*node, v, kundo2_i18n("Modify constraint type"));
            }
            break;
        }
        default:
            break;
    }
    return nullptr;
}

KUndo2Command *NodeModel::setConstraintStartTime(Node *node, const QVariant &value, int role)
{
    switch (role) {
        case Qt::EditRole: {
            QDateTime dt = value.toDateTime();
            dt.setTime(QTime(dt.time().hour(), dt.time().minute(), 0)); // reset possible secs/msecs
            if (dt != node->constraintStartTime()) {
                return new NodeModifyConstraintStartTimeCmd(*node, dt, kundo2_i18n("Modify constraint start time"));
            }
            break;
        }
        default:
            break;
    }
    return nullptr;
}

KUndo2Command *NodeModel::setConstraintEndTime(Node *node, const QVariant &value, int role)
{
    switch (role) {
        case Qt::EditRole: {
            QDateTime dt = value.toDateTime();
            dt.setTime(QTime(dt.time().hour(), dt.time().minute(), 0)); // reset possible secs/msecs
            if (dt != node->constraintEndTime()) {
                return new NodeModifyConstraintEndTimeCmd(*node, dt, kundo2_i18n("Modify constraint end time"));
            }
            break;
        }
        default:
            break;
    }
    return nullptr;
}

KUndo2Command *NodeModel::setEstimateType(Node *node, const QVariant &value, int role)
{
    if (node->estimate() == nullptr) {
        return nullptr;
    }
    switch (role) {
        case Qt::EditRole: {
            Estimate::Type v;
            QStringList lst = node->estimate()->typeToStringList(false);
            if (lst.contains(value.toString())) {
                v = Estimate::Type(lst.indexOf(value.toString()));
            } else {
                v = Estimate::Type(value.toInt());
            }
            if (v != node->estimate()->type()) {
                return new ModifyEstimateTypeCmd(*node, node->estimate()->type(), v, kundo2_i18n("Modify estimate type"));
            }
            break;
        }
        default:
            break;
    }
    return nullptr;
}

KUndo2Command *NodeModel::setEstimateCalendar(Node *node, const QVariant &value, int role)
{
    if (node->estimate() == nullptr) {
        return nullptr;
    }
    switch (role) {
        case Qt::EditRole: {
            Calendar *c = nullptr;
            Calendar *old = node->estimate()->calendar();
            if (value.toInt() > 0) {
                QStringList lst = estimateCalendar(node, Role::EnumList).toStringList();
                if (value.toInt() < lst.count()) {
                    c = m_project->calendarByName(lst.at(value.toInt()));
                }
            }
            if (c != old) {
                return new ModifyEstimateCalendarCmd(*node, old, c, kundo2_i18n("Modify estimate calendar"));
            }
            break;
        }
        default:
            break;
    }
    return nullptr;
}

KUndo2Command *NodeModel::setEstimate(Node *node, const QVariant &value, int role)
{
    if (node->estimate() == nullptr) {
        return nullptr;
    }
    switch (role) {
        case Qt::EditRole: {
            double d;
            Duration::Unit unit;
            if (value.toList().count() == 2) {
                d =  value.toList()[0].toDouble();
                unit = static_cast<Duration::Unit>(value.toList()[1].toInt());
            } else if (value.canConvert<QString>()) {
                bool ok = Duration::valueFromString(value.toString(), d, unit);
                if (! ok) {
                    return nullptr;
                }
            } else {
                return nullptr;
            }
            //debugPlan<<d<<","<<unit<<" ->"<<value.toList()[1].toInt();
            MacroCommand *cmd = nullptr;
            if (d != node->estimate()->expectedEstimate()) {
                if (cmd == nullptr) cmd = new MacroCommand(kundo2_i18n("Modify estimate"));
                cmd->addCommand(new ModifyEstimateCmd(*node, node->estimate()->expectedEstimate(), d));
            }
            if (unit != node->estimate()->unit()) {
                if (cmd == nullptr) cmd = new MacroCommand(kundo2_i18n("Modify estimate"));
                cmd->addCommand(new ModifyEstimateUnitCmd(*node, node->estimate()->unit(), unit));
            }
            if (cmd) {
                return cmd;
            }
            break;
        }
        default:
            break;
    }
    return nullptr;
}

KUndo2Command *NodeModel::setOptimisticRatio(Node *node, const QVariant &value, int role)
{
    if (node->estimate() == nullptr) {
        return nullptr;
    }
    switch (role) {
        case Qt::EditRole:
            if (value.toInt() != node->estimate()->optimisticRatio()) {
                return new EstimateModifyOptimisticRatioCmd(*node, node->estimate()->optimisticRatio(), value.toInt(), kundo2_i18n("Modify optimistic estimate"));
            }
            break;
        default:
            break;
    }
    return nullptr;
}

KUndo2Command *NodeModel::setPessimisticRatio(Node *node, const QVariant &value, int role)
{
    if (node->estimate() == nullptr) {
        return nullptr;
    }
    switch (role) {
        case Qt::EditRole:
            if (value.toInt() != node->estimate()->pessimisticRatio()) {
                return new EstimateModifyPessimisticRatioCmd(*node, node->estimate()->pessimisticRatio(), value.toInt(), kundo2_i18n("Modify pessimistic estimate"));
            }
        default:
            break;
    }
    return nullptr;
}

KUndo2Command *NodeModel::setRiskType(Node *node, const QVariant &value, int role)
{
    if (node->estimate() == nullptr) {
        return nullptr;
    }
    switch (role) {
        case Qt::EditRole: {
            int val = 0;
            QStringList lst = node->estimate()->risktypeToStringList(false);
            if (lst.contains(value.toString())) {
                val = lst.indexOf(value.toString());
            } else {
                val = value.toInt();
            }
            if (val != node->estimate()->risktype()) {
                Estimate::Risktype v = Estimate::Risktype(val);
                return new EstimateModifyRiskCmd(*node, node->estimate()->risktype(), v, kundo2_i18n("Modify risk type"));
            }
            break;
        }
        default:
            break;
    }
    return nullptr;
}

KUndo2Command *NodeModel::setPriority(Node *node, const QVariant &value, int role)
{
    switch (role) {
        case Qt::EditRole: {
            if (value.toInt() != node->priority()) {
                return new NodeModifyPriorityCmd(*node, node->priority(), value.toInt(), kundo2_i18n("Modify priority"));
            }
            break;
        }
        default:
            break;
    }
    return nullptr;
}

KUndo2Command *NodeModel::setRunningAccount(Node *node, const QVariant &value, int role)
{
    switch (role) {
        case Qt::EditRole: {
            //debugPlan<<node->name();
            QStringList lst = runningAccount(node, Role::EnumList).toStringList();
            if (value.toInt() < lst.count()) {
                Account *a = m_project->accounts().findAccount(lst.at(value.toInt()));
                Account *old = node->runningAccount();
                if (old != a) {
                    return new NodeModifyRunningAccountCmd(*node, old, a, kundo2_i18n("Modify running account"));
                }
            }
            break;
        }
        default:
            break;
    }
    return nullptr;
}

KUndo2Command *NodeModel::setStartupAccount(Node *node, const QVariant &value, int role)
{
    switch (role) {
        case Qt::EditRole: {
            //debugPlan<<node->name();
            QStringList lst = startupAccount(node, Role::EnumList).toStringList();
            if (value.toInt() < lst.count()) {
                Account *a = m_project->accounts().findAccount(lst.at(value.toInt()));
                Account *old = node->startupAccount();
                //debugPlan<<(value.toInt())<<";"<<(lst.at(value.toInt()))<<":"<<a;
                if (old != a) {
                    return new NodeModifyStartupAccountCmd(*node, old, a, kundo2_i18n("Modify startup account"));
                }
            }
            break;
        }
        default:
            break;
    }
    return nullptr;
}

KUndo2Command *NodeModel::setStartupCost(Node *node, const QVariant &value, int role)
{
    switch (role) {
        case Qt::EditRole: {
            double v = value.toDouble();
            if (v != node->startupCost()) {
                return new NodeModifyStartupCostCmd(*node, v, kundo2_i18n("Modify startup cost"));
            }
            break;
        }
        default:
            break;
    }
    return nullptr;
}

KUndo2Command *NodeModel::setShutdownAccount(Node *node, const QVariant &value, int role)
{
    switch (role) {
        case Qt::EditRole: {
            //debugPlan<<node->name();
            QStringList lst = shutdownAccount(node, Role::EnumList).toStringList();
            if (value.toInt() < lst.count()) {
                Account *a = m_project->accounts().findAccount(lst.at(value.toInt()));
                Account *old = node->shutdownAccount();
                if (old != a) {
                    return new NodeModifyShutdownAccountCmd(*node, old, a, kundo2_i18n("Modify shutdown account"));
                }
            }
            break;
        }
        default:
            break;
    }
    return nullptr;
}

KUndo2Command *NodeModel::setShutdownCost(Node *node, const QVariant &value, int role)
{
    switch (role) {
        case Qt::EditRole: {
            double v = value.toDouble();
            if (v != node->shutdownCost()) {
                return new NodeModifyShutdownCostCmd(*node, v, kundo2_i18n("Modify shutdown cost"));
            }
            break;
        }
        default:
            break;
    }
    return nullptr;
}

KUndo2Command *NodeModel::setCompletion(Node *node, const QVariant &value, int role)
{
    Q_UNUSED(node)
    Q_UNUSED(value)
    Q_UNUSED(role)
    return nullptr;
}

KUndo2Command *NodeModel::setRemainingEffort(Node *node, const QVariant &value, int role)
{
    if (role == Qt::EditRole && node->type() == Node::Type_Task) {
        Task *t = static_cast<Task*>(node);
        double d(value.toList()[0].toDouble());
        Duration::Unit unit = static_cast<Duration::Unit>(value.toList()[1].toInt());
        Duration dur(d, unit);
        return new ModifyCompletionRemainingEffortCmd(t->completion(), QDate::currentDate(), dur, kundo2_i18n("Modify remaining effort"));
    }
    return nullptr;
}

KUndo2Command *NodeModel::setActualEffort(Node *node, const QVariant &value, int role)
{
    if (role == Qt::EditRole && node->type() == Node::Type_Task) {
        Task *t = static_cast<Task*>(node);
        double d(value.toList()[0].toDouble());
        Duration::Unit unit = static_cast<Duration::Unit>(value.toList()[1].toInt());
        Duration dur(d, unit);
        return new ModifyCompletionActualEffortCmd(t->completion(), QDate::currentDate(), dur, kundo2_i18n("Modify actual effort"));
    }
    return nullptr;
}

KUndo2Command *NodeModel::setStartedTime(Node *node, const QVariant &value, int role)
{
    switch (role) {
        case Qt::EditRole: {
            Task *t = qobject_cast<Task*>(node);
            if (t == nullptr) {
                return nullptr;
            }
            MacroCommand *m = new MacroCommand(kundo2_i18n("Modify actual start time"));
            if (! t->completion().isStarted()) {
                m->addCommand(new ModifyCompletionStartedCmd(t->completion(), true));
            }
            m->addCommand(new ModifyCompletionStartTimeCmd(t->completion(), value.toDateTime()));
            if (t->type() == Node::Type_Milestone) {
                m->addCommand(new ModifyCompletionFinishedCmd(t->completion(), true));
                m->addCommand(new ModifyCompletionFinishTimeCmd(t->completion(), value.toDateTime()));
                if (t->completion().percentFinished() < 100) {
                    Completion::Entry *e = new Completion::Entry(100, Duration::zeroDuration, Duration::zeroDuration);
                    m->addCommand(new AddCompletionEntryCmd(t->completion(), value.toDate(), e));
                }
            }
            return m;
        }
        default:
            break;
    }
    return nullptr;
}

KUndo2Command *NodeModel::setFinishedTime(Node *node, const QVariant &value, int role)
{
    switch (role) {
        case Qt::EditRole: {
            Task *t = qobject_cast<Task*>(node);
            if (t == nullptr) {
                return nullptr;
            }
            MacroCommand *m = new MacroCommand(kundo2_i18n("Modify actual finish time"));
            if (! t->completion().isFinished()) {
                m->addCommand(new ModifyCompletionFinishedCmd(t->completion(), true));
                if (t->completion().percentFinished() < 100) {
                    Completion::Entry *e = new Completion::Entry(100, Duration::zeroDuration, Duration::zeroDuration);
                    m->addCommand(new AddCompletionEntryCmd(t->completion(), value.toDate(), e));
                }
            }
            m->addCommand(new ModifyCompletionFinishTimeCmd(t->completion(), value.toDateTime()));
            if (t->type() == Node::Type_Milestone) {
                m->addCommand(new ModifyCompletionStartedCmd(t->completion(), true));
                m->addCommand(new ModifyCompletionStartTimeCmd(t->completion(), value.toDateTime()));
            }
            return m;
        }
        default:
            break;
    }
    return nullptr;
}

//----------------------------
NodeItemModel::NodeItemModel(QObject *parent)
    : ItemModelBase(parent),
    m_node(nullptr),
    m_projectshown(false)
{
    setReadOnly(NodeModel::NodeDescription, true);
}

NodeItemModel::~NodeItemModel()
{
}

void NodeItemModel::setShowProject(bool on)
{
    beginResetModel();
    m_projectshown = on;
    endResetModel();
    Q_EMIT projectShownChanged(on);
}

void NodeItemModel::slotNodeToBeInserted(Node *parent, int row)
{
    //debugPlan<<parent->name()<<"; "<<row;
    Q_ASSERT(m_node == nullptr);
    m_node = parent;
    beginInsertRows(index(parent), row, row);
}

void NodeItemModel::slotNodeInserted(Node *node)
{
    //debugPlan<<node->parentNode()->name()<<"-->"<<node->name();
    Q_ASSERT(node->parentNode() == m_node);
    endInsertRows();
    m_node = nullptr;
    Q_EMIT nodeInserted(node);
}

void NodeItemModel::slotNodeToBeRemoved(Node *node)
{
    //debugPlan<<node->name();
    Q_ASSERT(m_node == nullptr);
    m_node = node;
    int row = index(node).row();
    beginRemoveRows(index(node->parentNode()), row, row);
}

void NodeItemModel::slotNodeRemoved(Node *node)
{
    //debugPlan<<node->name();
    Q_ASSERT(node == m_node);
#ifdef NDEBUG
    Q_UNUSED(node)
#endif
    endRemoveRows();
    m_node = nullptr;
}

void NodeItemModel::slotNodeToBeMoved(Node *node, int pos, Node *newParent, int newPos)
{
    //debugPlan<<node->parentNode()->name()<<pos<<":"<<newParent->name()<<newPos;
    beginMoveRows(index(node->parentNode()), pos, pos, index(newParent), newPos);
}

void NodeItemModel::slotNodeMoved(Node *node)
{
    Q_UNUSED(node);
    //debugPlan<<node->parentNode()->name()<<node->parentNode()->indexOf(node);
    endMoveRows();
}

void NodeItemModel::slotLayoutChanged()
{
    //debugPlan<<node->name();
    Q_EMIT layoutAboutToBeChanged();
    Q_EMIT layoutChanged();
}

void NodeItemModel::slotProjectCalculated(ScheduleManager *sm)
{
    debugPlan<<m_manager<<sm;
    if (sm && sm == m_manager) {
        slotLayoutChanged();
    }
}

void NodeItemModel::slotWbsDefinitionChanged()
{
    debugPlan;
    if (m_project == nullptr) {
        return;
    }
    if (m_projectshown) {
        QModelIndex idx = createIndex(0, NodeModel::NodeWBSCode, m_project);
        Q_EMIT dataChanged(idx, idx);
    }
    const QList<Node*> nodes = m_project->allNodes();
    for (Node *n : nodes) {
        int row = n->parentNode()->indexOf(n);
        QModelIndex idx = createIndex(row, NodeModel::NodeWBSCode, n);
        Q_EMIT dataChanged(idx, idx);
    }
}


void NodeItemModel::setProject(Project *project)
{
    beginResetModel();
    if (m_project) {
        disconnect(m_project, &Project::aboutToBeDeleted, this, &NodeItemModel::projectDeleted);
        disconnect(m_project, SIGNAL(localeChanged()), this, SLOT(slotLayoutChanged()));
        disconnect(m_project, &Project::wbsDefinitionChanged, this, &NodeItemModel::slotWbsDefinitionChanged);
        disconnect(m_project, &Project::nodeChanged, this, &NodeItemModel::slotNodeChanged);
        disconnect(m_project, &Project::nodeToBeAdded, this, &NodeItemModel::slotNodeToBeInserted);
        disconnect(m_project, &Project::nodeToBeRemoved, this, &NodeItemModel::slotNodeToBeRemoved);

        disconnect(m_project, &Project::nodeToBeMoved, this, &NodeItemModel::slotNodeToBeMoved);
        disconnect(m_project, &Project::nodeMoved, this, &NodeItemModel::slotNodeMoved);

        disconnect(m_project, &Project::nodeAdded, this, &NodeItemModel::slotNodeInserted);
        disconnect(m_project, &Project::nodeRemoved, this, &NodeItemModel::slotNodeRemoved);
        disconnect(m_project, &Project::projectCalculated, this, &NodeItemModel::slotProjectCalculated);
    }
    m_project = project;
    //debugPlan<<this<<m_project<<"->"<<project;
    m_nodemodel.setProject(project);
    if (project) {
        connect(m_project, &Project::aboutToBeDeleted, this, &NodeItemModel::projectDeleted);
        connect(m_project, SIGNAL(localeChanged()), this, SLOT(slotLayoutChanged()));
        connect(m_project, &Project::wbsDefinitionChanged, this, &NodeItemModel::slotWbsDefinitionChanged);
        connect(m_project, &Project::nodeChanged, this, &NodeItemModel::slotNodeChanged);
        connect(m_project, &Project::nodeToBeAdded, this, &NodeItemModel::slotNodeToBeInserted);
        connect(m_project, &Project::nodeToBeRemoved, this, &NodeItemModel::slotNodeToBeRemoved);

        connect(m_project, &Project::nodeToBeMoved, this, &NodeItemModel::slotNodeToBeMoved);
        connect(m_project, &Project::nodeMoved, this, &NodeItemModel::slotNodeMoved);

        connect(m_project, &Project::nodeAdded, this, &NodeItemModel::slotNodeInserted);
        connect(m_project, &Project::nodeRemoved, this, &NodeItemModel::slotNodeRemoved);
        connect(m_project, &Project::projectCalculated, this, &NodeItemModel::slotProjectCalculated);
    }
    endResetModel();
}

void NodeItemModel::setScheduleManager(ScheduleManager *sm)
{
    if (sm == m_nodemodel.manager()) {
        return;
    }
    beginResetModel();
    if (m_nodemodel.manager()) {
    }
    m_nodemodel.setManager(sm);
    ItemModelBase::setScheduleManager(sm);
    if (sm) {
    }
    //debugPlan<<this<<sm;
    endResetModel();
}

Qt::ItemFlags NodeItemModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = QAbstractItemModel::flags(index);
    if (!index.isValid()) {
        if (m_readWrite) {
            flags |= Qt::ItemIsDropEnabled;
        }
        return flags;
    }
    flags |= Qt::ItemIsDragEnabled;
    if (isColumnReadOnly(index.column())) {
        //debugPlan<<"Column is readonly:"<<index.column();
        return flags;
    }
    Node *n = node(index);
    if (m_readWrite && n != nullptr) {
        bool baselined = n->isBaselined();
        flags |= Qt::ItemIsDropEnabled;
        switch (index.column()) {
            case NodeModel::NodeName: // name
                flags |= Qt::ItemIsEditable;
                break;
            case NodeModel::NodeType: break; // Node type
            case NodeModel::NodeResponsible: // Responsible
                flags |= Qt::ItemIsEditable;
                break;
            case NodeModel::NodeAllocation: // allocation
                if (n->type() == Node::Type_Task) {
                    flags |= Qt::ItemIsEditable;
                }
                break;
            case NodeModel::NodeEstimateType: // estimateType
            {
                if (! baselined && (n->type() == Node::Type_Task || n->type() == Node::Type_Milestone)) {
                    flags |= Qt::ItemIsEditable;
                }
                break;
            }
            case NodeModel::NodeEstimate: // estimate
            {
                if (! baselined && (n->type() == Node::Type_Task || n->type() == Node::Type_Milestone)) {
                    flags |= Qt::ItemIsEditable;
                }
                break;
            }
            case NodeModel::NodeOptimisticRatio: // optimisticRatio
            case NodeModel::NodePessimisticRatio: // pessimisticRatio
            {
                if (! baselined && n->type() == Node::Type_Task) {
                    flags |= Qt::ItemIsEditable;
                }
                break;
            }
            case NodeModel::NodeEstimateCalendar:
            {
                if (! baselined && n->type() == Node::Type_Task)
                {
                    flags |= Qt::ItemIsEditable;
                }
                break;
            }
            case NodeModel::NodeRisk: // risktype
            {
                if (! baselined && n->type() == Node::Type_Task) {
                    flags |= Qt::ItemIsEditable;
                }
                break;
            }
            case NodeModel::NodePriority:
            {
                if (!baselined && (n->type() == Node::Type_Task || n->type() == Node::Type_Milestone)) {
                    flags |= Qt::ItemIsEditable;
                }
                break;
            }
            case NodeModel::NodeConstraint: // constraint type
                if (! baselined && (n->type() == Node::Type_Task || n->type() == Node::Type_Milestone)) {
                    flags |= Qt::ItemIsEditable;
                }
                break;
            case NodeModel::NodeConstraintStart: { // constraint start
                if (! baselined && n->type() == Node::Type_Project) {
                    flags |= Qt::ItemIsEditable;
                    break;
                }
                if (! baselined && ! (n->type() == Node::Type_Task || n->type() == Node::Type_Milestone)) {
                    break;
                }
                flags |= Qt::ItemIsEditable;
                break;
            }
            case NodeModel::NodeConstraintEnd: { // constraint end
                if (! baselined && n->type() == Node::Type_Project) {
                    flags |= Qt::ItemIsEditable;
                    break;
                }
                if (! baselined && ! (n->type() == Node::Type_Task || n->type() == Node::Type_Milestone)) {
                    break;
                }
                flags |= Qt::ItemIsEditable;
                break;
            }
            case NodeModel::NodeRunningAccount: // running account
                if (! baselined && n->type() == Node::Type_Task) {
                    flags |= Qt::ItemIsEditable;
                }
                break;
            case NodeModel::NodeStartupAccount: // startup account
            case NodeModel::NodeStartupCost: // startup cost
            case NodeModel::NodeShutdownAccount: // shutdown account
            case NodeModel::NodeShutdownCost: { // shutdown cost
                if (! baselined && (n->type() == Node::Type_Task || n->type() == Node::Type_Milestone)) {
                    flags |= Qt::ItemIsEditable;
                }
                break;
            }
            case NodeModel::NodeDescription: // description
                flags |= Qt::ItemIsEditable;
                break;
            default:
                break;
        }
        Task *t = static_cast<Task*>(n);
        if (manager() && t->isScheduled(id())) {
            if (! t->completion().isStarted()) {
                switch (index.column()) {
                    case NodeModel::NodeActualStart:
                        flags |= Qt::ItemIsEditable;
                        break;
                    case NodeModel::NodeActualFinish:
                        if (t->type() == Node::Type_Milestone) {
                            flags |= Qt::ItemIsEditable;
                        }
                        break;
                    case NodeModel::NodeCompleted:
                        if (t->state() & Node::State_ReadyToStart) {
                            flags |= Qt::ItemIsEditable;
                        }
                        break;

                    default: break;
                }
            } else if (! t->completion().isFinished()) {
                switch (index.column()) {
                    case NodeModel::NodeActualFinish:
                    case NodeModel::NodeRemainingEffort:
                        flags |= Qt::ItemIsEditable;
                        break;
                    case NodeModel::NodeCompleted:
                        flags |= Qt::ItemIsEditable;
                        break;
                    case NodeModel::NodeActualEffort:
                        if (t->completion().entrymode() == Completion::EnterEffortPerTask || t->completion().entrymode() == Completion::EnterEffortPerResource) {
                            flags |= Qt::ItemIsEditable;
                        }
                        break;
                    default: break;
                }
            }
        }
    }
    return flags;
}


QModelIndex NodeItemModel::parent(const QModelIndex &index) const
{
    if (! index.isValid()) {
        return QModelIndex();
    }
    Node *n = node(index);
    if (n == nullptr || n == m_project) {
        return QModelIndex();
    }
    Node *p = n->parentNode();
    if (p == m_project) {
        return m_projectshown ? createIndex(0, 0, p) : QModelIndex();
    }
    int row = p->parentNode()->indexOf(p);
    if (row == -1) {
        return QModelIndex();
    }
    return createIndex(row, 0, p);
}

QModelIndex NodeItemModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid()) {
        Q_ASSERT(parent.model() == this);
    }
    //debugPlan<<parent<<row<<column;
    if (m_project == nullptr || column < 0 || column >= columnCount() || row < 0) {
        //debugPlan<<m_project<<parent<<"No index for"<<row<<","<<column;
        return QModelIndex();
    }
    if (m_projectshown && ! parent.isValid()) {
        return createIndex(row, column, m_project);
    }
    Node *p = node(parent);
    if (row >= p->numChildren()) {
        errorPlan<<p->name()<<" row too high"<<row<<","<<column;
        debugPlan<<"parent:"<<parent<<"rows:"<<rowCount(parent)<<"children:"<<p->numChildren();
        return QModelIndex();
    }
    // now get the internal pointer for the index
    Node *n = p->childNode(row);
    QModelIndex idx = createIndex(row, column, n);
    //debugPlan<<idx;
    return idx;
}

QModelIndex NodeItemModel::index(const Node *node, int column) const
{
    if (m_project == nullptr || node == nullptr) {
        return QModelIndex();
    }
    Node *par = node->parentNode();
    if (par) {
        //debugPlan<<par<<"-->"<<node;
        return createIndex(par->indexOf(node), column, const_cast<Node*>(node));
    }
    if (m_projectshown && node == m_project) {
        return createIndex(0, column, m_project);
    }
    //debugPlan<<node;
    return QModelIndex();
}

bool NodeItemModel::setType(Node *, const QVariant &, int)
{
    return false;
}

bool NodeItemModel::setAllocation(Node *node, const QVariant &value, int role)
{
    Task *task = qobject_cast<Task*>(node);
    if (task == nullptr) {
        return false;
    }
    switch (role) {
        case Qt::EditRole:
        {
            MacroCommand *cmd = new MacroCommand();
            bool removedAllocations = false;
            bool addedAllocations = false;
            KUndo2Command *cc = nullptr;

            QStringList res = m_project->resourceNameList();
            const QStringList req = node->requestNameList();
            QStringList alloc;
            const QStringList allocations = value.toString().split(QRegularExpression(QStringLiteral(" *, *")), Qt::SkipEmptyParts);
            for (const QString &s : allocations) {
                alloc << s.trimmed();
            }
            // Handle deleted requests
            for (const QString &s : req) {
                // if a request is not in alloc, it must have been be removed by the user
                if (!alloc.contains(s)) {
                    // remove removed resource request
                    ResourceRequest *r = node->resourceRequest(s);
                    if (r) {
                        //debugPlan<<"delete request:"<<r->resource()->name()<<" group:"<<r->parent()->group()->name();
                        cc = new RemoveResourceRequestCmd(r);
                        cc->redo();
                        cmd->addCommand(cc);
                        removedAllocations = true;
                    }
                }
            }
            // Handle new requests
            for (const QString &s : std::as_const(alloc)) {
                // if an allocation is not in req, it must be added
                if (!req.contains(s)) {
                    Resource *r = m_project->resourceByName(s);
                    if (r == nullptr) {
                        // Non-existent resource, add it
                        r = new Resource();
                        r->setName(s);
                        cc = new AddResourceCmd(m_project, r);
                        //debugPlan<<"add resource:"<<r->name();
                        cc->redo();
                        cmd->addCommand(cc);
                        addedAllocations = true;
                    }
                    cc = new AddResourceRequestCmd(&node->requests(), new ResourceRequest(r, r->units()));
                    cc->redo();
                    cmd->addCommand(cc);
                }
            }
            if (!cmd->isEmpty()) {
                KUndo2MagicString s = kundo2_i18n("Add resource allocation");
                if (!addedAllocations && removedAllocations) {
                    s = kundo2_i18n("Removed resource allocation");
                }
                MacroCommand *m = new MacroCommand(s);
                Q_EMIT executeCommand(m);
                m->addCommand(cmd);
                return true;
            }
            delete cmd;
        }
    }
    return false;
}

bool NodeItemModel::setCompletion(Node *node, const QVariant &value, int role)
{
    debugPlan<<node->name()<<value<<role;
    if (role != Qt::EditRole) {
        return 0;
    }
    if (node->type() == Node::Type_Task) {
        Completion &c = static_cast<Task*>(node)->completion();
        QDateTime dt = QDateTime::currentDateTime();
        QDate date = dt.date();
        MacroCommand *m = new MacroCommand(kundo2_i18n("Modify completion"));
        if (! c.isStarted()) {
            m->addCommand(new ModifyCompletionStartTimeCmd(c, dt));
            m->addCommand(new ModifyCompletionStartedCmd(c, true));
        }
        m->addCommand(new ModifyCompletionPercentFinishedCmd(c, date, value.toInt()));
        if (value.toInt() == 100) {
            m->addCommand(new ModifyCompletionFinishTimeCmd(c, dt));
            m->addCommand(new ModifyCompletionFinishedCmd(c, true));
        }
        Q_EMIT executeCommand(m); // also adds a new entry if necessary
        if (c.entrymode() != Completion::EnterEffortPerResource) {
            Duration planned = static_cast<Task*>(node)->plannedEffort(m_nodemodel.id());
            Duration actual = (planned * value.toInt()) / 100;
            debugPlan<<planned.toString()<<value.toInt()<<actual.toString();
            NamedCommand *cmd = new ModifyCompletionActualEffortCmd(c, date, actual);
            cmd->execute();
            m->addCommand(cmd);
            cmd = new ModifyCompletionRemainingEffortCmd(c, date, planned - actual);
            cmd->execute();
            m->addCommand(cmd);
        }
        return true;
    }
    if (node->type() == Node::Type_Milestone) {
        Completion &c = static_cast<Task*>(node)->completion();
        if (value.toInt() > 0) {
            QDateTime dt = QDateTime::currentDateTime();
            QDate date = dt.date();
            MacroCommand *m = new MacroCommand(kundo2_i18n("Set finished"));
            m->addCommand(new ModifyCompletionStartTimeCmd(c, dt));
            m->addCommand(new ModifyCompletionStartedCmd(c, true));
            m->addCommand(new ModifyCompletionFinishTimeCmd(c, dt));
            m->addCommand(new ModifyCompletionFinishedCmd(c, true));
            m->addCommand(new ModifyCompletionPercentFinishedCmd(c, date, 100));
            Q_EMIT executeCommand(m); // also adds a new entry if necessary
            return true;
        }
        return false;
    }
    return false;
}

QVariant NodeItemModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::TextAlignmentRole) {
        return headerData(index.column(), Qt::Horizontal, role);
    }
    Node *n = node(index);
    if (role == Role::Object) {
        return n ? QVariant::fromValue(static_cast<QObject*>(n)) : QVariant();
    }
    QVariant result;
    if (n != nullptr) {
        result = m_nodemodel.data(n, index.column(), role);
        //debugPlan<<n->name()<<": "<<index.column()<<", "<<role<<result;
    }
    if (role == Qt::EditRole) {
        switch (index.column()) {
            case NodeModel::NodeActualStart:
            case NodeModel::NodeActualFinish:
                if (! result.isValid()) {
                    return QDateTime::currentDateTime();
                }
            break;
        }
    }
    return result;
}

bool NodeItemModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (! index.isValid()) {
        return ItemModelBase::setData(index, value, role);
    }
    if ((flags(index) &Qt::ItemIsEditable) == 0 || role != Qt::EditRole) {
        warnPlan<<index<<value<<role;
        return false;
    }
    Node *n = node(index);
    if (n) {
        switch (index.column()) {
            case NodeModel::NodeCompleted: return setCompletion(n, value, role);
            case NodeModel::NodeAllocation: return setAllocation(n, value, role);
            default: {
                KUndo2Command *c = m_nodemodel.setData(n, index.column(), value, role);
                if (c) {
                    Q_EMIT executeCommand(c);
                    return true;
                }
                break;
            }
        }
    }
    return false;
}

QVariant NodeItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal) {
        if  (role == Qt::DisplayRole || role == Qt::TextAlignmentRole || role == Qt::EditRole) {
            return m_nodemodel.headerData(section, role);
        }
    }
    if (role == Qt::ToolTipRole) {
        return NodeModel::headerData(section, role);
    } else if (role == Qt::WhatsThisRole) {
        return NodeModel::headerData(section, role);
    }
    return ItemModelBase::headerData(section, orientation, role);
}

QAbstractItemDelegate *NodeItemModel::createDelegate(int column, QWidget *parent) const
{
    switch (column) {
        //case NodeModel::NodeAllocation: return new ??Delegate(parent);
        case NodeModel::NodeEstimateType: return new EnumDelegate(parent);
        case NodeModel::NodeEstimateCalendar: return new EnumDelegate(parent);
        case NodeModel::NodeEstimate: return new DurationSpinBoxDelegate(parent);
        case NodeModel::NodeOptimisticRatio: return new SpinBoxDelegate(parent);
        case NodeModel::NodePessimisticRatio: return new SpinBoxDelegate(parent);
        case NodeModel::NodeRisk: return new EnumDelegate(parent);
        case NodeModel::NodePriority: return new SpinBoxDelegate(parent);
        case NodeModel::NodeConstraint: return new EnumDelegate(parent);
        case NodeModel::NodeConstraintStart: return new DateTimeCalendarDelegate(parent);
        case NodeModel::NodeConstraintEnd: return new DateTimeCalendarDelegate(parent);
        case NodeModel::NodeRunningAccount: return new EnumDelegate(parent);
        case NodeModel::NodeStartupAccount: return new EnumDelegate(parent);
//         case NodeModel::NodeStartupCost: return new MoneyDelegate(parent);
        case NodeModel::NodeShutdownAccount: return new EnumDelegate(parent);
//         case NodeModel::NodeShutdownCost: return new MoneyDelegate(parent);

        case NodeModel::NodeCompleted: return new TaskCompleteDelegate(parent);
        case NodeModel::NodeRemainingEffort: return new DurationSpinBoxDelegate(parent);
        case NodeModel::NodeActualEffort: return new DurationSpinBoxDelegate(parent);

        default: return nullptr;
    }
    return nullptr;
}

int NodeItemModel::columnCount(const QModelIndex &/*parent*/) const
{
    return m_nodemodel.propertyCount();
}

int NodeItemModel::rowCount(const QModelIndex &parent) const
{
    int rows = 0;
    Node *p = nullptr;
    if (m_projectshown && ! parent.isValid()) {
        rows = m_project == nullptr ? 0 : 1;
    } else {
        p = node(parent);
        rows = p == nullptr ? 0 : p->numChildren();
    }
    return rows;
}

Qt::DropActions NodeItemModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}


QStringList NodeItemModel::mimeTypes() const
{
    return ItemModelBase::mimeTypes()
            << QStringLiteral("text/html")
            << QStringLiteral("text/plain")
            << QStringLiteral("application/x-vnd.kde.plan.nodeitemmodel.internal")
            << QStringLiteral("application/x-vnd.kde.plan.resourceitemmodel.internal")
            << QStringLiteral("application/x-vnd.kde.plan.project")
            << QStringLiteral("application/x-vnd.kde.plan.taskmodule")
            << QStringLiteral("text/uri-list");
}

QMimeData *NodeItemModel::mimeData(const QModelIndexList & indexes) const
{
    QMimeData *m = ItemModelBase::mimeData(indexes);
    QByteArray encodedData;
    QDataStream stream(&encodedData, QIODevice::WriteOnly);
    QList<int> rows;
    for (const QModelIndex &index : indexes) {
        if (index.isValid() && !rows.contains(index.row())) {
            //debugPlan<<index.row();
            Node *n = node(index);
            if (n && n->type() != Node::Type_Project) {
                rows << index.row();
                stream << n->id();
            }
        }
    }
    if (!rows.isEmpty()) {
        m->setData(QStringLiteral("application/x-vnd.kde.plan.nodeitemmodel.internal"), encodedData);
    }

    QList<const Node*> nodes;
    for (const QModelIndex &index : indexes) {
        if (index.isValid()) {
            //debugPlan<<index.row();
            Node *n = node(index);
            if (n && n->type() != Node::Type_Project && !nodes.contains(n)) {
                nodes << n;
            }
        }
    }
    if (!nodes.isEmpty()) {
        XmlSaveContext context(project());
        context.nodes = nodes;
        context.options = XmlSaveContext::SaveSelectedNodes|XmlSaveContext::SaveRequests|XmlSaveContext::SaveRelations;
        context.save();
        m->setData(QStringLiteral("application/x-vnd.kde.plan.project"), context.document.toByteArray());
    }
    return m;
}

bool NodeItemModel::dropAllowed(const QModelIndex &index, int dropIndicatorPosition, const QMimeData *data)
{
    debugPlan<<index<<dropIndicatorPosition;
    if (m_projectshown && ! index.isValid()) {
        return false;
    }
    Node *dn = node(index); // returns project if ! index.isValid()
    if (dn == nullptr) {
        errorPlan<<"no node (or project) to drop on!";
        return false; // hmmm
    }
    if (data->hasFormat(QStringLiteral("application/x-vnd.kde.plan.resourceitemmodel.internal"))) {
        switch (dropIndicatorPosition) {
            case ItemModelBase::OnItem:
                if (index.column() == NodeModel::NodeAllocation) {
                    debugPlan<<"resource:"<<index<<(dn->type() == Node::Type_Task);
                    return dn->type() == Node::Type_Task;
                } else if (index.column() == NodeModel::NodeResponsible) {
                    debugPlan<<"resource:"<<index<<true;
                    return true;
                }
                break;
            default:
                break;
        }
    } else if (data->hasFormat(QStringLiteral("application/x-vnd.kde.plan.nodeitemmodel.internal"))
                || data->hasFormat(QStringLiteral("application/x-vnd.kde.plan.project"))
                || data->hasFormat(QStringLiteral("application/x-vnd.kde.plan.taskmodule"))
                || data->hasUrls())
    {
        switch (dropIndicatorPosition) {
            case ItemModelBase::AboveItem:
            case ItemModelBase::BelowItem:
                // dn == sibling, if not project
                if (dn == m_project) {
                    return dropAllowed(dn, data);
                }
                return dropAllowed(dn->parentNode(), data);
            case ItemModelBase::OnItem:
                // dn == new parent
                return dropAllowed(dn, data);
            default:
                break;
        }
    }
    return false;
}

QList<Resource*> NodeItemModel::resourceList(QDataStream &stream)
{
    QList<Resource*> lst;
    while (!stream.atEnd()) {
        QString id;
        stream >> id;
        debugPlan<<"id"<<id;
        Resource *r = m_project->findResource(id);
        if (r) {
            lst << r;
        }
    }
    debugPlan<<lst;
    return lst;
}

bool NodeItemModel::dropAllowed(Node *on, const QMimeData *data)
{
    if (! m_projectshown && on == m_project) {
        return true;
    }
    if (on->isBaselined() && on->type() != Node::Type_Summarytask && on->type() != Node::Type_Project) {
        return false;
    }
    if (data->hasFormat(QStringLiteral("application/x-vnd.kde.plan.nodeitemmodel.internal"))) {
        QByteArray encodedData = data->data(QStringLiteral("application/x-vnd.kde.plan.nodeitemmodel.internal"));
        QDataStream stream(&encodedData, QIODevice::ReadOnly);
        QList<Node*> lst = nodeList(stream);
        for (Node *n : std::as_const(lst)) {
            if (n->type() == Node::Type_Project || on == n || on->isChildOf(n)) {
                return false;
            }
        }
        lst = removeChildNodes(lst);
        for (Node *n : std::as_const(lst)) {
            if (! m_project->canMoveTask(n, on)) {
                return false;
            }
        }
    }
    return true;
}

QList<Node*> NodeItemModel::nodeList(QDataStream &stream)
{
    QList<Node*> lst;
    while (!stream.atEnd()) {
        QString id;
        stream >> id;
        Node *node = m_project->findNode(id);
        if (node) {
            lst << node;
        }
    }
    return lst;
}

QList<Node*> NodeItemModel::removeChildNodes(const QList<Node*> &nodes)
{
    QList<Node*> lst;
    for (Node *node : nodes) {
        bool ins = true;
        for (Node *n : lst) {
            if (node->isChildOf(n)) {
                //debugPlan<<node->name()<<" is child of"<<n->name();
                ins = false;
                break;
            }
        }
        if (ins) {
            //debugPlan<<" insert"<<node->name();
            lst << node;
        }
    }
    const QList<Node*> nl = lst;
    const QList<Node*> nlst = lst;
    for (Node *node : nl) {
        for (Node *n : nlst) {
            if (n->isChildOf(node)) {
                //debugPlan<<n->name()<<" is child of"<<node->name();
                int i = nodes.indexOf(n);
                lst.removeAt(i);
            }
        }
    }
    return lst;
}

bool NodeItemModel::dropResourceMimeData(const QMimeData *data, Qt::DropAction action, int /*row*/, int /*column*/, const QModelIndex &parent)
{
    QByteArray encodedData = data->data(QStringLiteral("application/x-vnd.kde.plan.resourceitemmodel.internal"));
    QDataStream stream(&encodedData, QIODevice::ReadOnly);
    Node *n = node(parent);
    debugPlan<<n<<parent;
    if (n == nullptr) {
        return true;
    }
    debugPlan<<n->name();
    if (parent.column() == NodeModel::NodeResponsible) {
        QString s;
        const QList<Resource*> resources = resourceList(stream);
        for (Resource *r : resources) {
            s += r->name();
        }
        if (! s.isEmpty()) {
            if (action == Qt::CopyAction && ! n->leader().isEmpty()) {
                s += QLatin1Char(',') + n->leader();
            }
            KUndo2Command *cmd = m_nodemodel.setLeader(n, s, Qt::EditRole);
            if (cmd) {
                Q_EMIT executeCommand(cmd);
            }
            debugPlan<<s;
        }
        return true;
    }
    if (n->type() == Node::Type_Task) {
        QList<Resource*> lst = resourceList(stream);
        if (action == Qt::CopyAction) {
            lst += static_cast<Task*>(n)->requestedResources();
        }
        KUndo2Command *cmd = createAllocationCommand(static_cast<Task&>(*n), lst);
        if (cmd) {
            Q_EMIT executeCommand(cmd);
        }
        return true;
    }
    return true;
}

bool NodeItemModel::dropProjectMimeData(const QMimeData *data, Qt::DropAction action, int row, int /*column*/, const QModelIndex &parent)
{
    Node *n = node(parent);
    if (n == nullptr) {
        n = m_project;
    }
    debugPlan<<n<<action<<row<<parent;

    KUndo2Command *cmd = new InsertProjectXmlCommand(project(), data->data(QStringLiteral("application/x-vnd.kde.plan.project")), n, n->childNode(row), kundo2_i18n("Insert tasks"));
    Q_EMIT executeCommand(cmd);
    return true;
}

bool NodeItemModel::dropTaskModuleMimeData(const QMimeData *data, Qt::DropAction action, int row, int /*column*/, const QModelIndex &parent)
{
    Node *n = node(parent);
    if (n == nullptr) {
        n = m_project;
    }
    debugPlan<<n<<action<<row<<parent;

    XMLLoaderObject context;
    KoXmlDocument doc;
    doc.setContent(QLatin1String(data->data(QStringLiteral("application/x-vnd.kde.plan.taskmodule")).constData()));
    Project moduleProject;
    context.loadProject(&moduleProject, doc);

    moduleProject.generateUniqueNodeIds();

    QStringList substitute;
    QRegularExpression reg(QStringLiteral("\\[\\[[^ ]*\\]\\]"));
    const auto nodes = moduleProject.allNodes();
    for (auto n : nodes) {
        QRegularExpressionMatchIterator it = reg.globalMatch(n->name());
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            QString param = match.captured().remove(QStringLiteral("[[")).remove(QStringLiteral("]]"));
            if (!substitute.contains(param)) {
                substitute << param;
            }
        }
        QTextEdit e(n->description());
        it = reg.globalMatch(e.toPlainText());
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            QString param = match.captured().remove(QStringLiteral("[[")).remove(QStringLiteral("]]"));
            if (!substitute.contains(param)) {
                substitute << param;
            }
        }
    }
    QMap<QString, QString> params;
    if (!substitute.isEmpty()) {
        ParameterSubstitutionDialog dlg(substitute);
        dlg.setCaption(xi18nc("@title:window", "Task Module Parameters"));
        if (!dlg.exec()) {
            return false;
        }
        params = dlg.parameters();
    }
    if (!params.isEmpty()) {
        for (auto n : nodes) {
            auto name = n->name();
            for (QMap<QString, QString>::const_iterator it = params.constBegin(); it != params.constEnd(); ++it) {
                name.replace(QStringLiteral("[[") + it.key() + QStringLiteral("]]"), it.value());
            }
            n->setName(name);

            bool changed = false;
            QTextEdit e(n->description());
            for (QMap<QString, QString>::const_iterator it = params.constBegin(); it != params.constEnd(); ++it) {
                QString param = QStringLiteral("[[%1]]").arg(it.key());
                while (e.find(param, QTextDocument::FindCaseSensitively)) {
                    e.textCursor().insertText(it.value());
                    changed = true;
                }
                e.moveCursor(QTextCursor::Start);
            }
            if (changed) {
                n->setDescription(e.toHtml());
            }
        }
    }
    QApplication::setOverrideCursor(Qt::WaitCursor);
    KUndo2Command *cmd = new InsertProjectCmd(moduleProject, n, n->childNode(row), kundo2_i18n("Insert task module"));
    Q_EMIT executeCommand(cmd);
    QApplication::restoreOverrideCursor();
    return true;
}


bool NodeItemModel::dropUrlMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    if (data->hasUrls()) {
        const QList<QUrl> urls = data->urls();
        debugPlan<<urls;
        for (const QUrl &url : urls) {
            const QMimeType mime = QMimeDatabase().mimeTypeForUrl(url);
            debugPlan<<url<<mime.name();
            if (mime.inherits(QStringLiteral("application/x-vnd.kde.plan"))) {
                importProjectFile(url, action, row, column, parent);
            }
        }
        return true;
    }
    return false;
}

bool NodeItemModel::importProjectFile(const QUrl &url, Qt::DropAction /*action*/, int row, int /*column*/, const QModelIndex &parent)
{
    if (! url.isLocalFile()) {
        debugPlan<<"TODO: download if url not local";
        return false;
    }
    KoStore *store = KoStore::createStore(url.path(), KoStore::Read, "", KoStore::Auto);
    if (store->bad()) {
        //        d->lastErrorMessage = i18n("Not a valid Calligra file: %1", file);
        debugPlan<<"bad store"<<url.toDisplayString();
        delete store;
        //        QApplication::restoreOverrideCursor();
        return false;
    }
    if (! store->open("root")) { // maindoc.xml
        debugPlan<<"No root"<<url.toDisplayString();
        delete store;
        return false;
    }
    KoXmlDocument doc;
    doc.setContent(store->device());
    KoXmlElement element = doc.documentElement().namedItem("project").toElement();
    Project project;
    XMLLoaderObject status;
    status.setVersion(doc.documentElement().attribute(QStringLiteral("version"), PLAN_FILE_SYNTAX_VERSION));
    status.setProject(&project);
    if (!status.loadProject(&project, doc)) {
        debugPlan<<"Failed to load project from:"<<url;
        return false;
    }
    project.generateUniqueNodeIds();
    Node *n = node(parent);
    debugPlan<<n<<parent;
    if (n == nullptr) {
        n = m_project;
    }
    KUndo2Command *cmd = new InsertProjectCmd(project, n, n->childNode(row - 1), kundo2_i18n("Insert %1", url.fileName()));
    Q_EMIT executeCommand(cmd);
    return true;
}

KUndo2Command *NodeItemModel::createAllocationCommand(Task &task, const QList<Resource*> &lst)
{
    MacroCommand *cmd = new MacroCommand(kundo2_i18n("Modify resource allocations"));
    const QList<Resource*> resources = task.requestedResources();
    for (Resource *r : lst) {
        if (resources.contains(r)) {
            continue;
        }
        cmd->addCommand(new AddResourceRequestCmd(&task.requests(), new ResourceRequest(r, 100)));
    }
    for (Resource *r : resources) {
        if (!lst.contains(r)) {
            ResourceRequest *rr = task.requests().find(r);
            if (rr) {
                cmd->addCommand(new RemoveResourceRequestCmd(rr));
            }
        }
    }
    if (cmd->isEmpty()) {
        delete cmd;
        return nullptr;
    }
    return cmd;
}

bool NodeItemModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    debugPlan<<action<<parent<<row<<column<<'\n'<<data->formats();

    if (action == Qt::IgnoreAction) {
        return true;
    }
    if (data->hasFormat(QStringLiteral("application/x-vnd.kde.plan.resourceitemmodel.internal"))) {
        return dropResourceMimeData(data, action, row, column, parent);
    }
    if (data->hasFormat(QStringLiteral("application/x-vnd.kde.plan.nodeitemmodel.internal"))) {
        if (action == Qt::MoveAction) {
            //debugPlan<<"MoveAction";

            QByteArray encodedData = data->data(QStringLiteral("application/x-vnd.kde.plan.nodeitemmodel.internal"));
            QDataStream stream(&encodedData, QIODevice::ReadOnly);
            Node *par = nullptr;
            if (parent.isValid()) {
                par = node(parent);
            } else {
                par = m_project;
            }
            QList<Node*> lst = nodeList(stream);
            const QList<Node*> nodes = removeChildNodes(lst); // children goes with their parent
            for (Node *n : nodes) {
                if (! m_project->canMoveTask(n, par)) {
                    //debugPlan<<"Can't move task:"<<n->name();
                    return false;
                }
            }
            int offset = 0;
            MacroCommand *cmd = nullptr;
            for (Node *n : nodes) {
                if (cmd == nullptr) cmd = new MacroCommand(kundo2_i18n("Move tasks"));
                // append nodes if dropped *on* another node, insert if dropped *after*
                int pos = row == -1 ? -1 : row + offset;
                if (pos >= 0 && n->parentNode() == par && par->indexOf(n) < pos) {
                    --pos;
                }
                if (n->parentNode() == par) {
                    // avoid drop into the same position, QAbstractItemModel does not like it
                    int crow = par->indexOf(n);
                    if (((pos == -1) && (crow == par->numChildren() - 1)) || (pos == crow)) {
                        delete cmd;
                        cmd = nullptr;
                        continue;
                    }
                }
                cmd->addCommand(new NodeMoveCmd(m_project, n, par, pos));
                offset++;
            }
            if (cmd) {
                Q_EMIT executeCommand(cmd);
            }
            //debugPlan<<row<<","<<column<<" parent="<<parent.row()<<","<<parent.column()<<":"<<par->name();
            return true;
        }
    }
    if (data->hasFormat(QStringLiteral("application/x-vnd.kde.plan.project"))) {
        debugPlan;
        return dropProjectMimeData(data, action, row, column, parent);

    }
    if (data->hasFormat(QStringLiteral("application/x-vnd.kde.plan.taskmodule"))) {
        debugPlan;
        return dropTaskModuleMimeData(data, action, row, column, parent);

    }
    if (data->hasUrls()) {
        return dropUrlMimeData(data, action, row, column, parent);
    }
    return false;
}

Node *NodeItemModel::node(const QModelIndex &index) const
{
    Node *n = m_project;
    if (index.isValid()) {
        //debugPlan<<index.internalPointer()<<":"<<index.row()<<","<<index.column();
        n = static_cast<Node*>(index.internalPointer());
        Q_ASSERT(n);
    }
    return n;
}

void NodeItemModel::slotNodeChanged(Node *node, int property)
{
    Q_UNUSED(property)
    if (node == nullptr || (! m_projectshown && node->type() == Node::Type_Project)) {
        return;
    }
    if (node->type() == Node::Type_Project) {
        Q_EMIT dataChanged(createIndex(0, 0, node), createIndex(0, columnCount()-1, node));
        return;
    }
    int row = node->parentNode()->findChildNode(node);
    Q_ASSERT(row >= 0);
    Q_EMIT dataChanged(createIndex(row, 0, node), createIndex(row, columnCount()-1, node));
}

QModelIndex NodeItemModel::insertTask(Node *node, Node *after)
{
    Q_EMIT executeCommand(new TaskAddCmd(m_project, node, after, kundo2_i18n("Add task")));
    int row = -1;
    if (node->parentNode()) {
        row = node->parentNode()->indexOf(node);
    }
    if (row != -1) {
        //debugPlan<<"Inserted: "<<node->name()<<"; "<<row;
        return createIndex(row, 0, node);
    }
    //debugPlan<<"Can't find "<<node->name();
    return QModelIndex();
}

QModelIndex NodeItemModel::insertSubtask(Node *node, Node *parent)
{
    Q_EMIT executeCommand(new SubtaskAddCmd(m_project, node, parent, kundo2_i18n("Add sub-task")));
    int row = -1;
    if (node->parentNode()) {
        row = node->parentNode()->indexOf(node);
    }
    if (row != -1) {
        //debugPlan<<node->parentNode()<<" inserted: "<<node->name()<<"; "<<row;
        return createIndex(row, 0, node);
    }
    //debugPlan<<"Can't find "<<node->name();
    return QModelIndex();
}

int NodeItemModel::sortRole(int column) const
{
    int v = Qt::DisplayRole;
    switch (column) {
        case NodeModel::NodeStartTime:
        case NodeModel::NodeEndTime:
        case NodeModel::NodeActualStart:
        case NodeModel::NodeActualFinish:
        case NodeModel::NodeEarlyStart:
        case NodeModel::NodeEarlyFinish:
        case NodeModel::NodeLateStart:
        case NodeModel::NodeLateFinish:
        case NodeModel::NodeConstraintStart:
        case NodeModel::NodeConstraintEnd:
            v = Qt::EditRole;
            break;
        case NodeModel::NodeWBSCode:
            v = NodeModel::SortableRole;
            break;
        default:
            break;
    }
    debugPlan<<column<<v;
    return v;
}

//------------------------------------------------
GanttItemModel::GanttItemModel(QObject *parent)
    : NodeItemModel(parent),
    m_showSpecial(false)
{
}

GanttItemModel::~GanttItemModel()
{
    QList<void*> lst = parentmap.values();
    while (! lst.isEmpty())
        delete (int*)(lst.takeFirst());
}

int GanttItemModel::rowCount(const QModelIndex &parent) const
{
    if (m_showSpecial) {
        if (parentmap.values().contains(parent.internalPointer())) { // clazy:exclude=container-anti-pattern
            return 0;
        }
        Node *n = node(parent);
        if (n && n->type() == Node::Type_Task) {
            return 5; // the task + early start + late finish ++
        }
    }
    return NodeItemModel::rowCount(parent);
}

QModelIndex GanttItemModel::index(int row, int column, const QModelIndex &parent) const
{
    if (m_showSpecial && parent.isValid()) {
        Node *p = node(parent);
        if (p->type() == Node::Type_Task) {
            void *v = nullptr;
            const QList<void*> parents = parentmap.values(p);
            for (void *i : parents) {
                if (*((int*)(i)) == row) {
                    v = i;
                    break;
                }
            }
            if (v == nullptr) {
                v = new int(row);
                const_cast<GanttItemModel*>(this)->parentmap.insert(p, v);
            }
            return createIndex(row, column, v);
        }
    }
    return NodeItemModel::index(row, column, parent);
}

QModelIndex GanttItemModel::parent(const QModelIndex &idx) const
{
    if (m_showSpecial) {
        QList<Node*> lst = parentmap.keys(idx.internalPointer());
        if (! lst.isEmpty()) {
            Q_ASSERT(lst.count() == 1);
            return index(lst.first());
        }
    }
    return NodeItemModel::parent(idx);
}

QVariant GanttItemModel::data(const QModelIndex &index, int role) const
{
    if (! index.isValid()) {
        return QVariant();
    }
    if (role == Qt::TextAlignmentRole) {
        return headerData(index.column(), Qt::Horizontal, role);
    }
    QModelIndex idx = index;
    QList<Node*> lst;
    if (m_showSpecial) {
        lst = parentmap.keys(idx.internalPointer());
    }
    if (! lst.isEmpty()) {
        Q_ASSERT(lst.count() == 1);
        int row = *((int*)(idx.internalPointer()));
        Node *n = lst.first();
        if (role == SpecialItemTypeRole) {
            return row; // 0=task, 1=early start, 2=late finish...
        }
        switch (row) {
            case 0:  // the task
                if (idx.column() == NodeModel::NodeType && role == KGantt::ItemTypeRole) {
                    switch (n->type()) {
                        case Node::Type_Task: return KGantt::TypeTask;
                        default: break;
                    }
                }
                break;
            case 1: { // early start
                if (role != Qt::DisplayRole && role != Qt::EditRole && role != KGantt::ItemTypeRole) {
                    return QVariant();
                }
                switch (idx.column()) {
                    case NodeModel::NodeName: return QStringLiteral("Early Start");
                    case NodeModel::NodeType: return KGantt::TypeEvent;
                    case NodeModel::NodeStartTime:
                    case NodeModel::NodeEndTime: return n->earlyStart(id());
                    default: break;
                }
                return QVariant();
            }
            case 2: { // late finish
                if (role != Qt::DisplayRole && role != Qt::EditRole && role != KGantt::ItemTypeRole) {
                    return QVariant();
                }
                switch (idx.column()) {
                    case NodeModel::NodeName: return QStringLiteral("Late Finish");
                    case NodeModel::NodeType: return KGantt::TypeEvent;
                    case NodeModel::NodeStartTime:
                    case NodeModel::NodeEndTime: return n->lateFinish(id());
                    default: break;
                }
                return QVariant();
            }
            case 3: { // late start
                if (role != Qt::DisplayRole && role != Qt::EditRole && role != KGantt::ItemTypeRole) {
                    return QVariant();
                }
                switch (idx.column()) {
                    case NodeModel::NodeName: return QStringLiteral("Late Start");
                    case NodeModel::NodeType: return KGantt::TypeEvent;
                    case NodeModel::NodeStartTime:
                    case NodeModel::NodeEndTime: return n->lateStart(id());
                    default: break;
                }
                return QVariant();
            }
            case 4: { // early finish
                if (role != Qt::DisplayRole && role != Qt::EditRole && role != KGantt::ItemTypeRole) {
                    return QVariant();
                }
                switch (idx.column()) {
                    case NodeModel::NodeName: return QStringLiteral("Early Finish");
                    case NodeModel::NodeType: return KGantt::TypeEvent;
                    case NodeModel::NodeStartTime:
                    case NodeModel::NodeEndTime: return n->earlyFinish(id());
                    default: break;
                }
                return QVariant();
            }
            default: return QVariant();
        }
        idx = createIndex(idx.row(), idx.column(), n);
    } else {
        if (role == SpecialItemTypeRole) {
            return 0; // task of some type
        }
        if (idx.column() == NodeModel::NodeType && role == KGantt::ItemTypeRole) {
            QModelIndex notScheduled = idx.sibling(idx.row(), NodeModel::NodeNotScheduled);
            if (notScheduled.data(Qt::EditRole).toBool()) {
                return QVariant();
            }
            QVariant result = NodeItemModel::data(idx, Qt::EditRole);
            switch (result.toInt()) {
                case Node::Type_Project: return KGantt::TypeSummary;
                case Node::Type_Summarytask: return KGantt::TypeSummary;
                case Node::Type_Milestone: return KGantt::TypeEvent;
                default: return m_showSpecial ? KGantt::TypeMulti : KGantt::TypeTask;
            }
        }
    }
    return NodeItemModel::data(idx, role);
}

//----------------------------
MilestoneItemModel::MilestoneItemModel(QObject *parent)
    : ItemModelBase(parent)
{
}

MilestoneItemModel::~MilestoneItemModel()
{
}

QList<Node*> MilestoneItemModel::mileStones() const
{
    QList<Node*> lst;
    for (Node* n : std::as_const(m_nodemap)) {
        if (n->type() == Node::Type_Milestone) {
            lst << n;
        }
    }
    return lst;
}

void MilestoneItemModel::slotNodeToBeInserted(Node *parent, int row)
{
    Q_UNUSED(parent);
    Q_UNUSED(row);
}

void MilestoneItemModel::slotNodeInserted(Node *node)
{
    Q_UNUSED(node);
    resetModel();
}

void MilestoneItemModel::slotNodeToBeRemoved(Node *node)
{
    Q_UNUSED(node);
    //debugPlan<<node->name();
/*   int row = m_nodemap.values().indexOf(node);
    if (row != -1) {
        Q_ASSERT(m_nodemap.contains(node->wbsCode()));
        Q_ASSERT(m_nodemap.keys().indexOf(node->wbsCode()) == row);
        beginRemoveRows(QModelIndex(), row, row);
        m_nodemap.remove(node->wbsCode());
        endRemoveRows();
    }*/
}

void MilestoneItemModel::slotNodeRemoved(Node *node)
{
    Q_UNUSED(node);
    resetModel();
    //endRemoveRows();
}

void MilestoneItemModel::slotLayoutChanged()
{
    //debugPlan<<node->name();
    Q_EMIT layoutAboutToBeChanged();
    Q_EMIT layoutChanged();
}

void MilestoneItemModel::slotNodeToBeMoved(Node *node, int pos, Node *newParent, int newPos)
{
    Q_UNUSED(node);
    Q_UNUSED(pos);
    Q_UNUSED(newParent);
    Q_UNUSED(newPos);
}

void MilestoneItemModel::slotNodeMoved(Node *node)
{
    Q_UNUSED(node);
    resetModel();
}

void MilestoneItemModel::setProject(Project *project)
{
    if (m_project) {
        disconnect(m_project, &Project::aboutToBeDeleted, this, &MilestoneItemModel::projectDeleted);
        disconnect(m_project, SIGNAL(localeChanged()), this, SLOT(slotLayoutChanged()));
        disconnect(m_project, &Project::wbsDefinitionChanged, this, &MilestoneItemModel::slotWbsDefinitionChanged);
        disconnect(m_project, &Project::nodeChanged, this, &MilestoneItemModel::slotNodeChanged);
        disconnect(m_project, &Project::nodeToBeAdded, this, &MilestoneItemModel::slotNodeToBeInserted);
        disconnect(m_project, &Project::nodeToBeRemoved, this, &MilestoneItemModel::slotNodeToBeRemoved);

        disconnect(m_project, &Project::nodeToBeMoved, this, &MilestoneItemModel::slotNodeToBeMoved);
        disconnect(m_project, &Project::nodeMoved, this, &MilestoneItemModel::slotNodeMoved);

        disconnect(m_project, &Project::nodeAdded, this, &MilestoneItemModel::slotNodeInserted);
        disconnect(m_project, &Project::nodeRemoved, this, &MilestoneItemModel::slotNodeRemoved);
    }
    m_project = project;
    //debugPlan<<m_project<<"->"<<project;
    m_nodemodel.setProject(project);
    if (project) {
        connect(m_project, &Project::aboutToBeDeleted, this, &MilestoneItemModel::projectDeleted);
        connect(m_project, SIGNAL(localeChanged()), this, SLOT(slotLayoutChanged()));
        connect(m_project, &Project::wbsDefinitionChanged, this, &MilestoneItemModel::slotWbsDefinitionChanged);
        connect(m_project, &Project::nodeChanged, this, &MilestoneItemModel::slotNodeChanged);
        connect(m_project, &Project::nodeToBeAdded, this, &MilestoneItemModel::slotNodeToBeInserted);
        connect(m_project, &Project::nodeToBeRemoved, this, &MilestoneItemModel::slotNodeToBeRemoved);

        connect(m_project, &Project::nodeToBeMoved, this, &MilestoneItemModel::slotNodeToBeMoved);
        connect(m_project, &Project::nodeMoved, this, &MilestoneItemModel::slotNodeMoved);

        connect(m_project, &Project::nodeAdded, this, &MilestoneItemModel::slotNodeInserted);
        connect(m_project, &Project::nodeRemoved, this, &MilestoneItemModel::slotNodeRemoved);
    }
    resetModel();
}

void MilestoneItemModel::setScheduleManager(ScheduleManager *sm)
{
    if (m_nodemodel.manager()) {
    }
    m_nodemodel.setManager(sm);
    ItemModelBase::setScheduleManager(sm);
    if (sm) {
    }
    //debugPlan<<sm;
    resetModel();
}

bool MilestoneItemModel::resetData()
{
    int cnt = m_nodemap.count();
    m_nodemap.clear();
    if (m_project != nullptr) {
        const QList<Node*> nodes = m_project->allNodes();
        for (Node *n : nodes) {
            m_nodemap.insert(n->wbsCode(true), n);
        }
    }
    return cnt != m_nodemap.count();
}

void MilestoneItemModel::resetModel()
{
    beginResetModel();
    resetData();
    endResetModel();
}

Qt::ItemFlags MilestoneItemModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = QAbstractItemModel::flags(index);
    if (!index.isValid()) {
        if (m_readWrite) {
            flags |= Qt::ItemIsDropEnabled;
        }
        return flags;
    }
    flags |= Qt::ItemIsDragEnabled;
    if (m_readWrite) {
        flags |= Qt::ItemIsDropEnabled;
        switch (index.column()) {
            case NodeModel::NodeName: // name
                if (!isColumnReadOnly(index.column())) {
                    flags |= Qt::ItemIsEditable;
                }
                break;
            case NodeModel::NodeType: break; // Node type
            case NodeModel::NodeResponsible: // Responsible
                if (!isColumnReadOnly(index.column())) {
                    flags |= Qt::ItemIsEditable;
                }
                break;
            case NodeModel::NodeConstraint: // constraint type
                if (!isColumnReadOnly(index.column())) {
                    flags |= Qt::ItemIsEditable;
                }
                break;
            case NodeModel::NodeConstraintStart: { // constraint start
                if (!isColumnReadOnly(index.column())) {
                    Node *n = node(index);
                    if (n == nullptr)
                        break;
                    int c = n->constraint();
                    if (c == Node::MustStartOn || c == Node::StartNotEarlier || c == Node::FixedInterval) {
                        flags |= Qt::ItemIsEditable;
                    }
                }
                break;
            }
            case NodeModel::NodeConstraintEnd: { // constraint end
                if (!isColumnReadOnly(index.column())) {
                    Node *n = node(index);
                    if (n == nullptr)
                        break;
                    int c = n->constraint();
                    if (c == Node::MustFinishOn || c == Node::FinishNotLater || c ==  Node::FixedInterval) {
                        flags |= Qt::ItemIsEditable;
                    }
                }
                break;
            }
            case NodeModel::NodeStartupAccount: // startup account
            case NodeModel::NodeStartupCost: // startup cost
            case NodeModel::NodeShutdownAccount: // shutdown account
            case NodeModel::NodeShutdownCost: { // shutdown cost
                if (!isColumnReadOnly(index.column())) {
                    Node *n = node(index);
                    if (n && (n->type() == Node::Type_Task || n->type() == Node::Type_Milestone)) {
                        flags |= Qt::ItemIsEditable;
                    }
                }
                break;
            }
            case NodeModel::NodeDescription: // description
                break;
            case NodeModel::NodeCompleted:
                if (!isColumnReadOnly(index.column())) {
                    flags |= Qt::ItemIsEditable;
                }
                break;
            default:
                flags &= ~Qt::ItemIsEditable;
        }
    }
    return flags;
}

QModelIndex MilestoneItemModel::parent(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return QModelIndex();
}

QModelIndex MilestoneItemModel::index(int row, int column, const QModelIndex &parent) const
{
    //debugPlan<<parent<<row<<", "<<m_nodemap.count();
    if (m_project == nullptr || row < 0 || column < 0) {
        //debugPlan<<"No project"<<m_project<<" or illegal row, column"<<row<<column;
        return QModelIndex();
    }
    if (parent.isValid() || row >= m_nodemap.count()) {
        //debugPlan<<"No index for"<<parent<<row<<","<<column;
        return QModelIndex();
    }
    return createIndex(row, column, m_nodemap.values().at(row)); // clazy:exclude=container-anti-pattern
}

QModelIndex MilestoneItemModel::index(const Node *node) const
{
    if (m_project == nullptr || node == nullptr) {
        return QModelIndex();
    }
    return createIndex(m_nodemap.values().indexOf(const_cast<Node*>(node)), 0, const_cast<Node*>(node)); // clazy:exclude=container-anti-pattern
}


QVariant MilestoneItemModel::data(const QModelIndex &index, int role) const
{
    QVariant result;
    if (role == Qt::TextAlignmentRole) {
        return headerData(index.column(), Qt::Horizontal, role);
    }
    Node *n = node(index);
    if (role == Role::Object) {
        return n ? QVariant::fromValue(static_cast<QObject*>(n)) : QVariant();
    }
    if (n != nullptr) {
        if (index.column() == NodeModel::NodeType && role == KGantt::ItemTypeRole) {
            result = m_nodemodel.data(n, index.column(), Qt::EditRole);
            switch (result.toInt()) {
                case Node::Type_Summarytask: return KGantt::TypeSummary;
                case Node::Type_Milestone: return KGantt::TypeEvent;
                default: return KGantt::TypeTask;
            }
            return result;
        }
    }
    result = m_nodemodel.data(n, index.column(), role);
    return result;
}

bool MilestoneItemModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if ((flags(index) &Qt::ItemIsEditable) == 0 || role != Qt::EditRole) {
        return false;
    }
    switch (index.column()) {
        case NodeModel::NodeCompleted: {
            auto n = node(index);
            if (n->type() == Node::Type_Milestone) {
                Completion &c = static_cast<Task*>(n)->completion();
                if (value.toInt() > 0) {
                    QDateTime dt = QDateTime::currentDateTime();
                    QDate date = dt.date();
                    MacroCommand *m = new MacroCommand(kundo2_i18n("Set finished"));
                    m->addCommand(new ModifyCompletionStartTimeCmd(c, dt));
                    m->addCommand(new ModifyCompletionStartedCmd(c, true));
                    m->addCommand(new ModifyCompletionFinishTimeCmd(c, dt));
                    m->addCommand(new ModifyCompletionFinishedCmd(c, true));
                    m->addCommand(new ModifyCompletionPercentFinishedCmd(c, date, 100));
                    Q_EMIT executeCommand(m); // also adds a new entry if necessary
                    return true;
                }
            }
            return false;
        }
        default:
            qWarning("data: invalid display value column %d", index.column());
            return false;
    }
    return false;
}

QVariant MilestoneItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal) {
        if (role == Qt::DisplayRole || role == Qt::TextAlignmentRole || role == Qt::EditRole) {
            return m_nodemodel.headerData(section, role);
        }
    }
    if (role == Qt::ToolTipRole) {
        return NodeModel::headerData(section, role);
    }
    return ItemModelBase::headerData(section, orientation, role);
}

QAbstractItemDelegate *MilestoneItemModel::createDelegate(int column, QWidget *parent) const
{
    switch (column) {
        case NodeModel::NodeEstimateType: return new EnumDelegate(parent);
        case NodeModel::NodeEstimateCalendar: return new EnumDelegate(parent);
        case NodeModel::NodeEstimate: return new DurationSpinBoxDelegate(parent);
        case NodeModel::NodeOptimisticRatio: return new SpinBoxDelegate(parent);
        case NodeModel::NodePessimisticRatio: return new SpinBoxDelegate(parent);
        case NodeModel::NodeRisk: return new EnumDelegate(parent);
        case NodeModel::NodeConstraint: return new EnumDelegate(parent);
        case NodeModel::NodeRunningAccount: return new EnumDelegate(parent);
        case NodeModel::NodeStartupAccount: return new EnumDelegate(parent);
        case NodeModel::NodeStartupCost: return new MoneyDelegate(parent);
        case NodeModel::NodeShutdownAccount: return new EnumDelegate(parent);
        case NodeModel::NodeShutdownCost: return new MoneyDelegate(parent);

        case NodeModel::NodeCompleted: return new TaskCompleteDelegate(parent);
        case NodeModel::NodeRemainingEffort: return new DurationSpinBoxDelegate(parent);
        case NodeModel::NodeActualEffort: return new DurationSpinBoxDelegate(parent);

        default: return nullptr;
    }
    return nullptr;
}

int MilestoneItemModel::columnCount(const QModelIndex &/*parent*/) const
{
    return m_nodemodel.propertyCount();
}

int MilestoneItemModel::rowCount(const QModelIndex &parent) const
{
    //debugPlan<<parent;
    if (parent.isValid()) {
        return 0;
    }
    //debugPlan<<m_nodemap.count();
    return m_nodemap.count();
}

Qt::DropActions MilestoneItemModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}


QStringList MilestoneItemModel::mimeTypes() const
{
    return QStringList();
}

QMimeData *MilestoneItemModel::mimeData(const QModelIndexList & indexes) const
{
    QMimeData *m = new QMimeData();
    QByteArray encodedData;
    QDataStream stream(&encodedData, QIODevice::WriteOnly);
    QList<int> rows;
    for (const QModelIndex &index : indexes) {
        if (index.isValid() && !rows.contains(index.row())) {
            //debugPlan<<index.row();
            Node *n = node(index);
            if (n) {
                rows << index.row();
                stream << n->id();
            }
        }
    }
    m->setData(QStringLiteral("application/x-vnd.kde.plan.nodeitemmodel.internal"), encodedData);
    return m;
}

bool MilestoneItemModel::dropAllowed(const QModelIndex &index, int dropIndicatorPosition, const QMimeData *data)
{
    //debugPlan;
    Node *dn = node(index);
    if (dn == nullptr) {
        errorPlan<<"no node to drop on!";
        return false; // hmmm
    }
    switch (dropIndicatorPosition) {
        case ItemModelBase::AboveItem:
        case ItemModelBase::BelowItem:
            // dn == sibling
            return dropAllowed(dn->parentNode(), data);
        case ItemModelBase::OnItem:
            // dn == new parent
            return dropAllowed(dn, data);
        default:
            break;
    }
    return false;
}

bool MilestoneItemModel::dropAllowed(Node *on, const QMimeData *data)
{
    if (!data->hasFormat(QStringLiteral("application/x-vnd.kde.plan.nodeitemmodel.internal"))) {
        return false;
    }
    if (on == m_project) {
        return true;
    }
    QByteArray encodedData = data->data(QStringLiteral("application/x-vnd.kde.plan.nodeitemmodel.internal"));
    QDataStream stream(&encodedData, QIODevice::ReadOnly);
    QList<Node*> lst = nodeList(stream);
    for (Node *n : std::as_const(lst)) {
        if (on == n || on->isChildOf(n)) {
            return false;
        }
    }
    lst = removeChildNodes(lst);
    for (Node *n : std::as_const(lst)) {
        if (! m_project->canMoveTask(n, on)) {
            return false;
        }
    }
    return true;
}

QList<Node*> MilestoneItemModel::nodeList(QDataStream &stream)
{
    QList<Node*> lst;
    while (!stream.atEnd()) {
        QString id;
        stream >> id;
        Node *node = m_project->findNode(id);
        if (node) {
            lst << node;
        }
    }
    return lst;
}

QList<Node*> MilestoneItemModel::removeChildNodes(const QList<Node*> &nodes)
{
    QList<Node*> lst;
    for (Node *node : nodes) {
        bool ins = true;
        for (Node *n : nodes) {
            if (node->isChildOf(n)) {
                //debugPlan<<node->name()<<" is child of"<<n->name();
                ins = false;
                break;
            }
        }
        if (ins) {
            //debugPlan<<" insert"<<node->name();
            lst << node;
        }
    }
    const QList<Node*> nl = lst;
    const QList<Node*> nlst = lst;
    for (Node *node : nl) {
        for (Node *n : nlst) {
            if (n->isChildOf(node)) {
                //debugPlan<<n->name()<<" is child of"<<node->name();
                int i = nodes.indexOf(n);
                lst.removeAt(i);
            }
        }
    }
    return lst;
}

bool MilestoneItemModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int /*column*/, const QModelIndex &parent)
{
    //debugPlan<<action;
    if (action == Qt::IgnoreAction) {
        return true;
    }
    if (!data->hasFormat(QStringLiteral("application/x-vnd.kde.plan.nodeitemmodel.internal"))) {
        return false;
    }
    if (action == Qt::MoveAction) {
        //debugPlan<<"MoveAction";

        QByteArray encodedData = data->data(QStringLiteral("application/x-vnd.kde.plan.nodeitemmodel.internal"));
        QDataStream stream(&encodedData, QIODevice::ReadOnly);
        Node *par = nullptr;
        if (parent.isValid()) {
            par = node(parent);
        } else {
            par = m_project;
        }
        const QList<Node*> lst = nodeList(stream);
        const QList<Node*> nodes = removeChildNodes(lst); // children goes with their parent
        for (Node *n : nodes) {
            if (! m_project->canMoveTask(n, par)) {
                //debugPlan<<"Can't move task:"<<n->name();
                return false;
            }
        }
        int offset = 0;
        MacroCommand *cmd = nullptr;
        for (Node *n : nodes) {
            if (cmd == nullptr) cmd = new MacroCommand(kundo2_i18n("Move tasks"));
            // append nodes if dropped *on* another node, insert if dropped *after*
            int pos = row == -1 ? -1 : row + offset;
            cmd->addCommand(new NodeMoveCmd(m_project, n, par, pos));
            offset++;
        }
        if (cmd) {
            Q_EMIT executeCommand(cmd);
        }
        //debugPlan<<row<<","<<column<<" parent="<<parent.row()<<","<<parent.column()<<":"<<par->name();
        return true;
    }
    return false;
}

Node *MilestoneItemModel::node(const QModelIndex &index) const
{
    Node *n = nullptr;
    if (index.isValid()) {
        //debugPlan<<index;
        n = static_cast<Node*>(index.internalPointer());
    }
    return n;
}

void MilestoneItemModel::slotNodeChanged(Node *node)
{
    //debugPlan<<node->name();
    if (node == nullptr) {
        return;
    }
    beginResetModel();
    resetData();
    endResetModel();
}

void MilestoneItemModel::slotWbsDefinitionChanged()
{
    //debugPlan;
    if (m_project == nullptr) {
        return;
    }
    if (! m_nodemap.isEmpty()) {
        beginResetModel();
        resetData();
        endResetModel();
    }
}

int MilestoneItemModel::sortRole(int column) const
{
    int v = Qt::DisplayRole;
    switch (column) {
        case NodeModel::NodeStartTime:
        case NodeModel::NodeEndTime:
        case NodeModel::NodeActualStart:
        case NodeModel::NodeActualFinish:
        case NodeModel::NodeEarlyStart:
        case NodeModel::NodeEarlyFinish:
        case NodeModel::NodeLateStart:
        case NodeModel::NodeLateFinish:
        case NodeModel::NodeConstraintStart:
        case NodeModel::NodeConstraintEnd:
            v = Qt::EditRole;
            break;
        case NodeModel::NodeWBSCode:
            v = NodeModel::SortableRole;
            break;
        default:
            break;
    }
    return v;
}

//--------------
NodeSortFilterProxyModel::NodeSortFilterProxyModel(ItemModelBase* model, QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setSourceModel(model);
    setDynamicSortFilter(true);
    m_invalidateFilter.setInterval(100);
    m_invalidateFilter.setSingleShot(true);
    connect(&m_invalidateFilter, &QTimer::timeout, this, [this]() {
        invalidateFilter();
    });
}

ItemModelBase *NodeSortFilterProxyModel::itemModel() const
{
    return static_cast<ItemModelBase *>(sourceModel());
}

bool NodeSortFilterProxyModel::showSummarytasks() const
{
    return m_showSummarytasks;
}

void NodeSortFilterProxyModel::setShowSummarytasks(bool on)
{
    m_showSummarytasks = on;
    setFilterInvalid();
}

bool NodeSortFilterProxyModel::tasksAndMilestonesGroupEnabled() const
{
    return m_tasksAndMilestonesGroupEnabled;
}

void NodeSortFilterProxyModel::setTasksAndMilestonesGroupEnabled(bool on)
{
    m_tasksAndMilestonesGroupEnabled = on;
    setFilterInvalid();
}

bool NodeSortFilterProxyModel::showProject() const
{
    return m_model->projectShown();
}

void NodeSortFilterProxyModel::setShowProject(bool on)
{
    m_model->setShowProject(on);
}

bool NodeSortFilterProxyModel::showUnscheduled() const
{
    return m_showUnscheduled;
}

void NodeSortFilterProxyModel::setShowUnscheduled(bool on)
{
    m_showUnscheduled = on;
    if (m_tasksAndMilestonesGroupEnabled) {
        setFilterInvalid();
    }
}

bool NodeSortFilterProxyModel::periodGroupEnabled() const
{
    return m_periodGroupEnabled;
}

void NodeSortFilterProxyModel::setPeriodGroupEnabled(bool on)
{
    m_periodGroupEnabled = on;
    setFilterInvalid();
}

void NodeSortFilterProxyModel::setShowTasks(bool on)
{
    m_showTasks = on;
    if (m_tasksAndMilestonesGroupEnabled) {
        setFilterInvalid();
    }
}

bool NodeSortFilterProxyModel::showTasks() const
{
    return m_showTasks;
}

void NodeSortFilterProxyModel::setShowMilestones(bool on)
{
    m_showMilestones = on;
    if (m_tasksAndMilestonesGroupEnabled) {
        setFilterInvalid();
    }
}

bool NodeSortFilterProxyModel::showMilestones() const
{
    return m_showMilestones;
}

bool NodeSortFilterProxyModel::showRunning() const
{
    return m_showRunning;
}

void NodeSortFilterProxyModel::setShowRunning(bool on)
{
    m_showRunning = on;
    if (m_periodGroupEnabled) {
        setFilterInvalid();
    }
}

bool NodeSortFilterProxyModel::showFinished() const
{
    return m_showFinished;
}

void NodeSortFilterProxyModel::setShowFinished(bool on)
{
    m_showFinished = on;
    if (m_periodGroupEnabled) {
        setFilterInvalid();
    }
}

bool NodeSortFilterProxyModel::showNotStarted() const
{
    return m_showNotStarted;
}

void NodeSortFilterProxyModel::setShowNotStarted(bool on)
{
    m_showNotStarted = on;
    if (m_periodGroupEnabled) {
        setFilterInvalid();
    }
}

QDate NodeSortFilterProxyModel::periodStart() const
{
    return m_periodStart;
}

void NodeSortFilterProxyModel::setPeriodStart(const QDate &dt)
{
    m_periodStart = dt;
    if (m_periodGroupEnabled) {
        setFilterInvalid();
    }
}

QDate NodeSortFilterProxyModel::periodEnd() const
{
    return m_periodEnd;
}

void NodeSortFilterProxyModel::setPeriodEnd(const QDate &dt)
{
    m_periodEnd = dt;
    if (m_periodGroupEnabled) {
        setFilterInvalid();
    }
}

void NodeSortFilterProxyModel::setFilterInvalid()
{
    m_invalidateFilter.start();
}

bool NodeSortFilterProxyModel::filterAcceptsRow (int row, const QModelIndex & parent) const
{
    //debugPlan<<sourceModel()<<row<<parent;
    if (itemModel()->project() == nullptr) {
        //debugPlan<<itemModel()->project();
        return false;
    }
    if (!QSortFilterProxyModel::filterAcceptsRow(row, parent)) {
        return false;
    }
    auto node = sourceModel()->data(sourceModel()->index(row, 0, parent), Role::Object).value<Node*>();
    if (node && node->type() == Node::Type_Project) {
        return true; // always show project if it appears here, it is filtered in sourcemodel
    }
    if (!m_showSummarytasks && node && node->type() == Node::Type_Summarytask) {
        return false;
    }
    if (m_tasksAndMilestonesGroupEnabled) {
        if (!node) {
            return true;
        }
        int type = node->type();
        if (!m_showTasks && type == Node::Type_Task) {
            return false;
        }
        if (!m_showMilestones && type == Node::Type_Milestone) {
            return false;
        }
        if (!m_showUnscheduled) {
            QString s = sourceModel()->data(sourceModel()->index(row, NodeModel::NodeNotScheduled, parent), Qt::EditRole).toString();
            if (s == QStringLiteral("true")) {
                //debugPlan<<"Filtered unscheduled:"<<sourceModel()->index(row, 0, parent);
                return false;
            }
        }
    } else {
        QString s = sourceModel()->data(sourceModel()->index(row, NodeModel::NodeNotScheduled, parent), Qt::EditRole).toString();
        if (s == QStringLiteral("true")) {
            //debugPlan<<"Filtered unscheduled:"<<sourceModel()->index(row, 0, parent);
            return false;
        }
    }
    if (m_periodGroupEnabled) {
        auto task = qobject_cast<Task*>(node);
        if (!task) {
            return true;
        }
        if (!m_showRunning && task->completion().isStarted() && !task->completion().isFinished()) {
            return false;
        }
        if (!m_showFinished && task->completion().isFinished()) {
            return false;
        }
        if (!m_showNotStarted && !task->completion().isStarted()) {
            return false;
        }
        const auto idx = sourceModel()->index(row, NodeModel::NodeStartTime, parent);
        auto start = idx.data(Qt::EditRole).toDate();
        if (task->completion().isStarted() && (task->completion().startTime().date() < start)) {
            start = task->completion().startTime().date();
        }
        if (start < m_periodStart || start > m_periodEnd) {
            return false;
        }
        auto end = idx.siblingAtColumn(NodeModel::NodeEndTime).data(Qt::EditRole).toDate();
        if (task->completion().isFinished() && task->completion().finishTime().date() > end) {
            end = task->completion().finishTime().date();
        }
        if (end < m_periodStart || end > m_periodEnd) {
            return false;
        }
    }
    return true;
}

void NodeSortFilterProxyModel::sort(int column, Qt::SortOrder order)
{
    setSortRole(itemModel()->sortRole(column));
    QSortFilterProxyModel::sort(column, order);
}

//------------------
TaskModuleModel::TaskModuleModel(QObject *parent)
    : QAbstractItemModel(parent)
    , m_project(nullptr)
{
}

void TaskModuleModel::setProject(Project *project)
{
    if (m_project) {
        disconnect(m_project, &Project::taskModulesChanged, this, &TaskModuleModel::slotTaskModulesChanged);
    }
    m_project = project;
    if (m_project) {
        connect(m_project, &Project::taskModulesChanged, this, &TaskModuleModel::slotTaskModulesChanged);
    }
    slotReset();
}

void TaskModuleModel::slotReset()
{
    slotTaskModulesChanged(m_project ? m_project->taskModules() : QList<QUrl>());
}

void TaskModuleModel::addTaskModule(Project *project, const QUrl &url)
{
    beginInsertRows(QModelIndex(), m_modules.count(), m_modules.count());
    m_modules << project;
    m_urls << url;
    endInsertRows();
}

Qt::ItemFlags TaskModuleModel::flags(const QModelIndex &idx) const
{
    Qt::ItemFlags f = QAbstractItemModel::flags(idx) | Qt::ItemIsDropEnabled;
    if (idx.isValid()) {
        f |=  Qt::ItemIsDragEnabled;
    }
    return f;
}

int TaskModuleModel::columnCount (const QModelIndex &/*idx*/) const
{
    return 1;
}

int TaskModuleModel::rowCount(const QModelIndex &idx) const
{
    return idx.isValid() ? 0 : m_modules.count();
}

QVariant TaskModuleModel::data(const QModelIndex& idx, int role) const
{
    if (!idx.isValid() || idx.row() >= m_modules.count()) {
        return QVariant();
    }
    switch (role) {
        case Qt::DisplayRole: return m_modules.value(idx.row())->name();
        case Qt::ToolTipRole: return m_modules.value(idx.row())->description();
        case Qt::WhatsThisRole: return QVariant();
        case Qt::UserRole: return m_urls.value(idx.row());
        default: break;
    }
    return QVariant();
}

QVariant TaskModuleModel::headerData(int /*section*/, Qt::Orientation orientation , int role) const
{
    if (orientation == Qt::Horizontal) {
        switch (role) {
            case Qt::DisplayRole: return xi18nc("@title:column", "Name");
            default: break;
        }
    }
    return QVariant();
}

QModelIndex TaskModuleModel::parent(const QModelIndex& /*idx*/) const
{
    return QModelIndex();
}

QModelIndex TaskModuleModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return QModelIndex();
    }
    return createIndex(row, column, m_modules.value(row));
}

QStringList TaskModuleModel::mimeTypes() const
{
    return QStringList() << QStringLiteral("application/x-vnd.kde.plan") << QStringLiteral("text/uri-list");
}

bool TaskModuleModel::dropMimeData(const QMimeData *data, Qt::DropAction /*action*/, int /*row*/, int /*column*/, const QModelIndex &/*parent*/)
{
    if (data->hasUrls()) {
        const QList<QUrl> urls = data->urls();
        debugPlan<<urls;
        for (const QUrl &url : urls) {
            const QMimeType mime = QMimeDatabase().mimeTypeForUrl(url);
            debugPlan<<url<<mime.name();
            if (mime.inherits(QStringLiteral("application/x-vnd.kde.plan")) || mime.inherits(QStringLiteral("application/xml"))) {
                importProject(url);
            }
        }
        return true;
    }
    return false;
}

bool TaskModuleModel::importProject(const QUrl &url, bool emitsignal)
{
    Project *project = loadProjectFromUrl(url);
    if (!project) {
        warnPlan<<Q_FUNC_INFO<<"Failed to load project from:"<<url;
        return false;
    }
    addTaskModule(project, url);
    if (emitsignal) {
        // FIXME: save destroys the project, so give it a copy (see kptview.cpp)
        project = loadProjectFromUrl(url);
        Q_EMIT saveTaskModule(url, project);
    }
    return true;
}

Project *TaskModuleModel::loadProjectFromUrl(const QUrl &url) const
{
    if (! url.isLocalFile()) {
        warnPlan<<Q_FUNC_INFO<<"TODO: download if url not local";
        return nullptr;
    }
    KoStore *store = KoStore::createStore(url.path(), KoStore::Read, "", KoStore::Auto);
    if (store->bad()) {
        //        d->lastErrorMessage = i18n("Not a valid Calligra file: %1", file);
        warnPlan<<Q_FUNC_INFO<<"bad store"<<url.toDisplayString()<<url.path();
        delete store;
        //        QApplication::restoreOverrideCursor();
        return nullptr;
    }
    if (! store->open("root")) { // maindoc.xml
        warnPlan<<Q_FUNC_INFO<<"No root"<<url.toDisplayString();
        delete store;
        return nullptr;
    }
    KoXmlDocument doc;
    doc.setContent(store->device());
    KoXmlElement element = doc.documentElement().namedItem("project").toElement();
    Project *project = new Project();
    XMLLoaderObject status;
    status.setVersion(doc.documentElement().attribute(QStringLiteral("version"), PLAN_FILE_SYNTAX_VERSION));
    status.setProject(project);
    if (status.loadProject(project, doc)) {
        stripProject(project);
    } else {
        warnPlan<<Q_FUNC_INFO<<"Failed to load project from:"<<url;
        delete project;
        return nullptr;
    }
    return project;
}

QMimeData* TaskModuleModel::mimeData(const QModelIndexList &lst) const
{
    QMimeData *mime = new QMimeData();
    if (lst.count() == 1) {
        QModelIndex idx = lst.at(0);
        if (idx.isValid()) {
            QUrl url = m_urls.value(idx.row());
            Project *project = loadProjectFromUrl(url);
            if (project) {
                XmlSaveContext context(project);
                context.save();
                mime->setData(QStringLiteral("application/x-vnd.kde.plan.taskmodule"), context.document.toByteArray());
                delete project;
            }
        }
    }
    return mime;
}

void TaskModuleModel::stripProject(Project *project) const
{
    const QList<ScheduleManager*> managers = project->scheduleManagers();
    for (ScheduleManager *sm : managers) {
        DeleteScheduleManagerCmd c(*project, sm);
    }
}

void TaskModuleModel::loadTaskModules(const QStringList &files)
{
    debugPlan<<files;
    beginResetModel();
    m_modules.clear();
    m_urls.clear();
    for (const QString &file : files) {
        importProject(QUrl::fromLocalFile(file), false);
    }
    endResetModel();
}

void TaskModuleModel::slotTaskModulesChanged(const QList<QUrl> &modules)
{
    debugPlan<<modules;
    beginResetModel();
    m_modules.clear();
    m_urls.clear();
    endResetModel();
    for (const QUrl &url : modules) {
        QDir dir(url.toLocalFile());
        const QStringList files = dir.entryList(QStringList() << QStringLiteral("*.plan"), QDir::Files);
        for (const QString &file : files) {
            QUrl u = QUrl::fromLocalFile(dir.path() + QLatin1Char('/') + file);
            importProject(u, false);
        }
    }
}

} //namespace KPlato
