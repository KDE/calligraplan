/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2017 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
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

using namespace KPlato;

const QLoggingCategory &PLANHELP_LOG()
{
    static const QLoggingCategory category("calligra.plan.help");
    return category;
}


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
        if (url.endsWith(':') || url.endsWith('/')) {
            url = QString("%1%2").arg(url, page);
        } else {
            url = QString("%1/%2").arg(url, page);
        }
    }
    return url;
}

void Help::invoke(const QString &page)
{
    invoke(QUrl(Help::page(page)));
}

void Help::invoke(const QUrl &xurl)
{
    debugPlanHelp<<"treat:"<<xurl;
    QUrl url = xurl;
    if (url.scheme() == QStringLiteral("help") || url.host() == QStringLiteral("docs.kde.org")) {
        // The doc is converted from wiki to docbook, using the following rules:
        // 1) Pages are accessed as a .html page
        // 2) Page- and fragment ids are lower case
        // 3) Spaces (or '_') in names and fragments are converted to '-'
        // 4) Parantheses are removed
        QString path = url.path();
        QString fileName = url.fileName();
        if (fileName.isEmpty()) {
            fileName = QStringLiteral("index.html");
        } else {
            path = path.left(path.lastIndexOf('/')+1);
            fileName.replace('_', '-');
            fileName.remove('(');
            fileName.remove(')');
            fileName = fileName.toLower();
            if (!fileName.endsWith(QStringLiteral(".html"))) {
                fileName.append(QStringLiteral(".html"));
            }
        }
        url.setPath(path + fileName);
        QString fragment = url.fragment();
        if (!fragment.isEmpty()) {
            fragment.replace('_', '-');
            fileName.remove('(');
            fileName.remove(')');
            fragment = fragment.toLower();
            url.setFragment(fragment);
        }
        if (url.scheme() == QStringLiteral("help") && !QDir::isAbsolutePath(url.path())) {
            url.setPath(url.path().prepend(QString("/%1/").arg(qApp->applicationName())));
        }
    }
    debugPlanHelp<<"open:"<<url;
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
            Help::invoke(url);
        }
        return true;
    }
    return false;
}
