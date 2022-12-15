/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 2022 Dag Andersen <dag.andersen@kdemail.net>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "FilterWidget.h"

#include <QHBoxLayout>
#include <QLineEdit>
#include <QToolButton>
#include <QLabel>
#include <QSize>

#include <KLocalizedString>

#include <KoIcon.h>

using namespace KPlato;

FilterWidget::FilterWidget(QWidget *parent)
    : QWidget(parent)
{
    init(true);
}

FilterWidget::FilterWidget(bool enableExtendedOptions, QWidget *parent)
    : QWidget(parent)
{
    init(enableExtendedOptions);
}

void FilterWidget::init(bool enableExtendedOptions)
{
    auto l = new QHBoxLayout(this);
    if (enableExtendedOptions) {
        extendedOptions = new QToolButton(this);
        extendedOptions->setIcon(koIcon("view-filter"));
        extendedOptions->setToolTip(i18nc("@info:tooltip", "Toggle extended filter options"));
        l->addWidget(extendedOptions);
    }
    lineedit = new QLineEdit();
//    lineedit->addAction(koIcon("view-filter"), QLineEdit::LeadingPosition);
    lineedit->setToolTip(i18nc("@info:tooltip", "Filter names"));
    l->addWidget(lineedit);

    l->addStretch();
}
