/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <plan_export.h>


#include <KConfigDialog>


class KConfigSkeleton;

namespace KPlato
{


class PLAN_EXPORT ConfigDialog : public KConfigDialog
{
    Q_OBJECT
public:
    ConfigDialog(QWidget *parent, const QString &name, KConfigSkeleton *config);
    
Q_SIGNALS:
    void updateWidgetsData();
    void updateWidgetsSettings();
    void settingsUpdated();

protected Q_SLOTS:
    void updateSettings() override;
    void updateWidgets() override;
    void showHelp() override;
    
protected:
    bool hasChanged() override;
    
private:
    QList<KPageWidgetItem*> m_pages;
};

} //KPlato namespace

#endif
