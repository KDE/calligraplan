/* This file is part of the Calligra project
 * SPDX-FileCopyrightText: 2008, 2011 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef SCRIPTING_RESOURCE_H
#define SCRIPTING_RESOURCE_H

#include <QObject>
#include <QVariant>


namespace KPlato {
    class Resource;
}

namespace Scripting {
    class Project;
    class Resource;

    /**
    * The Resource class represents a resource in a project.
    */
    class Resource : public QObject
    {
            Q_OBJECT
        public:
            /// Create a resource
            Resource(Project *project, KPlato::Resource *resource, QObject *parent);
            /// Destructor
            virtual ~Resource() {}

            KPlato::Resource *kplatoResource() const { return m_resource; }

        public Q_SLOTS:
            /// Return the project this resource is part of
            QObject* project();
            /// Return type of resource
            QVariant type();
            /// Return type of resource
            QString id() const;

            /// Add external appointments
            void addExternalAppointment(const KPlato::Resource *resource, const QString &/*start*/, const QString &/*end*/, int /*load */) {Q_UNUSED(resource)}

            /**
             * Return all internal appointments the resource has
             */
            QVariantList appointmentIntervals(qlonglong schedule) const;
            /**
             * Return all external appointments the resource has
             */
            QVariantList externalAppointments() const;
            /// Add an external appointment
            void addExternalAppointment(const QVariant &id, const QString &name, const QVariantList &lst);
            /// Clear appointments with identity @p id
            void clearExternalAppointments(const QString &id);

            /// Number of child resources. Only team resources has children (team members)
            int childCount() const;
            /// Return resource at @p index.  Only team resources has children (team members)
            QObject *childAt(int index) const;
            /// Set list of children. The list of existing children is cleared
            void setChildren(const QList<QObject*> &members);

        private:
            Project *m_project;
            KPlato::Resource *m_resource;
    };

}

#endif
