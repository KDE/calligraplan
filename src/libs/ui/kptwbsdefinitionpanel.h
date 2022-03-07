/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2005-2007 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTWBSDEFINITIONPANEL_H
#define KPTWBSDEFINITIONPANEL_H

#include "planui_export.h"

#include "ui_kptwbsdefinitionpanelbase.h"

#include <QStyledItemDelegate>
#include <QWidget>

class KUndo2Command;

namespace KPlato
{

class WBSDefinition;
class Project;

class ComboBoxDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit ComboBoxDelegate(QStringList &list, QObject *parent = nullptr);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                            const QModelIndex &index) const override;

    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                        const QModelIndex &index) const override;

    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
private:
    QStringList m_list;
};

//---------------
class WBSDefinitionPanel : public QWidget, public Ui_WBSDefinitionPanelBase {
    Q_OBJECT
public:
    explicit WBSDefinitionPanel(Project &project, WBSDefinition &def, QWidget *parent=nullptr);

    KUndo2Command *buildCommand();

    bool ok();

    void setStartValues();

Q_SIGNALS:
    void changed(bool enable);
    
protected Q_SLOTS:
    void slotChanged();
    void slotSelectionChanged();
    void slotRemoveBtnClicked();
    void slotAddBtnClicked();
    void slotLevelChanged(int);
    void slotLevelsGroupToggled(bool on);
private:
    Project &m_project;
    WBSDefinition &m_def;
    int selectedRow;
};

} //KPlato namespace

#endif // WBSDEFINITIONPANEL_H
