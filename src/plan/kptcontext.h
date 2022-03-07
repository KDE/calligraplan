/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2005, 2007 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/


#ifndef KPTCONTEXT_H
#define KPTCONTEXT_H

#include <QString>
#include <QStringList>

#include <KoXmlReader.h>

#define PLAN_CONTEXT_VERSION 1

namespace KPlato
{

class View;

class Context {
public:
    Context();
    virtual ~Context();
    
    virtual bool load(const KoXmlDocument &doc);
    virtual QDomDocument save(const View *view) const;
    const KoXmlElement &context() const;
    bool isLoaded() const { return m_contextLoaded; }
    int version() const;

    bool setContent(const QString &str);

    QDomDocument document() const;

    // View
    QString currentView;
    int currentEstimateType;
    long currentSchedule;
    bool actionViewExpected;
    bool actionViewOptimistic;
    bool actionViewPessimistic;

    struct Ganttview {
        int ganttviewsize;
        int taskviewsize;
        QString currentNode;
        bool showResources;
        bool showTaskName;
        bool showTaskLinks;
        bool showProgress;
        bool showPositiveFloat;
        bool showCriticalTasks;
        bool showCriticalPath;
        bool showNoInformation;
        QStringList closedNodes;
    } ganttview;    
    
    struct Pertview {
    } pertview;
    
    struct Resourceview {
    } resourceview;
    
    struct Accountsview {
        int accountsviewsize;
        int periodviewsize;
        QDate date;
        int period;
        bool cumulative;
        QStringList closedItems;
    } accountsview;
    
    struct Reportview {
    } reportview;

private:
    bool m_contextLoaded;
    KoXmlElement m_context;
    KoXmlDocument m_document;
    QDomDocument m_qDomDocument;
    int m_version;
};

}  //KPlato namespace

#endif //CONTEXT_H
