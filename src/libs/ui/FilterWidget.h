/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 2022 Dag Andersen <dag.andersen@kdemail.net>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef FILTERWIDGET_H
#define FILTERWIDGET_H

#include <QWidget>

class QLineEdit;
class QToolButton;

namespace KPlato
{

class FilterWidget : public QWidget
{
    Q_OBJECT
public:
    explicit FilterWidget(QWidget *parent = nullptr);
    explicit FilterWidget(bool enableExtendedOptions, QWidget *parent = nullptr);

    QLineEdit *lineedit = nullptr;
    QToolButton *extendedOptions = nullptr;

private:
    void init(bool enableExtendedOptions);
};

}
#endif // FILTERWIDGET_H
