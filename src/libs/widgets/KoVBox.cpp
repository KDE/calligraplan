/* This file is part of the KDE libraries
   SPDX-FileCopyrightText: 2005 David Faure <faure@kde.org>

   SPDX-License-Identifier: LGPL-2.0-only
*/

// clazy:excludeall=qstring-arg
#include "KoVBox.h"

#include <QEvent>
#include <QApplication>
#include <QVBoxLayout>

KoVBox::KoVBox(QWidget *parent)
    : QFrame(parent),
      d(nullptr)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);

    setLayout(layout);
}

KoVBox::~KoVBox()
{
}

void KoVBox::childEvent(QChildEvent *event)
{
    switch (event->type()) {
    case QEvent::ChildAdded: {
        QChildEvent *childEvent = static_cast<QChildEvent *>(event);
        if (childEvent->child()->isWidgetType()) {
            QWidget *widget = static_cast<QWidget *>(childEvent->child());
            static_cast<QBoxLayout *>(layout())->addWidget(widget);
        }

        break;
    }
    case QEvent::ChildRemoved: {
        QChildEvent *childEvent = static_cast<QChildEvent *>(event);
        if (childEvent->child()->isWidgetType()) {
            QWidget *widget = static_cast<QWidget *>(childEvent->child());
            static_cast<QBoxLayout *>(layout())->removeWidget(widget);
        }

        break;
    }
    default:
        break;
    }
    QFrame::childEvent(event);
}

QSize KoVBox::sizeHint() const
{
    KoVBox *that = const_cast<KoVBox *>(this);
    QApplication::sendPostedEvents(that, QEvent::ChildAdded);

    return QFrame::sizeHint();
}

QSize KoVBox::minimumSizeHint() const
{
    KoVBox *that = const_cast<KoVBox *>(this);
    QApplication::sendPostedEvents(that, QEvent::ChildAdded);

    return QFrame::minimumSizeHint();
}

void KoVBox::setSpacing(int spacing)
{
    layout()->setSpacing(spacing);
}

void KoVBox::setStretchFactor(QWidget *widget, int stretch)
{
    static_cast<QBoxLayout *>(layout())->setStretchFactor(widget, stretch);
}

void KoVBox::setMargin(int margin)
{
    layout()->setContentsMargins(margin, margin, margin, margin);
}

