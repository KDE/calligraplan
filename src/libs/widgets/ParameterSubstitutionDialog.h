/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PARAMETERSUBSTITUTIONPANEL_H
#define PARAMETERSUBSTITUTIONPANEL_H

#include <kowidgets_export.h>

#include "ui_ParameterSubstitutionPanel.h"

#include <KoDialog.h>

#include <QWidget>

namespace KPlato
{

class ParameterSubstitutionPanel : public QWidget
{
    Q_OBJECT
public:
    explicit ParameterSubstitutionPanel(const QStringList &parameters, QWidget *parent = nullptr);

    Ui::ParameterSubstitutionPanel ui;
};

class KOWIDGETS_EXPORT ParameterSubstitutionDialog : public KoDialog
{
    Q_OBJECT
public:
    explicit ParameterSubstitutionDialog(const QStringList &parameters, QWidget *parent = nullptr);
    QMap<QString, QString> parameters() const;

private:
    ParameterSubstitutionPanel *m_panel;
};

} //KPlato namespace

#endif
