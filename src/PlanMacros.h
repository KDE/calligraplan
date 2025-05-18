/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2004, 2007 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef PLANMACROS_H
#define PLANMACROS_H


#define CONTAINS(list, value) std::find(list.cbegin(), list.cend(), value) != list.cend()

#define OBJECTCONNECTIONS QList<QMetaObject::Connection> ObjectConnections;

#define CONNECT_TYPE(sender, signal, context, functor, type) ObjectConnections << connect(sender, signal, context, functor, type)

#define CONNECT(sender, signal, context, functor) ObjectConnections << connect(sender, signal, context, functor, Qt::AutoConnection)

#define DISCONNECT for (const QMetaObject::Connection &c : std::as_const(ObjectConnections)) { disconnect(c); }

#endif
