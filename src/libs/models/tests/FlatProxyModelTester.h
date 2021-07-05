/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2010 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPlato_FlatProxyModelTester_h
#define KPlato_FlatProxyModelTester_h

#include <QObject>

#include "kptflatproxymodel.h"

#include <QStandardItemModel>

namespace KPlato
{


class FlatProxyModelTester : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void emptyModel();
    void test();
    void testInsertRemoveTop();
    void testInsertRemoveChildren();
    void testInsertRemoveGrandChildren();
    void testWithNodeItemModel();

private:
    FlatProxyModel m_flatmodel;
    QStandardItemModel m_standardmodel;

};

} //namespace KPlato

#endif
