/* This file is part of the Calligra project
 * SPDX-FileCopyrightText: 2008 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef SCRIPTING_ACCOUNT_H
#define SCRIPTING_ACCOUNT_H

#include <QObject>
#include <QVariant>


namespace KPlato {
    class Account;
}

namespace Scripting {
    class Project;
    class Account;

    /**
    * The Account class represents an account in a project.
    */
    class Account : public QObject
    {
            Q_OBJECT
        public:
            /// Create a account
            Account(Project *project, KPlato::Account *account, QObject *parent);
            /// Destructor
            virtual ~Account() {}
        
            KPlato::Account *kplatoAccount() const { return static_cast<KPlato::Account*>(m_account); }
            
        public Q_SLOTS:
            /// Return the project this account is part of
            QObject* project();
            /// Return the accounts name
            QString name() const;
            
            /// Return the number of child accounts
            int childCount() const;
            /// Return the child account at @p index
            QObject *childAt(int index);

            /// Return a map of planned effort and cost pr day for interval @p start to @p end
            QVariant plannedEffortCostPrDay(const QVariant &start, const QVariant &end, const QVariant &schedule);
            /// Return a map of actual effort and cost pr day for interval @p start to @p end
            QVariant actualEffortCostPrDay(const QVariant &start, const QVariant &end, const QVariant &schedule);

            /// Return a map of planned effort and cost pr day
            QVariant plannedEffortCostPrDay(const QVariant &schedule);
            /// Return a map of actual effort and cost pr day
            QVariant actualEffortCostPrDay(const QVariant &schedule);

        private:
            Project *m_project;
            KPlato::Account *m_account;
    };

}

#endif
