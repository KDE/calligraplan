/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2000 David Faure <faure@kde.org>
   SPDX-FileCopyrightText: 2006 Fredrik Edemar <f_edemar@linux.se>

   SPDX-License-Identifier: LGPL-2.0-only
*/

// clazy:excludeall=qstring-arg
#include "KoApplicationAdaptor.h"

#include <MainDebug.h>
#include <KLocalizedString>
#include <KMessageBox>

#include "KoApplication.h"
#include "KoPart.h"
#include "KoDocument.h"
#include "KoMainWindow.h"
#include "KoDocumentEntry.h"
#include "KoView.h"

KoApplicationAdaptor::KoApplicationAdaptor(KoApplication *parent)
        : QDBusAbstractAdaptor(parent)
        , m_application(parent)
{
    // constructor
    setAutoRelaySignals(true);
}

KoApplicationAdaptor::~KoApplicationAdaptor()
{
    // destructor
}

//QString KoApplicationAdaptor::createDocument(const QString &nativeFormat)
//{
//    KoDocumentEntry entry = KoDocumentEntry::queryByMimeType(nativeFormat);
//    if (entry.isEmpty()) {
//        KMessageBox::questionTwoActions(0, i18n("Unknown Calligra MimeType %1. Check your installation.", nativeFormat));
//        return QString();
//    }
//    KoPart *part = entry.createKoPart(0);
//    if (part) {
//        m_application->addPart(part);
//        return '/' + part->document()->objectName();
//    }
//    else {
//        return QString();
//    }
//}

QStringList KoApplicationAdaptor::getDocuments()
{
    QStringList lst;
    const QList<KoPart*> parts = m_application->partList();
    for (KoPart *part : parts) {
        lst.append(QLatin1Char('/') + part->document()->objectName());
    }
    return lst;
}

QStringList KoApplicationAdaptor::getViews()
{
    QStringList lst;
    const QList<KoPart*> parts = m_application->partList();
    for (KoPart *part : parts) {
        const auto views = part->views();
        for (KoView* view : views) {
            lst.append(QLatin1Char('/') + view->objectName());
        }
    }

    return lst;
}

QStringList KoApplicationAdaptor::getWindows()
{
    QStringList lst;
    const QList<KMainWindow*> mainWindows = KMainWindow::memberList();
    if (!mainWindows.isEmpty()) {
        for (KMainWindow* mainWindow : mainWindows) {
            lst.append(static_cast<KoMainWindow*>(mainWindow)->objectName());
        }
    }
    return lst;
}
