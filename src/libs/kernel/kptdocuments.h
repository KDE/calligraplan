/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2007 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTDOCUMENTS_H
#define KPTDOCUMENTS_H

#include "plankernel_export.h"

#include <KoXmlReaderForward.h>

#include <QList>
#include <QUrl>

class KoStore;

class QDomElement;

namespace KPlato
{
    
class XMLLoaderObject;
class Node;
class Documents;

class PLANKERNEL_EXPORT Document
{
public:
    enum Type { Type_None, Type_Product, Type_Reference };
    enum SendAs { SendAs_None, SendAs_Copy, SendAs_Reference };
    
    Document();
    explicit Document(const QUrl &url, Type type = Type_Reference, SendAs sendAs = SendAs_Reference);
    ~Document();
    
    bool operator==(const Document &doc) const;
    bool operator!=(const Document &doc) const { return ! operator==(doc); }
    
    QString name() const { return m_name; }
    void setName(const QString &name);

    Type type() const { return m_type; }
    void  setType(Type type);
    static QStringList typeList(bool trans = false);
    static QString typeToString(Type type, bool trans = false);
    
    SendAs sendAs() const { return m_sendAs; }
    void setSendAs(SendAs snd);
    static QStringList sendAsList(bool trans = false);
    static QString sendAsToString(SendAs snd, bool trans = false);
    
    QUrl url() const { return m_url; }
    void setUrl(const QUrl &url);
    bool isValid() const;
    
    QString status() const { return m_status; }
    void setStatus(const QString &sts);
    
    bool load(KoXmlElement &element, XMLLoaderObject &status);
    void save(QDomElement &element) const;

private:
    Type m_type;
    QUrl m_url;
    QString m_status;
    SendAs m_sendAs;
    QString m_name;

    friend class Documents;
    Documents *parent;
};

class PLANKERNEL_EXPORT Documents
{
public:
    Documents();
    explicit Documents(const Documents &docs);
    ~Documents();
    
    bool operator==(const Documents &docs) const;
    bool operator!=(const Documents &docs) const { return ! operator==(docs); }
    
    void deleteAll();
    QList<Document*> documents() const { return m_docs; }
    void addDocument(Document *doc);
    void addDocument(const QUrl &url, Document::Type = Document::Type_None);
    Document *takeDocument(int index);
    Document *takeDocument(Document *doc);
    Document *findDocument(const Document *doc) const;
    Document *findDocument(const QUrl &url) const;
//    Document *document(int index) const;
    
    bool contains(const Document *doc) const { return m_docs.contains(const_cast<Document*>(doc) ); }
    int indexOf(const Document *doc) const { return m_docs.indexOf(const_cast<Document*>(doc) ); }
    int count() const { return m_docs.count(); }
    const Document *at(int index) const { return m_docs.at(index); }
    Document *value(int index) const { return m_docs.value(index); }
    
    bool load(KoXmlElement &element, XMLLoaderObject &status);
    void save(QDomElement &element) const;
    
    void saveToStore(KoStore *store) const;

    void documentChanged(Document *doc);

protected:
    QList<Document*> m_docs;

private:
    friend class Node;
    Node *node; // owner node
};

} //namespace KPlato

#endif
