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
#include <KPasswordDialog>
#include <KLocalizedString>

#include <QApplication>
#include <QProcess>
#include <QString>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QDebug>


Q_LOGGING_CATEGORY(mpxjImportLog, "calligra.plan.filter.mpxj.import")

#define debugMpxjImport qCDebug(mpxjImportLog)<<Q_FUNC_INFO
#define warnMpxjImport qCWarning(mpxjImportLog)<<Q_FUNC_INFO
#define errorMpxjImport qCCritical(mpxjImportLog)<<Q_FUNC_INFO

K_PLUGIN_FACTORY_WITH_JSON(MpxjImportFactory, "plan_mpxj_import.json", registerPlugin<MpxjImport>();)

/**
 * Imports the following filetypes:
 * "MPP"    application/vnd.ms-project      MS Project
 * "MPT"    application/x-project-template  MS Project
 * "MPX"    application/x-project           MS Project
 * "XML"    application/x-mspdi             MS Project mspdi
 * "MPD"    application/x-ms-project-db        MS Project Database
 * "XER"    application/x-primavera-xer     PrimaveraXERFile
 * "PMXML"  application/x-primavera-pmxml   PrimaveraPMFile
 * "PP"     application/x-asta              AstaFile
 * "PPX"    application/x-phoenix           Phoenix
 * "FTS"    application/x-fasttrack         FastTrack
 * "POD"    application/x-projectlibre      ProjectLibre
 * "GAN"    application/x-ganttproject      GanttProject
 * "CDPX"   application/x-conceptdraw-cdpx  Concept Draw
 * "CDPZ"   application/x-conceptdraw-cdpz  Concept Draw
 * "CDPTZ"  application/x-conceptdraw-cdptz Concept Draw
 * "SP"     application/x-syncroscheduler   Synchro Scheduler
 * "GNT"    application/x-ganttdesigner     Gantt Designer
 * "PC"     application/x-projectcommander  Project Commander
 * "PEP"    application/x-turboproject      Turbo Project
 * "SDEF"   application/x-sdef              Standard Data Exchange Format
 *
 * Unknown extension, not handled:
 *  Primavera suretrack
 *  Sage 100 Contractor
 *
 * Handled in separate import plugin:
 * "PLANNER", Planner        application/x-planner
*/

MpxjImport::MpxjImport(QObject* parent, const QVariantList &)
    : KoFilter(parent)
    , m_status(KoFilter::OK)
    , m_passwordTries(0)
{
}

QStringList MpxjImport::mimeTypes()
{
    return QStringList()
        << QStringLiteral("application/vnd.ms-project")
        << QStringLiteral("application/x-project-template")
        << QStringLiteral("application/x-project")
        << QStringLiteral("application/x-mspdi")
        << QStringLiteral("application/x-ms-project-db")
        << QStringLiteral("application/x-primavera-xer")
        << QStringLiteral("application/x-primavera-pmxml")
        << QStringLiteral("application/x-asta")
        << QStringLiteral("application/x-phoenix")
        << QStringLiteral("application/x-fasttrack")
        << QStringLiteral("application/x-projectlibre")
        << QStringLiteral("application/x-ganttproject")
        << QStringLiteral("application/x-conceptdraw-cdpx")
        << QStringLiteral("application/x-conceptdraw-cdpz")
        << QStringLiteral("application/x-conceptdraw-cdptz")
        << QStringLiteral("application/x-syncroscheduler")
        << QStringLiteral("application/x-ganttdesigner")
        << QStringLiteral("application/x-projectcommander")
        << QStringLiteral("application/x-turboproject")
        << QStringLiteral("application/x-sdef")
        ;
}

KoFilter::ConversionStatus MpxjImport::convert(const QByteArray& from, const QByteArray& to)
{
    if ( to != "application/x-vnd.kde.plan" || !mimeTypes().contains(QLatin1String(from))) {
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
    QTemporaryFile tmp;
    if (!tmp.open()) {
        errorMpxjImport<<"Temporary plan file has not been created";
        return KoFilter::CreationError;
    }
    const auto outFile = tmp.fileName();
    const auto inputFile = m_chain->inputFile();
    KoFilter::ConversionStatus sts = doImport(inputFile, outFile);
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
            } else if (!part->loadXML(doc, nullptr)) {
                errorMpxjImport<<"Failed to load temporary plan file";
                sts = KoFilter::InternalError;
            }
        }
    }
    return sts;
}

KoFilter::ConversionStatus MpxjImport::doImport(const QString &inFile, const QString &outFile)
{
    auto normalizedInFile = inFile;
    auto normalizedOutFile = outFile;
#ifdef Q_OS_WIN
    // Need to convert to "\" on Windows
    normalizedOutFile = QDir::toNativeSeparators(inFile);
    normalizedOutFile = QDir::toNativeSeparators(outFile);
#endif
    QString planConvert = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("calligraplan/java/planconvert.jar"), QStandardPaths::LocateFile).value(0);
    if (planConvert.isEmpty()) {
        return KoFilter::JavaJarNotFound;
    }
    QStringList args;
    args << QStringLiteral("-jar");
    args << planConvert;
    args << normalizedInFile;
    args << normalizedOutFile;
    run(args);
    while (m_status == KoFilter::PasswordProtected) {
        const QString fileName = normalizedInFile.right(normalizedInFile.length() - normalizedInFile.lastIndexOf(QLatin1Char('/')) - 1);
        QApplication::setOverrideCursor(Qt::ArrowCursor);
        if (m_passwordTries > 0) {
            if (KMessageBox::questionTwoActions(nullptr,
                                                xi18nc("@info", "Invalid password. Try again?"),
                                                i18nc("@title:window", "Enter Password"),
                                                KStandardGuiItem::cont(),
                                                KStandardGuiItem::cancel()) == KMessageBox::SecondaryAction) {
                m_status = KoFilter::UserCancelled;
                QApplication::restoreOverrideCursor();
                break;
            }
        }
        KPasswordDialog dlg(nullptr, KPasswordDialog::NoFlags);
        dlg.setPrompt(xi18nc("@info", "Please enter the password to open this file:<nl/>%1", fileName));
        if (!dlg.exec()) {
            m_status = KoFilter::UserCancelled;
            QApplication::restoreOverrideCursor();
            break;
        }
        QApplication::restoreOverrideCursor();
        ++m_passwordTries;
        // TODO: Make this more robust
        const QString type = fileName.right(fileName.length() - fileName.lastIndexOf(QLatin1Char('.')) - 1).toLower();
        QStringList args;
        args << QStringLiteral("-jar");
        args << planConvert;
        args << QLatin1String("--type");
        args << type;
        args << QLatin1String("--password");
        args << QLatin1String(dlg.password().toUtf8());
        args << normalizedInFile;
        args << normalizedOutFile;
        run(args);
    }
    return m_status;
}

void MpxjImport::run(const QStringList &args)
{
    debugMpxjImport<<args;
    QProcess java;
    connect(&java,  QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &MpxjImport::slotFinished);
    connect(&java,  &QProcess::errorOccurred, this, &MpxjImport::slotError);
    java.start(QStringLiteral("java"), args);
    if (!java.waitForFinished(60000)) {
        m_status = KoFilter::JavaExecutionError;
    }
}

void MpxjImport::slotFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    auto java = qobject_cast<QProcess*>(sender());
    Q_ASSERT(java);
    debugMpxjImport<<exitCode<<exitStatus;
    auto s = java->readAllStandardOutput();
    qInfo()<<Q_FUNC_INFO<<s;
    if (s.contains("Exception")) {
        m_status = KoFilter::ParsingError;
        if (s.contains("assword")) {
            m_status = KoFilter::PasswordProtected;
        } else if (s.contains("ClassNotFoundException: sun.jdbc.odbc.JdbcOdbcDriver")) {
            m_status = KoFilter::JdbcOdbcDriverException;
        } else if (s.contains("Invalid file format")) {
            errorMpxjImport<<"MPXJ failed to read the file";
            m_status = KoFilter::InvalidFormat;
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
