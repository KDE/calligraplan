/* This file is part of the Calligra project
 * SPDX-FileCopyrightText: 2008 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef SCRIPTING_NODE_H
#define SCRIPTING_NODE_H

#include <QObject>
#include <QVariant>
#include <QDate>

namespace KPlato {
    class Node;
}

namespace Scripting {
    class Project;
    class Node;

    /**
    * The Node class represents a node in a project.
    */
    class Node : public QObject
    {
            Q_OBJECT
        public:
            /// Create a node
            Node(Project *project, KPlato::Node *node, QObject *parent);
            /// Destructor
            virtual ~Node() {}
        
            KPlato::Node *kplatoNode() const { return m_node; }
            
        public Q_SLOTS:
            /// Return the project this task is part of
            QObject* project();
            /// Return the tasks name
            QString name();

            QDate startDate();
            QDate endDate();
            
            /// Return the nodes id
            QString id();
            /// Return type of node
            QVariant type();
            /// Return number of child nodes
            int childCount() const;
            /// Return the child node at @p index
            QObject *childAt(int index);
            /// Return the data
            QObject *parentNode();

            /// Return a map of planned effort and cost pr day
            QVariant plannedEffortCostPrDay(const QVariant &start, const QVariant &end, const QVariant &schedule);

            /// Return a map of Budgeted Cost of Work Scheduled pr day
            QVariant bcwsPrDay(const QVariant &schedule) const;
            /// Return a map of Budgeted Cost of Work Performed pr day
            QVariant bcwpPrDay(const QVariant &schedule) const;
            /// Return a map of Actual Cost of Work Performed pr day
            QVariant acwpPrDay(const QVariant &schedule) const;

        protected:
            Project *m_project;
            KPlato::Node *m_node;
    };

}

#endif
