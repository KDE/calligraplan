/* This file is part of the Calligra project
 * SPDX-FileCopyrightText: 2008 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef SCRIPTING_CALENDAR_H
#define SCRIPTING_CALENDAR_H

#include <QObject>
#include <QVariant>

namespace KPlato {
    class Calendar;
}

namespace Scripting {
    class Project;
    class Calendar;

    /**
    * The Calendar class represents a calendar in a project.
    */
    class Calendar : public QObject
    {
            Q_OBJECT
        public:
            /// Create a calendar
            Calendar(Project *project, KPlato::Calendar *calendar, QObject *parent);
            /// Destructor
            virtual ~Calendar() {}
        
            KPlato::Calendar *kplatoCalendar() const { return m_calendar; }
            
        public Q_SLOTS:
            /// Return the project this calendar is part of
            QObject* project();
            /// Return the calendars id
            QString id() const;
            /// Return the calendars id
            QString name() const;
            /// Return number of child calendars
            int childCount() const;
            /// Return the child calendar at @p index
            QObject *childAt(int index);
        
        protected:
            Project *m_project;
            KPlato::Calendar *m_calendar;
    };

}

#endif
