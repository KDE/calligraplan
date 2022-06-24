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


KPlato::Help* KPlato::Help::self = nullptr;

Help::Help()
{
    if (self) {
        delete self;
    }
    self = this;
    m_contentsUrl.setScheme(QStringLiteral("help"));
#ifdef Q_OS_WIN
    // Url looks like this:
    // https://docs.kde.org/trunk5/en/calligraplan/calligraplan/portfolio/index.html
    // https://docs.kde.org/trunk5/en/calligraplan/calligraplan/plan/index.html
    m_contextUrl.setScheme(QStringLiteral("https"));
    m_contextUrl.setHost(QStringLiteral("docs.kde.org"));
    m_contextUrl.setPath(QStringLiteral("/trunk5/en/calligraplan/calligraplan/")); // FIXME: language + trunk/stable
#else
    m_contextUrl.setScheme(QStringLiteral("help"));
#endif
}

Help::~Help()
{
    self = nullptr;
}

KPlato::Help *KPlato::Help::instance()
{
    if (!self) {
        self = new Help();
    }
    return self;
}

void KPlato::Help::setContentsUrl(const QUrl& url)
{
    m_contentsUrl = url;
}

void KPlato::Help::setContextUrl(const QUrl& url)
{
    m_contextUrl = url;
}


void Help::setDocs(const QStringList &docs)
{
    m_docs.clear();
    debugPlanHelp<<docs;
    for (const auto &s : docs) {
        int last = s.length() - 1;
        int pos = s.indexOf(QLatin1Char(':'));
        if (pos > 0 && pos < last) {
            m_docs.insert(s.left(pos), s.right(last - pos));
        }
    }
    debugPlanHelp<<m_docs;
}

QString Help::doc(const QString &key) const
{
    return m_docs.value(key);
}

bool Help::eventFilter(QObject *object, QEvent *event)
{
    Q_UNUSED(object);
    if (event->type() == QEvent::WhatsThisClicked) {
        QWhatsThisClickedEvent *e = static_cast<QWhatsThisClickedEvent*>(event);
        QUrl url(e->href());
        if (url.isValid()) {
            return Help::instance()->invokeContext(url);
        }
    }
    return false;
}

bool KPlato::Help::invokeContent(QUrl url)
{
    QDesktopServices::openUrl(url);
    return true;
}

bool KPlato::Help::invokeContext(QUrl url)
{
    debugPlanHelp<<"treat:"<<url.scheme()<<url.path()<<url.fragment()<<':'<<m_contextUrl;
    if (url.scheme().isEmpty()) {
        warnPlanHelp<<"Empty dcument type, cannot open document";
        return false;
    }
    QUrl helpUrl;
    helpUrl.setScheme(m_contextUrl.scheme());
    debugPlanHelp<<"open:"<<helpUrl<<helpUrl.scheme()<<helpUrl.host()<<helpUrl.path();
    if (helpUrl.scheme() == QStringLiteral("help")) {
        helpUrl.setPath(doc(url.scheme()) + QLatin1Char('/') + url.path() + QStringLiteral(".html"));
    } else {
        helpUrl.setHost(m_contextUrl.host());
        helpUrl.setPath(QLatin1Char('/') + m_contextUrl.path() + url.scheme() + QLatin1Char('/') + url.path() + QStringLiteral(".html"));
    }
    helpUrl.setFragment(url.fragment());
    debugPlanHelp<<"open:"<<helpUrl;
    QDesktopServices::openUrl(helpUrl);
    return true;
}
