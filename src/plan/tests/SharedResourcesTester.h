/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2022 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPlato_SharedResources_h
#define KPlato_SharedResources_h

#include <QObject>

namespace KPlato {
    class Part;
}

class SharedResources : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void init();
    void cleanup();
    void testEmpty();
    void testRemoveResource();
    void testConvertResource();

private:
    KPlato::Part *part;
};

#endif
