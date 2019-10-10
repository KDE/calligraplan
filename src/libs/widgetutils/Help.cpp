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

// clazy:excludeall=qstring-arg
#include "Help.h"

#include <KHelpClient>

#include <KDesktopFile>

#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QUrlQuery>
#include <QEvent>
#include <QWhatsThisClickedEvent>
#include <QDesktopServices>
#include <QWidget>
#include <QDebug>

using namespace KPlato;


Help* Help::self = nullptr;

Help::Help(const QString &docpath, const QString &language)
{
    if (self) {
        delete self;
    }
    self = this;
    m_docpath = docpath;
    if (!language.isEmpty()) {
        m_docpath += '/' + language;
    }
}

Help::~Help()
{
    self = nullptr;
}

void Help::add(QWidget *widget, const QString &text)
{
    widget->installEventFilter(new WhatsThisClickedEventHandler(widget));
    widget->setWhatsThis(text);
}

QString Help::page(const QString &page)
{
    if (!self) {
        new Help(QString());
    }
    QString url = self->m_docpath;
    if (!page.isEmpty()) {
        url = QString("%1/%2").arg(url, page);
    }
    return url;
}

void Help::invoke(const QString &page)
{
    QDesktopServices::openUrl(QUrl(Help::page(page)));
}

void Help::invoke(const QUrl &url)
{
    QDesktopServices::openUrl(url);
}

WhatsThisClickedEventHandler::WhatsThisClickedEventHandler(QObject *parent)
    : QObject(parent)
{

}

bool WhatsThisClickedEventHandler::eventFilter(QObject *object, QEvent *event)
{
    Q_UNUSED(object);
    if (event->type() == QEvent::WhatsThisClicked) {
        QWhatsThisClickedEvent *e = static_cast<QWhatsThisClickedEvent*>(event);
        QUrl url(e->href());
        if (url.isValid()) {
            QDesktopServices::openUrl(url);
        }
        return true;
    }
    return false;
}
