/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "MainWindow.h"

#include "DocumentsSaveDialog.h"

#include "MainDocument.h"
#include "PlanGroupDebug.h"

#include <KoStore.h>
#include <KoNetAccess.h>

#include <KMessageBox>
#include <KLocalizedString>

#include <QtGlobal>

MainWindow::MainWindow(const QByteArray &nativeMimeType, const KoComponentData &componentData)
    : KoMainWindow(nativeMimeType, componentData)
{
}


MainWindow::~MainWindow()
{
}

void MainWindow::setRootDocument(KoDocument* doc, KoPart* part, bool deletePrevious)
{
    KoMainWindow::setRootDocument(doc, part, deletePrevious);
    auto document = qobject_cast<MainDocument*>(rootDocument());
    if (document) {
        connect(document, &MainDocument::documentInserted, this, &MainWindow::slotDocumentInserted, Qt::UniqueConnection);
        connect(document, &MainDocument::documentRemoved, this, &MainWindow::slotDocumentInserted, Qt::UniqueConnection);
        connect(document, &MainDocument::documentModified, this, QOverload<>::of(&KoMainWindow::updateCaption));
        updateCaption();
    }
}

void MainWindow::slotDocumentInserted()
{
    updateCaption();
}

void MainWindow::slotDocumentRemoved()
{
    updateCaption();
}

bool MainWindow::isDocumentModified()
{
    auto document = qobject_cast<MainDocument*>(rootDocument());
    if (!document) {
        return false;
    }
    bool mod = document->isModified();
    if (!mod) {
        const auto docs = document->documents();
        for (const auto doc : docs) {
            if (doc->isModified()) {
                mod = true;
                break;
            }
        }
    }
    return mod;
}


bool MainWindow::saveDocumentInternal(bool saveas, bool silent, int specialOutputFlag)
{
    Q_UNUSED(silent)

    MainDocument *maindoc = qobject_cast<MainDocument*>(rootDocument());
    if (!maindoc || !maindoc->documentPart()) {
        return true;
    }
    int outputFlag = maindoc->specialOutputFlag();
    if (specialOutputFlag != 0) {
        outputFlag = specialOutputFlag;
    }
    //qInfo()<<Q_FUNC_INFO<<maindoc<<maindoc->specialOutputFlag()<<specialOutputFlag<<outputFlag;
    bool ret = false;
    if (saveas) {
        QList<KoDocument*> children;
        const auto docs = maindoc->documents();
        for (auto doc : docs) {
            if (!doc->property(SAVEEMBEDDED).toBool()) {
                children << doc;
            }
        }
        DocumentsSaveDialog dlg(maindoc, children);
        if (dlg.exec() == QDialog::Accepted) {
            if (dlg.saveMain()) {
                const auto url = dlg.mainUrl();
                maindoc->setUrl(url);
                ret = KoMainWindow::saveDocumentInternal(false, false, outputFlag);
            }
            const auto children = dlg.documentsToSave();
            for (const auto child : children) {
                child->setAlwaysAllowSaving(true);
                if (outputFlag == KoStore::Encrypted) {
                    child->setPassword(maindoc->password());
                }
                if (!child->property(SAVEEMBEDDED).toBool()) {
                    child->save();
                }
            }
        }
        return ret;
    }
    QList<KoDocument*> externalDocs;
    const auto children = maindoc->documents();
    for (const auto child : children) {
        if (child->isModified() && !child->property(SAVEEMBEDDED).toBool()) {
            child->setAlwaysAllowSaving(false);
            externalDocs << child;
        }
    }
    if (!externalDocs.isEmpty()) {
        DocumentsSaveDialog dlg(maindoc, externalDocs);
        if (dlg.exec() == QDialog::Accepted) {
            if (dlg.saveMain()) {
                const auto url = dlg.mainUrl();
                maindoc->setUrl(url);
                ret = KoMainWindow::saveDocumentInternal(false, false, outputFlag);
            }
            const auto children = dlg.documentsToSave();
            for (const auto child : children) {
                Q_ASSERT(!child->property(SAVEEMBEDDED).toBool());
                KIO::UDSEntry entry;
                if (KIO::NetAccess::stat(child->url(), entry, this)) {
                    if (entry.numberValue(KIO::UDSEntry::UDS_MODIFICATION_TIME) != child->property(ORIGINALMODIFICATIONTIME).toLongLong()) {
                        int res = KMessageBox::warningYesNo(this,
                                                  xi18nc("@info", "<p>The external project file <filename>%1</filename> has been modified.</p><p>Do you want to save it?</p>", child->url().toDisplayString()));
                        if (res == KMessageBox::No) {
                            continue;
                        }
                    }
                } else {
                    debugPortfolio<<"Project file does not exist:"<<child->url();
                }
                connect(child, &KoDocument::completed, this, &MainWindow::documentSaved);
                connect(child, &KoDocument::canceled, this, &MainWindow::slotDocumentSaveCanceled);
                child->save();
            }
        }
        return ret;
    }
    return KoMainWindow::saveDocumentInternal(false, false, outputFlag);
}

void MainWindow::slotDocumentSaved()
{
    auto doc = qobject_cast<KoDocument*>(sender());
    Q_ASSERT(doc);
    if (doc) {
        disconnect(doc, &KoDocument::completed, this, &MainWindow::documentSaved);
        disconnect(doc, &KoDocument::canceled, this, &MainWindow::slotDocumentSaveCanceled);
        KIO::UDSEntry entry;
        if (KIO::NetAccess::stat(doc->url(), entry, this)) {
            doc->setProperty(ORIGINALMODIFICATIONTIME, entry.numberValue(KIO::UDSEntry::UDS_MODIFICATION_TIME));
        }
    }
}

void MainWindow::slotDocumentSaveCanceled()
{
    auto doc = qobject_cast<KoDocument*>(sender());
    Q_ASSERT(doc);
    if (doc) {
        disconnect(doc, &KoDocument::completed, this, &MainWindow::documentSaved);
        disconnect(doc, &KoDocument::canceled, this, &MainWindow::slotDocumentSaveCanceled);
    }
}
