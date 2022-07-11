/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2022 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "SharedResourcesTester.h"

#include <kptpart.h>
#include <kptmaindocument.h>
#include <RemoveResourceCmd.h>
#include <Resource.h>
#include <ResourceGroup.h>

#include <KoXmlReader.h>

#include <QTest>
#include <QTemporaryFile>
#include <QString>

#include <tests/debug.cpp>

using namespace KPlato;

void SharedResources::init()
{
    part = nullptr;
}

void SharedResources::cleanup()
{
    delete part;
    part = nullptr;
}

QString resourceData_1()
{
    return QStringLiteral(
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<!DOCTYPE plan>"
        "<plan editor=\"Plan\" mime=\"application/x-vnd.kde.plan\" version=\"0.7.0\">"
        "<project scheduling=\"MustStartOn\" id=\"20211006081203ph1H0qup6f\" end-time=\"2023-10-06T00:00:00+02:00\" name=\"Version 0.7\" leader=\"\" timezone=\"Europe/Copenhagen\" description=\"&lt;!DOCTYPE HTML PUBLIC &quot;-//W3C//DTD HTML 4.0//EN&quot; &quot;http://www.w3.org/TR/REC-html40/strict.dtd&quot;>&#xa;&lt;html>&lt;head>&lt;meta name=&quot;qrichtext&quot; content=&quot;1&quot; />&lt;style type=&quot;text/css&quot;>&#xa;p, li { white-space: pre-wrap; }&#xa;&lt;/style>&lt;/head>&lt;body style=&quot; font-family:'Noto Sans'; font-size:10pt; font-weight:400; font-style:normal;&quot;>&#xa;&lt;p style=&quot;-qt-paragraph-type:empty; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px; &quot;>&amp;nbsp;&lt;/p>&lt;/body>&lt;/html>\" priority=\"0\" start-time=\"2021-10-06T00:00:00+02:00\">"
        "<project-settings>"
            "<wbs-definition project-separator=\"\" project-code=\"\" levels-enabled=\"0\">"
                "<default code=\"Number\" separator=\".\"/>"
            "</wbs-definition>"
            "<locale language=\"29\" country=\"58\" currency-digits=\"2\" currency-symbol=\"kr.\"/>"
            "<shared-resources file=\"\" use=\"0\"/>"
            "<workpackageinfo delete-after-retrieval=\"0\" check-for-workpackages=\"1\" archive-url=\"\" publish-url=\"\" archive-after-retrieval=\"1\" retrieve-url=\"\"/>"
            "<task-modules use-local-task-modules=\"1\"/>"
            "<standard-worktime month=\"176h0m\" year=\"1760h0m\" day=\"8h0m\" week=\"40h0m\"/>"
        "</project-settings>"
        "<calendars>"
            "<calendar origin=\"local\" id=\"20211006081255aRV6e7i99a\" holiday-region=\"Default\" name=\"Base\" timezone=\"Europe/Copenhagen\" default=\"1\">"
                "<weekday state=\"2\" day=\"0\">"
                "<time-interval start=\"08:00:00\" length=\"28800000\"/>"
                "</weekday>"
                "<weekday state=\"2\" day=\"1\">"
                "<time-interval start=\"08:00:00\" length=\"28800000\"/>"
                "</weekday>"
                "<weekday state=\"2\" day=\"2\">"
                "<time-interval start=\"08:00:00\" length=\"28800000\"/>"
                "</weekday>"
                "<weekday state=\"2\" day=\"3\">"
                "<time-interval start=\"08:00:00\" length=\"28800000\"/>"
                "</weekday>"
                "<weekday state=\"2\" day=\"4\">"
                "<time-interval start=\"08:00:00\" length=\"28800000\"/>"
                "</weekday>"
                "<weekday state=\"1\" day=\"5\"/>"
                "<weekday state=\"1\" day=\"6\"/>"
                "<cache version=\"11\"/>"
            "</calendar>"
        "</calendars>"
        "<resource-groups>"
            "<resource-group type=\"Work\" id=\"20211006081344t9ZgYiKN5y\" name=\"G1\" coordinator=\"\" shared=\"local\"/>"
            "<resource-group type=\"Material\" id=\"20211006081526jm5j2tne1F\" name=\"G2\" coordinator=\"\" shared=\"local\"/>"
        "</resource-groups>"
        "<resources>"
            "<resource units=\"100\" type=\"Work\" origin=\"local\" overtime-rate=\"0\" id=\"20211006081359ejV35m5lJA\" calendar-id=\"20211006081255aRV6e7i99a\" name=\"R1\" initials=\"\" normal-rate=\"100\" auto-allocate=\"0\" email=\"\"/>"
            "<resource units=\"100\" type=\"Work\" origin=\"local\" overtime-rate=\"0\" id=\"20211006081420J3YXxkpnL2\" calendar-id=\"20211006081255aRV6e7i99a\" name=\"R2\" initials=\"\" normal-rate=\"100\" auto-allocate=\"0\" email=\"\"/>"
            "<resource units=\"100\" type=\"Work\" origin=\"local\" overtime-rate=\"0\" id=\"20211006081430tze2MXczbe\" calendar-id=\"20211006081255aRV6e7i99a\" name=\"R3\" initials=\"\" normal-rate=\"100\" auto-allocate=\"0\" email=\"\"/>"
            "<resource units=\"100\" type=\"Team\" origin=\"local\" overtime-rate=\"0\" id=\"20211006081436rDOG9kV5Gp\" name=\"Team 1\" initials=\"\" normal-rate=\"100\" auto-allocate=\"0\" email=\"\"/>"
            "<resource units=\"100\" type=\"Material\" origin=\"local\" overtime-rate=\"0\" id=\"20211006081537NGbGyDTwBI\" name=\"M1\" initials=\"\" normal-rate=\"100\" auto-allocate=\"0\" email=\"\"/>"
            "<resource units=\"100\" type=\"Material\" origin=\"local\" overtime-rate=\"0\" id=\"20211006081545GOUuPpIwEX\" name=\"M2\" initials=\"\" normal-rate=\"100\" auto-allocate=\"0\" email=\"\"/>"
        "</resources>"
        "<resource-group-relations>"
            "<resource-group-relation group-id=\"20211006081344t9ZgYiKN5y\" resource-id=\"20211006081359ejV35m5lJA\"/>"
            "<resource-group-relation group-id=\"20211006081344t9ZgYiKN5y\" resource-id=\"20211006081420J3YXxkpnL2\"/>"
            "<resource-group-relation group-id=\"20211006081344t9ZgYiKN5y\" resource-id=\"20211006081430tze2MXczbe\"/>"
            "<resource-group-relation group-id=\"20211006081344t9ZgYiKN5y\" resource-id=\"20211006081436rDOG9kV5Gp\"/>"
            "<resource-group-relation group-id=\"20211006081526jm5j2tne1F\" resource-id=\"20211006081537NGbGyDTwBI\"/>"
            "<resource-group-relation group-id=\"20211006081526jm5j2tne1F\" resource-id=\"20211006081545GOUuPpIwEX\"/>"
            "</resource-group-relations>"
        "<required-resources>"
            "<required-resource required-id=\"20211006081537NGbGyDTwBI\" resource-id=\"20211006081359ejV35m5lJA\"/>"
            "<required-resource required-id=\"20211006081545GOUuPpIwEX\" resource-id=\"20211006081420J3YXxkpnL2\"/>"
        "</required-resources>"
        "<resource-teams>"
            "<team member-id=\"20211006081359ejV35m5lJA\" team-id=\"20211006081436rDOG9kV5Gp\"/>"
            "<team member-id=\"20211006081420J3YXxkpnL2\" team-id=\"20211006081436rDOG9kV5Gp\"/>"
            "</resource-teams>"
        "</project>"
        "</plan>"
     );
}

QString projectData_1()
{
    return QStringLiteral(
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<!DOCTYPE plan>"
        "<plan editor=\"Plan\" mime=\"application/x-vnd.kde.plan\" version=\"0.7.0\">"
        "<project scheduling=\"MustStartOn\" id=\"20211006081203ph1H0qup6f\" end-time=\"2023-10-06T00:00:00+02:00\" name=\"Version 0.7\" leader=\"\" timezone=\"Europe/Copenhagen\" description=\"&lt;!DOCTYPE HTML PUBLIC &quot;-//W3C//DTD HTML 4.0//EN&quot; &quot;http://www.w3.org/TR/REC-html40/strict.dtd&quot;>&#xa;&lt;html>&lt;head>&lt;meta name=&quot;qrichtext&quot; content=&quot;1&quot; />&lt;style type=&quot;text/css&quot;>&#xa;p, li { white-space: pre-wrap; }&#xa;&lt;/style>&lt;/head>&lt;body style=&quot; font-family:'Noto Sans'; font-size:10pt; font-weight:400; font-style:normal;&quot;>&#xa;&lt;p style=&quot;-qt-paragraph-type:empty; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px; &quot;>&amp;nbsp;&lt;/p>&lt;/body>&lt;/html>\" priority=\"0\" start-time=\"2021-10-06T00:00:00+02:00\">"
        "<project-settings>"
        "<wbs-definition project-separator=\"\" project-code=\"\" levels-enabled=\"0\">"
            "<default code=\"Number\" separator=\".\"/>"
        "</wbs-definition>"
        "<locale language=\"29\" country=\"58\" currency-digits=\"2\" currency-symbol=\"kr.\"/>"
        "<shared-resources file=\"\" use=\"0\"/>"
        "<workpackageinfo delete-after-retrieval=\"0\" check-for-workpackages=\"1\" archive-url=\"\" publish-url=\"\" archive-after-retrieval=\"1\" retrieve-url=\"\"/>"
        "<task-modules use-local-task-modules=\"1\"/>"
        "<standard-worktime month=\"176h0m\" year=\"1760h0m\" day=\"8h0m\" week=\"40h0m\"/>"
        "</project-settings>"
        "<accounts>"
        "<account name=\"Konto.1\" description=\"\">"
            "<account name=\"Konto.1.1\" description=\"\">"
            "<costplace startup-cost=\"0\" shutdown-cost=\"0\" running-cost=\"1\" object-id=\"202110060817364rucEZwY1J\"/>"
            "<costplace startup-cost=\"1\" shutdown-cost=\"0\" running-cost=\"0\" object-id=\"202110060818152MaYHDMFKB\"/>"
            "<costplace startup-cost=\"0\" shutdown-cost=\"1\" running-cost=\"0\" object-id=\"20211006081826IDvVovMHEG\"/>"
            "</account>"
        "</account>"
        "</accounts>"
        "<calendars>"
        "<calendar origin=\"local\" id=\"20211006081255aRV6e7i99a\" holiday-region=\"Default\" name=\"Base\" timezone=\"Europe/Copenhagen\" default=\"1\">"
            "<weekday state=\"2\" day=\"0\">"
            "<time-interval start=\"08:00:00\" length=\"28800000\"/>"
            "</weekday>"
            "<weekday state=\"2\" day=\"1\">"
            "<time-interval start=\"08:00:00\" length=\"28800000\"/>"
            "</weekday>"
            "<weekday state=\"2\" day=\"2\">"
            "<time-interval start=\"08:00:00\" length=\"28800000\"/>"
            "</weekday>"
            "<weekday state=\"2\" day=\"3\">"
            "<time-interval start=\"08:00:00\" length=\"28800000\"/>"
            "</weekday>"
            "<weekday state=\"2\" day=\"4\">"
            "<time-interval start=\"08:00:00\" length=\"28800000\"/>"
            "</weekday>"
            "<weekday state=\"1\" day=\"5\"/>"
            "<weekday state=\"1\" day=\"6\"/>"
            "<cache version=\"11\"/>"
        "</calendar>"
        "</calendars>"
        "<resource-groups>"
        "<resource-group type=\"Work\" id=\"20211006081344t9ZgYiKN5y\" name=\"G1\" coordinator=\"\" shared=\"local\"/>"
        "<resource-group type=\"Material\" id=\"20211006081526jm5j2tne1F\" name=\"G2\" coordinator=\"\" shared=\"local\"/>"
        "</resource-groups>"
        "<resources>"
        "<resource units=\"100\" type=\"Work\" origin=\"local\" overtime-rate=\"0\" id=\"20211006081359ejV35m5lJA\" calendar-id=\"20211006081255aRV6e7i99a\" name=\"R1\" initials=\"\" normal-rate=\"100\" auto-allocate=\"0\" email=\"\"/>"
        "<resource units=\"100\" type=\"Work\" origin=\"local\" overtime-rate=\"0\" id=\"20211006081420J3YXxkpnL2\" calendar-id=\"20211006081255aRV6e7i99a\" name=\"R2\" initials=\"\" normal-rate=\"100\" auto-allocate=\"0\" email=\"\"/>"
        "<resource units=\"100\" type=\"Work\" origin=\"local\" overtime-rate=\"0\" id=\"20211006081430tze2MXczbe\" calendar-id=\"20211006081255aRV6e7i99a\" name=\"R3\" initials=\"\" normal-rate=\"100\" auto-allocate=\"0\" email=\"\"/>"
        "<resource units=\"100\" type=\"Team\" origin=\"local\" overtime-rate=\"0\" id=\"20211006081436rDOG9kV5Gp\" name=\"Team 1\" initials=\"\" normal-rate=\"100\" auto-allocate=\"0\" email=\"\"/>"
        "<resource units=\"100\" type=\"Material\" origin=\"local\" overtime-rate=\"0\" id=\"20211006081537NGbGyDTwBI\" name=\"M1\" initials=\"\" normal-rate=\"100\" auto-allocate=\"0\" email=\"\"/>"
        "<resource units=\"100\" type=\"Material\" origin=\"local\" overtime-rate=\"0\" id=\"20211006081545GOUuPpIwEX\" name=\"M2\" initials=\"\" normal-rate=\"100\" auto-allocate=\"0\" email=\"\"/>"
        "</resources>"
        "<resource-group-relations>"
        "<resource-group-relation group-id=\"20211006081344t9ZgYiKN5y\" resource-id=\"20211006081359ejV35m5lJA\"/>"
        "<resource-group-relation group-id=\"20211006081344t9ZgYiKN5y\" resource-id=\"20211006081420J3YXxkpnL2\"/>"
        "<resource-group-relation group-id=\"20211006081344t9ZgYiKN5y\" resource-id=\"20211006081430tze2MXczbe\"/>"
        "<resource-group-relation group-id=\"20211006081344t9ZgYiKN5y\" resource-id=\"20211006081436rDOG9kV5Gp\"/>"
        "<resource-group-relation group-id=\"20211006081526jm5j2tne1F\" resource-id=\"20211006081537NGbGyDTwBI\"/>"
        "<resource-group-relation group-id=\"20211006081526jm5j2tne1F\" resource-id=\"20211006081545GOUuPpIwEX\"/>"
        "</resource-group-relations>"
        "<required-resources>"
        "<required-resource required-id=\"20211006081537NGbGyDTwBI\" resource-id=\"20211006081359ejV35m5lJA\"/>"
        "<required-resource required-id=\"20211006081545GOUuPpIwEX\" resource-id=\"20211006081420J3YXxkpnL2\"/>"
        "</required-resources>"
        "<resource-teams>"
        "<team member-id=\"20211006081359ejV35m5lJA\" team-id=\"20211006081436rDOG9kV5Gp\"/>"
        "<team member-id=\"20211006081420J3YXxkpnL2\" team-id=\"20211006081436rDOG9kV5Gp\"/>"
        "</resource-teams>"
        "<tasks>"
        "<task scheduling=\"ASAP\" startup-cost=\"0\" constraint-endtime=\"2021-10-06T08:17:30+02:00\" constraint-starttime=\"2021-10-06T08:17:30+02:00\" id=\"20211006081730Uss92rsFaL\" shutdown-cost=\"0\" name=\"S1\" leader=\"\" description=\"&lt;!DOCTYPE HTML PUBLIC &quot;-//W3C//DTD HTML 4.0//EN&quot; &quot;http://www.w3.org/TR/REC-html40/strict.dtd&quot;>&#xa;&lt;html>&lt;head>&lt;meta name=&quot;qrichtext&quot; content=&quot;1&quot; />&lt;style type=&quot;text/css&quot;>&#xa;p, li { white-space: pre-wrap; }&#xa;&lt;/style>&lt;/head>&lt;body style=&quot; font-family:'Noto Sans'; font-size:10pt; font-weight:400; font-style:normal;&quot;>&#xa;&lt;p style=&quot;-qt-paragraph-type:empty; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;&quot;>&lt;br />&lt;/p>&lt;/body>&lt;/html>\" priority=\"0\" wbs=\"1\">"
            "<estimate pessimistic=\"16\" type=\"Effort\" risk=\"None\" optimistic=\"2\" expected=\"8\" unit=\"h\"/>"
            "<task-schedules>"
            "<schedule start=\"2021-10-06T00:00:00+02:00\" resource-overbooked=\"0\" scheduling-conflict=\"0\" earlystart=\"2021-10-06T00:00:00+02:00\" in-critical-path=\"0\" scheduling-error=\"0\" negative-float=\"0 0:0:0.0\" id=\"1\" positive-float=\"0 0:0:0.0\" latefinish=\"2021-10-07T16:00:00+02:00\" type=\"Expected\" not-scheduled=\"0\" name=\"Schedule: Plan\" end=\"2021-10-07T16:00:00+02:00\" resource-error=\"0\" duration=\"1 16:0:0.0\" resource-not-available=\"0\" free-float=\"0 0:0:0.0\"/>"
            "<schedule start=\"2021-10-06T08:25:24+02:00\" resource-overbooked=\"0\" scheduling-conflict=\"0\" earlystart=\"2021-10-06T08:25:24+02:00\" in-critical-path=\"0\" scheduling-error=\"0\" negative-float=\"0 0:0:0.0\" id=\"2\" positive-float=\"0 0:0:0.0\" latefinish=\"2021-10-08T08:25:24+02:00\" type=\"Expected\" not-scheduled=\"0\" name=\"Schedule: Plan.1\" end=\"2021-10-08T08:25:24+02:00\" resource-error=\"0\" duration=\"2 0:0:0.0\" resource-not-available=\"0\" free-float=\"0 0:0:0.0\"/>"
            "</task-schedules>"
            "<progress entrymode=\"EnterEffortPerResource\" startTime=\"\" finishTime=\"\" started=\"0\" finished=\"0\"/>"
            "<workpackage owner=\"\" owner-id=\"\"/>"
            "<task scheduling=\"ASAP\" startup-cost=\"0\" constraint-endtime=\"2021-10-06T08:17:36+02:00\" constraint-starttime=\"2021-10-06T08:17:36+02:00\" id=\"202110060817364rucEZwY1J\" shutdown-cost=\"0\" name=\"T1\" leader=\"\" description=\"&lt;!DOCTYPE HTML PUBLIC &quot;-//W3C//DTD HTML 4.0//EN&quot; &quot;http://www.w3.org/TR/REC-html40/strict.dtd&quot;>&#xa;&lt;html>&lt;head>&lt;meta name=&quot;qrichtext&quot; content=&quot;1&quot; />&lt;style type=&quot;text/css&quot;>&#xa;p, li { white-space: pre-wrap; }&#xa;&lt;/style>&lt;/head>&lt;body style=&quot; font-family:'Noto Sans'; font-size:10pt; font-weight:400; font-style:normal;&quot;>&#xa;&lt;p style=&quot;-qt-paragraph-type:empty; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;&quot;>&lt;br />&lt;/p>&lt;/body>&lt;/html>\" priority=\"0\" wbs=\"1.1\">"
            "<estimate pessimistic=\"16\" type=\"Effort\" risk=\"None\" optimistic=\"2\" expected=\"8\" unit=\"h\"/>"
            "<task-schedules>"
            "<schedule start=\"2021-10-06T08:00:00+02:00\" resource-overbooked=\"0\" scheduling-conflict=\"0\" earlystart=\"2021-10-06T08:00:00+02:00\" in-critical-path=\"1\" scheduling-error=\"0\" negative-float=\"0 0:0:0.0\" id=\"1\" positive-float=\"0 0:0:0.0\" latefinish=\"2021-10-06T16:00:00+02:00\" type=\"Expected\" not-scheduled=\"0\" name=\"Schedule: Plan\" end=\"2021-10-06T16:00:00+02:00\" resource-error=\"0\" earlyfinish=\"2021-10-06T16:00:00+02:00\" duration=\"0 8:0:0.0\" latestart=\"2021-10-06T08:00:00+02:00\" resource-not-available=\"0\" free-float=\"0 16:0:0.0\"/>"
            "<schedule start=\"2021-10-06T08:25:24+02:00\" resource-overbooked=\"0\" scheduling-conflict=\"0\" earlystart=\"2021-10-06T08:25:24+02:00\" in-critical-path=\"1\" scheduling-error=\"0\" negative-float=\"0 0:0:0.0\" id=\"2\" positive-float=\"0 0:0:0.0\" latefinish=\"2021-10-07T08:25:24+02:00\" type=\"Expected\" not-scheduled=\"0\" name=\"Schedule: Plan.1\" end=\"2021-10-07T08:25:24+02:00\" resource-error=\"0\" earlyfinish=\"2021-10-07T08:25:24+02:00\" duration=\"1 0:0:0.0\" latestart=\"2021-10-06T08:25:24+02:00\" resource-not-available=\"0\" free-float=\"0 0:0:0.0\"/>"
            "</task-schedules>"
            "<progress entrymode=\"EnterEffortPerResource\" startTime=\"\" finishTime=\"\" started=\"0\" finished=\"0\"/>"
            "<workpackage owner=\"\" owner-id=\"\"/>"
            "</task>"
            "<task scheduling=\"ASAP\" startup-cost=\"0\" constraint-endtime=\"2021-10-06T08:18:15+02:00\" constraint-starttime=\"2021-10-06T08:18:15+02:00\" id=\"202110060818152MaYHDMFKB\" shutdown-cost=\"0\" name=\"T2\" leader=\"\" description=\"&lt;!DOCTYPE HTML PUBLIC &quot;-//W3C//DTD HTML 4.0//EN&quot; &quot;http://www.w3.org/TR/REC-html40/strict.dtd&quot;>&#xa;&lt;html>&lt;head>&lt;meta name=&quot;qrichtext&quot; content=&quot;1&quot; />&lt;style type=&quot;text/css&quot;>&#xa;p, li { white-space: pre-wrap; }&#xa;&lt;/style>&lt;/head>&lt;body style=&quot; font-family:'Noto Sans'; font-size:10pt; font-weight:400; font-style:normal;&quot;>&#xa;&lt;p style=&quot;-qt-paragraph-type:empty; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;&quot;>&lt;br />&lt;/p>&lt;/body>&lt;/html>\" priority=\"0\" wbs=\"1.2\">"
            "<estimate pessimistic=\"16\" type=\"Effort\" risk=\"None\" optimistic=\"2\" expected=\"8\" unit=\"h\"/>"
            "<task-schedules>"
            "<schedule start=\"2021-10-07T08:00:00+02:00\" resource-overbooked=\"0\" scheduling-conflict=\"0\" earlystart=\"2021-10-07T08:00:00+02:00\" in-critical-path=\"1\" scheduling-error=\"0\" negative-float=\"0 0:0:0.0\" id=\"1\" positive-float=\"0 0:0:0.0\" latefinish=\"2021-10-07T16:00:00+02:00\" type=\"Expected\" not-scheduled=\"0\" name=\"Schedule: Plan\" end=\"2021-10-07T16:00:00+02:00\" resource-error=\"0\" earlyfinish=\"2021-10-07T16:00:00+02:00\" duration=\"0 8:0:0.0\" latestart=\"2021-10-07T08:00:00+02:00\" resource-not-available=\"0\" free-float=\"0 0:0:0.0\"/>"
            "<schedule start=\"2021-10-07T08:25:24+02:00\" resource-overbooked=\"0\" scheduling-conflict=\"0\" earlystart=\"2021-10-07T08:25:24+02:00\" in-critical-path=\"1\" scheduling-error=\"0\" negative-float=\"0 0:0:0.0\" id=\"2\" positive-float=\"0 0:0:0.0\" latefinish=\"2021-10-08T08:25:24+02:00\" type=\"Expected\" not-scheduled=\"0\" name=\"Schedule: Plan.1\" end=\"2021-10-08T08:25:24+02:00\" resource-error=\"0\" earlyfinish=\"2021-10-08T08:25:24+02:00\" duration=\"1 0:0:0.0\" latestart=\"2021-10-07T08:25:24+02:00\" resource-not-available=\"0\" free-float=\"0 0:0:0.0\"/>"
            "</task-schedules>"
            "<progress entrymode=\"EnterEffortPerResource\" startTime=\"\" finishTime=\"\" started=\"0\" finished=\"0\"/>"
            "<workpackage owner=\"\" owner-id=\"\"/>"
            "</task>"
            "<task scheduling=\"ASAP\" startup-cost=\"0\" constraint-endtime=\"2021-10-06T08:18:26+02:00\" constraint-starttime=\"2021-10-06T08:18:26+02:00\" id=\"20211006081826IDvVovMHEG\" shutdown-cost=\"0\" name=\"T3\" leader=\"\" description=\"&lt;!DOCTYPE HTML PUBLIC &quot;-//W3C//DTD HTML 4.0//EN&quot; &quot;http://www.w3.org/TR/REC-html40/strict.dtd&quot;>&#xa;&lt;html>&lt;head>&lt;meta name=&quot;qrichtext&quot; content=&quot;1&quot; />&lt;style type=&quot;text/css&quot;>&#xa;p, li { white-space: pre-wrap; }&#xa;&lt;/style>&lt;/head>&lt;body style=&quot; font-family:'Noto Sans'; font-size:10pt; font-weight:400; font-style:normal;&quot;>&#xa;&lt;p style=&quot;-qt-paragraph-type:empty; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;&quot;>&lt;br />&lt;/p>&lt;/body>&lt;/html>\" priority=\"0\" wbs=\"1.3\">"
            "<estimate pessimistic=\"16\" type=\"Effort\" risk=\"None\" optimistic=\"2\" expected=\"8\" unit=\"h\"/>"
            "<task-schedules>"
            "<schedule start=\"2021-10-07T08:00:00+02:00\" resource-overbooked=\"0\" scheduling-conflict=\"0\" earlystart=\"2021-10-07T08:00:00+02:00\" in-critical-path=\"1\" scheduling-error=\"0\" negative-float=\"0 0:0:0.0\" id=\"1\" positive-float=\"0 0:0:0.0\" latefinish=\"2021-10-07T16:00:00+02:00\" type=\"Expected\" not-scheduled=\"0\" name=\"Schedule: Plan\" end=\"2021-10-07T16:00:00+02:00\" resource-error=\"0\" earlyfinish=\"2021-10-07T12:00:00+02:00\" duration=\"0 8:0:0.0\" latestart=\"2021-10-07T12:00:00+02:00\" resource-not-available=\"1\" free-float=\"0 0:0:0.0\"/>"
            "<schedule start=\"2021-10-07T08:25:24+02:00\" resource-overbooked=\"0\" scheduling-conflict=\"0\" earlystart=\"2021-10-07T08:25:24+02:00\" in-critical-path=\"1\" scheduling-error=\"0\" negative-float=\"0 0:0:0.0\" id=\"2\" positive-float=\"0 0:0:0.0\" latefinish=\"2021-10-08T08:25:24+02:00\" type=\"Expected\" not-scheduled=\"0\" name=\"Schedule: Plan.1\" end=\"2021-10-08T08:25:24+02:00\" resource-error=\"0\" earlyfinish=\"2021-10-07T12:25:24+02:00\" duration=\"1 0:0:0.0\" latestart=\"2021-10-07T12:25:24+02:00\" resource-not-available=\"1\" free-float=\"0 0:0:0.0\"/>"
            "</task-schedules>"
            "<progress entrymode=\"EnterEffortPerResource\" startTime=\"\" finishTime=\"\" started=\"0\" finished=\"0\"/>"
            "<workpackage owner=\"\" owner-id=\"\"/>"
            "</task>"
            "<task scheduling=\"ASAP\" startup-cost=\"0\" constraint-endtime=\"2021-10-06T08:18:45+02:00\" constraint-starttime=\"2021-10-06T08:18:45+02:00\" id=\"20211006081845PmuFUf7lQz\" shutdown-cost=\"0\" name=\"T4\" leader=\"\" description=\"&lt;!DOCTYPE HTML PUBLIC &quot;-//W3C//DTD HTML 4.0//EN&quot; &quot;http://www.w3.org/TR/REC-html40/strict.dtd&quot;>&#xa;&lt;html>&lt;head>&lt;meta name=&quot;qrichtext&quot; content=&quot;1&quot; />&lt;style type=&quot;text/css&quot;>&#xa;p, li { white-space: pre-wrap; }&#xa;&lt;/style>&lt;/head>&lt;body style=&quot; font-family:'Noto Sans'; font-size:10pt; font-weight:400; font-style:normal;&quot;>&#xa;&lt;p style=&quot;-qt-paragraph-type:empty; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;&quot;>&lt;br />&lt;/p>&lt;/body>&lt;/html>\" priority=\"0\" wbs=\"1.4\">"
            "<estimate pessimistic=\"16\" type=\"Duration\" risk=\"None\" calendar-id=\"20211006081255aRV6e7i99a\" optimistic=\"2\" expected=\"8\" unit=\"h\"/>"
            "<task-schedules>"
            "<schedule start=\"2021-10-07T08:00:00+02:00\" resource-overbooked=\"0\" scheduling-conflict=\"0\" earlystart=\"2021-10-07T08:00:00+02:00\" in-critical-path=\"1\" scheduling-error=\"0\" negative-float=\"0 0:0:0.0\" id=\"1\" positive-float=\"0 0:0:0.0\" latefinish=\"2021-10-07T16:00:00+02:00\" type=\"Expected\" not-scheduled=\"0\" name=\"Schedule: Plan\" end=\"2021-10-07T16:00:00+02:00\" resource-error=\"0\" earlyfinish=\"2021-10-07T16:00:00+02:00\" duration=\"0 8:0:0.0\" latestart=\"2021-10-07T08:00:00+02:00\" resource-not-available=\"0\" free-float=\"0 0:0:0.0\"/>"
            "<schedule start=\"2021-10-07T08:25:24+02:00\" resource-overbooked=\"0\" scheduling-conflict=\"0\" earlystart=\"2021-10-07T08:25:24+02:00\" in-critical-path=\"1\" scheduling-error=\"0\" negative-float=\"0 0:0:0.0\" id=\"2\" positive-float=\"0 0:0:0.0\" latefinish=\"2021-10-08T08:25:24+02:00\" type=\"Expected\" not-scheduled=\"0\" name=\"Schedule: Plan.1\" end=\"2021-10-08T08:25:24+02:00\" resource-error=\"0\" earlyfinish=\"2021-10-08T08:25:24+02:00\" duration=\"1 0:0:0.0\" latestart=\"2021-10-07T08:25:24+02:00\" resource-not-available=\"0\" free-float=\"0 0:0:0.0\"/>"
            "</task-schedules>"
            "<progress entrymode=\"EnterEffortPerResource\" startTime=\"\" finishTime=\"\" started=\"0\" finished=\"0\"/>"
            "<workpackage owner=\"\" owner-id=\"\"/>"
            "</task>"
            "<task scheduling=\"ASAP\" startup-cost=\"0\" constraint-endtime=\"2021-10-06T08:20:09+02:00\" constraint-starttime=\"2021-10-06T08:20:09+02:00\" id=\"2021100608200930vDAd9r1G\" shutdown-cost=\"0\" name=\"T5\" leader=\"\" description=\"&lt;!DOCTYPE HTML PUBLIC &quot;-//W3C//DTD HTML 4.0//EN&quot; &quot;http://www.w3.org/TR/REC-html40/strict.dtd&quot;>&#xa;&lt;html>&lt;head>&lt;meta name=&quot;qrichtext&quot; content=&quot;1&quot; />&lt;style type=&quot;text/css&quot;>&#xa;p, li { white-space: pre-wrap; }&#xa;&lt;/style>&lt;/head>&lt;body style=&quot; font-family:'Noto Sans'; font-size:10pt; font-weight:400; font-style:normal;&quot;>&#xa;&lt;p style=&quot;-qt-paragraph-type:empty; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;&quot;>&lt;br />&lt;/p>&lt;/body>&lt;/html>\" priority=\"0\" wbs=\"1.5\">"
            "<estimate pessimistic=\"16\" type=\"Duration\" risk=\"None\" optimistic=\"2\" expected=\"8\" unit=\"h\"/>"
            "<task-schedules>"
            "<schedule start=\"2021-10-06T00:00:00+02:00\" resource-overbooked=\"0\" scheduling-conflict=\"0\" earlystart=\"2021-10-06T00:00:00+02:00\" in-critical-path=\"0\" scheduling-error=\"0\" negative-float=\"0 0:0:0.0\" id=\"1\" positive-float=\"1 8:0:0.0\" latefinish=\"2021-10-07T16:00:00+02:00\" type=\"Expected\" not-scheduled=\"0\" name=\"Schedule: Plan\" end=\"2021-10-06T08:00:00+02:00\" resource-error=\"0\" earlyfinish=\"2021-10-06T08:00:00+02:00\" duration=\"0 8:0:0.0\" latestart=\"2021-10-07T08:00:00+02:00\" resource-not-available=\"1\" free-float=\"0 0:0:0.0\"/>"
            "<schedule start=\"2021-10-06T08:25:24+02:00\" resource-overbooked=\"0\" scheduling-conflict=\"0\" earlystart=\"2021-10-06T08:25:24+02:00\" in-critical-path=\"0\" scheduling-error=\"0\" negative-float=\"0 0:0:0.0\" id=\"2\" positive-float=\"1 16:0:0.0\" latefinish=\"2021-10-08T08:25:24+02:00\" type=\"Expected\" not-scheduled=\"0\" name=\"Schedule: Plan.1\" end=\"2021-10-06T16:25:24+02:00\" resource-error=\"0\" earlyfinish=\"2021-10-06T16:25:24+02:00\" duration=\"0 8:0:0.0\" latestart=\"2021-10-08T00:25:24+02:00\" resource-not-available=\"0\" free-float=\"0 0:0:0.0\"/>"
            "</task-schedules>"
            "<progress entrymode=\"EnterEffortPerResource\" startTime=\"\" finishTime=\"\" started=\"0\" finished=\"0\"/>"
            "<workpackage owner=\"\" owner-id=\"\"/>"
            "</task>"
        "</task>"
        "</tasks>"
        "<relations>"
        "<relation child-id=\"202110060818152MaYHDMFKB\" type=\"Finish-Start\" parent-id=\"202110060817364rucEZwY1J\" lag=\"0 0:0:0.0\"/>"
        "<relation child-id=\"20211006081826IDvVovMHEG\" type=\"Start-Start\" parent-id=\"202110060818152MaYHDMFKB\" lag=\"0 0:0:0.0\"/>"
        "<relation child-id=\"20211006081845PmuFUf7lQz\" type=\"Finish-Finish\" parent-id=\"20211006081826IDvVovMHEG\" lag=\"0 0:0:0.0\"/>"
        "</relations>"
        "<project-schedules>"
        "<schedule-management scheduling-mode=\"0\" id=\"DOeTk4zjIO\" baselined=\"0\" recalculate-from=\"\" overbooking=\"0\" scheduler-plugin-id=\"\" name=\"Plan\" scheduling-direction=\"0\" distribution=\"0\" granularityIndex=\"0\" check-external-appointments=\"1\" recalculate=\"0\">"
            "<schedule start=\"2021-10-06T00:00:00+02:00\" type=\"Expected\" id=\"1\" name=\"Schedule: Plan\" not-scheduled=\"0\" scheduling-conflict=\"0\" duration=\"1 16:0:0.0\" end=\"2021-10-07T16:00:00+02:00\" scheduling-error=\"0\">"
            "<criticalpath-list>"
            "<criticalpath>"
            "<node id=\"202110060817364rucEZwY1J\"/>"
            "<node id=\"202110060818152MaYHDMFKB\"/>"
            "<node id=\"20211006081826IDvVovMHEG\"/>"
            "<node id=\"20211006081845PmuFUf7lQz\"/>"
            "</criticalpath>"
            "</criticalpath-list>"
            "<appointment resource-id=\"20211006081359ejV35m5lJA\" task-id=\"202110060817364rucEZwY1J\">"
            "<appointment-interval start=\"2021-10-06T08:00:00+02:00\" load=\"100\" end=\"2021-10-06T16:00:00+02:00\"/>"
            "</appointment>"
            "<appointment resource-id=\"20211006081537NGbGyDTwBI\" task-id=\"202110060817364rucEZwY1J\">"
            "<appointment-interval start=\"2021-10-06T08:00:00+02:00\" load=\"100\" end=\"2021-10-06T16:00:00+02:00\"/>"
            "</appointment>"
            "<appointment resource-id=\"20211006081420J3YXxkpnL2\" task-id=\"202110060818152MaYHDMFKB\">"
            "<appointment-interval start=\"2021-10-07T08:00:00+02:00\" load=\"100\" end=\"2021-10-07T16:00:00+02:00\"/>"
            "</appointment>"
            "<appointment resource-id=\"20211006081545GOUuPpIwEX\" task-id=\"202110060818152MaYHDMFKB\">"
            "<appointment-interval start=\"2021-10-07T08:00:00+02:00\" load=\"100\" end=\"2021-10-07T16:00:00+02:00\"/>"
            "</appointment>"
            "<appointment resource-id=\"20211006081359ejV35m5lJA\" task-id=\"20211006081826IDvVovMHEG\">"
            "<appointment-interval start=\"2021-10-07T08:00:00+02:00\" load=\"100\" end=\"2021-10-07T16:00:00+02:00\"/>"
            "</appointment>"
            "<appointment resource-id=\"20211006081430tze2MXczbe\" task-id=\"20211006081845PmuFUf7lQz\">"
            "<appointment-interval start=\"2021-10-07T08:00:00+02:00\" load=\"100\" end=\"2021-10-07T16:00:00+02:00\"/>"
            "</appointment>"
            "</schedule>"
            "<schedule-management scheduling-mode=\"0\" id=\"UwMZ3fq6BM\" baselined=\"0\" recalculate-from=\"2021-10-06T08:25:24+02:00\" overbooking=\"0\" scheduler-plugin-id=\"\" name=\"Plan.1\" scheduling-direction=\"0\" distribution=\"0\" granularityIndex=\"0\" check-external-appointments=\"1\" recalculate=\"1\">"
            "<schedule start=\"2021-10-06T00:00:00+02:00\" type=\"Expected\" id=\"2\" name=\"Schedule: Plan.1\" not-scheduled=\"0\" scheduling-conflict=\"0\" duration=\"0 0:0:0.0\" end=\"2021-10-08T08:25:24+02:00\" scheduling-error=\"0\">"
            "<criticalpath-list>"
            "<criticalpath>"
                "<node id=\"202110060817364rucEZwY1J\"/>"
                "<node id=\"202110060818152MaYHDMFKB\"/>"
                "<node id=\"20211006081826IDvVovMHEG\"/>"
                "<node id=\"20211006081845PmuFUf7lQz\"/>"
            "</criticalpath>"
            "</criticalpath-list>"
            "<appointment resource-id=\"20211006081359ejV35m5lJA\" task-id=\"202110060817364rucEZwY1J\">"
            "<appointment-interval start=\"2021-10-06T08:25:24+02:00\" load=\"100\" end=\"2021-10-06T16:00:00+02:00\"/>"
            "<appointment-interval start=\"2021-10-07T08:00:00+02:00\" load=\"100\" end=\"2021-10-07T08:25:24+02:00\"/>"
            "</appointment>"
            "<appointment resource-id=\"20211006081537NGbGyDTwBI\" task-id=\"202110060817364rucEZwY1J\">"
            "<appointment-interval start=\"2021-10-06T08:25:24+02:00\" load=\"100\" end=\"2021-10-06T16:00:00+02:00\"/>"
            "<appointment-interval start=\"2021-10-07T08:00:00+02:00\" load=\"100\" end=\"2021-10-07T08:25:24+02:00\"/>"
            "</appointment>"
            "<appointment resource-id=\"20211006081420J3YXxkpnL2\" task-id=\"202110060818152MaYHDMFKB\">"
            "<appointment-interval start=\"2021-10-07T08:25:24+02:00\" load=\"100\" end=\"2021-10-07T16:00:00+02:00\"/>"
            "<appointment-interval start=\"2021-10-08T08:00:00+02:00\" load=\"100\" end=\"2021-10-08T08:25:24+02:00\"/>"
            "</appointment>"
            "<appointment resource-id=\"20211006081545GOUuPpIwEX\" task-id=\"202110060818152MaYHDMFKB\">"
            "<appointment-interval start=\"2021-10-07T08:25:24+02:00\" load=\"100\" end=\"2021-10-07T16:00:00+02:00\"/>"
            "<appointment-interval start=\"2021-10-08T08:00:00+02:00\" load=\"100\" end=\"2021-10-08T08:25:24+02:00\"/>"
            "</appointment>"
            "<appointment resource-id=\"20211006081359ejV35m5lJA\" task-id=\"20211006081826IDvVovMHEG\">"
            "<appointment-interval start=\"2021-10-07T08:25:24+02:00\" load=\"100\" end=\"2021-10-07T16:00:00+02:00\"/>"
            "<appointment-interval start=\"2021-10-08T08:00:00+02:00\" load=\"100\" end=\"2021-10-08T08:25:24+02:00\"/>"
            "</appointment>"
            "<appointment resource-id=\"20211006081430tze2MXczbe\" task-id=\"20211006081845PmuFUf7lQz\">"
            "<appointment-interval start=\"2021-10-07T08:25:24+02:00\" load=\"100\" end=\"2021-10-07T16:00:00+02:00\"/>"
            "<appointment-interval start=\"2021-10-08T08:00:00+02:00\" load=\"100\" end=\"2021-10-08T08:25:24+02:00\"/>"
            "</appointment>"
            "<appointment resource-id=\"20211006081430tze2MXczbe\" task-id=\"2021100608200930vDAd9r1G\">"
            "<appointment-interval start=\"2021-10-06T08:25:24+02:00\" load=\"100\" end=\"2021-10-06T16:00:00+02:00\"/>"
            "</appointment>"
            "</schedule>"
            "</schedule-management>"
        "</schedule-management>"
        "</project-schedules>"
        "<resource-requests>"
        "<resource-request units=\"100\" request-id=\"1\" resource-id=\"20211006081430tze2MXczbe\" task-id=\"20211006081845PmuFUf7lQz\"/>"
        "<resource-request units=\"100\" request-id=\"1\" resource-id=\"20211006081420J3YXxkpnL2\" task-id=\"202110060818152MaYHDMFKB\"/>"
        "<resource-request units=\"100\" request-id=\"1\" resource-id=\"20211006081430tze2MXczbe\" task-id=\"2021100608200930vDAd9r1G\"/>"
        "<resource-request units=\"100\" request-id=\"1\" resource-id=\"20211006081359ejV35m5lJA\" task-id=\"202110060817364rucEZwY1J\"/>"
        "<resource-request units=\"100\" request-id=\"1\" resource-id=\"20211006081436rDOG9kV5Gp\" task-id=\"20211006081826IDvVovMHEG\"/>"
        "</resource-requests>"
        "<required-resource-requests>"
        "<required-resource-request required-id=\"20211006081545GOUuPpIwEX\" request-id=\"1\" task-id=\"202110060818152MaYHDMFKB\"/>"
        "<required-resource-request required-id=\"20211006081537NGbGyDTwBI\" request-id=\"1\" task-id=\"202110060817364rucEZwY1J\"/>"
        "</required-resource-requests>"
        "</project>"
        "</plan>"
     );
}

MainDocument *loadXml(const QString &xml)
{
    Part *part = new Part(nullptr);
    auto doc = part->createDocument(part);
    KoXmlDocument xmlDoc;
    xmlDoc.setContent(xml);
    doc->loadXML(xmlDoc, nullptr);
    return qobject_cast<MainDocument*>(doc);
}

void checkSharedResources(const Project *sharedProject)
{
    QCOMPARE(sharedProject->calendarCount(), 1);
    QCOMPARE(sharedProject->resourceGroupCount(), 2);
    QCOMPARE(sharedProject->resourceCount(), 6);

    const auto G1 = sharedProject->resourceGroups().value(0);
    const auto G2 = sharedProject->resourceGroups().value(1);
    QCOMPARE(G1->resources().count(), 4);
    QCOMPARE(G1->resources().value(0), sharedProject->resourceAt(0));
    QCOMPARE(G1->resources().value(1), sharedProject->resourceAt(1));
    QCOMPARE(G1->resources().value(2), sharedProject->resourceAt(2));
    QCOMPARE(G1->resources().value(3), sharedProject->resourceAt(3));

    QCOMPARE(G2->resources().value(0), sharedProject->resourceAt(4));
    QCOMPARE(G2->resources().value(1), sharedProject->resourceAt(5));
}

void SharedResources::testEmpty()
{
    auto sharedDoc = loadXml(resourceData_1());
    QVERIFY(sharedDoc);
    auto sharedProject = sharedDoc->project();
    checkSharedResources(sharedProject);

    Part part(nullptr);
    auto doc = qobject_cast<MainDocument*>(part.createDocument(&part));
    part.setDocument(doc);
//    doc->setProperty(NOUI, true);
//    doc->setProperty(DEFAULTSHAREDRESOURCESRESULT, 2 /*keep*/);

    QVERIFY(static_cast<MainDocument*>(doc)->mergeResources(*sharedProject));

    QCOMPARE(doc->project()->calendarCount(), 1);
    QCOMPARE(doc->project()->resourceGroupCount(), 2);
    QCOMPARE(doc->project()->resourceCount(), 6);

    delete sharedDoc;
}

void SharedResources::testRemoveResource()
{
    auto sharedDoc = loadXml(resourceData_1());
    QVERIFY(sharedDoc);
    auto sharedProject = sharedDoc->project();
    checkSharedResources(sharedProject);

    Part part(nullptr);
    auto doc = qobject_cast<MainDocument*>(part.createDocument(&part));
    part.setDocument(doc);

    QVERIFY(doc->mergeResources(*sharedProject));
    QCOMPARE(doc->project()->calendarCount(), 1);
    QCOMPARE(doc->project()->resourceGroupCount(), 2);
    QCOMPARE(doc->project()->resourceCount(), 6);

    delete sharedDoc;
    sharedDoc = loadXml(resourceData_1());
    sharedProject = sharedDoc->project();

    RemoveResourceCmd cmd(sharedProject->resourceList().value(0));
    cmd.redo();
    QCOMPARE(sharedProject->resourceCount(), 5);

    doc->setProperty(NOUI, true);
    doc->setProperty(SHAREDRESOURCESACTION, SHAREDRESOURCESREMOVE);
    auto project = doc->project();
    doc->mergeResources(*sharedProject);

    QCOMPARE(project->calendarCount(), 1);
    QCOMPARE(project->resourceGroupCount(), 2);
    QCOMPARE(project->resourceCount(), 5);

    const auto G1 = project->resourceGroups().value(0);
    const auto G2 = project->resourceGroups().value(1);
    QCOMPARE(G1->resources().count(), 3);
    QStringList names = {QStringLiteral("R2"),QStringLiteral("R3"),QStringLiteral("Team 1")};
    for (const auto r : G1->resources()) {
        QVERIFY(names.contains(r->name()));
    }

    QStringList names2 = {QStringLiteral("M1"), QStringLiteral("M2")};
    for (const auto r : G2->resources()) {
        QVERIFY(names2.contains(r->name()));
    }

    delete sharedDoc;
}

void SharedResources::testConvertResource()
{
    auto sharedDoc = loadXml(resourceData_1());
    QVERIFY(sharedDoc);
    auto sharedProject = sharedDoc->project();
    checkSharedResources(sharedProject);

    Part part(nullptr);
    auto doc = qobject_cast<MainDocument*>(part.createDocument(&part));
    part.setDocument(doc);

    QVERIFY(doc->mergeResources(*sharedProject));
    QCOMPARE(doc->project()->calendarCount(), 1);
    QCOMPARE(doc->project()->resourceGroupCount(), 2);
    QCOMPARE(doc->project()->resourceCount(), 6);

    delete sharedDoc;
    sharedDoc = loadXml(resourceData_1());
    sharedProject = sharedDoc->project();

    RemoveResourceCmd cmd(sharedProject->resourceList().value(0));
    cmd.redo();
    QCOMPARE(sharedProject->resourceCount(), 5);

    doc->setProperty(NOUI, true);
    doc->setProperty(SHAREDRESOURCESACTION, SHAREDRESOURCESCONVERT);
    auto project = doc->project();
    doc->mergeResources(*sharedProject);

    QCOMPARE(project->calendarCount(), 1);
    QCOMPARE(project->resourceGroupCount(), 2);
    QCOMPARE(project->resourceCount(), 6);

    //Debug::print(project, "", true);

    QVERIFY(!project->resourceList().value(0)->isShared());

    const auto G1 = project->resourceGroups().value(0);
    const auto G2 = project->resourceGroups().value(1);
    QCOMPARE(G1->resources().count(), 4);
    QStringList names = {QStringLiteral("R1"), QStringLiteral("R2"),QStringLiteral("R3"),QStringLiteral("Team 1")};
    for (const auto r : G1->resources()) {
        QVERIFY(names.contains(r->name()));
    }

    QStringList names2 = {QStringLiteral("M1"), QStringLiteral("M2")};
    for (const auto r : G2->resources()) {
        QVERIFY(names2.contains(r->name()));
    }

    delete sharedDoc;
}


QTEST_MAIN(SharedResources)
