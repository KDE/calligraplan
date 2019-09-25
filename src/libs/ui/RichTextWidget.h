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

#ifndef RICHTEXTWIDGET_H
#define RICHTEXTWIDGET_H

// clazy:excludeall=qstring-arg
#include <KRichTextWidget>

class QMenu;

namespace KPlato
{

/**
 * Extends KRichTextWidget with capability to open links.
 * 
 * If SupportHyperlinks is enabled, an action with object name 'open_link'
 * is ceated that can be used in a tool bar, and an action is also
 * added to the context menu when the caret is on a link.
 */ 
class RichTextWidget : public KRichTextWidget
{
    Q_OBJECT
public:
    RichTextWidget(QWidget *parent);

    QList<QAction *> createActions() override;
    QMenu *mousePopupMenu() override;

private Q_SLOTS:
    void slotOpenLink();

private:
    QAction *openLink;
};

}  //KPlato namespace

#endif
