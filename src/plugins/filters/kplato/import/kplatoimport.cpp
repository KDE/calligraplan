/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2009, 2011, 2012 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kplatoimport.h"

#include <kpttask.h>
#include <kptnode.h>
#include <kptresource.h>
#include <kptdocuments.h>
#include "kptdebug.h"

#include <QByteArray>
#include <QString>
#include <QTextStream>
#include <QFile>

#include <KPluginFactory>

#include <KoFilterChain.h>
#include <KoFilterManager.h>
#include <KoDocument.h>


using namespace KPlato;

K_PLUGIN_FACTORY_WITH_JSON(KPlatoImportFactory, "plan_kplato_import.json",
                           registerPlugin<KPlatoImport>();)

KPlatoImport::KPlatoImport(QObject* parent, const QVariantList &)
    : KoFilter(parent)
{
}

KoFilter::ConversionStatus KPlatoImport::convert(const QByteArray& from, const QByteArray& to)
{
    debugPlan << from << to;
    if ((from != "application/x-vnd.kde.kplato") || (to != "application/x-vnd.kde.plan")) {
        return KoFilter::NotImplemented;
    }
    KoDocument *part = nullptr;
    bool batch = false;
    if (m_chain->manager()) {
        batch = m_chain->manager()->getBatchMode();
    }
    if (batch) {
        //TODO
        debugPlan << "batch";
    } else {
        //debugPlan<<"online";
        part = m_chain->outputDocument();
    }
    if (part == nullptr) {
        errorPlan << "Cannot open document";
        return KoFilter::InternalError;
    }
    if (! part->loadNativeFormat(m_chain->inputFile())) {
        return KoFilter::ParsingError;
    }

    return KoFilter::OK;
}


#include "kplatoimport.moc"
