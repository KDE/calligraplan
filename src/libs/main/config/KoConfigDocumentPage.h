/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2002 Laurent Montel <lmontel@mandrakesoft.com>
   SPDX-FileCopyrightText: 2006-2007 Jan Hambrecht <jaham@gmx.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KOCONFIGDOCUMENTPAGE_H
#define KOCONFIGDOCUMENTPAGE_H

#include <QWidget>

#include "komain_export.h"

class KoPart;

class KOMAIN_EXPORT KoConfigDocumentPage : public QWidget
{
    Q_OBJECT

public:
    explicit KoConfigDocumentPage(KoPart *part);
    ~KoConfigDocumentPage() override;

private:
    class Private;
    Private * const d;
};

#endif // KOCONFIGDOCUMENTPAGE_H
