/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2017 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "ConfigWorkVacationPanel.h"

#include "calligraplansettings.h"


#include <kactioncollection.h>

#ifdef HAVE_KHOLIDAYS
#include <KHolidays/HolidayRegion>
#endif

#include <QFileDialog>

namespace KPlato
{

ConfigWorkVacationPanel::ConfigWorkVacationPanel(QWidget *parent)
    : ConfigWorkVacationPanelImpl(parent)
{
}

//-----------------------------
ConfigWorkVacationPanelImpl::ConfigWorkVacationPanelImpl(QWidget *p)
    : QWidget(p)
{
    setupUi(this);
    kcfg_Region->hide();
#ifdef HAVE_KHOLIDAYS
    int idx = 0;
    const QString regionCode = kcfg_Region->text();

    region->addItem(i18n("Default"), "Default");
    const QList<QString> codes = KHolidays::HolidayRegion::regionCodes(); 
    for(const QString &s : codes) {
        region->addItem(KHolidays::HolidayRegion::name(s), s);
        int row = region->count() - 1;
        region->setItemData(row, KHolidays::HolidayRegion::description(s), Qt::ToolTipRole);
        if (s == regionCode) {
            idx = row;
        }
    }
    connect(region, SIGNAL(currentIndexChanged(int)), this, SLOT(slotRegionChanged(int)));
    connect(kcfg_Region, &QLineEdit::textChanged, this, &ConfigWorkVacationPanelImpl::slotRegionCodeChanged);
    region->setCurrentIndex(idx);
#else
    holidaysWidget->hide();
#endif
}

#ifdef HAVE_KHOLIDAYS
void ConfigWorkVacationPanelImpl::slotRegionChanged(int idx)
{
    QString r = region->itemData(idx).toString();
    if (r != kcfg_Region->text()) {
        kcfg_Region->setText(r);
    }
}

void ConfigWorkVacationPanelImpl::slotRegionCodeChanged(const QString &code)
{
    QString r = region->itemData(region->currentIndex()).toString();
    if (r != code) {
        for (int idx = 0; idx < region->count(); ++idx) {
            if (region->itemData(idx).toString() == code) {
                region->setCurrentIndex(idx);
                break;
            }
        }
    }
}
#endif

}  //KPlato namespace
