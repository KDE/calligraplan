/* This file is part of the KDE project
 SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>

 SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPlato_InsertProjectXmlCommandTester_h
#define KPlato_InsertProjectXmlCommandTester_h

#include <QObject>

#include "kptresourceappointmentsmodel.h"

#include "kptproject.h"
#include "kptdatetime.h"

namespace KPlato
{

class Task;

class InsertProjectXmlCommandTester : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void init();
    void cleanup();

    void copyBasics();
    void copyRequests();
    void copyDependency();
    void copyToPosition();

private:
    Project *m_project;
};

} //namespace KPlato

#endif
