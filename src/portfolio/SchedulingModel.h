/* This file is part of the KDE project
* SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
*
* SPDX-License-Identifier: LGPL-2.0-or-later
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
    Q_PROPERTY(MainDocument* portfolio READ portfolio WRITE setPortfolio NOTIFY portfolioChanged)

public:
    explicit SchedulingModel(QObject *parent = nullptr);
    ~SchedulingModel();

    Qt::ItemFlags flags(const QModelIndex &idx) const override;
    bool setExtraColumnData(const QModelIndex &parent, int row, int extraColumn, const QVariant &value, int role = Qt::EditRole) override;

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
