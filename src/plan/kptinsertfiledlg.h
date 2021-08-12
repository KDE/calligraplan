/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2009 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTINSERTFILEDLG_H
#define KPTINSERTFILEDLG_H

#include "ui_kptinsertfilepanel.h"

#include <KoDialog.h>

class QUrl;

namespace KPlato
{

class Project;
class Node;
class InsertFilePanel;

class InsertFileDialog : public KoDialog
{
    Q_OBJECT
public:
    InsertFileDialog(Project &project, Node *currentNode, QWidget *parent=nullptr);

    QUrl url() const;
    Node *parentNode() const;
    Node *afterNode() const;

private:
    InsertFilePanel *m_panel;
};


class InsertFilePanel : public QWidget
{
    Q_OBJECT
public:
    InsertFilePanel(Project &project, Node *currentNode, QWidget *parent);

    QUrl url() const;
    Node *parentNode() const;
    Node *afterNode() const;

    Ui::InsertFilePanel ui;

Q_SIGNALS:
    void enableButtonOk(bool);

protected Q_SLOTS:
    void changed(const QString&);

    void slotOpenFileDialog(KUrlRequester *);

private:
    Project &m_project;
    Node *m_node;
};


} //KPlato namespace

#endif
