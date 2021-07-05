/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2009 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPlato_ResourceTester_h
#define KPlato_ResourceTester_h

#include <QObject>

namespace KPlato
{

class ResourceTester : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testAvailable();
    void testSingleDay();
    void team();
    void required();
};

} //namespace KPlato

#endif
