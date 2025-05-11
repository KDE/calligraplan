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
#include <QProcess>

class QFile;
class QByteArray;


class MpxjImport : public KoFilter
{
    Q_OBJECT
public:
    MpxjImport(QObject* parent, const QVariantList &);
    virtual ~MpxjImport() {}

    KoFilter::ConversionStatus convert(const QByteArray& from, const QByteArray& to) override;

    static QStringList mimeTypes();

protected:
    KoFilter::ConversionStatus doImport( const QString &inFile, const QString &outFile );
    void run(const QStringList &args);

    static const QHash<QString, QString> fileTypeMap();

private Q_SLOTS:
    void slotFinished(int exitCode, QProcess::ExitStatus exitStatus = QProcess::NormalExit);
    void slotError(QProcess::ProcessError error);

private:
    KoFilter::ConversionStatus m_status;
    int m_passwordTries;
};

#endif // MPXJIMPORT_H
