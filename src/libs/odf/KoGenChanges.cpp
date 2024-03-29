/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2008 Pierre Stirnweiss <pierre.stirnweiss_calligra@gadz.org>
   SPDX-FileCopyrightText: 2010 Thomas Zander <zander@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "KoGenChanges.h"
#include <KoXmlWriter.h>
#include <KoElementReference.h>

#include <QList>
#include <QMap>
#include <QString>

#include <OdfDebug.h>

class Q_DECL_HIDDEN KoGenChanges::Private
{
public:
    Private(KoGenChanges *q)
       : q(q)
    { }


    struct NamedChange {
        const KoGenChange* change; ///< @note owned by the collection
        QString name;
    };

    /// style definition -> name
    QMap<KoGenChange, QString>  changeMap;

    /// List of styles (used to preserve ordering)
    QList<NamedChange> changeArray;

    QMap<KoGenChange, QString> ::iterator insertChange(const KoGenChange &change);

    KoGenChanges *q;
};

KoGenChanges::KoGenChanges()
    : d(new Private(this))
{
}

KoGenChanges::~KoGenChanges()
{
    delete d;
}

QString KoGenChanges::insert(const KoGenChange& change)
{
    QMap<KoGenChange, QString> ::iterator it = d->changeMap.find(change);
    if (it == d->changeMap.end()) {
        it = d->insertChange(change);
    }
    return it.value();
}

QMap<KoGenChange, QString>::iterator KoGenChanges::Private::insertChange(const KoGenChange &change)
{
    QString changeName;
    switch (change.type()) {
    case KoGenChange::InsertChange: changeName = QLatin1Char('I'); break;
    case KoGenChange::FormatChange: changeName = QLatin1Char('F'); break;
    case KoGenChange::DeleteChange: changeName = QLatin1Char('D'); break;
    default:
        changeName = QLatin1Char('C');
    }
    KoElementReference ref(changeName);
    changeName = ref.toString();

    QMap<KoGenChange, QString>::iterator it = changeMap.insert(change, changeName);
    NamedChange s;
    s.change = &it.key();
    s.name = changeName;
    changeArray.append(s);

    return it;
}

void KoGenChanges::saveOdfChanges(KoXmlWriter* xmlWriter, bool trackChanges) const
{
    QMap<KoGenChange, QString>::const_iterator it = d->changeMap.constBegin();

    if ((it != d->changeMap.constEnd()) && (it.key().changeFormat() == KoGenChange::DELTAXML)) {
        xmlWriter->startElement("delta:tracked-changes");
    } else {
        xmlWriter->startElement("text:tracked-changes");
        xmlWriter->addAttribute("text:track-changes", trackChanges);
   }

    for (; it != d->changeMap.constEnd() ; ++it) {
        KoGenChange change = it.key();
        change.writeChange(xmlWriter, it.value());
    }

    xmlWriter->endElement(); // text:tracked-changes
}
