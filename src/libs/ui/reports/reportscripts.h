/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2010 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPLATO_REPORTSCRIPTS_H
#define KPLATO_REPORTSCRIPTS_H

#include <QObject>
#include <QString>

class QVariant;

namespace KPlato
{

class ReportData;

class ProjectAccess : public QObject
{
    Q_OBJECT
public:
    explicit ProjectAccess(ReportData *rd);

public Q_SLOTS:
    QVariant Name() const;
    QVariant Manager() const;
    QVariant Plan() const;
    QVariant BCWS() const;
    QVariant BCWP() const;
    QVariant ACWP() const;
    QVariant CPI() const;
    QVariant SPI() const;

private:
    ReportData *m_reportdata;
};


} // namespace KPlato

#endif
