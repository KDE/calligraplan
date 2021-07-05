/*This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <calligra-devel@kde.org
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
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
