/*This file is part of the KDE project
 * Copyright (C) 2019 Dag Andersen <calligra-devel@kde.org
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
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
        openLink = new QAction(QIcon::fromTheme("link"), xi18nc("@action:intoolbar", "Open Link"));
        openLink->setShortcut(Qt::CTRL + Qt::Key_O);
        openLink->setObjectName("open_link");
        connect(openLink, &QAction::triggered, this, &RichTextWidget::slotOpenLink);
        lst.append(openLink);
    }
    return lst;
}

QMenu *RichTextWidget::mousePopupMenu() {
    QMenu *menu = KRichTextWidget::mousePopupMenu();
    if (!currentLinkUrl().isEmpty()) {
        menu->insertSeparator(menu->actions().first());
        QAction *a = new QAction(QIcon::fromTheme("link"), xi18nc("@action:intoolbar", "Open Link"));
        if (openLink) {
            a->setShortcut(openLink->shortcut());
        }
        menu->insertAction(menu->actions().first(), a);
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
