/* This file is part of the KDE project
   SPDX-FileCopyrightText: 1998, 1999 Torben Weis <weis@kde.org>
   SPDX-FileCopyrightText: 2007 David Faure <faure@kde.org>

SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "KoFilterEntry.h"

#include "KoDocument.h"
#include "KoFilter.h"

#include <MainDebug.h>
#include <KoPluginLoader.h>
#include <QPluginLoader>
#include <KPluginFactory>
#include <QFile>

#include <limits.h> // UINT_MAX


KoFilterEntry::KoFilterEntry(QPluginLoader *loader)
        : m_loader(loader)
{
    QJsonObject metadata = loader->metaData().value(QStringLiteral("MetaData")).toObject();
    import_ = metadata.value(QStringLiteral("X-KDE-Import")).toVariant().toStringList();
    export_ = metadata.value(QStringLiteral("X-KDE-Export")).toVariant().toStringList();
    int w = metadata.value(QStringLiteral("X-KDE-Weight")).toInt();
    weight = w < 0 ? UINT_MAX : static_cast<unsigned int>(w);
    available = metadata.value(QStringLiteral("X-KDE-Available")).toString();
}

KoFilterEntry::~KoFilterEntry()
{
    delete m_loader;
}

QString KoFilterEntry::fileName() const
{
    return m_loader->fileName();
}

QList<KoFilterEntry::Ptr> KoFilterEntry::query()
{
    QList<KoFilterEntry::Ptr> lst;

    QList<QPluginLoader *> offers = KoPluginLoader::pluginLoaders(QStringLiteral("calligraplan/formatfilters"));
    QList<QPluginLoader *>::ConstIterator it = offers.constBegin();
    unsigned int max = offers.count();
    //debugFilter <<"Query returned" << max <<" offers";
    for (unsigned int i = 0; i < max; i++) {
        //debugFilter <<"   desktopEntryPath=" << (*it)->entryPath()
        //               << "   library=" << (*it)->library() << '\n';
        // Append converted offer
        lst.append(KoFilterEntry::Ptr(new KoFilterEntry(*it)));
        // Next service
        it++;
    }

    return lst;
}

KoFilter* KoFilterEntry::createFilter(KoFilterChain* chain, QObject* parent)
{
    KPluginFactory *factory = qobject_cast<KPluginFactory *>(m_loader->instance());

    if (!factory) {
        warnMain << m_loader->errorString();
        return nullptr;
    }

    QObject* obj = factory->create<KoFilter>(parent);
    if (!obj || !obj->inherits("KoFilter")) {
        delete obj;
        return nullptr;
    }

    KoFilter* filter = static_cast<KoFilter*>(obj);
    filter->m_chain = chain;
    return filter;
}

