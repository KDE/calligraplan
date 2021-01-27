/* This file is part of the KDE project
* Copyright (C) 2021 Dag Andersen <dag.andersen@kdemail.net>
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Library General Public
* License as published by the Free Software Foundation; either
* version 2 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Library General Public License for more details.
*
* You should have received a copy of the GNU Library General Public License
* along with this library; see the file COPYING.LIB.  If not, write to
* the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
* Boston, MA 02110-1301, USA.
*/

#ifndef PLANPORTFOLIO_SCHEDULINGMODEL_H
#define PLANPORTFOLIO_SCHEDULINGMODEL_H

#include <ProjectsModel.h>

#include <KExtraColumnsProxyModel>

#include <QString>

class MainDocument;
class QAbstractItemView;

class SchedulingModel : public KExtraColumnsProxyModel
{
    Q_OBJECT
    Q_PROPERTY(MainDocument* portfolio READ portfolio WRITE setPortfolio NOTIFY portfolioChanged);

public:
    explicit SchedulingModel(QObject *parent = nullptr);
    ~SchedulingModel();

    Qt::ItemFlags flags(const QModelIndex &idx) const override;
    bool setData(const QModelIndex &idx, const QVariant &value, int role = Qt::EditRole) override;

    void setDelegates(QAbstractItemView *view);
    MainDocument *portfolio() const;
    
public Q_SLOTS:
    void setPortfolio(MainDocument *portfolio);
    
Q_SIGNALS:
    void portfolioChanged();

protected:
    QVariant extraColumnData(const QModelIndex &parent, int row, int extraColumn, int role = Qt::DisplayRole) const override;

    QString displayString(const QString &key) const;
    QString keyString(const QString &value) const;

private:
    ProjectsFilterModel *m_baseModel;
    QStringList m_controlKeys;
    QStringList m_controlDisplay;
};

#endif
