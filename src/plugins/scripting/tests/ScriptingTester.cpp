/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2007-2011 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/
// clazy:excludeall=qstring-arg
#include "ScriptingTester.h"

#include "TestResult.h"

#include "Module.h"

#include <QTest>
#include <Kross/Core/Action>
#include <Kross/Core/Manager>

namespace KPlato
{


QStringList ScriptingTester::initTestList()
{
    QStringList scripts;
    scripts << "project_access.py"
        << "account_access.py"
        << "account_readwrite.py"
        << "calendar_access.py"
        << "calendar_readwrite.py"
        << "task_access.py"
        << "task_readwrite.py"
        << "resource_access.py"
        << "resource_readwrite.py"
        << "resource_team.py"
        ;
    return scripts;
}

void ScriptingTester::initTestCase()
{
    Kross::Action *a = new Kross::Action(0, "PythonDetection");
    a->setInterpreter("python");
    bool py = a->initialize();
    a->finalize();
    if (! py) {
        QEXPECT_FAIL("", "Python not available, tests will not be executed", Continue);
        QVERIFY(py == true);
    } else {
        m_module = new Scripting::Module(this);
        m_result = new TestResult(this);
        Kross::Manager::self().addObject(m_module, "Plan");
        Kross::Manager::self().addObject(m_result, "TestResult");

        QStringList scripts = initTestList();
        for (int i = 0; i < scripts.count(); ++i) {
            //Create a new Kross::Action instance.
            Kross::Action* action = new Kross::Action(0, QString("%1").arg(scripts.at(i)));
            // Set the script file we like to execute.
            action->setFile(QString("%1/%2").arg(FILES_DATA_DIR).arg(scripts.at(i)));
            m_tests << action;
        }
    }
}

void ScriptingTester::cleanupTestCase()
{
    while (! m_tests.isEmpty()) {
        delete m_tests.takeFirst();
    }
}

void ScriptingTester::test()
{
    qDebug()<<m_tests;
    for (Kross::Action *a : std::as_const(m_tests)) {
        m_result->setResult(false);
        m_result->message = QString("%1: Failed to run test").arg(a->objectName());
        a->trigger();
        QVERIFY2(m_result->isOk(), m_result->message.toLocal8Bit());
        qDebug() << "PASS: " << a->objectName();
    }
}

} //namespace KPlato

QTEST_GUILESS_MAIN(KPlato::ScriptingTester)
