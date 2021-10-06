/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
   
   SPDX-License-Identifier: LGPL-2.0-or-later
*/


#include "XmlLoaderTester.h"

#include "kptxmlloaderobject.h"

#include <QTest>
#include <QString>

#include <KoXmlReader.h>

#include "debug.cpp"

using namespace KPlato;

QString XmlLoaderTester::data_v0_6() const
{
    return QString(
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<!DOCTYPE plan>"
        "<plan version=\"0.6.7\" editor=\"Plan\" mime=\"application/x-vnd.kde.plan\">"
        "<project description=\"&lt;!DOCTYPE HTML PUBLIC &quot;-//W3C//DTD HTML 4.0//EN&quot; &quot;http://www.w3.org/TR/REC-html40/strict.dtd&quot;>&#xa;&lt;html>&lt;head>&lt;meta name=&quot;qrichtext&quot; content=&quot;1&quot; />&lt;style type=&quot;text/css&quot;>&#xa;p, li { white-space: pre-wrap; }&#xa;&lt;/style>&lt;/head>&lt;body style=&quot; font-family:'Noto Sans'; font-size:10pt; font-weight:400; font-style:normal;&quot;>&#xa;&lt;p style=&quot;-qt-paragraph-type:empty; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px; &quot;>&amp;nbsp;&lt;/p>&lt;/body>&lt;/html>\" priority=\"0\" end-time=\"2023-10-06T00:00:00\" leader=\"\" id=\"20211006081203ph1H0qup6f\" start-time=\"2021-10-06T00:00:00\" timezone=\"Europe/Copenhagen\" scheduling=\"MustStartOn\" name=\"Version 0.6\">"
        "<wbs-definition project-code=\"\" levels-enabled=\"0\" project-separator=\"\">"
        "<default separator=\".\" code=\"Number\"/>"
        "</wbs-definition>"
        "<locale language=\"29\" currency-symbol=\"kr.\" country=\"58\" currency-digits=\"2\"/>"
        "<shared-resources file=\"\" projects-url=\"\" use=\"0\" projects-loadatstartup=\"0\"/>"
        "<workpackageinfo archive-after-retrieval=\"1\" archive-url=\"\" retrieve-url=\"\" delete-after-retrieval=\"0\" publish-url=\"\" check-for-workpackages=\"1\"/>"
        "<task-modules use-local-task-modules=\"1\"/>"
        "<accounts>"
        "<account description=\"\" name=\"Konto.1\">"
            "<account description=\"\" name=\"Konto.1.1\">"
            "<costplace shutdown-cost=\"0\" object-id=\"202110060817364rucEZwY1J\" startup-cost=\"0\" running-cost=\"1\"/>"
            "<costplace shutdown-cost=\"0\" object-id=\"202110060818152MaYHDMFKB\" startup-cost=\"1\" running-cost=\"0\"/>"
            "<costplace shutdown-cost=\"1\" object-id=\"20211006081826IDvVovMHEG\" startup-cost=\"0\" running-cost=\"0\"/>"
            "</account>"
        "</account>"
        "</accounts>"
        "<calendar shared=\"0\" holiday-region=\"Default\" default=\"1\" id=\"20211006081255aRV6e7i99a\" timezone=\"Europe/Copenhagen\" name=\"Base\">"
        "<weekday day=\"0\" state=\"2\">"
            "<interval start=\"08:00:00\" length=\"28800000\"/>"
        "</weekday>"
        "<weekday day=\"1\" state=\"2\">"
            "<interval start=\"08:00:00\" length=\"28800000\"/>"
        "</weekday>"
        "<weekday day=\"2\" state=\"2\">"
            "<interval start=\"08:00:00\" length=\"28800000\"/>"
        "</weekday>"
        "<weekday day=\"3\" state=\"2\">"
            "<interval start=\"08:00:00\" length=\"28800000\"/>"
        "</weekday>"
        "<weekday day=\"4\" state=\"2\">"
            "<interval start=\"08:00:00\" length=\"28800000\"/>"
        "</weekday>"
        "<weekday day=\"5\" state=\"1\"/>"
        "<weekday day=\"6\" state=\"1\"/>"
        "<cache version=\"11\"/>"
        "</calendar>"
        "<standard-worktime year=\"1760h0m\" month=\"176h0m\" day=\"8h0m\" week=\"40h0m\"/>"
        "<resource-group type=\"Work\" shared=\"0\" id=\"20211006081344t9ZgYiKN5y\" name=\"G1\">"
        "<resource type=\"Work\" shared=\"0\" overtime-rate=\"0\" email=\"\" units=\"100\" normal-rate=\"100\" initials=\"\" id=\"20211006081359ejV35m5lJA\" auto-allocate=\"0\" calendar-id=\"20211006081255aRV6e7i99a\" name=\"R1\">"
            "<required-resources>"
            "<resource id=\"20211006081537NGbGyDTwBI\"/>"
            "</required-resources>"
            "<work-intervals-cache effort=\"0 00:00:00.0\" start=\"2021-10-06T08:00:00+02:00\" version=\"11\" end=\"2021-10-08T08:25:24+02:00\">"
            "<intervals>"
            "<interval start=\"2021-10-06T08:00:00+02:00\" load=\"100\" end=\"2021-10-06T16:00:00+02:00\"/>"
            "<interval start=\"2021-10-07T08:00:00+02:00\" load=\"100\" end=\"2021-10-07T16:00:00+02:00\"/>"
            "<interval start=\"2021-10-08T08:00:00+02:00\" load=\"100\" end=\"2021-10-08T08:25:24+02:00\"/>"
            "</intervals>"
            "</work-intervals-cache>"
        "</resource>"
        "<resource type=\"Work\" shared=\"0\" overtime-rate=\"0\" email=\"\" units=\"100\" normal-rate=\"100\" initials=\"\" id=\"20211006081420J3YXxkpnL2\" auto-allocate=\"0\" calendar-id=\"20211006081255aRV6e7i99a\" name=\"R2\">"
            "<required-resources>"
            "<resource id=\"20211006081545GOUuPpIwEX\"/>"
            "</required-resources>"
            "<work-intervals-cache effort=\"0 00:00:00.0\" start=\"2021-10-06T08:00:00+02:00\" version=\"11\" end=\"2021-10-08T12:25:24+02:00\">"
            "<intervals>"
            "<interval start=\"2021-10-06T08:00:00+02:00\" load=\"100\" end=\"2021-10-06T16:00:00+02:00\"/>"
            "<interval start=\"2021-10-07T08:00:00+02:00\" load=\"100\" end=\"2021-10-07T16:00:00+02:00\"/>"
            "<interval start=\"2021-10-08T08:00:00+02:00\" load=\"100\" end=\"2021-10-08T12:00:00+02:00\"/>"
            "<interval start=\"2021-10-08T12:00:00+02:00\" load=\"100\" end=\"2021-10-08T12:25:24+02:00\"/>"
            "</intervals>"
            "</work-intervals-cache>"
        "</resource>"
        "<resource type=\"Work\" shared=\"0\" overtime-rate=\"0\" email=\"\" units=\"100\" normal-rate=\"100\" initials=\"\" id=\"20211006081430tze2MXczbe\" auto-allocate=\"0\" calendar-id=\"20211006081255aRV6e7i99a\" name=\"R3\">"
            "<work-intervals-cache effort=\"0 00:00:00.0\" start=\"2021-10-06T08:25:24+02:00\" version=\"11\" end=\"2021-10-08T08:25:24+02:00\">"
            "<intervals>"
            "<interval start=\"2021-10-06T08:25:24+02:00\" load=\"100\" end=\"2021-10-06T16:00:00+02:00\"/>"
            "<interval start=\"2021-10-07T08:00:00+02:00\" load=\"100\" end=\"2021-10-07T16:00:00+02:00\"/>"
            "<interval start=\"2021-10-08T08:00:00+02:00\" load=\"100\" end=\"2021-10-08T08:25:24+02:00\"/>"
            "</intervals>"
            "</work-intervals-cache>"
        "</resource>"
        "<resource type=\"Team\" shared=\"0\" overtime-rate=\"0\" email=\"\" units=\"100\" normal-rate=\"100\" initials=\"\" id=\"20211006081436rDOG9kV5Gp\" auto-allocate=\"0\" name=\"Team 1\">"
            "<work-intervals-cache effort=\"0 00:00:00.0\" start=\"\" version=\"-1\" end=\"\">"
            "<intervals/>"
            "</work-intervals-cache>"
        "</resource>"
        "</resource-group>"
        "<resource-group type=\"Material\" shared=\"0\" id=\"20211006081526jm5j2tne1F\" name=\"G2\">"
        "<resource type=\"Material\" shared=\"0\" overtime-rate=\"0\" email=\"\" units=\"100\" normal-rate=\"100\" initials=\"\" id=\"20211006081537NGbGyDTwBI\" auto-allocate=\"0\" name=\"M1\">"
            "<work-intervals-cache effort=\"0 00:00:00.0\" start=\"\" version=\"-1\" end=\"\">"
            "<intervals/>"
            "</work-intervals-cache>"
        "</resource>"
        "<resource type=\"Material\" shared=\"0\" overtime-rate=\"0\" email=\"\" units=\"100\" normal-rate=\"100\" initials=\"\" id=\"20211006081545GOUuPpIwEX\" auto-allocate=\"0\" name=\"M2\">"
            "<work-intervals-cache effort=\"0 00:00:00.0\" start=\"\" version=\"-1\" end=\"\">"
            "<intervals/>"
            "</work-intervals-cache>"
        "</resource>"
        "</resource-group>"
        "<task shutdown-cost=\"0\" description=\"&lt;!DOCTYPE HTML PUBLIC &quot;-//W3C//DTD HTML 4.0//EN&quot; &quot;http://www.w3.org/TR/REC-html40/strict.dtd&quot;>&#xa;&lt;html>&lt;head>&lt;meta name=&quot;qrichtext&quot; content=&quot;1&quot; />&lt;style type=&quot;text/css&quot;>&#xa;p, li { white-space: pre-wrap; }&#xa;&lt;/style>&lt;/head>&lt;body style=&quot; font-family:'Noto Sans'; font-size:10pt; font-weight:400; font-style:normal;&quot;>&#xa;&lt;p style=&quot;-qt-paragraph-type:empty; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;&quot;>&lt;br />&lt;/p>&lt;/body>&lt;/html>\" wbs=\"1\" constraint-starttime=\"2021-10-06T08:17:30\" startup-cost=\"0\" priority=\"0\" constraint-endtime=\"2021-10-06T08:17:30\" id=\"20211006081730Uss92rsFaL\" leader=\"\" scheduling=\"ASAP\" name=\"S1\">"
        "<estimate expected=\"8\" type=\"Effort\" unit=\"h\" optimistic=\"2\" risk=\"None\" pessimistic=\"16\"/>"
        "<schedules>"
            "<schedule free-float=\"0 00:00:00.0\" earlystart=\"2021-10-06T00:00:00+02:00\" end=\"2021-10-07T16:00:00+02:00\" resource-overbooked=\"0\" start=\"2021-10-06T00:00:00+02:00\" resource-not-available=\"0\" not-scheduled=\"0\" id=\"1\" negative-float=\"0 00:00:00.0\" duration=\"1 16:00:00.0\" scheduling-error=\"0\" positive-float=\"0 00:00:00.0\" latefinish=\"2021-10-07T16:00:00+02:00\" in-critical-path=\"0\" scheduling-conflict=\"0\" type=\"Expected\" name=\"Schedule: Plan\" resource-error=\"0\"/>"
            "<schedule free-float=\"0 00:00:00.0\" earlystart=\"2021-10-06T08:25:24+02:00\" end=\"2021-10-08T08:25:24+02:00\" resource-overbooked=\"0\" start=\"2021-10-06T08:25:24+02:00\" resource-not-available=\"0\" not-scheduled=\"0\" id=\"2\" negative-float=\"0 00:00:00.0\" duration=\"2 00:00:00.0\" scheduling-error=\"0\" positive-float=\"0 00:00:00.0\" latefinish=\"2021-10-08T08:25:24+02:00\" in-critical-path=\"0\" scheduling-conflict=\"0\" type=\"Expected\" name=\"Schedule: Plan.1\" resource-error=\"0\"/>"
        "</schedules>"
        "<progress started=\"0\" entrymode=\"EnterEffortPerResource\" startTime=\"\" finished=\"0\" finishTime=\"\"/>"
        "<workpackage owner-id=\"\" owner=\"\"/>"
        "<task shutdown-cost=\"0\" description=\"&lt;!DOCTYPE HTML PUBLIC &quot;-//W3C//DTD HTML 4.0//EN&quot; &quot;http://www.w3.org/TR/REC-html40/strict.dtd&quot;>&#xa;&lt;html>&lt;head>&lt;meta name=&quot;qrichtext&quot; content=&quot;1&quot; />&lt;style type=&quot;text/css&quot;>&#xa;p, li { white-space: pre-wrap; }&#xa;&lt;/style>&lt;/head>&lt;body style=&quot; font-family:'Noto Sans'; font-size:10pt; font-weight:400; font-style:normal;&quot;>&#xa;&lt;p style=&quot;-qt-paragraph-type:empty; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;&quot;>&lt;br />&lt;/p>&lt;/body>&lt;/html>\" wbs=\"1.1\" constraint-starttime=\"2021-10-06T08:17:36\" startup-cost=\"0\" priority=\"0\" constraint-endtime=\"2021-10-06T08:17:36\" id=\"202110060817364rucEZwY1J\" leader=\"\" scheduling=\"ASAP\" name=\"T1\">"
            "<estimate expected=\"8\" type=\"Effort\" unit=\"h\" optimistic=\"2\" risk=\"None\" pessimistic=\"16\"/>"
            "<resourcegroup-request group-id=\"20211006081344t9ZgYiKN5y\" units=\"0\">"
            "<resource-request units=\"100\" resource-id=\"20211006081359ejV35m5lJA\">"
            "<required-resources>"
            "<resource id=\"20211006081537NGbGyDTwBI\"/>"
            "</required-resources>"
            "</resource-request>"
            "</resourcegroup-request>"
            "<schedules>"
            "<schedule free-float=\"0 16:00:00.0\" earlystart=\"2021-10-06T08:00:00+02:00\" end=\"2021-10-06T16:00:00+02:00\" resource-overbooked=\"0\" start=\"2021-10-06T08:00:00+02:00\" resource-not-available=\"0\" not-scheduled=\"0\" id=\"1\" duration=\"0 08:00:00.0\" negative-float=\"0 00:00:00.0\" latestart=\"2021-10-06T08:00:00+02:00\" earlyfinish=\"2021-10-06T16:00:00+02:00\" scheduling-error=\"0\" positive-float=\"0 00:00:00.0\" latefinish=\"2021-10-06T16:00:00+02:00\" in-critical-path=\"1\" scheduling-conflict=\"0\" type=\"Expected\" name=\"Schedule: Plan\" resource-error=\"0\"/>"
            "<schedule free-float=\"0 00:00:00.0\" earlystart=\"2021-10-06T08:25:24+02:00\" end=\"2021-10-07T08:25:24+02:00\" resource-overbooked=\"0\" start=\"2021-10-06T08:25:24+02:00\" resource-not-available=\"0\" not-scheduled=\"0\" id=\"2\" duration=\"1 00:00:00.0\" negative-float=\"0 00:00:00.0\" latestart=\"2021-10-06T08:25:24+02:00\" earlyfinish=\"2021-10-07T08:25:24+02:00\" scheduling-error=\"0\" positive-float=\"0 00:00:00.0\" latefinish=\"2021-10-07T08:25:24+02:00\" in-critical-path=\"1\" scheduling-conflict=\"0\" type=\"Expected\" name=\"Schedule: Plan.1\" resource-error=\"0\"/>"
            "</schedules>"
            "<progress started=\"0\" entrymode=\"EnterEffortPerResource\" startTime=\"\" finished=\"0\" finishTime=\"\"/>"
            "<workpackage owner-id=\"\" owner=\"\"/>"
        "</task>"
        "<task shutdown-cost=\"0\" description=\"&lt;!DOCTYPE HTML PUBLIC &quot;-//W3C//DTD HTML 4.0//EN&quot; &quot;http://www.w3.org/TR/REC-html40/strict.dtd&quot;>&#xa;&lt;html>&lt;head>&lt;meta name=&quot;qrichtext&quot; content=&quot;1&quot; />&lt;style type=&quot;text/css&quot;>&#xa;p, li { white-space: pre-wrap; }&#xa;&lt;/style>&lt;/head>&lt;body style=&quot; font-family:'Noto Sans'; font-size:10pt; font-weight:400; font-style:normal;&quot;>&#xa;&lt;p style=&quot;-qt-paragraph-type:empty; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;&quot;>&lt;br />&lt;/p>&lt;/body>&lt;/html>\" wbs=\"1.2\" constraint-starttime=\"2021-10-06T08:18:15\" startup-cost=\"0\" priority=\"0\" constraint-endtime=\"2021-10-06T08:18:15\" id=\"202110060818152MaYHDMFKB\" leader=\"\" scheduling=\"ASAP\" name=\"T2\">"
            "<estimate expected=\"8\" type=\"Effort\" unit=\"h\" optimistic=\"2\" risk=\"None\" pessimistic=\"16\"/>"
            "<resourcegroup-request group-id=\"20211006081344t9ZgYiKN5y\" units=\"0\">"
            "<resource-request units=\"100\" resource-id=\"20211006081420J3YXxkpnL2\">"
            "<required-resources>"
            "<resource id=\"20211006081545GOUuPpIwEX\"/>"
            "</required-resources>"
            "</resource-request>"
            "</resourcegroup-request>"
            "<schedules>"
            "<schedule free-float=\"0 00:00:00.0\" earlystart=\"2021-10-07T08:00:00+02:00\" end=\"2021-10-07T16:00:00+02:00\" resource-overbooked=\"0\" start=\"2021-10-07T08:00:00+02:00\" resource-not-available=\"0\" not-scheduled=\"0\" id=\"1\" duration=\"0 08:00:00.0\" negative-float=\"0 00:00:00.0\" latestart=\"2021-10-07T08:00:00+02:00\" earlyfinish=\"2021-10-07T16:00:00+02:00\" scheduling-error=\"0\" positive-float=\"0 00:00:00.0\" latefinish=\"2021-10-07T16:00:00+02:00\" in-critical-path=\"1\" scheduling-conflict=\"0\" type=\"Expected\" name=\"Schedule: Plan\" resource-error=\"0\"/>"
            "<schedule free-float=\"0 00:00:00.0\" earlystart=\"2021-10-07T08:25:24+02:00\" end=\"2021-10-08T08:25:24+02:00\" resource-overbooked=\"0\" start=\"2021-10-07T08:25:24+02:00\" resource-not-available=\"0\" not-scheduled=\"0\" id=\"2\" duration=\"1 00:00:00.0\" negative-float=\"0 00:00:00.0\" latestart=\"2021-10-07T08:25:24+02:00\" earlyfinish=\"2021-10-08T08:25:24+02:00\" scheduling-error=\"0\" positive-float=\"0 00:00:00.0\" latefinish=\"2021-10-08T08:25:24+02:00\" in-critical-path=\"1\" scheduling-conflict=\"0\" type=\"Expected\" name=\"Schedule: Plan.1\" resource-error=\"0\"/>"
            "</schedules>"
            "<progress started=\"0\" entrymode=\"EnterEffortPerResource\" startTime=\"\" finished=\"0\" finishTime=\"\"/>"
            "<workpackage owner-id=\"\" owner=\"\"/>"
        "</task>"
        "<task shutdown-cost=\"0\" description=\"&lt;!DOCTYPE HTML PUBLIC &quot;-//W3C//DTD HTML 4.0//EN&quot; &quot;http://www.w3.org/TR/REC-html40/strict.dtd&quot;>&#xa;&lt;html>&lt;head>&lt;meta name=&quot;qrichtext&quot; content=&quot;1&quot; />&lt;style type=&quot;text/css&quot;>&#xa;p, li { white-space: pre-wrap; }&#xa;&lt;/style>&lt;/head>&lt;body style=&quot; font-family:'Noto Sans'; font-size:10pt; font-weight:400; font-style:normal;&quot;>&#xa;&lt;p style=&quot;-qt-paragraph-type:empty; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;&quot;>&lt;br />&lt;/p>&lt;/body>&lt;/html>\" wbs=\"1.3\" constraint-starttime=\"2021-10-06T08:18:26\" startup-cost=\"0\" priority=\"0\" constraint-endtime=\"2021-10-06T08:18:26\" id=\"20211006081826IDvVovMHEG\" leader=\"\" scheduling=\"ASAP\" name=\"T3\">"
            "<estimate expected=\"8\" type=\"Effort\" unit=\"h\" optimistic=\"2\" risk=\"None\" pessimistic=\"16\"/>"
            "<resourcegroup-request group-id=\"20211006081344t9ZgYiKN5y\" units=\"0\">"
            "<resource-request units=\"100\" resource-id=\"20211006081436rDOG9kV5Gp\"/>"
            "</resourcegroup-request>"
            "<schedules>"
            "<schedule free-float=\"0 00:00:00.0\" earlystart=\"2021-10-07T08:00:00+02:00\" end=\"2021-10-07T16:00:00+02:00\" resource-overbooked=\"0\" start=\"2021-10-07T08:00:00+02:00\" resource-not-available=\"1\" not-scheduled=\"0\" id=\"1\" duration=\"0 08:00:00.0\" negative-float=\"0 00:00:00.0\" latestart=\"2021-10-07T12:00:00+02:00\" earlyfinish=\"2021-10-07T12:00:00+02:00\" scheduling-error=\"0\" positive-float=\"0 00:00:00.0\" latefinish=\"2021-10-07T16:00:00+02:00\" in-critical-path=\"1\" scheduling-conflict=\"0\" type=\"Expected\" name=\"Schedule: Plan\" resource-error=\"0\"/>"
            "<schedule free-float=\"0 00:00:00.0\" earlystart=\"2021-10-07T08:25:24+02:00\" end=\"2021-10-08T08:25:24+02:00\" resource-overbooked=\"0\" start=\"2021-10-07T08:25:24+02:00\" resource-not-available=\"1\" not-scheduled=\"0\" id=\"2\" duration=\"1 00:00:00.0\" negative-float=\"0 00:00:00.0\" latestart=\"2021-10-07T12:25:24+02:00\" earlyfinish=\"2021-10-07T12:25:24+02:00\" scheduling-error=\"0\" positive-float=\"0 00:00:00.0\" latefinish=\"2021-10-08T08:25:24+02:00\" in-critical-path=\"1\" scheduling-conflict=\"0\" type=\"Expected\" name=\"Schedule: Plan.1\" resource-error=\"0\"/>"
            "</schedules>"
            "<progress started=\"0\" entrymode=\"EnterEffortPerResource\" startTime=\"\" finished=\"0\" finishTime=\"\"/>"
            "<workpackage owner-id=\"\" owner=\"\"/>"
        "</task>"
        "<task shutdown-cost=\"0\" description=\"&lt;!DOCTYPE HTML PUBLIC &quot;-//W3C//DTD HTML 4.0//EN&quot; &quot;http://www.w3.org/TR/REC-html40/strict.dtd&quot;>&#xa;&lt;html>&lt;head>&lt;meta name=&quot;qrichtext&quot; content=&quot;1&quot; />&lt;style type=&quot;text/css&quot;>&#xa;p, li { white-space: pre-wrap; }&#xa;&lt;/style>&lt;/head>&lt;body style=&quot; font-family:'Noto Sans'; font-size:10pt; font-weight:400; font-style:normal;&quot;>&#xa;&lt;p style=&quot;-qt-paragraph-type:empty; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;&quot;>&lt;br />&lt;/p>&lt;/body>&lt;/html>\" wbs=\"1.4\" constraint-starttime=\"2021-10-06T08:18:45\" startup-cost=\"0\" priority=\"0\" constraint-endtime=\"2021-10-06T08:18:45\" id=\"20211006081845PmuFUf7lQz\" leader=\"\" scheduling=\"ASAP\" name=\"T4\">"
            "<estimate expected=\"8\" type=\"Duration\" unit=\"h\" optimistic=\"2\" risk=\"None\" pessimistic=\"16\" calendar-id=\"20211006081255aRV6e7i99a\"/>"
            "<resourcegroup-request group-id=\"20211006081344t9ZgYiKN5y\" units=\"0\">"
            "<resource-request units=\"100\" resource-id=\"20211006081430tze2MXczbe\"/>"
            "</resourcegroup-request>"
            "<schedules>"
            "<schedule free-float=\"0 00:00:00.0\" earlystart=\"2021-10-07T08:00:00+02:00\" end=\"2021-10-07T16:00:00+02:00\" resource-overbooked=\"0\" start=\"2021-10-07T08:00:00+02:00\" resource-not-available=\"0\" not-scheduled=\"0\" id=\"1\" duration=\"0 08:00:00.0\" negative-float=\"0 00:00:00.0\" latestart=\"2021-10-07T08:00:00+02:00\" earlyfinish=\"2021-10-07T16:00:00+02:00\" scheduling-error=\"0\" positive-float=\"0 00:00:00.0\" latefinish=\"2021-10-07T16:00:00+02:00\" in-critical-path=\"1\" scheduling-conflict=\"0\" type=\"Expected\" name=\"Schedule: Plan\" resource-error=\"0\"/>"
            "<schedule free-float=\"0 00:00:00.0\" earlystart=\"2021-10-07T08:25:24+02:00\" end=\"2021-10-08T08:25:24+02:00\" resource-overbooked=\"0\" start=\"2021-10-07T08:25:24+02:00\" resource-not-available=\"0\" not-scheduled=\"0\" id=\"2\" duration=\"1 00:00:00.0\" negative-float=\"0 00:00:00.0\" latestart=\"2021-10-07T08:25:24+02:00\" earlyfinish=\"2021-10-08T08:25:24+02:00\" scheduling-error=\"0\" positive-float=\"0 00:00:00.0\" latefinish=\"2021-10-08T08:25:24+02:00\" in-critical-path=\"1\" scheduling-conflict=\"0\" type=\"Expected\" name=\"Schedule: Plan.1\" resource-error=\"0\"/>"
            "</schedules>"
            "<progress started=\"0\" entrymode=\"EnterEffortPerResource\" startTime=\"\" finished=\"0\" finishTime=\"\"/>"
            "<workpackage owner-id=\"\" owner=\"\"/>"
        "</task>"
        "<task shutdown-cost=\"0\" description=\"&lt;!DOCTYPE HTML PUBLIC &quot;-//W3C//DTD HTML 4.0//EN&quot; &quot;http://www.w3.org/TR/REC-html40/strict.dtd&quot;>&#xa;&lt;html>&lt;head>&lt;meta name=&quot;qrichtext&quot; content=&quot;1&quot; />&lt;style type=&quot;text/css&quot;>&#xa;p, li { white-space: pre-wrap; }&#xa;&lt;/style>&lt;/head>&lt;body style=&quot; font-family:'Noto Sans'; font-size:10pt; font-weight:400; font-style:normal;&quot;>&#xa;&lt;p style=&quot;-qt-paragraph-type:empty; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;&quot;>&lt;br />&lt;/p>&lt;/body>&lt;/html>\" wbs=\"1.5\" constraint-starttime=\"2021-10-06T08:20:09\" startup-cost=\"0\" priority=\"0\" constraint-endtime=\"2021-10-06T08:20:09\" id=\"2021100608200930vDAd9r1G\" leader=\"\" scheduling=\"ASAP\" name=\"T5\">"
            "<estimate expected=\"8\" type=\"Duration\" unit=\"h\" optimistic=\"2\" risk=\"None\" pessimistic=\"16\"/>"
            "<resourcegroup-request group-id=\"20211006081344t9ZgYiKN5y\" units=\"0\">"
            "<resource-request units=\"100\" resource-id=\"20211006081430tze2MXczbe\"/>"
            "</resourcegroup-request>"
            "<schedules>"
            "<schedule free-float=\"0 00:00:00.0\" earlystart=\"2021-10-06T00:00:00+02:00\" end=\"2021-10-06T08:00:00+02:00\" resource-overbooked=\"0\" start=\"2021-10-06T00:00:00+02:00\" resource-not-available=\"1\" not-scheduled=\"0\" id=\"1\" duration=\"0 08:00:00.0\" negative-float=\"0 00:00:00.0\" latestart=\"2021-10-07T08:00:00+02:00\" earlyfinish=\"2021-10-06T08:00:00+02:00\" scheduling-error=\"0\" positive-float=\"1 08:00:00.0\" latefinish=\"2021-10-07T16:00:00+02:00\" in-critical-path=\"0\" scheduling-conflict=\"0\" type=\"Expected\" name=\"Schedule: Plan\" resource-error=\"0\"/>"
            "<schedule free-float=\"0 00:00:00.0\" earlystart=\"2021-10-06T08:25:24+02:00\" end=\"2021-10-06T16:25:24+02:00\" resource-overbooked=\"0\" start=\"2021-10-06T08:25:24+02:00\" resource-not-available=\"0\" not-scheduled=\"0\" id=\"2\" duration=\"0 08:00:00.0\" negative-float=\"0 00:00:00.0\" latestart=\"2021-10-08T00:25:24+02:00\" earlyfinish=\"2021-10-06T16:25:24+02:00\" scheduling-error=\"0\" positive-float=\"1 16:00:00.0\" latefinish=\"2021-10-08T08:25:24+02:00\" in-critical-path=\"0\" scheduling-conflict=\"0\" type=\"Expected\" name=\"Schedule: Plan.1\" resource-error=\"0\"/>"
            "</schedules>"
            "<progress started=\"0\" entrymode=\"EnterEffortPerResource\" startTime=\"\" finished=\"0\" finishTime=\"\"/>"
            "<workpackage owner-id=\"\" owner=\"\"/>"
        "</task>"
        "</task>"
        "<relation child-id=\"202110060818152MaYHDMFKB\" type=\"Finish-Start\" parent-id=\"202110060817364rucEZwY1J\" lag=\"0 00:00:00.0\"/>"
        "<relation child-id=\"20211006081826IDvVovMHEG\" type=\"Start-Start\" parent-id=\"202110060818152MaYHDMFKB\" lag=\"0 00:00:00.0\"/>"
        "<relation child-id=\"20211006081845PmuFUf7lQz\" type=\"Finish-Finish\" parent-id=\"20211006081826IDvVovMHEG\" lag=\"0 00:00:00.0\"/>"
        "<schedules>"
        "<plan scheduling-mode=\"0\" overbooking=\"0\" distribution=\"0\" scheduler-plugin-id=\"\" scheduling-direction=\"0\" granularity=\"0\" id=\"DOeTk4zjIO\" recalculate=\"0\" baselined=\"0\" recalculate-from=\"\" name=\"Plan\" check-external-appointments=\"1\">"
            "<schedule scheduling-error=\"0\" not-scheduled=\"0\" type=\"Expected\" start=\"2021-10-06T00:00:00+02:00\" scheduling-conflict=\"0\" duration=\"1 16:00:00.0\" id=\"1\" end=\"2021-10-07T16:00:00+02:00\" name=\"Schedule: Plan\">"
            "<criticalpath-list>"
            "<criticalpath>"
            "<node id=\"202110060817364rucEZwY1J\"/>"
            "<node id=\"202110060818152MaYHDMFKB\"/>"
            "<node id=\"20211006081826IDvVovMHEG\"/>"
            "<node id=\"20211006081845PmuFUf7lQz\"/>"
            "</criticalpath>"
            "</criticalpath-list>"
            "<appointment resource-id=\"20211006081359ejV35m5lJA\" task-id=\"202110060817364rucEZwY1J\">"
            "<interval start=\"2021-10-06T08:00:00+02:00\" load=\"100\" end=\"2021-10-06T16:00:00+02:00\"/>"
            "</appointment>"
            "<appointment resource-id=\"20211006081537NGbGyDTwBI\" task-id=\"202110060817364rucEZwY1J\">"
            "<interval start=\"2021-10-06T08:00:00+02:00\" load=\"100\" end=\"2021-10-06T16:00:00+02:00\"/>"
            "</appointment>"
            "<appointment resource-id=\"20211006081420J3YXxkpnL2\" task-id=\"202110060818152MaYHDMFKB\">"
            "<interval start=\"2021-10-07T08:00:00+02:00\" load=\"100\" end=\"2021-10-07T16:00:00+02:00\"/>"
            "</appointment>"
            "<appointment resource-id=\"20211006081545GOUuPpIwEX\" task-id=\"202110060818152MaYHDMFKB\">"
            "<interval start=\"2021-10-07T08:00:00+02:00\" load=\"100\" end=\"2021-10-07T16:00:00+02:00\"/>"
            "</appointment>"
            "<appointment resource-id=\"20211006081359ejV35m5lJA\" task-id=\"20211006081826IDvVovMHEG\">"
            "<interval start=\"2021-10-07T08:00:00+02:00\" load=\"100\" end=\"2021-10-07T16:00:00+02:00\"/>"
            "</appointment>"
            "<appointment resource-id=\"20211006081430tze2MXczbe\" task-id=\"20211006081845PmuFUf7lQz\">"
            "<interval start=\"2021-10-07T08:00:00+02:00\" load=\"100\" end=\"2021-10-07T16:00:00+02:00\"/>"
            "</appointment>"
            "</schedule>"
            "<plan scheduling-mode=\"0\" overbooking=\"0\" distribution=\"0\" scheduler-plugin-id=\"\" scheduling-direction=\"0\" granularity=\"0\" id=\"UwMZ3fq6BM\" recalculate=\"1\" baselined=\"0\" recalculate-from=\"2021-10-06T08:25:24\" name=\"Plan.1\" check-external-appointments=\"1\">"
            "<schedule scheduling-error=\"0\" not-scheduled=\"0\" type=\"Expected\" start=\"2021-10-06T00:00:00+02:00\" scheduling-conflict=\"0\" duration=\"0 00:00:00.0\" id=\"2\" end=\"2021-10-08T08:25:24+02:00\" name=\"Schedule: Plan.1\">"
            "<criticalpath-list>"
            "<criticalpath>"
                "<node id=\"202110060817364rucEZwY1J\"/>"
                "<node id=\"202110060818152MaYHDMFKB\"/>"
                "<node id=\"20211006081826IDvVovMHEG\"/>"
                "<node id=\"20211006081845PmuFUf7lQz\"/>"
            "</criticalpath>"
            "</criticalpath-list>"
            "<appointment resource-id=\"20211006081359ejV35m5lJA\" task-id=\"202110060817364rucEZwY1J\">"
            "<interval start=\"2021-10-06T08:25:24+02:00\" load=\"100\" end=\"2021-10-06T16:00:00+02:00\"/>"
            "<interval start=\"2021-10-07T08:00:00+02:00\" load=\"100\" end=\"2021-10-07T08:25:24+02:00\"/>"
            "</appointment>"
            "<appointment resource-id=\"20211006081537NGbGyDTwBI\" task-id=\"202110060817364rucEZwY1J\">"
            "<interval start=\"2021-10-06T08:25:24+02:00\" load=\"100\" end=\"2021-10-06T16:00:00+02:00\"/>"
            "<interval start=\"2021-10-07T08:00:00+02:00\" load=\"100\" end=\"2021-10-07T08:25:24+02:00\"/>"
            "</appointment>"
            "<appointment resource-id=\"20211006081420J3YXxkpnL2\" task-id=\"202110060818152MaYHDMFKB\">"
            "<interval start=\"2021-10-07T08:25:24+02:00\" load=\"100\" end=\"2021-10-07T16:00:00+02:00\"/>"
            "<interval start=\"2021-10-08T08:00:00+02:00\" load=\"100\" end=\"2021-10-08T08:25:24+02:00\"/>"
            "</appointment>"
            "<appointment resource-id=\"20211006081545GOUuPpIwEX\" task-id=\"202110060818152MaYHDMFKB\">"
            "<interval start=\"2021-10-07T08:25:24+02:00\" load=\"100\" end=\"2021-10-07T16:00:00+02:00\"/>"
            "<interval start=\"2021-10-08T08:00:00+02:00\" load=\"100\" end=\"2021-10-08T08:25:24+02:00\"/>"
            "</appointment>"
            "<appointment resource-id=\"20211006081359ejV35m5lJA\" task-id=\"20211006081826IDvVovMHEG\">"
            "<interval start=\"2021-10-07T08:25:24+02:00\" load=\"100\" end=\"2021-10-07T16:00:00+02:00\"/>"
            "<interval start=\"2021-10-08T08:00:00+02:00\" load=\"100\" end=\"2021-10-08T08:25:24+02:00\"/>"
            "</appointment>"
            "<appointment resource-id=\"20211006081430tze2MXczbe\" task-id=\"20211006081845PmuFUf7lQz\">"
            "<interval start=\"2021-10-07T08:25:24+02:00\" load=\"100\" end=\"2021-10-07T16:00:00+02:00\"/>"
            "<interval start=\"2021-10-08T08:00:00+02:00\" load=\"100\" end=\"2021-10-08T08:25:24+02:00\"/>"
            "</appointment>"
            "<appointment resource-id=\"20211006081430tze2MXczbe\" task-id=\"2021100608200930vDAd9r1G\">"
            "<interval start=\"2021-10-06T08:25:24+02:00\" load=\"100\" end=\"2021-10-06T16:00:00+02:00\"/>"
            "</appointment>"
            "</schedule>"
            "</plan>"
        "</plan>"
        "</schedules>"
        "<resource-teams>"
        "<team member-id=\"20211006081359ejV35m5lJA\" team-id=\"20211006081436rDOG9kV5Gp\"/>"
        "<team member-id=\"20211006081420J3YXxkpnL2\" team-id=\"20211006081436rDOG9kV5Gp\"/>"
        "</resource-teams>"
        "</project>"
        "</plan>"
    );
}

QString XmlLoaderTester::data_v0_7() const
{
    return QString(
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
        "<schedule-management scheduling-mode=\"0\" id=\"DOeTk4zjIO\" baselined=\"0\" recalculate-from=\"\" overbooking=\"0\" scheduler-plugin-id=\"\" name=\"Plan\" scheduling-direction=\"0\" distribution=\"0\" granularity=\"0\" check-external-appointments=\"1\" recalculate=\"0\">"
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
            "<schedule-management scheduling-mode=\"0\" id=\"UwMZ3fq6BM\" baselined=\"0\" recalculate-from=\"2021-10-06T08:25:24+02:00\" overbooking=\"0\" scheduler-plugin-id=\"\" name=\"Plan.1\" scheduling-direction=\"0\" distribution=\"0\" granularity=\"0\" check-external-appointments=\"1\" recalculate=\"1\">"
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

void XmlLoaderTester::test(const KoXmlDocument &doc)
{
    XMLLoaderObject loader;
    Project p;

    QVERIFY(loader.loadProject(&p, doc));
    QVERIFY(!p.calendars().isEmpty());
    QCOMPARE(p.accounts().accountCount(), 1);
    QCOMPARE(p.accounts().accountList().at(0)->childCount(), 1);

    QCOMPARE(p.resourceGroups().count(), 2);
    auto G1 = p.resourceGroups().at(0);
    auto G2 = p.resourceGroups().at(1);

    QCOMPARE(p.resourceGroups().at(0)->resources().count(), 4);
    QCOMPARE(p.resourceGroups().at(1)->resources().count(), 2);
    auto R1 = G1->resources().at(0);
    auto R2 = G1->resources().at(1);
    auto R3 = G1->resources().at(2);
    auto Team1 = G1->resources().at(3);
    auto M1 = G2->resources().at(0);
    auto M2 = G2->resources().at(1);

    QCOMPARE(R1->requiredResources().count(), 1);
    QCOMPARE(R1->requiredResources().at(0)->name(), M1->name());
    QCOMPARE(R2->requiredResources().count(), 1);
    QCOMPARE(R2->requiredResources().at(0)->name(), M2->name());
    QCOMPARE(R3->requiredResources().count(), 0);
    QCOMPARE(Team1->requiredResources().count(), 0);


    QCOMPARE(p.numChildren(), 1);
    QCOMPARE(p.childNode(0)->numChildren(), 5);
    auto S1 = qobject_cast<Task*>(p.childNode(0));
    auto T1 = qobject_cast<Task*>(S1->childNode(0));
    auto T2 = qobject_cast<Task*>(S1->childNode(1));
    auto T3 = qobject_cast<Task*>(S1->childNode(2));
    auto T4 = qobject_cast<Task*>(S1->childNode(3));
    auto T5 = qobject_cast<Task*>(S1->childNode(4));

    QCOMPARE(T1->requests().requestedResources().count(), 1);
    QCOMPARE(T1->requests().requestedResources().at(0)->name(), R1->name());
    QCOMPARE(T2->requests().requestedResources().count(), 1);
    QCOMPARE(T2->requests().requestedResources().at(0)->name(), R2->name());
    QCOMPARE(T3->requests().requestedResources().count(), 1);
    QCOMPARE(T3->requests().requestedResources().at(0)->name(), Team1->name());
    QCOMPARE(T4->requests().requestedResources().count(), 1);
    QCOMPARE(T4->requests().requestedResources().at(0)->name(), R3->name());
    QCOMPARE(T5->requests().requestedResources().count(), 1);
    QCOMPARE(T5->requests().requestedResources().at(0)->name(), R3->name());

    QCOMPARE(p.scheduleManagers().count(), 1);
    auto sid = p.scheduleManagers().at(0)->scheduleId();
    auto rlst = T1->assignedResources(sid);
    QCOMPARE(rlst.count(), 2);
    QVERIFY(rlst.contains(R1));
    QVERIFY(rlst.contains(M1));

    rlst = T2->assignedResources(sid);
    QCOMPARE(rlst.count(), 2);
    QVERIFY(rlst.contains(R2));
    QVERIFY(rlst.contains(M2));

    // T3 requests Team1 consisting of R1 and R2
    rlst = T3->assignedResources(sid);
    QCOMPARE(rlst.count(), 1);
    QCOMPARE(rlst.at(0)->name(), R1->name());
    //QCOMPARE(rlst.at(1)->name(), R2->name()); // NOTE: R2 was not available, so R1 does it all

    rlst = T4->assignedResources(sid);
    QCOMPARE(rlst.count(), 1);
    QVERIFY(rlst.contains(R3));

    // R3 is requested but is not available so is not assigned
    rlst = T5->assignedResources(sid);
    QCOMPARE(rlst.count(), 0);

    // appointments
    auto appointments = T1->schedule(sid)->appointments();
    QCOMPARE(appointments.count(), 2);
    QCOMPARE(appointments.at(0)->resource()->resource()->name(), R1->name());
    QCOMPARE(appointments.at(1)->resource()->resource()->name(), M1->name());

    auto intervals = appointments.at(0)->intervals().map();
    QCOMPARE(intervals.count(), 1);
    auto interval = intervals.values().at(0);
    QCOMPARE(interval.startTime(), DateTime::fromString("2021-10-06T08:00:00+02:00"));
    QCOMPARE(interval.endTime(), DateTime::fromString("2021-10-06T16:00:00+02:00"));
    QCOMPARE(interval.load(), 100);

    intervals = appointments.at(1)->intervals().map();
    QCOMPARE(intervals.count(), 1);
    interval = intervals.values().at(0);
    QCOMPARE(interval.startTime(), DateTime::fromString("2021-10-06T08:00:00+02:00"));
    QCOMPARE(interval.endTime(), DateTime::fromString("2021-10-06T16:00:00+02:00"));
    QCOMPARE(interval.load(), 100);

    // TODO: Rest of tasks

    // sub-schedule Plan.1
    QCOMPARE(p.scheduleManagers().at(0)->childCount(), 1);
    sid = p.scheduleManagers().at(0)->childAt(0)->scheduleId();

    rlst = T1->assignedResources(sid);
    QCOMPARE(rlst.count(), 2);
    QVERIFY(rlst.contains(R1));
    QVERIFY(rlst.contains(M1));

    rlst = T2->assignedResources(sid);
    QCOMPARE(rlst.count(), 2);
    QVERIFY(rlst.contains(R2));
    QVERIFY(rlst.contains(M2));

    // T3 requests Team1 consisting of R1 and R2
    rlst = T3->assignedResources(sid);
    QCOMPARE(rlst.count(), 1);
    QCOMPARE(rlst.at(0)->name(), R1->name());
    //QCOMPARE(rlst.at(1)->name(), R2->name()); // NOTE: R2 was not available, so R1 does it all

    rlst = T4->assignedResources(sid);
    QCOMPARE(rlst.count(), 1);
    QVERIFY(rlst.contains(R3));

    rlst = T5->assignedResources(sid);
    QCOMPARE(rlst.count(), 1);
    QVERIFY(rlst.contains(R3));

    // appointments
    appointments = T1->schedule(sid)->appointments();
    QCOMPARE(appointments.count(), 2);
    QCOMPARE(appointments.at(0)->resource()->resource()->name(), R1->name());
    QCOMPARE(appointments.at(1)->resource()->resource()->name(), M1->name());

    intervals = appointments.at(0)->intervals().map();
    QCOMPARE(intervals.count(), 2);

    interval = intervals.values().at(0);
    QCOMPARE(interval.startTime(), DateTime::fromString("2021-10-06T08:25:24+02:00"));
    QCOMPARE(interval.endTime(), DateTime::fromString("2021-10-06T16:00:00+02:00"));
    QCOMPARE(interval.load(), 100);

    interval = intervals.values().at(1);
    QCOMPARE(interval.startTime(), DateTime::fromString("2021-10-07T08:00:00+02:00"));
    QCOMPARE(interval.endTime(), DateTime::fromString("2021-10-07T08:25:24+02:00"));
    QCOMPARE(interval.load(), 100);

    intervals = appointments.at(1)->intervals().map();
    QCOMPARE(intervals.count(), 2);

    interval = intervals.values().at(0);
    QCOMPARE(interval.startTime(), DateTime::fromString("2021-10-06T08:25:24+02:00"));
    QCOMPARE(interval.endTime(), DateTime::fromString("2021-10-06T16:00:00+02:00"));
    QCOMPARE(interval.load(), 100);

    interval = intervals.values().at(1);
    QCOMPARE(interval.startTime(), DateTime::fromString("2021-10-07T08:00:00+02:00"));
    QCOMPARE(interval.endTime(), DateTime::fromString("2021-10-07T08:25:24+02:00"));
    QCOMPARE(interval.load(), 100);

    // TODO: Rest of tasks
}

void XmlLoaderTester::version_0_6()
{
    KoXmlDocument doc;
    doc.setContent(data_v0_6());
    test(doc);
}

void XmlLoaderTester::version_0_7()
{
    KoXmlDocument doc;
    doc.setContent(data_v0_7());
    test(doc);
}

QTEST_GUILESS_MAIN(KPlato::XmlLoaderTester)
