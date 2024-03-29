/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2004, 2007 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kpttaskdefaultpanel.h"

#include "kptduration.h"
#include "kptmycombobox_p.h"
#include "calligraplansettings.h"

#ifdef PLAN_KDEPIMLIBS_FOUND
#include <akonadi/contact/emailaddressselectiondialog.h>
#include <akonadi/contact/emailaddressselectionwidget.h>
#include <akonadi/contact/emailaddressselection.h>
#endif

#include <QDateTime>

#include <KActionCollection>
#include <KTextEdit>

namespace KPlato
{

TaskDefaultPanel::TaskDefaultPanel(QWidget *parent)
    : ConfigTaskPanelImpl(parent)
{
}

//-----------------------------
ConfigTaskPanelImpl::ConfigTaskPanelImpl(QWidget *p)
    : QWidget(p)
{

    setupUi(this);
    kcfg_ExpectedEstimate->setMinimumUnit((Duration::Unit)KPlatoSettings::self()->minimumDurationUnit());
    kcfg_ExpectedEstimate->setMaximumUnit((Duration::Unit)KPlatoSettings::self()->maximumDurationUnit());

#ifndef PLAN_KDEPIMLIBS_FOUND
    chooseLeader->hide();
#endif

    // FIXME
    // [Bug 311940] New: Plan crashes when typing a text in the filter textbox before the textbook is fully loaded when selecting a contact from the addressbook
    chooseLeader->hide();

    initDescription();

    connect(chooseLeader, &QAbstractButton::clicked, this, &ConfigTaskPanelImpl::changeLeader);
    
    connect(kcfg_ConstraintStartTime, &QDateTimeEdit::dateTimeChanged, this, &ConfigTaskPanelImpl::startDateTimeChanged);
    
    connect(kcfg_ConstraintEndTime, &QDateTimeEdit::dateTimeChanged, this, &ConfigTaskPanelImpl::endDateTimeChanged);

    // Hack to have an interface to kcfg wo adding a custom class for this
    kcfg_Unit->addItems(Duration::unitList(true));
    connect(kcfg_ExpectedEstimate, &DurationSpinBox::unitChanged, this, &ConfigTaskPanelImpl::unitChanged);
    kcfg_Unit->hide();
    connect(kcfg_Unit, SIGNAL(currentIndexChanged(int)), SLOT(currentUnitChanged(int)));
}

void ConfigTaskPanelImpl::initDescription()
{
    toolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);

    KActionCollection *collection = new KActionCollection(this); //krazy:exclude=tipsandthis
    kcfg_Description->setRichTextSupport(KRichTextWidget::SupportBold |
                                            KRichTextWidget::SupportItalic |
                                            KRichTextWidget::SupportUnderline |
                                            KRichTextWidget::SupportStrikeOut |
                                            KRichTextWidget::SupportChangeListStyle |
                                            KRichTextWidget::SupportAlignment |
                                            KRichTextWidget::SupportFormatPainting);

    collection->addActions(kcfg_Description->createActions());

    toolbar->addAction(collection->action(QStringLiteral("format_text_bold")));
    toolbar->addAction(collection->action(QStringLiteral("format_text_italic")));
    toolbar->addAction(collection->action(QStringLiteral("format_text_underline")));
    toolbar->addAction(collection->action(QStringLiteral("format_text_strikeout")));
    toolbar->addSeparator();

    toolbar->addAction(collection->action(QStringLiteral("format_list_style")));
    toolbar->addSeparator();

    toolbar->addAction(collection->action(QStringLiteral("format_align_left")));
    toolbar->addAction(collection->action(QStringLiteral("format_align_center")));
    toolbar->addAction(collection->action(QStringLiteral("format_align_right")));
    toolbar->addAction(collection->action(QStringLiteral("format_align_justify")));
    toolbar->addSeparator();

//    toolbar->addAction(collection->action(QStringLiteral("format_painter")));

//     kcfg_Description->append("");
    kcfg_Description->setReadOnly(false);
    kcfg_Description->setOverwriteMode(false);
    kcfg_Description->setLineWrapMode(KTextEdit::WidgetWidth);
    kcfg_Description->setTabChangesFocus(true);

}


void ConfigTaskPanelImpl::changeLeader()
{
#ifdef PLAN_KDEPIMLIBS_FOUND
    QPointer<Akonadi::EmailAddressSelectionDialog> dlg = new Akonadi::EmailAddressSelectionDialog(this);
    if (dlg->exec() && dlg) {
        QStringList names;
        const Akonadi::EmailAddressSelection::List selections = dlg->selectedAddresses();
        for (const Akonadi::EmailAddressSelection &selection : selections) {
            QString s = selection.name();
            if (! selection.email().isEmpty()) {
                if (! selection.name().isEmpty()) {
                    s += " <";
                }
                s += selection.email();
                if (! selection.name().isEmpty()) {
                    s += '>';
                }
                if (! s.isEmpty()) {
                    names << s;
                }
            }
        }
        if (! names.isEmpty()) {
            kcfg_Leader->setText(names.join(", "));
        }
    }
#endif
}

void ConfigTaskPanelImpl::startDateTimeChanged(const QDateTime &dt)
{
    if (dt > kcfg_ConstraintEndTime->dateTime()) {
        kcfg_ConstraintEndTime->setDateTime(dt);
    }
}

void ConfigTaskPanelImpl::endDateTimeChanged(const QDateTime &dt)
{
    if (kcfg_ConstraintStartTime->dateTime() > dt) {
        kcfg_ConstraintStartTime->setDateTime(dt);
    }
}

void ConfigTaskPanelImpl::unitChanged(int unit)
{
    if (kcfg_Unit->currentIndex() != unit) {
        kcfg_Unit->setCurrentIndex(unit);
        // kcfg uses the activated() signal to track changes
        kcfg_Unit->emitActivated(unit);
    }
}

void ConfigTaskPanelImpl::currentUnitChanged(int unit)
{
    // to get values set at startup
    if (unit != kcfg_ExpectedEstimate->unit()) {
        kcfg_ExpectedEstimate->setUnit((Duration::Unit)unit);
    }
}

}  //KPlato namespace
