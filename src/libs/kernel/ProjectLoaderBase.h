/* This file is part of the KDE project
 SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>

 SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef PROJECTLOADERBASE_H
#define PROJECTLOADERBASE_H

#include "plankernel_export.h"

#include <KoXmlReader.h>

#include <QObject>


namespace KPlato
{
class XMLLoaderObject;

class PLANKERNEL_EXPORT ProjectLoaderBase : public QObject
{
public:
    ProjectLoaderBase() {};

    virtual bool load(XMLLoaderObject &context, const KoXmlDocument &document = KoXmlDocument()) {
        Q_UNUSED(context)
        Q_UNUSED(document)
        return false;
    }
};

} // namespace KPlato

#endif
