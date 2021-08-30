/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "MainWindow.h"

#include "DocumentsSaveDialog.h"

#include <MainDocument.h>

MainWindow::MainWindow(const QByteArray &nativeMimeType, const KoComponentData &componentData)
    : KoMainWindow(nativeMimeType, componentData)
{
}


MainWindow::~MainWindow()
{
}

bool MainWindow::saveDocumentInternal(bool saveas, bool silent, int specialOutputFlag)
{
    qInfo()<<Q_FUNC_INFO<<saveas<<silent<<specialOutputFlag;
    Q_UNUSED(silent)
    Q_UNUSED(specialOutputFlag)

    MainDocument *maindoc = qobject_cast<MainDocument*>(rootDocument());
    if (!maindoc || !maindoc->documentPart()) {
        return true;
    }
    bool ret = false;
    if (saveas || maindoc->isChildrenModified()) {
        DocumentsSaveDialog dlg(maindoc);
        if (dlg.exec() == QDialog::Accepted) {
            if (dlg.saveMain()) {
                const auto url = dlg.mainUrl();
                qInfo()<<Q_FUNC_INFO<<url;
                ret = KoMainWindow::saveDocumentInternal(false, false, false);
            }
            const auto children = dlg.modifiedDocuments();
            for (const auto child : children) {
                child->save();
            }
        }
    } else if (maindoc->isModified()) {
        ret = KoMainWindow::saveDocumentInternal(false, false, false);
    }
    return ret;
}
