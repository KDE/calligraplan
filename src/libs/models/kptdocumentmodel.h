/* This file is part of the KDE project
   Copyright (C) 2007 Dag Andersen <dag.andersen@kdemail.net>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#ifndef KPTDOCUMENTMODEL_H
#define KPTDOCUMENTMODEL_H

#include "kptitemmodelbase.h"

namespace KPlato
{

class Document;
class Documents;

class PLANMODELS_EXPORT DocumentModel : public QObject
{
    Q_OBJECT
public:
    enum Properties {
        Property_Url = 0,
        Property_Name,
        Property_Type,
        Property_SendAs,
        Property_Status
    };

    DocumentModel()
        : QObject()
    {}
    ~DocumentModel() override {}

    QVariant data(const Document *doc, int property, int role = Qt::DisplayRole) const; 
    static bool setData(Document *doc, int property, const QVariant & value, int role = Qt::EditRole);

    static QVariant headerData(int section, int role = Qt::DisplayRole);

    static int propertyCount();

    QVariant url(const Document *doc, int role) const;
    QVariant name(const Document *doc, int role) const;
    bool setName(Document *doc, const QVariant &value, int role);
    QVariant type(const Document *doc, int role) const;
    bool setType(Document *doc, const QVariant &value, int role);
    QVariant status(const Document *doc, int role) const;
    QVariant sendAs(const Document *doc, int role) const;
    bool setSendAs(Document *doc, const QVariant &value, int role);
};

class PLANMODELS_EXPORT DocumentItemModel : public ItemModelBase
{
    Q_OBJECT
public:
    explicit DocumentItemModel(QObject *parent = 0);
    ~DocumentItemModel() override;

    virtual void setDocuments(Documents *docs);
    Documents *documents() const;

    Qt::ItemFlags flags(const QModelIndex & index) const override;

    QModelIndex parent(const QModelIndex & index) const override;
    QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const override;
    virtual QModelIndex index(const Document *doc) const;

    int columnCount(const QModelIndex & parent = QModelIndex()) const override; 
    int rowCount(const QModelIndex & parent = QModelIndex()) const override; 

    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override; 
    bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole) override;


    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    QMimeData * mimeData(const QModelIndexList & indexes) const override;
    QStringList mimeTypes () const override;
    Qt::DropActions supportedDropActions() const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;

    Document *document(const QModelIndex &index) const;

    QAbstractItemDelegate *createDelegate(int column, QWidget *parent) const override;

    QModelIndex insertDocument(Document *doc, Document *after);

    bool dropAllowed(Document *on, const QMimeData *data);

    bool dropAllowed(const QModelIndex &index, int dropIndicatorPosition, const QMimeData *data) override;

protected Q_SLOTS:
    void slotDocumentChanged(KPlato::Document*);
    void slotDocumentToBeInserted(KPlato::Documents*, int row);
    void slotDocumentInserted(KPlato::Document*);
    void slotDocumentToBeRemoved(KPlato::Document*);
    void slotDocumentRemoved(KPlato::Document*);

protected:
    bool setUrl(Document *doc, const QVariant &value, int role);
    bool setName(Document *doc, const QVariant &value, int role);
    bool setType(Document *doc, const QVariant &value, int role);
    bool setSendAs(Document *doc, const QVariant &value, int role);

private:
    Documents *m_documents;
    DocumentModel m_model;
};


} //namespace KPlato

#endif
