/*
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "DateTimeTimeLine.h"

#include <QApplication>
#include <QDateTime>
#include <QString>
#include <QPalette>
#include <QPen>
#include <QTimer>
#include <QDebug>



namespace KPlato {
    class Q_DECL_HIDDEN DateTimeTimeLine::Private
    {
    public:
        Private() : options(Foreground) {}
        
        DateTimeTimeLine::Options options;
        QDateTime dateTime;
        QPen pen;
        QTimer timer;
    };
    
}

using namespace KPlato;

/*!\class KPlato::DateTimeTimeLine
 * \ingroup KPlato
 *
 * This class implements a timeline.
 *
 * The timeline can optionally be shown in the Background or in the Foreground.
 * Default is Foreground.
 * 
 * The pen can be set with setPen(), and must be activated by setting the option UseCustomPen.
 * 
 * The datetime can be set using setDateTime().
 * 
 * The timeline can be periodically moved to the current datetime
 * by setting the interval > 0 with setInterval().
 * Setting a zero interval turns the periodic update off.
 * 
 * The timeline is off by default.
 *
 * For example:
 * \code
 *  // Show a red timeline in the foreground
 *  timeLine->setOptions(UseCustomPen);
 *  timeLine->setPen(QPen(Qt:red));
 *  // Update the timeline every 5 seconds
 *  timeLine->setInterval(5000);
 * \endcode
 */

/**
 * Create a timeline object.
 * 
 * By default, no timeline is displayed.
 */
DateTimeTimeLine::DateTimeTimeLine()
    : d(new Private())
{
    d->pen = QPen(QApplication::palette().color(QPalette::Highlight), 0);
    connect(&d->timer, SIGNAL(timeout()), this, SIGNAL(updated()));
}

void DateTimeTimeLine::setEnabled(bool enable)
{
    if (enable) {
        d->timer.start();
    } else {
        d->timer.stop();
    }
    Q_EMIT updated();
}

bool DateTimeTimeLine::isEnabled() const
{
    return d->timer.isActive();
}

/**
 * @return options
 */
DateTimeTimeLine::Options DateTimeTimeLine::options() const
{
    return d->options;
}

/**
 * Set options to @p options.
 * If both Background and Foreground are set, Foreground is used.
 */
void DateTimeTimeLine::setOptions(DateTimeTimeLine::Options options)
{
    d->options = options;
    if (options & Foreground) {
        d->options &= ~Background;
    }
    Q_EMIT updated();
}

/**
 * @return the datetime
 * If the datetime is not valid, the current datetime is returned.
 */
QDateTime DateTimeTimeLine::dateTime() const
{
    return d->dateTime.isValid() ? d->dateTime : QDateTime::currentDateTime();
}

/**
 * Set datetime to @p dt.
 */
void DateTimeTimeLine::setDateTime(const QDateTime &dt)
{
    d->dateTime = dt;
    Q_EMIT updated();
}

/**
 * Set timer interval to @p msecs milliseconds.
 * Setting a zero time disables the timer.
 */
void DateTimeTimeLine::setInterval(int msecs)
{
    d->timer.stop();
    d->timer.setInterval(msecs);
    Q_EMIT updated();
    if (msecs > 0) {
        d->timer.start();
    }
}

int DateTimeTimeLine::interval() const
{
    return d->timer.interval();
}

/**
 * @return the pen to be used to draw the timeline.
 * If option UseCustomPen is not set a default pen is returned.
 */
QPen DateTimeTimeLine::pen() const
{
    if (d->options & DateTimeTimeLine::UseCustomPen) {
        return d->pen;
    }
    return QPen(QApplication::palette().color(QPalette::Highlight), 0);
}

/**
 * Set the custom pen to @p pen.
 */
void DateTimeTimeLine::setPen(const QPen &pen)
{
    d->pen = pen;
    Q_EMIT updated();
}
