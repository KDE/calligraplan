/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
   
   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPLATO_XMLLOADERTESTER_H
#define KPLATO_XMLLOADERTESTER_H

#include <QObject>

class KoXmlDocument;

namespace KPlato
{

class XmlLoaderTester : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void version_0_6();
    void version_0_7();

private:
    void test(const KoXmlDocument &doc);
    QString data_v0_6() const;
    QString data_v0_7() const;
};

} //namespace KPlato

#endif
