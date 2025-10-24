/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2001 David Faure <faure@kde.org>
   SPDX-FileCopyrightText: 2003 Nicolas GOUTTE <goutte@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "KoGlobal.h"

#include <config.h>
#include <KoResourcePaths.h>

#include <QPaintDevice>
#include <QFont>
#include <QFontInfo>
#include <QFontDatabase>
#include <QGlobalStatic>

#include <WidgetsDebug.h>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>
#include <KConfig>

Q_GLOBAL_STATIC(KoGlobal, s_instance)

KoGlobal* KoGlobal::self()
{
    return s_instance;
}

KoGlobal::KoGlobal()
    : m_pointSize(-1)
    , m_planConfig(nullptr)
{
    // Fixes a bug where values from some config files are not picked up
    // due to  KSharedConfig::openConfig() being initialized before paths have been set up above.
    // NOTE: Values set without a sync() call before KoGlobal has been initialized will not stick
     KSharedConfig::openConfig()->reparseConfiguration();
}

KoGlobal::~KoGlobal()
{
    delete m_planConfig;
}

QFont KoGlobal::_defaultFont()
{
    QFont font = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
    // we have to use QFontInfo, in case the font was specified with a pixel size
    if (font.pointSize() == -1) {
        // cache size into m_pointSize, since QFontInfo loads the font -> slow
        if (m_pointSize == -1)
            m_pointSize = QFontInfo(font).pointSize();
        Q_ASSERT(m_pointSize != -1);
        font.setPointSize(m_pointSize);
    }
    //debugWidgets<<"QFontInfo(font).pointSize() :"<<QFontInfo(font).pointSize();
    //debugWidgets<<"font.name() :"<<font.family ();
    return font;
}

QStringList KoGlobal::_listOfLanguageTags()
{
    if (m_langMap.isEmpty())
        createListOfLanguages();
    return m_langMap.values();
}

QStringList KoGlobal::_listOfLanguages()
{
    if (m_langMap.empty())
        createListOfLanguages();
    return m_langMap.keys();
}

void KoGlobal::createListOfLanguages()
{
    KConfig config(QStringLiteral("all_languages"), KConfig::NoGlobals);

    QMap<QString, bool> seenLanguages;
    const QStringList langlist = config.groupList();
    for (QStringList::ConstIterator itall = langlist.begin();
            itall != langlist.end(); ++itall) {
        const QString tag = *itall;
        const QString name = config.group(tag).readEntry("Name", tag);
        // e.g. name is "French" and tag is "fr"

        // The QMap does the sorting on the display-name, so that
        // comboboxes are sorted.
        m_langMap.insert(name, tag);

        seenLanguages.insert(tag, true);
    }

    // Also take a look at the installed translations.
    // Many of them are already in all_languages but all_languages doesn't
    // currently have en_US (etc?).

    const QStringList translationList = KoResourcePaths::findAllResources("locale",
                                        QString::fromLatin1("*/kf5_entry.desktop"));
    for (QStringList::ConstIterator it = translationList.begin();
            it != translationList.end(); ++it) {
        // Extract the language tag from the directory name
        QString tag = *it;
        int index = tag.lastIndexOf(QLatin1Char('/'));
        tag = tag.left(index);
        index = tag.lastIndexOf(QLatin1Char('/'));
        tag = tag.mid(index + 1);

        if (seenLanguages.find(tag) == seenLanguages.end()) {
            KConfig entry(*it, KConfig::SimpleConfig);

            const QString name = entry.group(QStringLiteral("KCM Locale")).readEntry("Name", tag);
            // e.g. name is "US English" and tag is "en_US"
            m_langMap.insert(name, tag);

            // enable this if writing a third way of finding languages below
            //seenLanguages.insert(tag, true);
        }

    }

    // #### We also might not have an entry for a language where spellchecking is supported,
    //      but no KDE translation is available, like fr_CA.
    // How to add them?
}

QString KoGlobal::tagOfLanguage(const QString & _lang)
{
    const LanguageMap& map = self()->m_langMap;
    QMap<QString, QString>::ConstIterator it = map.find(_lang);
    if (it != map.end())
        return *it;
    return QString();
}

QString KoGlobal::languageFromTag(const QString &langTag)
{
    const LanguageMap& map = self()->m_langMap;
    QMap<QString, QString>::ConstIterator it = map.begin();
    const QMap<QString, QString>::ConstIterator end = map.end();
    for (; it != end; ++it)
        if (it.value() == langTag)
            return it.key();

    // Language code not found. Better return the code (tag) than nothing.
    return langTag;
}

KConfig* KoGlobal::_planConfig()
{
    if (!m_planConfig) {
        m_planConfig = new KConfig(QStringLiteral("calligraplanrc"));
    }
    return m_planConfig;
}
