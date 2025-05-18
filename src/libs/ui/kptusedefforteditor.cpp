/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2007, 2011, 2012 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kptusedefforteditor.h"

#include "kptitemmodelbase.h"

#include <QDate>
#include <QHeaderView>
#include <QLocale>

#include <KLocalizedString>

#include "kptproject.h"
#include "kpttask.h"
#include "kptresource.h"
#include "kptdebug.h"


namespace KPlato
{

UsedEffortItemModel::UsedEffortItemModel (QWidget *parent)
    : QAbstractItemModel(parent),
    m_completion(nullptr),
    m_readonly(false)
{
    m_headers << i18n("Resource");
    QLocale locale;
    for (int i = 1; i <= 7; ++i) {
        m_headers << locale.dayName(i, QLocale::ShortFormat);
    }
    m_headers << i18n("This Week");
}

Qt::ItemFlags UsedEffortItemModel::flags (const QModelIndex &index) const
{
    Qt::ItemFlags flags = QAbstractItemModel::flags(index);
    if (m_readonly || ! index.isValid() || index.column() == 8) {
        return flags;
    }
    if (index.column() == 0) {
        const Resource *r = resource(index);
        if (r) {
            if (m_resourcelist.contains(r) && ! m_completion->usedEffortMap().contains(r)) {
                return flags | Qt::ItemIsEditable;
            }
        }
        return flags;
    }
    return flags | Qt::ItemIsEditable;
}

QVariant UsedEffortItemModel::data (const QModelIndex &index, int role) const
{
    if (! index.isValid()) {
        return QVariant();
    }
    switch (role) {
        case Qt::DisplayRole: {
            if (index.column() == 0) {
                const Resource *r = resource(index);
                //debugPlan<<index.row()<<","<<index.column()<<""<<r;
                if (r) {
                    return r->name();
                }
                break;
            }
            Completion::UsedEffort *ue = usedEffort(index);
            if (ue == nullptr) {
                return QVariant();
            }
            if (index.column() == 8) {
                // Total
                //debugPlan<<index.row()<<","<<index.column()<<" total"<<'\n';
                double res = 0.0;
                for (const QDate &d: std::as_const(m_dates)) {
                    Completion::UsedEffort::ActualEffort e = ue->effort(d);
                    res += e.normalEffort().toDouble(Duration::Unit_h);
                }
                return QLocale().toString(res, 'f',  1);
            }
            Completion::UsedEffort::ActualEffort e = ue->effort(m_dates.value(index.column() - 1));
            double res = e.normalEffort().toDouble(Duration::Unit_h);
            return QLocale().toString(res, 'f',  1);
        }
        case Qt::EditRole: {
            if (index.column() == 8) {
                return QVariant();
            }
            if (index.column() == 0) {
                const Resource *r = resource(index);
                //debugPlan<<index.row()<<","<<index.column()<<" "<<r<<'\n';
                if (r) {
                    return r->name();
                }
            } else {
                Completion::UsedEffort *ue = usedEffort(index);
                if (ue == nullptr) {
                    return QVariant();
                }
                Completion::UsedEffort::ActualEffort e = ue->effort(m_dates.value(index.column() - 1));
                double res = e.normalEffort().toDouble(Duration::Unit_h);
                return QLocale().toString(res, 'f',  1);
            }
            break;
        }
        case Role::EnumList: {
            if (index.column() == 0) {
                QStringList lst = m_editlist.keys();
                return lst;
            }
            break;
        }
        case Role::EnumListValue: {
            if (index.column() == 0) {
                return m_editlist.values().indexOf(resource(index)); // clazy:exclude=container-anti-pattern
            }
            break;
        }
        default: break;
    }
    return QVariant();
}

bool UsedEffortItemModel::setData (const QModelIndex &idx, const QVariant &value, int role)
{
    debugPlan;
    switch (role) {
        case Qt::EditRole: {
            if (idx.column() == 8) {
                return false;
            }
            if (idx.column() == 0) {
                const Resource *er = resource(idx);
                Q_ASSERT(er != nullptr);

                Q_ASSERT (m_editlist.count() > value.toInt());

                const Resource *v = m_editlist.values().value(value.toInt()); // clazy:exclude=container-anti-pattern
                Q_ASSERT(v != nullptr);

                int x = m_resourcelist.indexOf(er);
                Q_ASSERT(x != -1);
                m_resourcelist.replace(x, v);
                m_completion->addUsedEffort(v);
                Q_EMIT dataChanged(createIndex(idx.row(), 1), createIndex(idx.row(), columnCount() - 1));
                Q_EMIT rowInserted(createIndex(idx.row(), 0));
                return true;
            }
            Completion::UsedEffort *ue = usedEffort(idx);
            if (ue == nullptr) {
                return false;
            }
            QDate d = m_dates.value(idx.column() - 1);
            Completion::UsedEffort::ActualEffort e = ue->effort(d);
            e.setNormalEffort(Duration(value.toDouble(), Duration::Unit_h));
            ue->setEffort(d, e);
            Q_EMIT effortChanged(d);
            Q_EMIT dataChanged(idx, idx);
            return true;
        }
        default: break;
    }
    return false;
}

bool UsedEffortItemModel::submit()
{
    debugPlan;
    return QAbstractItemModel::submit();
}

void UsedEffortItemModel::revert()
{
    debugPlan;
    const QList<const Resource*> lst = m_resourcelist;
    for (const Resource *r : lst) {
        if (! m_completion->usedEffortMap().contains(r)) {
            int row = m_resourcelist.indexOf(r);
            if (row != -1) {
                beginRemoveRows(QModelIndex(), row, row);
                m_resourcelist.removeAt(row);
                endRemoveRows();
            }
        }
    }
}

QVariant UsedEffortItemModel::headerData (int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Vertical) {
        return data(index(section, 0), role);
    }
    if (section < 0 || section >= m_headers.count()) {
        return QVariant();
    }
    switch (role) {
        case Qt::DisplayRole:
            return m_headers.at(section);
        case Qt::ToolTipRole: {
            if (section >= 1 && section <= 7) {
                return QLocale().toString(m_dates.at(section - 1), QLocale::LongFormat);
            }
            if (section == 8) {
                return i18n("Total effort this week");
            }
            break;
        }
        case Qt::TextAlignmentRole:
            return Qt::AlignLeading;
        default: break;
    }
    return QVariant();
}

int UsedEffortItemModel::columnCount(const QModelIndex & parent) const
{
    int c = 0;
    if (m_completion && ! parent.isValid()) {
        c = 9;
    }
    return c;
}

int UsedEffortItemModel::rowCount(const QModelIndex &) const
{
    int rows = 0;
    if (m_completion) {
        rows = m_resourcelist.count();
    }
    return rows;
}

QModelIndex UsedEffortItemModel::index (int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return QModelIndex();
    }
    return createIndex(row, column);
}

void UsedEffortItemModel::setCompletion(Completion *completion)
{
    beginResetModel();
    m_completion = completion;
    m_resourcelist.clear();
    QMultiMap<QString, const Resource*> lst;
    const Completion::ResourceUsedEffortMap &map = completion->usedEffortMap();
    Completion::ResourceUsedEffortMap::const_iterator it;
    for (it = map.constBegin(); it != map.constEnd(); ++it) {
        lst.insert(it.key()->name(), it.key());
    }
    m_resourcelist = lst.values();
    endResetModel();
}

const Resource *UsedEffortItemModel::resource(const QModelIndex &index) const
{
    int row = index.row();
    if (m_completion == nullptr || row < 0 || row >= m_resourcelist.count()) {
        return nullptr;
    }
    return m_resourcelist.value(row);
}

Completion::UsedEffort *UsedEffortItemModel::usedEffort(const QModelIndex &index) const
{
    const Resource *r = resource(index);
    if (r == nullptr) {
        return nullptr;
    }
    return m_completion->usedEffort(r);
}

void UsedEffortItemModel::setCurrentMonday(const QDate &date)
{
    beginResetModel();
    m_dates.clear();
    for (int i = 0; i < 7; ++i) {
        m_dates << date.addDays(i);
    }
    endResetModel();
    Q_EMIT headerDataChanged (Qt::Horizontal, 1, 7);
}

QModelIndex UsedEffortItemModel::addRow()
{
    if (m_project == nullptr) {
        return QModelIndex();
    }
    m_editlist.clear();
    m_editlist = freeResources();
    if (m_editlist.isEmpty()) {
        return QModelIndex();
    }
    int row = rowCount();
    beginInsertRows(QModelIndex(), row, row);
    m_resourcelist.append(m_editlist.first());
    endInsertRows();
    return createIndex(row, 0, const_cast<Resource*>(m_editlist.first()));
}

void UsedEffortItemModel::addResource(const QString &name)
{
    const Resource *resource = freeResources().value(name);
    if (!resource) {
        return;
    }
    m_editlist.clear();
    m_editlist.insert(name, resource);
    int row = rowCount();
    beginInsertRows(QModelIndex(), row, row);
    m_resourcelist.append(resource);
    endInsertRows();
    setData(createIndex(row, 0, const_cast<Resource*>(resource)), 0, Qt::EditRole);
}

QMultiMap<QString, const Resource*> UsedEffortItemModel::freeResources() const
{
    QMultiMap<QString, const Resource*> map;
    const QList<Resource*> resources = m_project->resourceList();
    for (Resource *r : resources) {
        if (! m_resourcelist.contains(r)) {
            map.insert(r->name(), r);
        }
    }
    return map;
}

//-----------
UsedEffortEditor::UsedEffortEditor(QWidget *parent)
    : QTableView(parent)
{
    UsedEffortItemModel *m = new UsedEffortItemModel(this);
    setModel(m);

    setItemDelegateForColumn (1, new DoubleSpinBoxDelegate(this));
    setItemDelegateForColumn (2, new DoubleSpinBoxDelegate(this));
    setItemDelegateForColumn (3, new DoubleSpinBoxDelegate(this));
    setItemDelegateForColumn (4, new DoubleSpinBoxDelegate(this));
    setItemDelegateForColumn (5, new DoubleSpinBoxDelegate(this));
    setItemDelegateForColumn (6, new DoubleSpinBoxDelegate(this));
    setItemDelegateForColumn (7, new DoubleSpinBoxDelegate(this));

    connect (model(), &QAbstractItemModel::dataChanged, this, &UsedEffortEditor::changed);

    connect (m, &UsedEffortItemModel::rowInserted, this, &UsedEffortEditor::resourceAdded);
    connect (m, &UsedEffortItemModel::rowInserted, this, &UsedEffortEditor::scrollToBottom);
}

bool UsedEffortEditor::hasFreeResources() const
{
    return ! model()->freeResources().isEmpty();
}

void UsedEffortEditor::setProject(Project *p)
{
    model()->setProject(p);
}

void UsedEffortEditor::setCompletion(Completion *completion)
{
    model()->setCompletion(completion);
    setColumnHidden(0, true);
}

void UsedEffortEditor::setCurrentMonday(const QDate &date)
{
    static_cast<UsedEffortItemModel*>(model())->setCurrentMonday(date);
}

void UsedEffortEditor::addResource()
{
    UsedEffortItemModel *m = model();
    QModelIndex i = m->addRow();
    if (i.isValid()) {
        setCurrentIndex(i);
        edit(i);
    }
}

//----------------------------------------
CompletionEntryItemModel::CompletionEntryItemModel (QObject *parent)
    : QAbstractItemModel(parent),
    m_node(nullptr),
    m_project(nullptr),
    m_manager(nullptr),
    m_completion(nullptr)
{
    m_headers << i18n("Date")
            // xgettext: no-c-format
            << i18n("% Completed")
            << i18n("Used Effort")
            << i18n("Remaining Effort")
            << i18n("Planned Effort");

    m_flags.insert(Property_Date, Qt::NoItemFlags);
    m_flags.insert(Property_Completion, Qt::ItemIsEditable);
    m_flags.insert(Property_UsedEffort, Qt::NoItemFlags);
    m_flags.insert(Property_RemainingEffort, Qt::ItemIsEditable);
    m_flags.insert(Property_PlannedEffort, Qt::NoItemFlags);
}

void CompletionEntryItemModel::setTask(Task *t)
{
    m_node = t;
    m_project = nullptr;
    if (m_node && m_node->projectNode()) {
        m_project = static_cast<Project*>(m_node->projectNode());
    }
}

void CompletionEntryItemModel::slotDataChanged()
{
    refresh();
}

void CompletionEntryItemModel::setManager(ScheduleManager *sm)
{
    m_manager = sm;
    refresh();
}

Qt::ItemFlags CompletionEntryItemModel::flags (const QModelIndex &index) const
{
    if (index.isValid() && index.column() < m_flags.count()) {
        return QAbstractItemModel::flags(index) | m_flags[ index.column() ];
    }
    return QAbstractItemModel::flags(index);
}

QVariant CompletionEntryItemModel::date (int row, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            return m_datelist.value(row);
        default: break;
    }
    return QVariant();
}

QVariant CompletionEntryItemModel::percentFinished (int row, int role) const
{
    Completion::Entry *e = m_completion->entry(date(row).toDate());
    if (e == nullptr) {
        return QVariant();
    }
    switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            return e->percentFinished;
        default: break;
    }
    return QVariant();
}

QVariant CompletionEntryItemModel::remainingEffort (int row, int role) const
{
    Completion::Entry *e = m_completion->entry(date(row).toDate());
    if (e == nullptr) {
        return QVariant();
    }
    switch (role) {
        case Qt::DisplayRole:
        case Qt::ToolTipRole: {
            return e->remainingEffort.format();
        }
        case Qt::EditRole:
            return e->remainingEffort.toDouble(Duration::Unit_h);
        case Role::DurationScales: {
            QVariantList lst; // TODO: week
            if (m_node && m_project) {
                if (m_node->estimate()->type() == Estimate::Type_Effort) {
                    lst.append(m_project->standardWorktime()->day());
                }
            }
            if (lst.isEmpty()) {
                lst.append(24.0);
            }
            lst << 60.0 << 60.0 << 1000.0;
            return lst;
        }
        case Role::DurationUnit:
            return static_cast<int>(Duration::Unit_h);
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

QVariant CompletionEntryItemModel::actualEffort (int row, int role) const
{
    Completion::Entry *e = m_completion->entry(date(row).toDate());
    if (e == nullptr) {
        return QVariant();
    }
    switch (role) {
        case Qt::DisplayRole: {
            Duration v;
            if (m_completion->entrymode() == Completion::EnterEffortPerResource) {
                v = m_completion->actualEffortTo(date(row).toDate());
            } else {
                v = e->totalPerformed;
            }
            //debugPlan<<m_node->name()<<": "<<v<<" "<<unit<<" : "<<scales<<'\n';
            return v.format();
        }
        case Qt::EditRole:
            return e->totalPerformed.toDouble(Duration::Unit_h);
        case Role::DurationScales: {
            QVariantList lst; // TODO: week
            if (m_node && m_project) {
                if (m_node->estimate()->type() == Estimate::Type_Effort) {
                    lst.append(m_project->standardWorktime()->day());
                }
            }
            if (lst.isEmpty()) {
                lst.append(24);
            }
            lst << 60 << 60 << 1000;
            return lst;
        }
        case Role::DurationUnit:
            return static_cast<int>(Duration::Unit_h);
        case Role::Minimum:
            return m_project->config().minimumDurationUnit();
        case Role::Maximum:
            return m_project->config().maximumDurationUnit();
        case Qt::ToolTipRole:
            return xi18nc("@info:tooltip", "Accumulated effort %1", actualEffort(row, Qt::DisplayRole).toString());
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant CompletionEntryItemModel::plannedEffort (int /*row*/, int role) const
{
    if (m_node == nullptr) {
        return QVariant();
    }
    switch (role) {
        case Qt::DisplayRole:
        case Qt::ToolTipRole: {
            Duration v = m_node->plannedEffort(id(), ECCT_EffortWork);
            //debugPlan<<m_node->name()<<": "<<v<<" "<<unit;
            return v.format();
        }
        case Qt::EditRole:
            return QVariant();
        case Role::DurationScales: {
            QVariantList lst; // TODO: week
            if (m_node && m_project) {
                if (m_node->estimate()->type() == Estimate::Type_Effort) {
                    lst.append(m_project->standardWorktime()->day());
                }
            }
            if (lst.isEmpty()) {
                lst.append(24.0);
            }
            lst << 60.0 << 60.0 << 1000.0;
            return lst;
        }
        case Role::DurationUnit:
            return static_cast<int>(Duration::Unit_h);
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

QVariant CompletionEntryItemModel::data (const QModelIndex &index, int role) const
{
    if (! index.isValid()) {
        return QVariant();
    }
    switch (index.column()) {
        case Property_Date: return date(index.row(), role);
        case Property_Completion: return percentFinished(index.row(), role);
        case Property_UsedEffort: return actualEffort(index.row(), role);
        case Property_RemainingEffort: return remainingEffort(index.row(), role);
        case Property_PlannedEffort: return plannedEffort(index.row(), role);
        default: break;
    }
    return QVariant();
}

QList<qint64> CompletionEntryItemModel::scales() const
{
    QList<qint64> lst;
    if (m_node && m_project) {
        if (m_node->estimate()->type() == Estimate::Type_Effort) {
            lst = m_project->standardWorktime()->scales();
        }
    }
    if (lst.isEmpty()) {
        lst = Estimate::defaultScales();
    }
    //debugPlan<<lst;
    return lst;

}

bool CompletionEntryItemModel::setData (const QModelIndex &idx, const QVariant &value, int role)
{
    //debugPlan;
    switch (role) {
        case Qt::EditRole: {
            if (idx.column() == Property_Date) {
                QDate od = date(idx.row()).toDate();
                QDate nd = value.toDate();
                if (od == nd) {
                    // new unedited entry
                    addEntry(nd);
                    // Q_EMIT dataChanged(idx, idx);
                    return true;
                }
                if (!m_datelist.contains(nd)) {
                    // date is edited so remove original
                    removeEntry(od);
                    // add new edited entry
                    addEntry(nd);
                    // Q_EMIT dataChanged(idx, idx);
                    return true;
                }
                // probably a duplicate of an existing date
                removeEntry(od); // just remove the new entry date to avoid duplicates for now
                return false;
            }
            if (idx.column() == Property_Completion) {
                Completion::Entry *e = m_completion->entry(date(idx.row()).toDate());
                if (e == nullptr) {
                    return false;
                }
                e->percentFinished = value.toInt();
                if (m_completion->entrymode() != Completion::EnterEffortPerResource && m_node) {
                    // calculate used/remaining
                    Duration est = m_node->plannedEffort(id(), ECCT_EffortWork);
                    e->totalPerformed = est * e->percentFinished / 100;
                    e->remainingEffort = est - e->totalPerformed;
                }
                Q_EMIT dataChanged(idx, createIndex(idx.row(), 3));
                return true;
            }
            if (idx.column() == Property_UsedEffort) {
                Completion::Entry *e = m_completion->entry(date(idx.row()).toDate());
                if (e == nullptr) {
                    return false;
                }
                double v(value.toList()[0].toDouble());
                Duration::Unit unit = static_cast<Duration::Unit>(value.toList()[1].toInt());
                Duration d = Estimate::scale(v, unit, scales());
                if (d == e->totalPerformed) {
                    return false;
                }
                e->totalPerformed = d;
                Q_EMIT dataChanged(idx, idx);
                return true;
            }
            if (idx.column() == Property_RemainingEffort) {
                Completion::Entry *e = m_completion->entry(date(idx.row()).toDate());
                if (e == nullptr) {
                    return false;
                }
                double v(value.toList()[0].toDouble());
                Duration::Unit unit = static_cast<Duration::Unit>(value.toList()[1].toInt());
                Duration d = Estimate::scale(v, unit, scales());
                if (d == e->remainingEffort) {
                    return false;
                }
                e->remainingEffort = d;
                Q_EMIT dataChanged(idx, idx);
                return true;
            }
        }
        default: break;
    }
    return false;
}

bool CompletionEntryItemModel::submit()
{
    debugPlan<<'\n';
    return QAbstractItemModel::submit();
}

void CompletionEntryItemModel::revert()
{
    debugPlan<<'\n';
    refresh();
}

QVariant CompletionEntryItemModel::headerData (int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Vertical) {
        return QVariant();
    }
    if (section < 0 || section >= m_headers.count()) {
        return QVariant();
    }
    switch (role) {
        case Qt::DisplayRole:
            return m_headers.at(section);
        case Qt::ToolTipRole:
            switch (section) {
                case Property_UsedEffort:
                    if (m_completion->entrymode() == Completion::EnterEffortPerResource) {
                        return xi18nc("@info", "Accumulated used effort.<nl/><note>Used effort must be entered per resource</note>");
                    }
                    return xi18nc("@info:tooltip", "Accumulated used effort");
                case Property_RemainingEffort:
                    return xi18nc("@info:tooltip", "Remaining effort to complete the task");
                case Property_PlannedEffort:
                    return xi18nc("@info:tooltip", "Planned effort accumulated until date");
            }
            break;
        default: break;
    }
    return QVariant();
}

int CompletionEntryItemModel::columnCount(const QModelIndex & /*parent */) const
{
    return 5;
}

int CompletionEntryItemModel::rowCount(const QModelIndex &idx) const
{
    if (m_completion == nullptr || idx.isValid()) {
        return 0;
    }
    return m_datelist.count();
}

QModelIndex CompletionEntryItemModel::index (int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return QModelIndex();
    }
    return createIndex(row, column);
}

void CompletionEntryItemModel::setCompletion(Completion *completion)
{
    m_completion = completion;
    refresh();
}

void CompletionEntryItemModel::refresh()
{
    beginResetModel();
    m_datelist.clear();
    m_flags[ Property_UsedEffort ] = Qt::NoItemFlags;
    if (m_completion) {
        m_datelist = m_completion->entries().keys();
        if (m_completion->entrymode() != Completion::EnterEffortPerResource) {
            m_flags[ Property_UsedEffort ] = Qt::ItemIsEditable;
        }
    }
    debugPlan<<m_datelist;
    endResetModel();
}

QModelIndex CompletionEntryItemModel::addRow()
{
    if (m_completion == nullptr) {
        return QModelIndex();
    }
    int row = rowCount();
    QDate d = QDate::currentDate();
    if (row > 0 && d <= m_datelist.last()) {
        d = m_datelist.last().addDays(1);
    }
    beginInsertRows(QModelIndex(), row, row);
    m_datelist.append(d);
    endInsertRows();
    return createIndex(row, 0);
}

void CompletionEntryItemModel::removeEntry(const QDate& date)
{
    removeRow(m_datelist.indexOf(date));
}

void CompletionEntryItemModel::removeRow(int row)
{
    debugPlan<<row;
    if (row < 0 || row >= rowCount()) {
        return;
    }
    QDate date = m_datelist.value(row);
    beginRemoveRows(QModelIndex(), row, row);
    m_datelist.removeAt(row);
    endRemoveRows();
    debugPlan<<date<<" removed row"<<row;
    m_completion->takeEntry(date);
    Q_EMIT rowRemoved(date);
    Q_EMIT changed();
}

void CompletionEntryItemModel::addEntry(const QDate& date)
{
    debugPlan<<date<<'\n';
    Completion::Entry *e = new Completion::Entry();
    if (m_completion->entries().isEmpty()) {
        if (m_node) {
            e->remainingEffort = m_node->plannedEffort(id(), ECCT_EffortWork);
        }
    } else {
        e->percentFinished = m_completion->percentFinished();
        e->totalPerformed = m_completion->actualEffort();
        e->remainingEffort = m_completion->remainingEffort();
    }
    m_completion->addEntry(date, e);
    refresh();
    int i = m_datelist.indexOf(date);
    if (i != -1) {
        Q_EMIT rowInserted(date);
        Q_EMIT dataChanged(createIndex(i, 1), createIndex(i, rowCount() - 1));
    } else  errorPlan<<"Failed to find added entry: "<<date<<'\n';
}

void CompletionEntryItemModel::addRow(const QDate &date)
{
    for (int i = 0; i < rowCount(); ++i) {
        if (this->date(i).toDate() == date) {
            const QModelIndex idx1 = index(i, Property_UsedEffort);
            const QModelIndex idx2 = index(rowCount()-1, Property_UsedEffort);
            Q_EMIT dataChanged(idx1, idx2);
            return;
        }
    }
    addEntry(date);
}

//-----------
CompletionEntryEditor::CompletionEntryEditor(QWidget *parent)
    : QTableView(parent)
{
    verticalHeader()->hide();

    CompletionEntryItemModel *m = new CompletionEntryItemModel(this);
    setItemDelegateForColumn (1, new ProgressBarDelegate(this));
    setItemDelegateForColumn (2, new DurationSpinBoxDelegate(this));
    setItemDelegateForColumn (3, new DurationSpinBoxDelegate(this));
    setCompletionModel(m);
    resizeColumnToContents(1);
    resizeColumnToContents(2);
    resizeColumnToContents(3);
    resizeColumnToContents(4);
}

void CompletionEntryEditor::setCompletionModel(CompletionEntryItemModel *m)
{
    if (model()) {
        disconnect(model(), &CompletionEntryItemModel::rowInserted, this, &CompletionEntryEditor::rowInserted);
        disconnect(model(), &CompletionEntryItemModel::rowRemoved, this, &CompletionEntryEditor::rowRemoved);
        disconnect(model(), &QAbstractItemModel::dataChanged, this, &CompletionEntryEditor::changed);
        disconnect(model(), &CompletionEntryItemModel::changed, this, &CompletionEntryEditor::changed);
        disconnect(selectionModel(), &QItemSelectionModel::selectionChanged, this, &CompletionEntryEditor::selectedItemsChanged);
    }
    setModel(m);
    if (model()) {
        connect(model(), &CompletionEntryItemModel::rowInserted, this, &CompletionEntryEditor::rowInserted);
        connect(model(), &CompletionEntryItemModel::rowRemoved, this, &CompletionEntryEditor::rowRemoved);
        connect(model(), &QAbstractItemModel::dataChanged, this, &CompletionEntryEditor::changed);
        connect(model(), &CompletionEntryItemModel::changed, this, &CompletionEntryEditor::changed);
        connect(selectionModel(), &QItemSelectionModel::selectionChanged, this, &CompletionEntryEditor::selectedItemsChanged);
    }
}

void CompletionEntryEditor::setCompletion(Completion *completion)
{
    model()->setCompletion(completion);
}

void CompletionEntryEditor::insertEntry(const QDate &date)
{
    model()->addRow(date);
}

void CompletionEntryEditor::addEntry()
{
    debugPlan<<'\n';
    QModelIndex i = model()->addRow();
    if (i.isValid()) {
        model()->setFlags(i.column(), Qt::ItemIsEditable);
        setCurrentIndex(i);
        Q_EMIT selectedItemsChanged(QItemSelection(), QItemSelection()); //hmmm, control removeEntryBtn
        scrollTo(i);
        edit(i);
        model()->setFlags(i.column(), Qt::NoItemFlags);
    }
}

void CompletionEntryEditor::removeEntry()
{
    //debugPlan;
    QModelIndexList lst = selectedIndexes();
    debugPlan<<lst;
    QMap<int, int> rows;
    while (! lst.isEmpty()) {
        QModelIndex idx = lst.takeFirst();
        rows[ idx.row() ] = 0;
    }
    const auto r = rows.keys();
    QList<int> removed;
    debugPlan<<rows<<r;
    for (int i = r.count() - 1; i >= 0; --i) {
        if (removed.contains(r.at(i))) {
            continue;
        }
        removed << r.at(i);
        model()->removeRow(r.at(i));
    }
}


}  //KPlato namespace
