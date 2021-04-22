/* This file is part of the KDE project
 * Copyright (C) 2021 Dag Andersen <dag.andersen@kdemail.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef KPlato_TJSchedulerTester_h
#define KPlato_TJSchedulerTester_h

#include <QObject>


namespace KPlato
{
class Node;
class Part;
class MainDocument;
class SchedulerPlugin;
class SchedulingContext;

class TJSchedulerTester : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
private:
    void testSingleProject();
    void testSingleProjectWithBookings();
    void testMultiple();
    void testMultipleWithBookings();

private Q_SLOTS:
    void testRecalculate();

private:
    void populateSchedulingContext(SchedulingContext &context, const QString &name, const QList<Part*> &projects, const QList<Part*> &bookings = QList<Part*>()) const;
    Part *loadDocument(const QString &dir, const QString &fname);
    
    QList<Part*> loadDocuments(QString &dir, QList<QString> files);
//     void testProject(const QString &fname, const KoXmlDocument &doc);
//     void compare(const QString &fname, Node *n, long id1, long id2);

    SchedulerPlugin *m_scheduler;
};

} //namespace KPlato

#endif
