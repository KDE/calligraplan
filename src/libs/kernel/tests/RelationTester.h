/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2022 Dag Andersen <dag.andersen@kdemail.net>
   
   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPlato_RelationTester_h
#define KPlato_RelationTester_h

#include <QObject>

#include "kptproject.h"
#include "kptdatetime.h"

namespace KPlato
{

class Task;

class RelationTester : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void proxyRelations();

private:
    Project *m_project;
    Calendar *m_calendar;
};

} //namespace KPlato

#endif
