/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2004, 2007 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef INTERVALITEM_H
#define INTERVALITEM_H

#include <QTreeWidgetItem>
#include <QLocale>


namespace KPlato
{

class IntervalItem : public QTreeWidgetItem
{
public:
    explicit IntervalItem(QTreeWidget * parent, QTime start, int length)
    : QTreeWidgetItem(parent)
    {
        setInterval(start, (double)(length) / (1000. * 60. * 60.) ); // ms -> hours
    }
    explicit IntervalItem(QTreeWidget * parent, QTime start, double length)
    : QTreeWidgetItem(parent)
    {
        setInterval(start, length);
    }
      
    TimeInterval interval() { return TimeInterval(m_start, (int)(m_length * (1000. * 60. * 60.) )); }

    void setInterval(const QTime &time, double length)
    {
        m_start = time;
        m_length = length;
        QLocale locale;
        setText(0, locale.toString(time, QLocale::ShortFormat));
        setText(1, locale.toString(length, 'f', 2));
    }

private:
    QTime m_start;
    double m_length; // In hours
};

}  //KPlato namespace

#endif /* INTERVALITEM_H */

