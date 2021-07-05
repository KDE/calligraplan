/* This file is part of the KDE project
   SPDX-FileCopyrightText: 1998, 1999 Torben Weis <weis@kde.org>
   SPDX-FileCopyrightText: 2000-2005 David Faure <faure@kde.org>
   SPDX-FileCopyrightText: 2007 Thorsten Zachmann <zachmann@kde.org>
   SPDX-FileCopyrightText: 2009 Boudewijn Rempt <boud@valdyas.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/


// clazy:excludeall=qstring-arg
#include "KoDocumentBase.h"

#include <QtGlobal>

class Q_DECL_HIDDEN KoDocumentBase::Private {
public:
};

KoDocumentBase::KoDocumentBase()
    : d(new Private)
{
}


KoDocumentBase::~KoDocumentBase()
{
    delete d;
}
