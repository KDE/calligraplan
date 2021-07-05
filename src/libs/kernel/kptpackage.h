/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 2010, 2011 Dag Andersen <dag.andersen@kdemail.net>

    SPDX-License-Identifier: GPL-2.0-or-later
*/


#ifndef KPLATO_KPTPACKAGE_H
#define KPLATO_KPTPACKAGE_H

#include "plankernel_export.h"

#include "kpttask.h"

#include <QUrl>
#include <QString>

namespace KPlato {

class Project;

// temporary convenience class
class PLANKERNEL_EXPORT Package
{
public:
    Package();
    QUrl url;
    Project *project;
    QDateTime timeTag;
    QString ownerId;
    QString ownerName;

    WorkPackageSettings settings;

    Task *task;
    Task *toTask;
    QMap<QString, QUrl> documents;
};

}

#endif // KPLATO_KPTPACKAGE_H
