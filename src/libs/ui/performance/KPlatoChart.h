/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2007-2010 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KPLATOCHART_H
#define KPLATOCHART_H

#include <KChartChart>


namespace KPlato
{

/*
 * class KPlato::Chart
 * 
 * Wrapper for KChart::Chart to allow rubberband selection
 * with no keyboard modifiers when cursor is inside a coordinate plan.
 * 
 * When not inside a plane drag&drop and context menu can be activated.
 */
class Chart : public KChart::Chart
{
    Q_OBJECT
public:
    Chart(QWidget *parent);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    bool m_inplane;
};

}

#endif
