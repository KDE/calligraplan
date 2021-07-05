/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2007 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef RESOURCEMODEL_H
#define RESOURCEMODEL_H

#include "planmodels_export.h"

#include <QObject>
#include <QMetaEnum>

class QByteArray;

namespace KIO {
    class Job;
}
class KJob;

namespace KPlato
{

class Project;
class Resource;

class PLANMODELS_EXPORT ResourceModel : public QObject
{
    Q_OBJECT
public:
    explicit ResourceModel(QObject *parent = nullptr);
    ~ResourceModel() override;

    enum Properties {
        ResourceName = 0,
        ResourceOrigin,
        ResourceType,
        ResourceInitials,
        ResourceEmail,
        ResourceCalendar,
        ResourceLimit,
        ResourceAvailableFrom,
        ResourceAvailableUntil,
        ResourceNormalRate,
        ResourceOvertimeRate,
        ResourceAccount
    };
    Q_ENUM(Properties)
    
    const QMetaEnum columnMap() const;
    void setProject(Project *project);
    int propertyCount() const;
    QVariant data(const Resource *resource, int property, int role = Qt::DisplayRole) const;
    static QVariant headerData(int section, int role = Qt::DisplayRole);

    QVariant name(const Resource *res, int role) const;
    QVariant origin(const Resource *res, int role) const;
    QVariant type(const Resource *res, int role) const;
    QVariant initials(const Resource *res, int role) const;
    QVariant email(const Resource *res, int role) const;
    QVariant calendar(const Resource *res, int role) const;
    QVariant units(const Resource *res, int role) const;
    QVariant availableFrom(const Resource *res, int role) const;
    QVariant availableUntil(const Resource *res, int role) const;
    QVariant normalRate(const Resource *res, int role) const;
    QVariant overtimeRate(const Resource *res, int role) const;
    QVariant account(const Resource *res, int role) const;

private:
    Project *m_project;
};

}  //KPlato namespace

#endif
