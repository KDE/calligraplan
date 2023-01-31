/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PLANPORTFOLIO_PART_H
#define PLANPORTFOLIO_PART_H

#include "planportfolio_export.h"

#include <KoPart.h>

#include <QPointer>

class KoView;
class KoMainWindow;

class QStackedWidget;


class MainDocument;

class PLANPORTFOLIO_EXPORT Part : public KoPart
{
    Q_OBJECT

public:
    explicit Part(QObject *parent);

    ~Part() override;

    KoDocument *createDocument(KoPart *part) const override;

    KoView *createViewInstance(KoDocument *document, QWidget *parent) override;

    KoMainWindow *createMainWindow() override;

public Q_SLOTS:
    void configure(KoMainWindow *mw) override;
    void slotSettingsUpdated();
};

#endif
