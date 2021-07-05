/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2007, 2012 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KPTDOCUMENTSPANEL_H
#define KPTDOCUMENTSPANEL_H

#include "planui_export.h"

#include "ui_kptdocumentspanel.h"

#include "kptdocuments.h"

#include <QModelIndexList>
#include <QWidget>
#include <kundo2qstack.h>

class QModelIndex;

namespace KPlato
{

class Node;
class DocumentItemModel;
class MacroCommand;
class DocumentTreeView;

class PLANUI_EXPORT DocumentsPanel : public QWidget
{
    Q_OBJECT
public:
    explicit DocumentsPanel(Node &node, QWidget *parent = nullptr);
    ~DocumentsPanel() override {}
    
    MacroCommand *buildCommand();
    
    Ui::DocumentsPanel widget;

    DocumentItemModel* model() const;
    Document *selectedDocument() const;

    void addNodeName();

Q_SIGNALS:
    void changed(bool value);
    
protected Q_SLOTS:
    void slotAddUrl();
    void slotChangeUrl();
    void slotRemoveUrl();
    void slotViewUrl();
    
    void dataChanged(const QModelIndex&);
    
    void slotSelectionChanged(const QModelIndexList&);
    void currentChanged(const QModelIndex &index);
    
private:
    Node &m_node;
    Documents m_docs;
    enum State { Unmodified = 1, Modified = 2, Added = 4, Removed = 8 };
    QMap<Document*, State> m_state;
    QMap<Document*, QUrl> m_orgurl;
    KUndo2QStack m_cmds;
    DocumentTreeView *m_view;
};

}

#endif
