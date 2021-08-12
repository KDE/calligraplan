/* This file is part of the KDE project
 SPDX-FileCopyrightText: 2010 Dag Andersen <dag.andersen@kdemail.net>

 SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPLATOXMLLOADER_H
#define KPLATOXMLLOADER_H

#include "KPlatoXmlLoaderBase.h"

#include "KoXmlReaderForward.h"

#include <QString>

namespace KPlato
{
    class Package;
    class XMLLoaderObject;
    class Project;

class KPlatoXmlLoader : public KPlatoXmlLoaderBase
{
    Q_OBJECT
public:
    KPlatoXmlLoader(XMLLoaderObject &loader, Project *project);

    QString errorMessage() const;
    Package *package() const;
    QString timeTag() const;

    using KPlatoXmlLoaderBase::load;
    bool load(const KoXmlElement& plan);
    bool loadWorkpackage(const KoXmlElement &plan);

private:
    XMLLoaderObject &m_loader;
    Project *m_project;
    QString m_message;
    Package *m_package;
    QString m_timeTag;
};

}

#endif // KPLATOXMLLOADER_H
