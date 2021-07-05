/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2006-2007 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPlato_DurationTester_h
#define KPlato_DurationTester_h

#include <QObject>

namespace KPlato
{

class DurationTester : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void add();
    void subtract();
    void divide();
    void equal();
    void lessThanOrEqual();
    void greaterThanOrEqual();
    void notEqual();
    void greaterThan();
    void lessThan();

};

}

#endif
