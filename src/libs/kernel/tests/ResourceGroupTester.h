/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2020 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KPlato_ResourceGroupTester_h
#define KPlato_ResourceGroupTester_h

#include <QObject>

namespace KPlato
{
class ResourceGroup;
class Project;

class ResourceGroupTester : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void topLevelGroups();
    void childGroups();
    void resources();

private:
    ResourceGroup *createResourceGroup(Project &project, const QString name, ResourceGroup *parent = nullptr);
};

} //namespace KPlato

#endif
