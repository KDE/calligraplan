/* This file is part of the Calligra project
 * SPDX-FileCopyrightText: 2008 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KPTNODECHARTMODEL_H
#define KPTNODECHARTMODEL_H

#include "planmodels_export.h"

#include "kptitemmodelbase.h"

#include "kpteffortcostmap.h"

#include <QSortFilterProxyModel>

#include "kptdebug.h"

#include <KChartGlobal>

namespace KPlato
{

class Resource;
class Project;
class ScheduleManager;
class Node;

class PLANMODELS_EXPORT ChartProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit ChartProxyModel(QObject *parent = nullptr) : QSortFilterProxyModel(parent) {}

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override {
        //if (role == Qt::DisplayRole && orientation == Qt::Vertical) debugPlan<<"fetch:"<<orientation<<section<<mapToSource(index(0, section)).column()<<m_rejects;
        return QSortFilterProxyModel::headerData(section, orientation, role);
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override {
        if (role == Qt::DisplayRole && ! m_zerocolumns.isEmpty()) {
            int column = mapToSource(index).column();
            if (m_zerocolumns.contains(column) ) {
                //debugPlan<<"zero:"<<index.column()<<mapToSource(index).column();
                return QVariant();
            }
        }
        //if (role == Qt::DisplayRole) debugPlan<<"fetch:"<<index.column()<<mapToSource(index).column()<<m_rejects;
        QVariant v = QSortFilterProxyModel::data(index, role);
        //if (role == Qt::DisplayRole) debugPlan<<index.row()<<","<<index.column()<<"("<<columnCount()<<")"<<v;
        return v;
    }
    void setRejectColumns(const QList<int> &columns) { beginResetModel(); m_rejects = columns; endResetModel(); }
    QList<int> rejectColumns() const { return m_rejects; }
    void setZeroColumns(const QList<int> &columns) { m_zerocolumns = columns; }
    QList<int> zeroColumns() const { return m_zerocolumns; }

    void reset() { beginResetModel(); endResetModel(); }

protected:
    bool filterAcceptsColumn (int source_column, const QModelIndex &/*source_parent */) const override {
        //debugPlan<<this<<source_column<<m_rejects<<(! m_rejects.contains(source_column));
        return ! m_rejects.contains(source_column);
    }

private:
    QList<int> m_rejects;
    QList<int> m_zerocolumns;
};

class PLANMODELS_EXPORT ChartItemModel : public ItemModelBase
{
    Q_OBJECT
public:
    enum Properties {
        BCWSCost,
        BCWPCost,
        ACWPCost,
        BCWSEffort,
        BCWPEffort,
        ACWPEffort,
        SPICost,
        CPICost,
        SPIEffort,
        CPIEffort
    };
    Q_ENUM(Properties)
    const QMetaEnum columnMap() const override;

    explicit ChartItemModel(QObject *parent = nullptr);


//    virtual Qt::ItemFlags flags(const QModelIndex & index) const;

    QModelIndex parent(const QModelIndex & index) const override;
    QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const override;

    int columnCount(const QModelIndex & parent = QModelIndex()) const override;
    int rowCount(const QModelIndex & parent = QModelIndex()) const override;

    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;


    const EffortCostMap &bcwp() const { return m_bcws; }
    const EffortCostMap &acwp() const { return m_acwp; }

    void setProject(Project *project) override;

    void setNodes(const QList<Node*> &nodes);
    void addNode(Node *node);
    void clearNodes();
    QDate startDate() const;
    QDate endDate() const;
    void calculate();

    void setLocalizeValues(bool on);

    int rowForDate(const QDate &date) const;

public Q_SLOTS:
    void setScheduleManager(KPlato::ScheduleManager *sm) override;
    void slotNodeRemoved(KPlato::Node *node);
    void slotNodeChanged(KPlato::Node *node);
    void slotResourceChanged();
    void slotResourceRemoved();

    void slotSetScheduleManager(KPlato::ScheduleManager *sm);

protected:
    double bcwsEffort(int day) const;
    double bcwpEffort(int day) const;
    double acwpEffort(int day) const;
    double bcwsCost(int day) const;
    double bcwpCost(int day) const;
    double acwpCost(int day) const;
    double spiEffort(int day) const;
    double cpiEffort(int day) const;
    double spiCost(int day) const;
    double cpiCost(int day) const;

protected:
    QList<Node*> m_nodes;
    EffortCostMap m_bcws;
    EffortCostMap m_acwp;
    bool m_localizeValues;
};

class PLANMODELS_EXPORT PerformanceDataCurrentDateModel : public QAbstractProxyModel
{
    Q_OBJECT
public:
    explicit PerformanceDataCurrentDateModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &idx) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &proxyIndex, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    QModelIndex mapToSource(const QModelIndex &idx) const override;
    QModelIndex mapFromSource(const QModelIndex &idx) const override;

    Project *project() const;
    ScheduleManager *scheduleManager() const;
    bool isReadWrite() const;

    void setNodes(const QList<Node*> &nodes);
    void addNode(Node *node);
    void clearNodes();

    QDate startDate() const;
    QDate endDate() const;

public Q_SLOTS:
    virtual void setProject(KPlato::Project *project);
    virtual void setScheduleManager(KPlato::ScheduleManager *sm);
    virtual void setReadWrite(bool rw);

};

} //namespace KPlato

#endif
