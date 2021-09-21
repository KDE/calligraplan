/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PLANPORTFOLIO_CHARTWRAPPER_H
#define PLANPORTFOLIO_CHARTWRAPPER_H


#include <QWidget>

class ChartWrapper : public QWidget
{
    Q_OBJECT
public:
    explicit ChartWrapper(QWidget *parent = nullptr) : QWidget(parent) {}

Q_SIGNALS:
    void sizeChanged();

protected:
    void resizeEvent(QResizeEvent *event) override;
};

#endif
