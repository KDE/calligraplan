/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2009 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTMYCOMBOBOX_P_H
#define KPTMYCOMBOBOX_P_H

#include "plan_export.h"

#include <KComboBox>

namespace KPlato
{

class MyComboBox : public KComboBox
{
public:
    explicit MyComboBox(QWidget *parent = nullptr) : KComboBox(parent) {}

    void emitActivated(int i) { Q_EMIT activated(i); }

};


} //KPlato namespace

#endif // KPTMYCOMBOBOX_P_H
