/* This file is part of the KDE project
  Copyright (C) 2017 Dag Andersen <danders@get2net.dk>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#ifndef KPLATO_HELP_H
#define KPLATO_HELP_H

#include "kowidgetutils_export.h"

#include <QObject>

class QEvent;

// FIXME: do not leak this
namespace KPlato
{

    class KOWIDGETUTILS_EXPORT Help
{
public:
    Help(const QString &docpath, const QString &language = QString());
    static void add(QWidget *widget, const QString &text);
    static QString page(const QString &page = QString());
    static void invoke(const QString &page);

protected:
    ~Help();

private:
    static Help* self;
    QString m_docpath;
};


class KOWIDGETUTILS_EXPORT WhatsThisClickedEventHandler : public QObject
{
    Q_OBJECT
public:
    WhatsThisClickedEventHandler(QObject *parent=0);

    bool eventFilter(QObject *object, QEvent *event);

};


} // namespace KPlato

#endif // KPLATO_HELP_H
