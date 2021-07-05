/*
 * TjMessageHandler.h - TaskJuggler
 *
 * Copyright (c) 2001, 2002, 2003, 2004, 2005, 2006, 2007
 *               by Chris Schlaeger <cs@kde.org>
 * SPDX-FileCopyrightText: 2011 Dag Andersen <dag.andersen@kdemail.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * $Id$
 */

// clazy:excludeall=qstring-arg
#include "TjMessageHandler.h"
#include "taskjuggler.h"
#include "Utility.h"

#include <QDebug>

namespace TJ
{

TjMessageHandler TJMH(false);

void
TjMessageHandler::warningMessage(const QString& msg, const CoreAttributes *object)
{
    warningMessage(msg, QString());
    Q_EMIT message((int)TJ::WarningMsg, msg, const_cast<CoreAttributes*>(object));
}
void
TjMessageHandler::warningMessage(const QString& msg, const QString& file, int
                                 line)
{
    warnings++;
    warningPositions << messages.count();
    messages << msg;

    if (consoleMode)
    {
        if (file.isEmpty())
            qWarning()<<msg;
        else
            qWarning()<<file<<":"<<line<<":"<<msg;
    }
    else
        Q_EMIT printWarning(msg, file, line);
}

void
TjMessageHandler::errorMessage(const QString& msg, const CoreAttributes *object)
{
    errorMessage(msg, QString());
    Q_EMIT message((int)TJ::ErrorMsg, msg, const_cast<CoreAttributes*>(object));
}

void
TjMessageHandler::errorMessage(const QString& msg, const QString& file, int
                               line)
{
    errors++;
    errorPositions << messages.count();
    messages << msg;

    if (consoleMode)
    {
        if (file.isEmpty())
            qWarning()<<msg;
        else
            qWarning()<<file<<":"<<line<<":"<<msg;
    }
    else
        Q_EMIT printError(msg, file, line);
}

void
TjMessageHandler::fatalMessage(const QString& msg, const QString& file, int
                               line)
{
    if (consoleMode)
    {
        if (file.isEmpty())
            qWarning()<<msg;
        else
            qWarning()<<file<<":"<<line<<":"<<msg;
    }
    else
        Q_EMIT printFatal(msg, file, line);
}

void
TjMessageHandler::infoMessage(const QString &msg, const CoreAttributes *object)
{
    ++infos;
    infoPositions << messages.count();
    messages << msg;
    Q_EMIT message((int)TJ::InfoMsg, msg, const_cast<CoreAttributes*>(object));
}
void
TjMessageHandler::debugMessage(const QString &msg, const CoreAttributes *object)
{
    ++debugs;
    debugPositions << messages.count();
    messages << msg;
    Q_EMIT message((int)TJ::DebugMsg, msg, const_cast<CoreAttributes*>(object));
}

} // namespace TJ
