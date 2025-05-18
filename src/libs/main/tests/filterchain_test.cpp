/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2001 Werner Trobin <trobin@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "KoFilterChain.h"
#include "KoFilterManager.h"

#include <MainDebug.h>

int main(int /*argc*/, char ** /*argv*/)
{
    CalligraFilter::Graph g("application/x-kspread");
    g.dump();
    g.setSourceMimeType("application/vnd.oasis.opendocument.text");
    g.dump();

    KoFilterManager *manager = new KoFilterManager(nullptr);
    qDebug() << "Trying to build some filter chains...";
    QByteArray mimeType("foo/bar");
    KoFilterChain::Ptr chain = g.chain(manager, mimeType);
    if (!chain)
        qDebug() << "Chain for 'foo/bar' is not available, OK";
    else {
        qCritical() << "Chain for 'foo/bar' is available!" << '\n';
        chain->dump();
    }

    mimeType = "text/csv";
    chain = g.chain(manager, mimeType);
    if (!chain)
        qCritical() << "Chain for 'text/csv' is not available!" << '\n';
    else {
        qDebug() << "Chain for 'text/csv' is available, OK";
        chain->dump();
    }

    // Try to find the closest Calligra part
    mimeType = "";
    chain = g.chain(manager, mimeType);
    if (!chain)
        qDebug() << "It was already a Calligra part, OK";
    else
        qCritical() << "We really got a chain? ugh :}" << '\n';

    g.setSourceMimeType("text/csv");
    mimeType = "";
    chain = g.chain(manager, mimeType);
    if (!chain)
        qCritical() << "Hmm... why didn't we find a chain?" << '\n';
    else {
        qDebug() << "Chain for 'text/csv' -> closest part is available ("
        << mimeType << "), OK" << '\n';
        chain->dump();
    }

    qDebug() << "Checking mimeFilter() for Import:";
    QStringList list = KoFilterManager::mimeFilter("application/vnd.oasis.opendocument.text",  KoFilterManager::Import);
    for (const QString& it : std::as_const(list))
        qDebug() << "" << it;
    qDebug() << "" << list.count() << " entries.";

    qDebug() << "Checking mimeFilter() for Export:";
    list = KoFilterManager::mimeFilter("application/vnd.oasis.opendocument.text",  KoFilterManager::Export);
    for (const QString& it : std::as_const(list))
        qDebug() << "" << it;
    qDebug() << "" << list.count() << " entries.";

    qDebug() << "Checking KoShell's mimeFilter():";
    list = KoFilterManager::mimeFilter();
    for (const QString& it : std::as_const(list))
        qDebug() << "" << it;
    qDebug() << "" << list.count() << " entries.";

    delete manager;
    return 0;
}
