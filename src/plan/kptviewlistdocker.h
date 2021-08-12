/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2007 Fredy Yanardi <fyanardi@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KPTVIEWLISTDOCKER_H
#define KPTVIEWLISTDOCKER_H

#include <QDockWidget>
#include <KoDockFactoryBase.h>

namespace KPlato
{

class View;
class ViewListWidget;

class ViewListDocker : public QDockWidget
{
    Q_OBJECT

public:
    explicit ViewListDocker(View *view);
    ~ViewListDocker() override;
    View *view();
    void setView(View *view);
    ViewListWidget *viewList() const { return m_viewlist; }

public Q_SLOTS:
    void slotModified();
    void updateWindowTitle(bool modified);

private:
    View *m_view;
    ViewListWidget *m_viewlist;
};

class ViewListDockerFactory : public KoDockFactoryBase
{
public:
    explicit ViewListDockerFactory(View *view);

    QString id() const override;
    QDockWidget* createDockWidget() override;
    /// @return the dock widget area the widget should appear in by default
    KoDockFactoryBase::DockPosition defaultDockPosition() const override { return DockLeft; }

private:
    View *m_view;
};

} //namespace KPlato

#endif
