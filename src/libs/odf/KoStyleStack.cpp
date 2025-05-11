/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2003 Lukas Tinkl <lukas@kde.org>
   SPDX-FileCopyrightText: 2003 David Faure <faure@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "KoStyleStack.h"
#include "KoUnit.h"
#include "KoXmlNS.h"

#include <OdfDebug.h>

//#define DEBUG_STYLESTACK

class KoStyleStack::KoStyleStackPrivate
{
};

KoStyleStack::KoStyleStack()
        : m_styleNSURI(KoXmlNS::style), m_foNSURI(KoXmlNS::fo), d(nullptr)
{
    clear();
}

KoStyleStack::KoStyleStack(const char* styleNSURI, const char* foNSURI)
        : m_styleNSURI(QLatin1String(styleNSURI)), m_foNSURI(QLatin1String(foNSURI)), d(nullptr)
{
    m_propertiesTagNames.append(QStringLiteral("properties"));
    clear();
}

KoStyleStack::~KoStyleStack()
{
    delete d;
}

void KoStyleStack::clear()
{
    m_stack.clear();
#ifdef DEBUG_STYLESTACK
    debugOdf << "clear!";
#endif
}

void KoStyleStack::save()
{
    m_marks.push(m_stack.count());
#ifdef DEBUG_STYLESTACK
    debugOdf << "save (level" << m_marks.count() << ") -> index" << m_stack.count();
#endif
}

void KoStyleStack::restore()
{
    Q_ASSERT(!m_marks.isEmpty());
    int toIndex = m_marks.pop();
#ifdef DEBUG_STYLESTACK
    debugOdf << "restore (level" << m_marks.count() + 1 << ") -> to index" << toIndex;
#endif
    Q_ASSERT(toIndex > -1);
    Q_ASSERT(toIndex <= (int)m_stack.count());   // If equal, nothing to remove. If greater, bug.
    for (int index = (int)m_stack.count() - 1; index >= toIndex; --index)
        m_stack.pop_back();
}

void KoStyleStack::pop()
{
    Q_ASSERT(!m_stack.isEmpty());
    m_stack.pop_back();
#ifdef DEBUG_STYLESTACK
    debugOdf << "pop -> count=" << m_stack.count();
#endif
}

void KoStyleStack::push(const KoXmlElement& style)
{
    m_stack.append(style);
#ifdef DEBUG_STYLESTACK
    debugOdf << "pushed" << style.attributeNS(m_styleNSURI, "name", QString()) << " -> count=" << m_stack.count();
#endif
}

QString KoStyleStack::property(const QString &nsURI, const QString &name) const
{
    return property(nsURI, name, nullptr);
}
QString KoStyleStack::property(const QString &nsURI, const char *name) const
{
    return property(nsURI, QLatin1String(name), nullptr);
}
QString KoStyleStack::property(const QString &nsURI, const QString &name, const QString &detail) const
{
    return property(nsURI, name, &detail);
}
QString KoStyleStack::property(const QString &nsURI, const char *name, const QString &detail) const
{
    return property(nsURI, QLatin1String(name), &detail);
}
QString KoStyleStack::property(const QString &nsURI, const char *name, const char *detail) const
{
    return property(nsURI, QLatin1String(name), QLatin1String(detail));
}

inline QString KoStyleStack::property(const QString &nsURI, const QString &name, const QString *detail) const
{
    QString fullName(name);
    if (detail) {
        fullName += QLatin1Char('-') + *detail;
    }
    QList<KoXmlElement>::ConstIterator it = m_stack.end();
    while (it != m_stack.begin()) {
        --it;
        for (const QString &propertyTagName : qAsConst(m_propertiesTagNames)) {
            KoXmlElement properties = KoXml::namedItemNS(*it, m_styleNSURI, propertyTagName);
            if (detail) {
                QString attribute(properties.attributeNS(nsURI, fullName));
                if (!attribute.isEmpty()) {
                    return attribute;
                }
            }
            QString attribute(properties.attributeNS(nsURI, name));
            if (!attribute.isEmpty()) {
                return attribute;
            }
        }
    }
    return QString();
}

bool KoStyleStack::hasProperty(const QString &nsURI, const QString &name) const
{
    return hasProperty(nsURI, name, nullptr);
}
bool KoStyleStack::hasProperty(const QString &nsURI, const char *name) const
{
    return hasProperty(nsURI, QLatin1String(name), nullptr);
}

bool KoStyleStack::hasProperty(const QString &nsURI, const QString &name, const QString &detail) const
{
    return hasProperty(nsURI, name, &detail);
}
bool KoStyleStack::hasProperty(const QString &nsURI, const char *name, const QString &detail) const
{
    return hasProperty(nsURI, QLatin1String(name), &detail);
}

inline bool KoStyleStack::hasProperty(const QString &nsURI, const QString &name, const QString *detail) const
{
    QString fullName(name);
    if (detail) {
        fullName += QLatin1Char('-') + *detail;
    }
    QList<KoXmlElement>::ConstIterator it = m_stack.end();
    while (it != m_stack.begin()) {
        --it;
        for (const QString &propertiesTagName : qAsConst(m_propertiesTagNames)) {
            const KoXmlElement properties = KoXml::namedItemNS(*it, m_styleNSURI, propertiesTagName);
            if (properties.hasAttributeNS(nsURI, name) ||
                    (detail && properties.hasAttributeNS(nsURI, fullName)))
                return true;
        }
    }
    return false;
}

// Font size is a bit special. "115%" applies to "the fontsize of the parent style".
// This can be generalized though (hasPropertyThatCanBePercentOfParent() ? :)
QPair<qreal,qreal> KoStyleStack::fontSize(const qreal defaultFontPointSize) const
{
    const QString name = QStringLiteral("font-size");
    qreal percent = 100;
    QList<KoXmlElement>::ConstIterator it = m_stack.end(); // reverse iterator

    while (it != m_stack.begin()) {
        --it;
        for (const QString &propertiesTagName : qAsConst(m_propertiesTagNames)) {
            KoXmlElement properties = KoXml::namedItemNS(*it, m_styleNSURI, propertiesTagName).toElement();
            if (properties.hasAttributeNS(m_foNSURI, name)) {
                const QString value = properties.attributeNS(m_foNSURI, name, QString());
                if (value.endsWith(QLatin1Char('%'))) {
                    //sebsauer, 20070609, the specs don't say that we have to calc them together but
                    //just that we are looking for a valid parent fontsize. So, let's only take the
                    //first percent definition into account and keep on to seek for a valid parent,
                    //percent *= value.left(value.length() - 1).toDouble() / 100.0;
                    if (percent == 100)
                        percent = value.left(value.length() - 1).toDouble();
                } else {
                    // e.g. 12pt and indicate that there was not percentage there
                    return QPair<qreal,qreal> ((percent * KoUnit::parseValue(value))/100.0, 0.0);
                }
                break;
            }
        }
    }

    //if there was no valid parent, we return the default fontsize together with an optional calculated percent-value.
    return QPair<qreal,qreal> ((percent * defaultFontPointSize)/100.0, percent);
}

bool KoStyleStack::hasChildNode(const QString &nsURI, const QString &localName) const
{
    QList<KoXmlElement>::ConstIterator it = m_stack.end();
    while (it != m_stack.begin()) {
        --it;
        for (const QString &propertiesTagName : qAsConst(m_propertiesTagNames)) {
            KoXmlElement properties = KoXml::namedItemNS(*it, m_styleNSURI, propertiesTagName);
            if (!KoXml::namedItemNS(properties, nsURI, localName).isNull())
                return true;
        }
    }

    return false;
}

KoXmlElement KoStyleStack::childNode(const QString &nsURI, const QString &localName) const
{
    QList<KoXmlElement>::ConstIterator it = m_stack.end();

    while (it != m_stack.begin()) {
        --it;
        for (const QString &propertiesTagName : qAsConst(m_propertiesTagNames)) {
            KoXmlElement properties = KoXml::namedItemNS(*it, m_styleNSURI, propertiesTagName);
            KoXmlElement e = KoXml::namedItemNS(properties, nsURI, localName);
            if (!e.isNull())
                return e;
        }
    }

    return KoXmlElement();          // a null element
}

bool KoStyleStack::isUserStyle(const KoXmlElement& e, const QString& family) const
{
    if (e.attributeNS(m_styleNSURI, "family", QString()) != family)
        return false;
    const KoXmlElement parent = e.parentNode().toElement();
    //debugOdf <<"tagName=" << e.tagName() <<" parent-tagName=" << parent.tagName();
    return parent.localName() == QStringLiteral("styles") /*&& parent.namespaceURI() == KoXmlNS::office*/;
}

QString KoStyleStack::userStyleName(const QString& family) const
{
    QList<KoXmlElement>::ConstIterator it = m_stack.end();
    while (it != m_stack.begin()) {
        --it;
        //debugOdf << (*it).attributeNS(m_styleNSURI,"name", QString());
        if (isUserStyle(*it, family))
            return (*it).attributeNS(m_styleNSURI, "name", QString());
    }
    // Can this ever happen?
    return QStringLiteral("Standard");
}

QString KoStyleStack::userStyleDisplayName(const QString& family) const
{
    QList<KoXmlElement>::ConstIterator it = m_stack.end();
    while (it != m_stack.begin()) {
        --it;
        //debugOdf << (*it).attributeNS(m_styleNSURI,"display-name");
        if (isUserStyle(*it, family))
            return (*it).attributeNS(m_styleNSURI, "display-name", QString());
    }
    return QString(); // no display name, this can happen since it's optional
}

void KoStyleStack::setTypeProperties(const char* typeProperties)
{
    m_propertiesTagNames.clear();
    QString tp = QLatin1String(typeProperties);
    m_propertiesTagNames.append(tp.isEmpty() ? QStringLiteral("properties") : (tp + QStringLiteral("-properties")));
}

void KoStyleStack::setTypeProperties(const QList<QString> &typeProperties)
{
    m_propertiesTagNames.clear();
    for (const QString &typeProperty : typeProperties) {
        if (!typeProperty.isEmpty()) {
            m_propertiesTagNames.append(typeProperty + QStringLiteral("-properties"));
        }
    }
    if (m_propertiesTagNames.empty()) {
        m_propertiesTagNames.append(QStringLiteral("properties"));
    }
}
