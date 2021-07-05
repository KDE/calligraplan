/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2017 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "ConfigProjectPanel.h"

#include "calligraplansettings.h"


#include <kactioncollection.h>

#include <QFileDialog>

namespace KPlato
{

ConfigProjectPanel::ConfigProjectPanel(QWidget *parent)
    : ConfigProjectPanelImpl(parent)
{
}

//-----------------------------
ConfigProjectPanelImpl::ConfigProjectPanelImpl(QWidget *p)
    : QWidget(p)
{

    setupUi(this);

    initDescription();

    connect(resourceFileBrowseBtn, &QAbstractButton::clicked, this, &ConfigProjectPanelImpl::resourceFileBrowseBtnClicked);
    connect(projectsPlaceBrowseBtn, &QAbstractButton::clicked, this, &ConfigProjectPanelImpl::projectsPlaceBrowseBtnClicked);

    QString tooltip = xi18nc("@info:tooltip", "The project manager of this project.");
    kcfg_Manager->setToolTip(tooltip);
    leaderlabel->setToolTip(tooltip);
    tooltip = xi18nc("@info:tooltip", "Check to enable shared resources");
    kcfg_UseSharedResources->setToolTip(tooltip);
    usrlabel->setToolTip(tooltip);
    tooltip = xi18nc("@info:tooltip", "The file where shared resources are defined");
    kcfg_SharedResourcesFile->setToolTip(tooltip);
    srflabel->setToolTip(tooltip);
    tooltip = xi18nc("@info:tooltip", "The directory where projects sharing the resources are placed");
    kcfg_SharedProjectsPlace->setToolTip(tooltip);
    spplabel->setToolTip(tooltip);
}

void ConfigProjectPanelImpl::resourceFileBrowseBtnClicked()
{
    QFileDialog dialog(this, tr("Shared resources file"));
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setNameFilters(QStringList()<<tr("Plan file (*.plan)"));
    if (dialog.exec()) {
        kcfg_SharedResourcesFile->setText(dialog.selectedFiles().value(0));
    }
}

void ConfigProjectPanelImpl::projectsPlaceBrowseBtnClicked()
{
    QFileDialog dialog(this, tr("Shared projects place"));
    dialog.setFileMode(QFileDialog::Directory);
    if (dialog.exec()) {
        kcfg_SharedProjectsPlace->setText(dialog.directory().absolutePath());
    }
}

void ConfigProjectPanelImpl::initDescription()
{
    toolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);

    KActionCollection *collection = new KActionCollection(this); //krazy:exclude=tipsandthis
    kcfg_ProjectDescription->setRichTextSupport(KRichTextWidget::SupportBold |
                                            KRichTextWidget::SupportItalic |
                                            KRichTextWidget::SupportUnderline |
                                            KRichTextWidget::SupportStrikeOut |
                                            KRichTextWidget::SupportChangeListStyle |
                                            KRichTextWidget::SupportAlignment |
                                            KRichTextWidget::SupportFormatPainting);

    collection->addActions(kcfg_ProjectDescription->createActions());

    toolbar->addAction(collection->action("format_text_bold"));
    toolbar->addAction(collection->action("format_text_italic"));
    toolbar->addAction(collection->action("format_text_underline"));
    toolbar->addAction(collection->action("format_text_strikeout"));
    toolbar->addSeparator();

    toolbar->addAction(collection->action("format_list_style"));
    toolbar->addSeparator();

    toolbar->addAction(collection->action("format_align_left"));
    toolbar->addAction(collection->action("format_align_center"));
    toolbar->addAction(collection->action("format_align_right"));
    toolbar->addAction(collection->action("format_align_justify"));
    toolbar->addSeparator();

//    toolbar->addAction(collection->action("format_painter"));

    kcfg_ProjectDescription->append("");
    kcfg_ProjectDescription->setReadOnly(false);
    kcfg_ProjectDescription->setOverwriteMode(false);
    kcfg_ProjectDescription->setLineWrapMode(KTextEdit::WidgetWidth);
    kcfg_ProjectDescription->setTabChangesFocus(true);

}


}  //KPlato namespace
