/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KPlato_TJSchedulerTester_h
#define KPlato_TJSchedulerTester_h

#include <QObject>

class PlanTJScheduler;

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
    void init();
    void cleanup();
    void testSingleProject();
    void testSingleProjectWithBookings();
    void testMultiple();
    void testMultipleWithBookingsParalell();
    void testMultipleWithBookingsSequential();
    void testRecalculate();
    void testRecalculateMultiple();

    void testRecalculateMultipleSeq();

private:
    void populateSchedulingContext(SchedulingContext &context, const QString &name, const QList<Part*> &projects, const QList<Part*> &bookings = QList<Part*>()) const;
    Part *loadDocument(const QString &dir, const QString &fname);
    
    QList<Part*> loadDocuments(QString &dir, QList<QString> files);
//     void testProject(const QString &fname, const KoXmlDocument &doc);
//     void compare(const QString &fname, Node *n, long id1, long id2);
    void deleteAll(const QList<Part*> parts);

    PlanTJScheduler *m_scheduler;
};

} //namespace KPlato

#endif
