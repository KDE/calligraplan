/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2005, 2007, 2011 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kptcontext.h"
#include "kptview.h"
#include <MimeTypes.h>
#include "kptdebug.h"

#include <QDomDocument>

namespace KPlato
{

Context::Context()
    : currentEstimateType(0),
      currentSchedule(0),
      m_contextLoaded(false),
      m_version(0)
{
    ganttview.ganttviewsize = -1;
    ganttview.taskviewsize = -1;

    accountsview.accountsviewsize = -1;
    accountsview.periodviewsize = -1;


}

Context::~Context() {
}

const KoXmlElement &Context::context() const
{
    return m_context;
}

int Context::version() const
{
    return m_version;
}
bool Context::setContent(const QString &str)
{
    KoXmlDocument doc;
    if (doc.setContent(str)) {
        return load(doc);
    }
    return false;
}

QDomDocument Context::document() const
{
    QDomDocument document(QStringLiteral("plan.context"));

    document.appendChild(document.createProcessingInstruction(
        QStringLiteral("xml"),
        QStringLiteral("version=\"1.0\" encoding=\"UTF-8\"")));

    KoXml::asQDomElement(document, m_document.documentElement());
    return document;
}

bool Context::load(const KoXmlDocument &document) {
    m_document = document; // create a copy, document is deleted under our feet

    // Check if this is the right app
    KoXmlElement elm = m_document.documentElement();
    QString value = elm.attribute("mime", QString());
    if (value.isEmpty()) {
        errorPlan << "No mime type specified!";
//        setErrorMessage(i18n("Invalid document. No mimetype specified."));
        return false;
    } else if (value != PLAN_MIME_TYPE) {
        if (value == KPLATO_MIME_TYPE) {
            // accept, since we forgot to change kplato to plan for so long...
        } else {
            errorPlan << "Unknown mime type " << value;
//        setErrorMessage(i18n("Invalid document. Expected mimetype %2, got %1", value, KPLATO_MIME_TYPE));
            return false;
        }
    }
    m_version = elm.attribute("version", QString::number(0)).toInt();

/*
#ifdef KOXML_USE_QDOM
    int numNodes = elm.childNodes().count();
#else
    int numNodes = elm.childNodesCount();
#endif
*/
    KoXmlNode n = elm.firstChild();
    for (; ! n.isNull(); n = n.nextSibling()) {
        if (! n.isElement()) {
            continue;
        }
        KoXmlElement element = n.toElement();
        if (element.tagName() == QStringLiteral("context")) {
            m_context = element;
            m_contextLoaded = true;
        }
    }
    return true;
}

QDomDocument Context::save(const View *view) const {
    QDomDocument document(QStringLiteral("plan.context"));

    document.appendChild(document.createProcessingInstruction(
                              QStringLiteral("xml"),
                              QStringLiteral("version=\"1.0\" encoding=\"UTF-8\"")));

    QDomElement doc = document.createElement(QStringLiteral("context"));
    doc.setAttribute(QStringLiteral("editor"), QStringLiteral("Plan"));
    doc.setAttribute(QStringLiteral("mime"), QStringLiteral("application/x-vnd.kde.plan"));
    doc.setAttribute(QStringLiteral("version"), QString::number(PLAN_CONTEXT_VERSION));
    document.appendChild(doc);

    QDomElement e = doc.ownerDocument().createElement(QStringLiteral("context"));
    doc.appendChild(e);
    view->saveContext(e);

    return document;
}

}  //KPlato namespace
