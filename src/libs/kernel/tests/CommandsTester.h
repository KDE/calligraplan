/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2011 Pierre Stirnweiss <pstirnweiss@googlemail.com>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef COMMANDSTESTER_H
#define COMMANDSTESTER_H

#include <QObject>

namespace KPlato {

class Project;

class CommandsTester : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void testNamedCommand();

    void testCalendarAddCmd();
    void testCalendarRemoveCmd();
    void testCalendarMoveCmd();
    void testCalendarModifyNameCmd();
    void testCalendarModifyParentCmd();
    void testCalendarModifyTimeZoneCmd();
    void testCalendarAddDayCmd();
    void testCalendarRemoveDayCmd();
    void testCalendarModifyDayCmd();
    void testCalendarModifyStateCmd();
    void testCalendarModifyTimeIntervalCmd();
    void testCalendarAddTimeIntervalCmd();
    void testCalendarRemoveTimeIntervalCmd();
    void testCalendarModifyWeekdayCmd();
    void testCalendarModifyDateCmd();
    void testProjectModifyDefaultCalendarCmd();

private:
    Project *m_project;
};

} // namespace KPlato

#endif // COMMANDSTESTER_H
