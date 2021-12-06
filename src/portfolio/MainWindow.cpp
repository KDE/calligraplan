/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "MainWindow.h"

#include "DocumentsSaveDialog.h"

#include "MainDocument.h"

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
    Q_UNUSED(specialOutputFlag)

    MainDocument *maindoc = qobject_cast<MainDocument*>(rootDocument());
    if (!maindoc || !maindoc->documentPart()) {
        return true;
    }
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
                ret = KoMainWindow::saveDocumentInternal(false, false, false);
            }
            const auto children = dlg.documentsToSave();
            for (const auto child : children) {
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
            externalDocs << child;
        }
    }
    if (!externalDocs.isEmpty()) {
        DocumentsSaveDialog dlg(maindoc, externalDocs);
        if (dlg.exec() == QDialog::Accepted) {
            if (dlg.saveMain()) {
                const auto url = dlg.mainUrl();
                maindoc->setUrl(url);
                ret = KoMainWindow::saveDocumentInternal(false, false, false);
            }
            const auto children = dlg.documentsToSave();
            for (const auto child : children) {
                Q_ASSERT(!child->property(SAVEEMBEDDED).toBool());
                child->save();
            }
        }
        return ret;
    }
    return KoMainWindow::saveDocumentInternal(false, false, false);
}
