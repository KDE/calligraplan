/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2023 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPlato_MpxjImportTester_h
#define KPlato_MpxjImportTester_h

#include <QObject>


class MpxjImportTester : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testGanttProject();
    void testProjectLibre();
};

#endif
