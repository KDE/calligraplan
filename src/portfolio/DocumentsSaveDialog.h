/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef PORTFOLIO_DOCUMENTSSAVEDIALOG_H
#define PORTFOLIO_DOCUMENTSSAVEDIALOG_H

#include <KoDialog.h>

#include <ui_DocumentsSaveDialog.h>

#include <QStandardItemModel>

class MainDocument;
class KoDocument;

class DocumentsSaveModel : public QStandardItemModel
{
public:
    DocumentsSaveModel(MainDocument *doc, const QList<KoDocument*> children, QObject *parent = nullptr);

    MainDocument *m_doc;
};

class DocumentsSaveDialog : public KoDialog
{
    Q_OBJECT
public:

    explicit DocumentsSaveDialog(MainDocument *doc, const QList<KoDocument*> children, QWidget *parent = nullptr);

    bool saveMain() const;
    QUrl mainUrl() const;

    QList<KoDocument*> documentsToSave() const;

private Q_SLOTS:
    void dialogAccepted();

private:
    Ui::DocumentsSaveDialog ui;
    MainDocument *m_doc;
};

#endif
