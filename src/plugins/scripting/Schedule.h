/* This file is part of the Calligra project
 * SPDX-FileCopyrightText: 2008 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef SCRIPTING_SCHEDULE_H
#define SCRIPTING_SCHEDULE_H

#include <QObject>
#include <QDate>
#include <QVariant>


namespace KPlato {
    class ScheduleManager;
}

namespace Scripting {
    class Project;
    class Schedule;

    /**
    * The Schedule class represents a schedule manager in a project.
    */
    class Schedule : public QObject
    {
            Q_OBJECT
        public:
            /// Create a schedule
            Schedule(Project *project, KPlato::ScheduleManager *schedule, QObject *parent);
            /// Destructor
            virtual ~Schedule() {}
        
        public Q_SLOTS:
            qlonglong id() const;
            QString name() const;
            bool isScheduled() const;
            
            QDate startDate();
            QDate endDate();
            
            /// Return type of schedule
            int childCount() const;
            /// Return the child schedule at @p index
            QObject *childAt(int index);

        private:
            Project *m_project;
            KPlato::ScheduleManager *m_schedule;
    };

}

#endif
