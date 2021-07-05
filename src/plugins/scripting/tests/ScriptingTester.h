/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2007 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPlato_ScriptingTester_h
#define KPlato_ScriptingTester_h

#include <QObject>

class TestResult;

namespace Kross {
    class Action;
}

namespace Scripting {
    class Module;
}

namespace KPlato
{


class ScriptingTester : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    
    void test();

private:
    QStringList initTestList();
    
private:
    QList<Kross::Action*> m_tests;
    Scripting::Module *m_module;
    TestResult *m_result;
};

} //namespace KPlato

#endif
