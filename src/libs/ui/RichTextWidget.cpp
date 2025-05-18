/*This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <calligra-devel@kde.org
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include <RichTextWidget.h>

#include <KLocalizedString>

#include <QDesktopServices>
#include <QMenu>

using namespace KPlato;

RichTextWidget::RichTextWidget(QWidget *parent)
    : KRichTextWidget(parent)
{
}

QList<QAction*> RichTextWidget::createActions()
{
    QList<QAction*> lst = KRichTextWidget::createActions();
    if (richTextSupport() & RichTextWidget::SupportHyperlinks) {
        openLink = new QAction(QIcon::fromTheme(QStringLiteral("link")), xi18nc("@action:intoolbar", "Open Link"));
        openLink->setShortcut(Qt::CTRL | Qt::Key_O);
        openLink->setObjectName(QStringLiteral("open_link"));
        connect(openLink, &QAction::triggered, this, &RichTextWidget::slotOpenLink);
        lst.append(openLink);
    }
    return lst;
}

QMenu *RichTextWidget::mousePopupMenu() {
    QMenu *menu = KRichTextWidget::mousePopupMenu();
    if (!currentLinkUrl().isEmpty()) {
        menu->insertSeparator(menu->actions().constFirst());
        QAction *a = new QAction(QIcon::fromTheme(QStringLiteral("link")), xi18nc("@action:intoolbar", "Open Link"));
        if (openLink) {
            a->setShortcut(openLink->shortcut());
        }
        menu->insertAction(menu->actions().constFirst(), a);
        connect(a, &QAction::triggered, this, &RichTextWidget::slotOpenLink);
    }
    return menu;
}

void RichTextWidget::slotOpenLink()
{
    QUrl url = QUrl::fromUserInput(currentLinkUrl());
    if (url.isValid()) {
        QDesktopServices::openUrl(url);
    }
}
