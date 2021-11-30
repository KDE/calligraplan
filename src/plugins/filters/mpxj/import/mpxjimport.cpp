/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "mpxjimport.h"

#include <KoFilterChain.h>
#include <KoFilterManager.h>
#include <KoDocument.h>

#include <KoXmlReader.h>

#include <KMessageBox>
#include <KPluginFactory>

#include <QProcess>
#include <QString>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QDebug>


K_PLUGIN_FACTORY_WITH_JSON(MpxjImportFactory, "plan_mpxj_import.json", registerPlugin<MpxjImport>();)

/**
 * Imports the following filetypes:
 * "MPP"    MPP                application/vnd.ms-project
 * "MPT"    MPP                application/x-project
 * "MPX"    MPX                application/x-project
 * "XML"    MSPDI              application/...
 * "MPD"    MPDDatabase        application/...
 * "XER"    PrimaveraXERFile   application/...
 * "PMXML"  PrimaveraPMFiler   application/...
 * "PP"     AstaFile           application/...
 * "PPX"    Phoenix            application/...
 * "FTS"    FastTrack          application/...
 * "POD"    ProjectLibre       application/x-projectlibre
 * "GAN"    GanttProject       application/...
 *
 * Handled in separate import plugin:
 * "PLANNER", Planner        application/x-planner
*/

MpxjImport::MpxjImport(QObject* parent, const QVariantList &)
    : KoFilter(parent)
{
}

QStringList MpxjImport::mimeTypes()
{
    return QStringList()
        << QLatin1String("application/vnd.ms-project")
        << QLatin1String("application/x-project")
        << QLatin1String("application/x-projectlibre")
        << QLatin1String("application/x-mspdi")
        << QLatin1String("application/x-ganttproject")
        << QLatin1String("application/x-primavera-xer")
        << QLatin1String("application/x-primavera-pmxml")
        << QLatin1String("application/x-asta")
        << QLatin1String("application/x-phoenix")
        << QLatin1String("application/x-fasttrack")
        ;
}

KoFilter::ConversionStatus MpxjImport::convert(const QByteArray& from, const QByteArray& to)
{
    qInfo()<<Q_FUNC_INFO<<from<<to;
    if ( to != "application/x-vnd.kde.plan" || !mimeTypes().contains( from ) ) {
        qInfo()<<Q_FUNC_INFO<<"Bad mime types:"<<from<<"->"<<to;
        return KoFilter::BadMimeType;
    }
    bool batch = false;
    if ( m_chain->manager() ) {
        batch = m_chain->manager()->getBatchMode();
    }
    if (batch) {
        //TODO
        qInfo()<<Q_FUNC_INFO<<"batch mode not implemented";
        return KoFilter::NotImplemented;
    }
    KoDocument *part = m_chain->outputDocument();
    if (! part) {
        return KoFilter::InternalError;
    }
    QString inputFile = m_chain->inputFile();
    QTemporaryDir *tmp = new QTemporaryDir();
    QString outFile(tmp->path() + "/maindoc.plan");
    KoFilter::ConversionStatus sts = doImport(inputFile.toUtf8(), outFile.toUtf8());
    if (sts == KoFilter::OK) {
        QFile file(outFile);
        KoXmlDocument doc;
        if (!doc.setContent(&file)) {
            sts = KoFilter::InternalError;
        } else if (!part->loadXML(doc, 0)) {
            sts = KoFilter::InternalError;
        }
    }
    delete tmp;
    return sts;
}

KoFilter::ConversionStatus MpxjImport::doImport(QByteArray inFile, QByteArray outFile)
{
    // Need to convert to "\" on Windows
    QString normalizedInFile = QDir::toNativeSeparators(inFile);
    QString normalizedOutFile = QDir::toNativeSeparators(outFile);

    QString planConvert = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("calligraplan/java/planconvert.jar"), QStandardPaths::LocateFile).value(0);
    if (planConvert.isEmpty()) {
        return KoFilter::InternalError;
    }
    QString exe = "java";
    QStringList args;
    args << "-jar";
    args << planConvert;
    args << normalizedInFile;
    args << normalizedOutFile;
    int res = QProcess::execute(exe, args);
    qInfo()<<Q_FUNC_INFO<<res;
    return res == 0 ? KoFilter::OK : KoFilter::InternalError;
}

#include "mpxjimport.moc"
