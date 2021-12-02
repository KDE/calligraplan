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


#define MPXJIMPORT_LOG "calligra.plan.filter.mpxj.import"
#define debugMpxjImport qCDebug(QLoggingCategory(MPXJIMPORT_LOG))<<Q_FUNC_INFO
#define warnMpxjImport qCWarning(QLoggingCategory(MPXJIMPORT_LOG))<<Q_FUNC_INFO
#define errorMpxjImport qCCritical(QLoggingCategory(MPXJIMPORT_LOG))<<Q_FUNC_INFO

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
    , m_status(KoFilter::OK)
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
    if ( to != "application/x-vnd.kde.plan" || !mimeTypes().contains( from ) ) {
        errorMpxjImport<<"Bad mime types:"<<from<<"->"<<to;
        return KoFilter::BadMimeType;
    }
    bool batch = false;
    if ( m_chain->manager() ) {
        batch = m_chain->manager()->getBatchMode();
    }
    if (batch) {
        //TODO
        errorMpxjImport<<"batch mode not implemented";
        return KoFilter::NotImplemented;
    }
    KoDocument *part = m_chain->outputDocument();
    if (!part) {
        errorMpxjImport<<"Internal error, no document";
        return KoFilter::InternalError;
    }
    QString inputFile = m_chain->inputFile();
    QTemporaryDir *tmp = new QTemporaryDir();
    QString outFile(tmp->path() + "/maindoc.plan");
    KoFilter::ConversionStatus sts = doImport(inputFile.toUtf8(), outFile.toUtf8());
    if (sts == KoFilter::OK) {
        QFile file(outFile);
        if (!file.exists()) {
            errorMpxjImport<<"Temporary plan file has not been created";
            sts = KoFilter::CreationError;
        } else {
            KoXmlDocument doc;
            if (!doc.setContent(&file)) {
                errorMpxjImport<<"Content of temporary plan file is invalid";
                sts = KoFilter::InvalidFormat;
            } else if (!part->loadXML(doc, 0)) {
                errorMpxjImport<<"Failed to load temporary plan file";
                sts = KoFilter::InternalError;
            }
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
        return KoFilter::JavaJarNotFound;
    }
    QString exe = "java";
    QStringList args;
    args << "-jar";
    args << planConvert;
    args << normalizedInFile;
    args << normalizedOutFile;
    QProcess java;
    connect(&java,  QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &MpxjImport::slotFinished);
    connect(&java,  &QProcess::errorOccurred, this, &MpxjImport::slotError);
    java.start(exe, args);
    java.waitForFinished(60000); // filters cannot run in the background
    return m_status;
}

void MpxjImport::slotFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    auto java = qobject_cast<QProcess*>(sender());
    Q_ASSERT(java);
    debugMpxjImport<<exitCode<<exitStatus;
    auto s = java->readAllStandardOutput();
    if (s.contains("Exception")) {
        m_status = KoFilter::ParsingError;
        if (s.contains("Invalid file format")) {
            errorMpxjImport<<"MPXJ failed to read the file";
            m_status = KoFilter::InvalidFormat;
        } else if (s.contains("assword")) {
            errorMpxjImport<<"Reading passsword protected files are not implemented";
            m_status = KoFilter::PasswordProtected;
        }
    }
}

void MpxjImport::slotError(QProcess::ProcessError error)
{
    switch(error) {
        case QProcess::FailedToStart:
        case QProcess::Crashed:
            errorMpxjImport<<error;
            m_status = KoFilter::JavaExecutionError;
            break;
        case QProcess::Timedout:
            errorMpxjImport<<error;
            m_status = KoFilter::ReadTimeout;
            break;
        case QProcess::UnknownError:
            errorMpxjImport<<error;
            m_status = KoFilter::UnknownError;
            break;
        default:
            debugMpxjImport<<"Unhandled error:"<<error;
            break;
    }
}

#include "mpxjimport.moc"
