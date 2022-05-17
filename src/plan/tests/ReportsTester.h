/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2022 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPlato_ReportsTester_h
#define KPlato_ReportsTester_h

#include <QObject>

namespace KPlato {
    class Part;
}

class ReportsTester : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void init();
    void cleanup();
    void testReportGeneration();

private:
    void loadDocument(const QString &file);
    KPlato::Part *part;
};

#endif
