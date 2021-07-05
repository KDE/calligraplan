/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2011 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPlato_SchedulerTester_h
#define KPlato_SchedulerTester_h

#include <QObject>

class KoXmlDocument;

namespace KPlato
{
class Node;

class SchedulerTester : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testSingle();

private:
    QStringList data();
    void loadDocument(const QString &dir, const QString &fname, KoXmlDocument &doc) const;
    void testProject(const QString &fname, const KoXmlDocument &doc);
    void compare(const QString &fname, Node *n, long id1, long id2);
    void loadDocuments(QString &dir, QList<QString> files, QList<KoXmlDocument> &docs) const;
};

} //namespace KPlato

#endif
