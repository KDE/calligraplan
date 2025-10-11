/* This file is part of the KDE project
SPDX-FileCopyrightText: 2002, 2003 Laurent Montel <lmontel@mandrakesoft.com>
SPDX-FileCopyrightText: 2006-2007 Jan Hambrecht <jaham@gmx.net>

SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "KoConfigDocumentPage.h"

#include "ui_KoConfigDocumentPage.h"

#include <KoDocument.h>
#include <KoPart.h>
#include <KoComponentData.h>

#include <KLocalizedString>
#include <KConfig>
#include <KConfigGroup>

#include <QFormLayout>
#include <QCheckBox>
#include <QGroupBox>
#include <QSpinBox>
#include <QtGlobal>

class Q_DECL_HIDDEN KoConfigDocumentPage::Private
{
public:
    Private(KoPart* part)
    : part(part)
    {}

    Ui::KoConfigDocumentPage ui;

    KoPart *part;
    KSharedConfigPtr config;

    int oldAutoSave;
    bool oldBackupFile;
};

KoConfigDocumentPage::KoConfigDocumentPage(KoPart *part)
    : QWidget()
    , d(new Private(part))
{
    d->ui.setupUi(this);

    d->config = part->componentData().config();

    d->oldAutoSave = part->document() ? part->document()->defaultAutoSave() / 60 : 5;
    d->oldBackupFile = true;

    if(d->config->hasGroup(QStringLiteral("Interface"))) {
        KConfigGroup interfaceGroup = d->config->group(QStringLiteral("Interface"));
        d->oldAutoSave = interfaceGroup.readEntry("AutoSave", d->oldAutoSave);
        d->oldBackupFile = interfaceGroup.readEntry("BackupFile", d->oldBackupFile);
    }

    d->ui.kcfg_AutoSave->setSingleStep(1);
    d->ui.kcfg_AutoSave->setSpecialValueText(i18n("No autosave"));
    d->ui.kcfg_AutoSave->setSuffix(i18nc("unit symbol for minutes, leading space as separator", " min"));
    d->ui.kcfg_AutoSave->setValue(d->oldAutoSave);

    d->ui.kcfg_CreateBackupFile->setChecked(d->oldBackupFile);
}

KoConfigDocumentPage::~KoConfigDocumentPage()
{
    delete d;
}
