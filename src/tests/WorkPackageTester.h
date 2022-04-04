/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2022 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPlato_WorkPackageTester_h
#define KPlato_WorkPackageTester_h

#include <QObject>
#include <QList>

class QString;

namespace KPlato
{
class Part;
}

class WorkPackageTester : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void init();
    void loadPlan_v06();
    void loadPlanWork_v06();
    void loadPlan_v07();
    void loadPlanWork_v07();

private:
    KPlato::Part *loadDocument(const QString &fname);
};

#endif
