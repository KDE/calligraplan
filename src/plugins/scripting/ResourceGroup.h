/* This file is part of the Calligra project
 * SPDX-FileCopyrightText: 2008 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef SCRIPTING_RESOURCEGROUP_H
#define SCRIPTING_RESOURCEGROUP_H

#include <QObject>
#include <QVariant>


namespace KPlato {
    class ResourceGroup;
}

namespace Scripting {
    class Project;
    class ResourceGroup;
    
    /**
    * The ResourceGroup class represents a resource group in a project.
    */
    class ResourceGroup : public QObject
    {
            Q_OBJECT
        public:
            /// Create a group
            ResourceGroup(Project *project, KPlato::ResourceGroup *group, QObject *parent);
            /// Destructor
            virtual ~ResourceGroup() {}
            
            KPlato::ResourceGroup *kplatoResourceGroup() const { return m_group; }
        
        public Q_SLOTS:
            /// Return the project this resource group is part of
            QObject* project();
            /// Return the identity of resource group
            QString id();
            /// Return type of resource group
            QString type();
            /// Number of resources in this group
            int resourceCount() const;
            /// Return resource at @p index
            QObject *resourceAt(int index) const;
            
            /// Number of resources in this group
            int childCount() const;
            /// Return resource at @p index
            QObject *childAt(int index) const;

        private:
            Project *m_project;
            KPlato::ResourceGroup *m_group;
    };

}

#endif
