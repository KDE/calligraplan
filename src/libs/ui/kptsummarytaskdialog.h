/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2002 Bo Thorsen bo @sonofthor.dk
   SPDX-FileCopyrightText: 2004-2010 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTSUMMARYTASKDIALOG_H
#define KPTSUMMARYTASKDIALOG_H

#include "planui_export.h"

#include <KoDialog.h>

namespace KPlato
{

class SummaryTaskGeneralPanel;
class Task;
class Node;
class MacroCommand;

/**
 * The dialog that shows and allows you to alter summary tasks.
 */
class PLANUI_EXPORT SummaryTaskDialog : public KoDialog {
    Q_OBJECT
public:
    /**
     * The constructor for the summary task settings dialog.
     * @param task the task to edit
     * @param parent parent widget
     */
    explicit SummaryTaskDialog(Task &task,  QWidget *parent=nullptr);

    MacroCommand *buildCommand();

protected Q_SLOTS:
    void slotButtonClicked(int button) override;
    void slotTaskRemoved(KPlato::Node *node);

private:
    Node *m_node;

    SummaryTaskGeneralPanel *m_generalTab;
};

} //KPlato namespace

#endif // SUMMARYTASKDIALOG_H
