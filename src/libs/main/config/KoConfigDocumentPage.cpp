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

#include <klocalizedstring.h>
#include <kconfig.h>
#include <kconfiggroup.h>

#include <QFormLayout>
#include <QCheckBox>
#include <QGroupBox>
#include <QSpinBox>
#include <QtGlobal>

class Q_DECL_HIDDEN KoConfigDocumentPage::Private
{
public:
    Private(KoDocument* doc)
    : doc(doc)
    {}

    Ui::KoConfigDocumentPage ui;

    KoDocument* doc;
    KSharedConfigPtr config;

    int oldAutoSave;
    bool oldBackupFile;
};

KoConfigDocumentPage::KoConfigDocumentPage(KoDocument* doc)
    : QWidget()
    , d(new Private(doc))
{
    d->ui.setupUi(this);

    d->config = d->doc->documentPart()->componentData().config();

    d->oldAutoSave = doc->defaultAutoSave() / 60;
    d->oldBackupFile = true;

    if(d->config->hasGroup("Interface")) {
        KConfigGroup interfaceGroup = d->config->group("Interface");
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
