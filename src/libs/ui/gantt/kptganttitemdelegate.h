/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2008 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTGANTTITEMDELEGATE_H
#define KPTGANTTITEMDELEGATE_H

#include "planui_export.h"

#include <KGanttGlobal>

#include <KGanttItemDelegate>

#include <QBrush>

namespace KGantt
{
    class StyleOptionGanttItem;
    class Constraint;
}

class QPainter;
class QModelIndex;


namespace KPlato
{

class PLANUI_EXPORT GanttItemDelegate : public KGantt::ItemDelegate
{
    Q_OBJECT
public:
    explicit GanttItemDelegate(QObject *parent = nullptr);

    QString toolTip(const QModelIndex& idx) const override;
    KGantt::Span itemBoundingSpan(const KGantt::StyleOptionGanttItem& opt, const QModelIndex& idx) const override;
    void paintGanttItem(QPainter* painter, const KGantt::StyleOptionGanttItem& opt, const QModelIndex& idx) override;
    
    void paintConstraintItem(QPainter* p, const QStyleOptionGraphicsItem& opt, const QPointF& start, const QPointF& end, const KGantt::Constraint &constraint) override;

    QVariant data(const QModelIndex& idx, int column, int role = Qt::DisplayRole) const;
    QString itemText(const QModelIndex& idx, int type) const;
    QRectF itemPositiveFloatRect(const KGantt::StyleOptionGanttItem& opt, const QModelIndex& idx) const;
    QRectF itemNegativeFloatRect(const KGantt::StyleOptionGanttItem& opt, const QModelIndex& idx) const;

    bool hasStartConstraint(const QModelIndex& idx) const;
    bool hasEndConstraint(const QModelIndex& idx) const;
    QRectF itemStartConstraintRect(const KGantt::StyleOptionGanttItem& opt, const QModelIndex& idx) const;
    QRectF itemEndConstraintRect(const KGantt::StyleOptionGanttItem& opt, const QModelIndex& idx) const;

    bool showResources;
    bool showTaskName;
    bool showTaskLinks;
    bool showProgress;
    bool showPositiveFloat;
    bool showNegativeFloat;
    bool showCriticalPath;
    bool showCriticalTasks;
    bool showAppointments;
    bool showNoInformation;
    bool showTimeConstraint;
    bool showSchedulingError;

protected:
    void paintSpecialItem(QPainter* painter, const KGantt::StyleOptionGanttItem& opt, const QModelIndex& idx, int typ);

protected:
    QBrush m_criticalBrush;
    QBrush m_schedulingErrorBrush;
    qreal m_constraintXOffset;

private:
    Q_DISABLE_COPY(GanttItemDelegate)

};

class PLANUI_EXPORT ResourceGanttItemDelegate : public KGantt::ItemDelegate
{
    Q_OBJECT
public:
    explicit ResourceGanttItemDelegate(QObject *parent = nullptr);

    QVariant data(const QModelIndex& idx, int column, int role = Qt::DisplayRole) const;

    void paintGanttItem(QPainter* painter, const KGantt::StyleOptionGanttItem& opt, const QModelIndex& idx) override;

protected:
    void paintResourceItem(QPainter* painter, const KGantt::StyleOptionGanttItem& opt, const QModelIndex& idx);

private:
    Q_DISABLE_COPY(ResourceGanttItemDelegate)
    QBrush m_overloadBrush;
    QBrush m_underloadBrush;

};

} // namespace KPlato

#endif
