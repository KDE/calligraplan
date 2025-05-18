/* This file is part of the KDE project
   SPDX-FileCopyrightText: 1998, 1999 Torben Weis <weis@kde.org>
   SPDX-FileCopyrightText: 2007 David Faure <faure@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "KoDocumentEntry.h"

#include "KoPart.h"
#include "KoDocument.h"
#include "KoFilter.h"
#include <MainDebug.h>

#include <KoPluginLoader.h>

#include <KPluginFactory>

#include <QCoreApplication>
#include <QMimeDatabase>

#include <limits.h> // UINT_MAX

KoDocumentEntry::KoDocumentEntry()
        : m_loader(nullptr)
{
}

KoDocumentEntry::KoDocumentEntry(QPluginLoader *loader)
        : m_loader(loader)
{
}

KoDocumentEntry::~KoDocumentEntry()
{
}


QJsonObject KoDocumentEntry::metaData() const
{
    return m_loader ? m_loader->metaData().value(QStringLiteral("MetaData")).toObject() : QJsonObject();
}

QString KoDocumentEntry::fileName() const
{
    return m_loader ? m_loader->fileName() : QString();
}

/**
 * @return TRUE if the service pointer is null
 */
bool KoDocumentEntry::isEmpty() const {
    return (m_loader == nullptr);
}

/**
 * @return name of the associated service
 */
QString KoDocumentEntry::name() const {
    QJsonObject json = metaData();
    json = json.value(QStringLiteral("KPlugin")).toObject();
    return json.value(QStringLiteral("Name")).toString();
}

/**
 *  Mimetypes (and other service types) which this document can handle.
 */
QStringList KoDocumentEntry::mimeTypes() const {
    QJsonObject json = metaData();
    QJsonObject pluginData = json.value(QStringLiteral("KPlugin")).toObject();
    return pluginData.value(QStringLiteral("MimeTypes")).toVariant().toStringList();
}

/**
 *  @return TRUE if the document can handle the requested mimetype.
 */
bool KoDocumentEntry::supportsMimeType(const QString & _mimetype) const {
    return mimeTypes().contains(_mimetype);
}

KoPart *KoDocumentEntry::createKoPart(QString* errorMsg) const
{
    if (!m_loader) {
        return nullptr;
    }

    QObject *obj = m_loader->instance();
    KPluginFactory *factory = qobject_cast<KPluginFactory *>(obj);
    KoPart *part = factory->create<KoPart>(nullptr, QVariantList());

    if (!part) {
        if (errorMsg)
            *errorMsg = m_loader->errorString();
        return nullptr;
    }

    return part;
}

KoDocumentEntry KoDocumentEntry::queryByMimeType(const QString & mimetype)
{
    QList<KoDocumentEntry> vec = query(mimetype);

    if (vec.isEmpty()) {
        warnMain << "Got no results with " << mimetype;
        // Fallback to the old way (which was probably wrong, but better be safe)
        vec = query(mimetype);

        if (vec.isEmpty()) {
            // Still no match. Either the mimetype itself is unknown, or we have no service for it.
            // Help the user debugging stuff by providing some more diagnostics
            QMimeDatabase db;
            QMimeType mime = db.mimeTypeForName(mimetype);
            if (!mime.isValid()) {
                errorMain << "Unknown Calligra Plan MimeType " << mimetype << "." << '\n';
            } else {
                errorMain << "Found no Calligra part able to handle " << mimetype << "!" << '\n';
                errorMain << "Check your installation (does the desktop file have X-KDE-NativeMimeType and Calligraplan/Part, did you install Calligra in a different prefix than KDE, without adding the prefix to /etc/kderc ?)" << '\n';
            }
            return KoDocumentEntry();
        }
    }
#if 0
    // Filthy hack alert -- this'll be properly fixed in the mvc branch.
    if (qApp->applicationName() == "flow" && vec.size() == 2) {
        return KoDocumentEntry(vec[1]);
    }
#endif
    return KoDocumentEntry(vec[0]);
}

QList<KoDocumentEntry> KoDocumentEntry::query(const QString & mimetype)
{

    QList<KoDocumentEntry> lst;

    // Query the trader
    const QList<QPluginLoader *> offers = KoPluginLoader::pluginLoaders(QStringLiteral("calligraplan/parts"), mimetype);

    for (QPluginLoader *pluginLoader : std::as_const(offers)) {
        lst.append(KoDocumentEntry(pluginLoader));
    }

    if (lst.count() > 1 && !mimetype.isEmpty()) {
        warnMain << "KoDocumentEntry::query " << mimetype << " got " << lst.count() << " offers!";
        for (const KoDocumentEntry &entry : std::as_const(lst)) {
            warnMain << entry.name();
        }
    }

    return lst;
}
