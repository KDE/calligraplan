/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef PORTFOLIO_MAINWINDOW_H
#define PORTFOLIO_MAINWINDOW_H

#include "KoMainWindow.h"


/**
 * @brief Main window for a Calligra Plan Portfolio application
 *
 * This class is used to represent a main window
 * of a portfolio component
 */
class MainWindow : public KoMainWindow
{
    Q_OBJECT
public:

    /**
     *  Constructor.
     *
     *  Initializes a portfolio main window
     */
    explicit MainWindow(const QByteArray &nativeMimeType, const KoComponentData &instance);

    /**
     *  Destructor.
     */
    ~MainWindow() override;

protected:
    /**
     * Handles saving of both main document *and* sub-documents
     *
     * If only main document is modified, it is saved as normal.
     *
     * If any sub-documents are modified a list of the modified docs
     * is shown where you can select what to do with each doc.
     *
     * @param saveas if set to TRUE the user is always prompted for a filename
     *
     * @param silent if set to TRUE rootDocument()->setTitleModified will not be called.
     *
     * @param specialOutputFlag set to enums defined in KoDocument if save to special output format
     *
     * @return TRUE on success, false on error or cancel
     *         (don't display anything in this case, the error dialog box is also implemented here
     *         but restore the original URL in slotFileSaveAs)
     */
    bool saveDocumentInternal(bool saveas, bool silent, int specialOutputFlag) override;

};

#endif
