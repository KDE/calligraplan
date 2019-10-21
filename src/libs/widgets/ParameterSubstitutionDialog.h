/* This file is part of the KDE project
 * Copyright (C) 2019 Dag Andersen <danders@get2net.dk>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 * 
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
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
