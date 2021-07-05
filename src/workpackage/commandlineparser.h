/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2016 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef CommandLineParser_H
#define CommandLineParser_H

#include <QObject>
#include <QCommandLineParser>

class KPlatoWork_MainWindow;
class QDir;

class CommandLineParser : public QObject
{
    Q_OBJECT

public:
    CommandLineParser();
    ~CommandLineParser() override;

public:
    void handleCommandLine(const QDir &workingDirectory);

public Q_SLOTS:
    void handleActivateRequest(const QStringList &arguments, const QString &workingDirectory);

private:
    KPlatoWork_MainWindow *m_mainwindow;
    QCommandLineParser m_commandLineParser;
};


#endif // CommandLineParser_H

