/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2017 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "Help.h"
#include <config.h>

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
#include <QProcess>

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

    auto stable = QStringLiteral(PLAN_VERSION_STRING).split(QLatin1Char('.')).value(2).toInt() < 80;
    if (stable) {
        m_onlineBaseUrl = QStringLiteral("https://docs.kde.org/stable5/%1/calligraplan/calligraplan/");
    } else {
        m_onlineBaseUrl = QStringLiteral("https://docs.kde.org/trunk5/%1/calligraplan/calligraplan/");
    }
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

void Help::setOnline(bool b)
{
    m_initiated = false;
    m_online = b;
   initiate();
}

void KPlato::Help::setOnlineBaseUrl(const QString& url)
{
    m_onlineBaseUrl = url;
    m_initiated = false;
}

QString KPlato::Help::onlineBaseUrl() const
{
    return m_onlineBaseUrl;
}

void Help::setDocs(const QStringList &docs)
{
    m_docs.clear();

    for (const auto &doc : docs) {
        const auto s = doc.split(QLatin1Char(':'));
        if (s.count() == 2) {
            setDoc(s.value(0), s.value(1));
        }
    }
    m_initiated = false;
}

void Help::setDoc(const QString &id, const QString &doc)
{
    m_docs[id] = doc;
    m_initiated = false;
}

QString Help::doc(const QString &id) const
{
    if (!m_docs.contains(id)) {
        return QCoreApplication::applicationName();
    }
    return m_docs[id];
}

void Help::setDocLanguage(const QString &doc, const QString &language)
{
    m_languages[doc] = language;
}

QString Help::language(const QString &doc) const
{
    if (!m_languages.contains(doc)) {
        return QStringLiteral("en");
    }
    return m_languages[doc];
}

void Help::setLanguage(const QString &language)
{
    m_language = language.trimmed();
}

bool KPlato::Help::invokeContent(QUrl url)
{
    debugPlanHelp<<url;
    QDesktopServices::openUrl(url);
    return true;
}

bool KPlato::Help::invokeContext(QUrl url)
{
    debugPlanHelp<<"treat:"<<url.scheme()<<url.path()<<url.fragment()<<':'<<m_onlineBaseUrl;
    initiate();
    if (url.scheme().isEmpty()) {
        warnPlanHelp<<"Empty document type, cannot open document";
        return false;
    }
    qInfo()<<Q_FUNC_INFO<<"helpcenter:"<<m_khelpcenter<<"online:"<<m_online<<"languages:"<<m_languages;
    QUrl helpUrl;
    if (m_online || !m_khelpcenter) {
        const auto c = m_onlineBaseUrl.arg(language(url.scheme()));
        QUrl contentsUrl = QUrl::fromUserInput(c);
        helpUrl.setScheme(contentsUrl.scheme());
        helpUrl.setHost(contentsUrl.host());
        helpUrl.setPath(QStringLiteral("/%1%2/%3.html").arg(contentsUrl.path()).arg(url.scheme()).arg(url.path()));
        helpUrl.setFragment(url.fragment());
    } else {
        helpUrl.setScheme(QStringLiteral("help"));
        helpUrl.setPath(QStringLiteral("%1/%2.html").arg(doc(url.scheme())).arg(url.path()));
        helpUrl.setFragment(url.fragment());
    }
    debugPlanHelp<<"open:"<<helpUrl;
    QDesktopServices::openUrl(helpUrl);
    return true;
}

void Help::initiate()
{
    if (m_initiated) {
        return;
    }
    m_initiated = true;
    if (m_khelpcenter) {
        // Check if we have khelpcenter.
        // If not we must try online docs.
        QProcess process;
        process.setProgram(QStringLiteral("khelpcenter"));
        process.setArguments(QStringList()<<QStringLiteral("--version"));
        process.start();
        m_khelpcenter = process.waitForFinished(3000);
        debugPlanHelp<<"khelpcenter found?"<<m_khelpcenter;
    }
    if (m_khelpcenter && !m_online) {
        return;
    }
    // Check if we have documentation in current language
    // Fallback to 'en'
    QStringList languages;
    auto language = QLocale().uiLanguages().value(0);
    if (language == QStringLiteral("C")) {
        language = QStringLiteral("en"); // default
    }
    language.replace(QLatin1Char('-'), QLatin1Char('_')); // docs.kde.org uses '_'
    languages << language;
    if (language.contains(QLatin1Char('_'))) {
        languages << language.mid(0, language.indexOf(QLatin1Char('_')));
    }
    if (!languages.contains( QStringLiteral("en"))) {
        languages << QStringLiteral("en"); // fallback
    }
    const auto docIds = m_docs.keys();
    for (const auto &type : docIds) {
        for (const auto &lang : std::as_const(languages)) {
            auto page = m_onlineBaseUrl.arg(lang).append(type).append(QStringLiteral("/index.html"));
            QProcess process;
            process.setProgram(QStringLiteral("wget"));
            process.setArguments(QStringList()<<page);
            qInfo()<<Q_FUNC_INFO<<page;
            process.start();
            if (process.waitForFinished(3000)) {
                if (process.exitCode() == 0) {
                    setDocLanguage(type, lang);
                    debugPlanHelp<<"Found:"<<page;
                    break;
                } else {
                    warnPlanHelp<<"Failed to find:"<<page;
                }
            } else {
                errorPlanHelp<<process.program()<<"not found, cannot search for documentation for language"<<lang;
            }
        }
    }
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
