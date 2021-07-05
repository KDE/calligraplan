/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2009 Dag Andersen <calligra-devel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kpttaskcompletedelegate.h"

#include "kptnodeitemmodel.h"
#include "kptnode.h"

#include <QModelIndex>
#include <QApplication>
#include <QStyleOptionViewItem>
#include <QStyle>
#include <QPainter>

namespace KPlato
{

//-----------------------------
TaskCompleteDelegate::TaskCompleteDelegate(QObject *parent)
 : ProgressBarDelegate(parent)
{
}

TaskCompleteDelegate::~TaskCompleteDelegate()
{
}

void TaskCompleteDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QModelIndex typeidx = index.model()->index(index.row(), NodeModel::NodeType, index.parent());
    if (! typeidx.isValid()) {
        errorPlan<<"Cannot find nodetype for index:"<<index;
        return;
    }
    int type = typeidx.data(Qt::EditRole).toInt();
    if (type == Node::Type_Task || type == Node::Type_Milestone) {
        ProgressBarDelegate::paint(painter, option, index);
    } else {
        QStyle *style;
        QStyleOptionViewItem opt = option;
        //initStyleOption(&opt, index);
        style = opt.widget ? opt.widget->style() : QApplication::style();
        style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter);

        //debugPlan<<"Draw something else, type="<<type<<index.model()->index(index.row(), NodeModel::NodeName, index.parent()).data().toString();

        ProgressBarDelegate::paint(painter, option, index);
    }
}

} //namespace KPlato
