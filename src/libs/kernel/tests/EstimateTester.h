/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2007 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPlato_EstimateTester_h
#define KPlato_EstimateTester_h

#include <QObject>

namespace KPlato
{

class EstimateTester : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void expected();
    void optimistic();
    void pessimistic();
    void ratio();
    void defaultScale();
    void scale();
    void pert();
};

}

#endif
