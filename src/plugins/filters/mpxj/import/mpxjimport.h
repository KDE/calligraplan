/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef MPXJIMPORT_H
#define MPXJIMPORT_H


#include <KoFilter.h>


#include <QObject>
#include <QVariantList>

class QFile;
class QByteArray;
class QStringList;


class MpxjImport : public KoFilter
{
    Q_OBJECT
public:
    MpxjImport(QObject* parent, const QVariantList &);
    virtual ~MpxjImport() {}

    virtual KoFilter::ConversionStatus convert(const QByteArray& from, const QByteArray& to);

    static QStringList mimeTypes();

protected:
    KoFilter::ConversionStatus doImport( const QByteArray inFile, const QByteArray outFile );
};

#endif // MPXJIMPORT_H
