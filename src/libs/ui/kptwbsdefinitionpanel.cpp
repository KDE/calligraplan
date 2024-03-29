/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2005-2007, 2012 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kptwbsdefinitionpanel.h"
#include "kptwbsdefinition.h"
#include "kptcommand.h"
#include "kptproject.h"
#include "kptdebug.h"

#include <KComboBox>

#include <QComboBox>
#include <QTableWidget>
#include <QStringList>

#include <kundo2command.h>


namespace KPlato
{

ComboBoxDelegate::ComboBoxDelegate(QStringList &list, QObject *parent)
    : QStyledItemDelegate(parent)
{
    debugPlan;
    setObjectName(QStringLiteral("ComboBoxDelegate"));
    m_list = list;
}

QWidget *ComboBoxDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/* option */, const QModelIndex &/* index */) const
{
    debugPlan;
    QComboBox *editor = new KComboBox(parent);
    editor->installEventFilter(const_cast<ComboBoxDelegate*>(this));
    return editor;
}

void ComboBoxDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QString value = index.model()->data(index, Qt::DisplayRole).toString();
    debugPlan<<value<<":"<<m_list;
    QComboBox *comboBox = static_cast<QComboBox*>(editor);

    comboBox->insertItems(0, m_list);
    comboBox->setCurrentIndex(comboBox->findText(value));
}

void ComboBoxDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QComboBox *comboBox = static_cast<QComboBox*>(editor);
    debugPlan<<comboBox->currentText();
    model->setData(index, comboBox->currentText());
}

void ComboBoxDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
    editor->setGeometry(option.rect);
}

//----------------------

WBSDefinitionPanel::WBSDefinitionPanel(Project &project, WBSDefinition &def, QWidget *p)
    : QWidget(p),
      m_project(project),
      m_def(def),
      selectedRow(-1)
{
    setupUi(this);
    
    projectCode->setText(m_def.projectCode());
    projectSeparator->setText(m_def.projectSeparator());
    QStringList codeList = m_def.codeList();
    debugPlan<<codeList;
    defaultSeparator->setText(m_def.defaultSeparator());
    defaultCode->addItems(codeList);
    defaultCode->setCurrentIndex(m_def.defaultCodeIndex());
    defaultCode->setFocus();
    levelsTable->setItemDelegateForColumn(0, new ComboBoxDelegate(codeList, this));
    levelsGroup->setChecked(m_def.isLevelsDefEnabled());
    int i = 0;
    const QMap<int, WBSDefinition::CodeDef> &lev = m_def.levelsDef();
    levelsTable->setRowCount(lev.count());
    QStringList sl;
    debugPlan<<"Map size="<<lev.count();
    QMap<int, WBSDefinition::CodeDef>::const_iterator it;
    for (it = lev.begin(); it != lev.end(); ++it) {
        sl << QStringLiteral("%1").arg(it.key());
        QTableWidgetItem *item = new QTableWidgetItem();
        item->setData(Qt::DisplayRole, it.value().code);
        levelsTable->setItem(i, 0, item);
        item = new QTableWidgetItem();
        item->setText(it.value().separator);
        levelsTable->setItem(i, 1, item);
        i++;
    }
    levelsTable->setVerticalHeaderLabels(sl);
    //levelsTable->setColumnStretchable(0, true);
    slotLevelChanged(level->value());
    
    connect(projectCode, &QLineEdit::textChanged, this, &WBSDefinitionPanel::slotChanged);
    connect(projectSeparator, &QLineEdit::textChanged, this, &WBSDefinitionPanel::slotChanged);
    connect(defaultCode, SIGNAL(activated(int)), SLOT(slotChanged()));
    connect(defaultSeparator, &QLineEdit::textChanged, this, &WBSDefinitionPanel::slotChanged);
    connect(levelsGroup, &QGroupBox::toggled, this, &WBSDefinitionPanel::slotLevelsGroupToggled);
    connect(levelsTable, &QTableWidget::cellChanged, this, &WBSDefinitionPanel::slotChanged);
    connect(levelsTable, &QTableWidget::itemSelectionChanged, this, &WBSDefinitionPanel::slotSelectionChanged);
    connect(level, SIGNAL(valueChanged(int)), SLOT(slotLevelChanged(int)));
    connect(removeBtn, &QAbstractButton::clicked, this, &WBSDefinitionPanel::slotRemoveBtnClicked);
    connect(addBtn, &QAbstractButton::clicked, this, &WBSDefinitionPanel::slotAddBtnClicked);

    removeBtn->setEnabled(false);
}

void WBSDefinitionPanel::setStartValues() {
}

KUndo2Command *WBSDefinitionPanel::buildCommand() {
    WBSDefinition def = m_def;
    def.setProjectCode(projectCode->text());
    def.setProjectSeparator(projectSeparator->text());
    def.setDefaultCode(defaultCode->currentIndex());
    def.setDefaultSeparator(defaultSeparator->text());
    
    def.setLevelsDefEnabled(levelsGroup->isChecked());

    def.clearLevelsDef();
    for (int i = 0; i < levelsTable->rowCount(); ++i) {
        def.setLevelsDef(levelsTable->verticalHeaderItem(i)->text().toInt(), levelsTable->item(i, 0)->text(), levelsTable->item(i, 1)->text());
    }
    WBSDefinitionModifyCmd *cmd = new WBSDefinitionModifyCmd(m_project, def, kundo2_i18n("Modify WBS Code Definition"));
    return cmd;
}

bool WBSDefinitionPanel::ok() {
    debugPlan;
    return true;
}

void WBSDefinitionPanel::slotChanged() {
    Q_EMIT changed(true);
}

void WBSDefinitionPanel::slotSelectionChanged() {
    QString s;
    selectedRow = -1;
    QList<QTableWidgetItem *> items = levelsTable->selectedItems();
    if (items.count() == 2 && items[0]->row() == items[1]->row()) {
        selectedRow = items[0]->row();
        s = QStringLiteral("Row[%1]=selected ").arg(selectedRow);
    } else {
        s = QStringLiteral("None selected");
    }
    removeBtn->setEnabled(selectedRow != -1);
    debugPlan<<s;
}

void WBSDefinitionPanel::slotRemoveBtnClicked() {
    debugPlan<<selectedRow;
    if (selectedRow == -1) {
        return;
    }
    levelsTable->removeRow(selectedRow);
    removeBtn->setEnabled(false);
    slotLevelChanged(level->value());
}

void WBSDefinitionPanel::slotAddBtnClicked() {
    debugPlan;
    int i=levelsTable->rowCount()-1;
    for (; i >= 0; --i) {
        debugPlan<<"Checking row["<<i<<"]="<<levelsTable->verticalHeaderItem(i)->text()<<" with"<<level->value();
        if (level->value() > levelsTable->verticalHeaderItem(i)->text().toInt()) {
            break;
        }
    }
    i++;
    levelsTable->insertRow(i);
    levelsTable->setVerticalHeaderItem(i, new QTableWidgetItem(QStringLiteral("%1").arg(level->value())));
    QTableWidgetItem *item = new QTableWidgetItem();
    item->setData(Qt::DisplayRole, (m_def.codeList().value(m_def.defaultCodeIndex())));
    levelsTable->setItem(i, 0, item);
    item = new QTableWidgetItem();
    item->setText(m_def.defaultSeparator());
    levelsTable->setItem(i, 1, item);
    
    addBtn->setEnabled(false);
    slotChanged();
    
    debugPlan<<"Added row="<<i<<" level="<<level->value();
}

void WBSDefinitionPanel::slotLevelChanged(int value) {
    for (int i=0; i < levelsTable->rowCount(); ++i) {
        if (value == levelsTable->verticalHeaderItem(i)->text().toInt()) {
            addBtn->setEnabled(false);
            return;
        }
    }
    addBtn->setEnabled(levelsGroup->isChecked());
    slotChanged();
}
void WBSDefinitionPanel::slotLevelsGroupToggled(bool /*on*/) {
    debugPlan;
    slotLevelChanged(level->value());
}


}  //KPlato namespace
