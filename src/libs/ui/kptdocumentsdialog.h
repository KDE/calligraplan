/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2007, 2012 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KPTDOCUMENTSDIALOG_H
#define KPTDOCUMENTSDIALOG_H

#include "planui_export.h"
#include "PlanMacros.h"

#include <KoDialog.h>

namespace KPlato
{

class DocumentsPanel;
class Node;
class MacroCommand;
        
class PLANUI_EXPORT DocumentsDialog : public KoDialog
{
    Q_OBJECT
public:
    /**
     * The constructor for the documents dialog.
     * @param node the task or project to show documents for
     * @param parent parent widget
     * @param readOnly determines whether the data are read-only
     */
    explicit DocumentsDialog(Node &node, QWidget *parent = nullptr, bool readOnly = false  );

    MacroCommand *buildCommand();

protected Q_SLOTS:
    void slotButtonClicked(int button) override;

protected:
    DocumentsPanel *m_panel;
};

} //KPlato namespace

#endif
