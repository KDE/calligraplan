/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2007, 2012 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kptdocuments.h"
#include "kptnode.h"
#include "kptxmlloaderobject.h"
#include "kptdebug.h"

#include <KoXmlReader.h>
#include <KoStore.h>

#include <KLocalizedString>

namespace KPlato
{
    
Document::Document()
    : m_type(Type_None),
    m_sendAs(SendAs_None),
    parent (nullptr)
{
    //debugPlan<<this;
}

Document::Document(const QUrl &url, Document::Type type, Document::SendAs sendAs)
    : m_type(type),
    m_sendAs(sendAs),
    parent (nullptr)
{
    setUrl(url);
    //debugPlan<<this;
}

Document::~Document()
{
    //debugPlan<<this;
}

bool Document::operator==(const Document &doc) const
{
    bool res = (m_url == doc.url() &&
                 m_name == doc.m_name &&
                 m_type == doc.type() && 
                 m_status == doc.status() &&
                 m_sendAs == doc.sendAs() 
               );
    return res;
}

bool Document::isValid() const
{
    return m_url.isValid();
}

QStringList Document::typeList(bool trans)
{
    return QStringList()
            << (trans ? xi18nc("@item", "Unknown") : QStringLiteral("Unknown"))
            << (trans ? xi18nc("@item The produced document", "Product") : QStringLiteral("Product"))
            << (trans ? xi18nc("@item Document is used for reference", "Reference") : QStringLiteral("Reference"));
}

QString Document::typeToString(Document::Type type, bool trans)
{
    return typeList(trans).at(type);
}

QStringList Document::sendAsList(bool trans)
{
    return QStringList()
            << (trans ? xi18nc("@item", "Unknown") : QStringLiteral("Unknown"))
            << (trans ? xi18nc("@item Send a copy of the document", "Copy") : QStringLiteral("Copy"))
            << (trans ? xi18nc("@item Send the reference (url) of the document", "Reference") : QStringLiteral("Reference"));
}

QString Document::sendAsToString(Document::SendAs snd, bool trans)
{
    return sendAsList(trans).at(snd);
}

void Document::setName(const QString &name)
{
    if (m_name != name) {
        m_name = name;
        if (parent) {
            parent->documentChanged(this);
        }
    }
}

void Document::setType(Type type)
{
    if (type != m_type) {
        m_type = type;
        if (parent) {
            parent->documentChanged(this);
        }
    }
}

void Document::setSendAs(SendAs snd)
{
    if (m_sendAs != snd) {
        m_sendAs = snd;
        if (parent) {
            parent->documentChanged(this);
        }
    }
}

void Document::setUrl(const QUrl &url)
{
    if (m_url != url) {
        m_url = url;
        if (m_name.isEmpty()) {
            m_name = url.fileName();
        }
        if (parent) {
            parent->documentChanged(this);
        }
    }
}

void Document::setStatus(const QString &sts)
{
    if (m_status != sts) {
        m_status = sts;
        if (parent) {
            parent->documentChanged(this);
        }
    }
}

bool Document::load(KoXmlElement &element, XMLLoaderObject &status)
{
    Q_UNUSED(status);
    m_url = QUrl(element.attribute(QStringLiteral("url")));
    m_name = element.attribute(QStringLiteral("name"), m_url.fileName());
    m_type = (Type)(element.attribute(QStringLiteral("type")).toInt());
    m_status = element.attribute(QStringLiteral("status"));
    m_sendAs = (SendAs)(element.attribute(QStringLiteral("sendas")).toInt());
    return true;
}

void Document::save(QDomElement &element) const
{
    element.setAttribute(QStringLiteral("url"), m_url.url());
    element.setAttribute(QStringLiteral("name"), m_name);
    element.setAttribute(QStringLiteral("type"), QString::number(m_type));
    element.setAttribute(QStringLiteral("status"), m_status);
    element.setAttribute(QStringLiteral("sendas"), QString::number(m_sendAs));
}

//----------------
Documents::Documents()
    : node(nullptr)
{
    //debugPlan<<this;
}

Documents::Documents(const Documents &docs)
    : node(nullptr)
{
    //debugPlan<<this;
    const auto documentList = docs.documents();
    for (const auto doc : documentList) {
        m_docs.append(new Document(*doc));
    }
}

Documents::~Documents()
{
    //debugPlan<<this;
    deleteAll();
}

bool Documents::operator==(const Documents &docs) const
{
    int cnt = m_docs.count();
    if (cnt != docs.count()) {
        return false;
    }
    for (int i = 0; i < cnt; ++i) {
        if (*(m_docs.at(i)) != *(docs.at(i))) {
            return false;
        }
    }
    return true;
}

void Documents::deleteAll()
{
    while (! m_docs.isEmpty()) {
        delete m_docs.takeFirst();
    }
}

void Documents::addDocument(Document *doc)
{
    Q_ASSERT(doc);
    m_docs.append(doc);
    doc->parent = this;
    if (node) {
        node->emitDocumentAdded(node, doc, m_docs.count() - 1);
    }
}

void Documents::addDocument(const QUrl &url, Document::Type type)
{
    addDocument(new Document(url, type));
}

Document *Documents::takeDocument(int index)
{
    if (index >= 0 && index < m_docs.count()) {
        Document *doc = m_docs.takeAt(index);
        if (doc) {
            doc->parent = nullptr;
            if (node) {
                node->emitDocumentRemoved(node, doc, index);
            }
        }
        return doc;
    }
    return nullptr;
}

Document *Documents::takeDocument(Document *doc)
{
    Q_ASSERT(m_docs.contains(doc));
    int idx = m_docs.indexOf(doc);
    if (idx >= 0) {
        takeDocument(idx);
        doc->parent = nullptr;
        if (node) {
            node->emitDocumentRemoved(node, doc, idx);
        }
        return doc;
    }
    return nullptr;
}

Document *Documents::findDocument(const Document *doc) const
{
    return findDocument(doc->url());
}

Document *Documents::findDocument(const QUrl &url) const
{
    for (int i = 0; i < m_docs.count(); ++i) {
        if (m_docs.at(i)->url() == url) {
            return m_docs.at(i);
        }
    }
    return nullptr;
}

bool Documents::load(KoXmlElement &element, XMLLoaderObject &status)
{
    debugPlan;
    KoXmlNode n = element.firstChild();
    for (; ! n.isNull(); n = n.nextSibling()) {
        if (! n.isElement()) {
            continue;
        }
        KoXmlElement e = n.toElement();
        if (e.tagName() == QStringLiteral("document")) {
            Document *doc = new Document();
            if (!doc->load(e, status)) {
                warnPlan<<"Failed to load document";
                status.addMsg(XMLLoaderObject::Errors, QStringLiteral("Failed to load document"));
                delete doc;
            } else {
                addDocument(doc);
                status.addMsg(i18n("Document loaded, URL=%1",  doc->url().url()));
            }
        }
    }
    return true;
}

void Documents::save(QDomElement &element) const
{
    if (m_docs.isEmpty()) {
        return;
    }
    QDomElement e = element.ownerDocument().createElement(QStringLiteral("documents"));
    element.appendChild(e);
    for (Document *d : std::as_const(m_docs)) {
        QDomElement me = element.ownerDocument().createElement(QStringLiteral("document"));
        e.appendChild(me);
        d->save(me);
    }
}

void Documents::saveToStore(KoStore *store) const
{
    for (Document *doc : std::as_const(m_docs)) {
        if (doc->sendAs() == Document::SendAs_Copy) {
            QString path = doc->url().url();
            if (doc->url().isLocalFile()) {
                path = doc->url().toLocalFile();
            }
            debugPlan<<"Copy file to store: "<<path<<doc->url().fileName();
            store->addLocalFile(path, doc->url().fileName());

        }
    }
}

void Documents::documentChanged(Document *doc)
{
    if (node) {
        node->emitDocumentChanged(node, doc, indexOf(doc));
    }
}

} //namespace KPlato
