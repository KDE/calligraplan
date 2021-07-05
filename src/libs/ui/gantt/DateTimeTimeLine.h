/*
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 *
 * This file is part of the KGantt library.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KPLATODATETIMETIMELINE_H
#define KPLATODATETIMETIMELINE_H

#include "kganttglobal.h"

#include <QObject>

class QDateTime;

namespace KPlato {

class DateTimeTimeLine : public QObject
{
    Q_OBJECT
public:
    enum Option {
        Foreground = 1,     /// Display the timeline in the foreground.
        Background = 2,     /// Display the timeline in the background.
        UseCustomPen = 4,   /// Paint the timeline using the pen set with setTimelinePen().
        MaxOptions = 0xFFFF
    };
    Q_DECLARE_FLAGS(Options, Option)

    DateTimeTimeLine();

    void setEnabled(bool enable);
    bool isEnabled() const;
    DateTimeTimeLine::Options options() const;
    void setOptions(DateTimeTimeLine::Options options);
    QDateTime dateTime() const;
    void setDateTime(const QDateTime &dt);
    void setInterval(int msec);
    int interval() const;
    QPen pen() const;
    void setPen(const QPen &pen);

Q_SIGNALS:
    void updated();

private:
    class Private;
    Private *d;
};

}

Q_DECLARE_OPERATORS_FOR_FLAGS(KPlato::DateTimeTimeLine::Options)

#endif /* KPLATODATETIMETIMELINE_H */

