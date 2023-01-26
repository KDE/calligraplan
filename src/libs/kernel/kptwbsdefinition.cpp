/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2005 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kptwbsdefinition.h"

#include "kptdebug.h"

#include <KoXmlReader.h>

#include <KLocalizedString>

#include <QStringList>

namespace KPlato
{


WBSDefinition::WBSDefinition() {
    m_levelsEnabled = false;

    m_defaultDef.code = QStringLiteral("Number");
    m_defaultDef.separator = QLatin1Char('.');

    m_codeLists.append(qMakePair(QStringLiteral("Number"), i18n("Number")));
    m_codeLists.append(qMakePair(QStringLiteral("Roman, upper case"), i18n("Roman, upper case")));
    m_codeLists.append(qMakePair(QStringLiteral("Roman, lower case"), i18n("Roman, lower case")));
    m_codeLists.append(qMakePair(QStringLiteral("Letter, upper case"), i18n("Letter, upper case")));
    m_codeLists.append(qMakePair(QStringLiteral("Letter, lower case"), i18n("Letter, lower case")));
}

WBSDefinition::WBSDefinition(const WBSDefinition &def) {
    (void)this->operator=(def);
}

WBSDefinition::~WBSDefinition() {
}

WBSDefinition &WBSDefinition::operator=(const WBSDefinition &def) {
    m_projectCode = def.m_projectCode;
    m_projectSeparator = def.m_projectSeparator;
    m_defaultDef.code = def.m_defaultDef.code;
    m_defaultDef.separator = def.m_defaultDef.separator;
    m_levelsEnabled = def.m_levelsEnabled;
    m_levelsDef = def.m_levelsDef;
    m_codeLists = def.m_codeLists;
    return *this;
}

void WBSDefinition::clear() {
    m_defaultDef.clear();
    m_levelsDef.clear();
}
    
QString WBSDefinition::wbs(uint index, int level) const {
    if (isLevelsDefEnabled()) {
        CodeDef def = levelsDef(level);
        if (!def.isEmpty()) {
            return code(def, index) + def.separator;
        }
    }
    return code(m_defaultDef, index) + m_defaultDef.separator;
}


QString WBSDefinition::code(uint index, int level) const {
    if (isLevelsDefEnabled()) {
        CodeDef def = levelsDef(level);
        if (!def.isEmpty()) {
            return code(def, index);
        }
    }
    return code(m_defaultDef, index);
}

QString WBSDefinition::separator(int level) const {
    if (isLevelsDefEnabled()) {
        CodeDef def = levelsDef(level);
        if (!def.isEmpty()) {
            return def.separator;
        }
    }
    return m_defaultDef.separator;
}

void WBSDefinition::setLevelsDef(const QMap<int, CodeDef> &def) {
    m_levelsDef.clear();
    m_levelsDef = def; 
}

WBSDefinition::CodeDef WBSDefinition::levelsDef(int level) const { 
    return m_levelsDef.contains(level) ? m_levelsDef[level] : CodeDef(); 
}
    
void WBSDefinition::setLevelsDef(int level, const CodeDef &def) {
    m_levelsDef.insert(level, def);
}

void WBSDefinition::setLevelsDef(int level, const QString& c, const QString& s) {
    m_levelsDef.insert(level, CodeDef(c, s));
}

bool WBSDefinition::level0Enabled() const {
    return m_levelsEnabled && !levelsDef(0).isEmpty();
}

const char Letters[] = { '?','a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z' };

QString WBSDefinition::code(const CodeDef &def, uint index) const {
    if (def.code == QStringLiteral("Number")) {
        return QStringLiteral("%1").arg(index);
    }
    if (def.code == QStringLiteral("Roman, lower case")) {
        return QStringLiteral("%1").arg(toRoman(index));
    }
    if (def.code == QStringLiteral("Roman, upper case")) {
        return QStringLiteral("%1").arg(toRoman(index, true));
    }
    if (def.code == QStringLiteral("Letter, lower case")) {
        if (index > 26) {
            index = 0;
        }
        return QStringLiteral("%1").arg(QLatin1Char(Letters[index]));
    }
    if (def.code == QStringLiteral("Letter, upper case")) {
        if (index > 26) {
            index = 0;
        }
        return QStringLiteral("%1").arg(QChar(QLatin1Char(Letters[index])).toUpper());
    }
    return QString();
}

// Nicked from koparagcounter.cc
QString WBSDefinition::toRoman(int n, bool upper) const
{
    static const QString RNUnits[] = {QString(), QStringLiteral("i"), QStringLiteral("ii"), QStringLiteral("iii"), QStringLiteral("iv"), QStringLiteral("v"), QStringLiteral("vi"), QStringLiteral("vii"), QStringLiteral("viii"), QStringLiteral("ix")};
    static const QString RNTens[] = {QString(), QStringLiteral("x"), QStringLiteral("xx"), QStringLiteral("xxx"), QStringLiteral("xl"), QStringLiteral("l"), QStringLiteral("lx"), QStringLiteral("lxx"), QStringLiteral("lxxx"), QStringLiteral("xc")};
    static const QString RNHundreds[] = {QString(), QStringLiteral("c"), QStringLiteral("cc"), QStringLiteral("ccc"), QStringLiteral("cd"), QStringLiteral("d"), QStringLiteral("dc"), QStringLiteral("dcc"), QStringLiteral("dccc"), QStringLiteral("cm")};
    static const QString RNThousands[] = {QString(), QStringLiteral("m"), QStringLiteral("mm"), QStringLiteral("mmm")};

    if (n < 0) { // should never happen, but better not crash if it does
        warnPlan << "intToRoman called with negative number: n=" << n;
        return QString::number(n);
    }

    QString s = RNThousands[ (n / 1000) ] +
                RNHundreds[ (n / 100) % 10 ] +
                RNTens[ (n / 10) % 10 ] +
                RNUnits[ (n) % 10 ];
    return upper ? s.toUpper() : s;
}

QStringList WBSDefinition::codeList() const {
    QStringList cl;
    QList<QPair<QString, QString> >::ConstIterator it;
    for (it = m_codeLists.constBegin(); it != m_codeLists.constEnd(); ++it) {
        cl.append((*it).second);
    }
    return cl;
}

int WBSDefinition::defaultCodeIndex() const {
    int index = -1;
    for(int i = 0; i < m_codeLists.count(); ++i) {
        if (m_defaultDef.code == m_codeLists.at(i).first) {
            index = i;
            break;
        }
    }
    return index;
}

bool WBSDefinition::setDefaultCode(uint index) {
    if ((int)index >= m_codeLists.size()) {
        return false;
    }
    m_defaultDef.code = m_codeLists[index].first;
    return true;
}

void WBSDefinition::setDefaultSeparator(const QString& s) {
    m_defaultDef.separator = s;
}

bool WBSDefinition::loadXML(KoXmlElement &element, XMLLoaderObject &) {
    m_projectCode = element.attribute(QStringLiteral("project-code"));
    m_projectSeparator = element.attribute(QStringLiteral("project-separator"));
    m_levelsEnabled = (bool)element.attribute(QStringLiteral("levels-enabled")).toInt();
    KoXmlNode n = element.firstChild();
    for (; ! n.isNull(); n = n.nextSibling()) {
        if (! n.isElement()) {
            continue;
        }
        KoXmlElement e = n.toElement();
        if (e.tagName() == QStringLiteral("default")) {
            m_defaultDef.code = e.attribute(QStringLiteral("code"), QStringLiteral("Number"));
            m_defaultDef.separator = e.attribute(QStringLiteral("separator"), QStringLiteral("."));
        } else if (e.tagName() == QStringLiteral("levels")) {
            KoXmlNode n = e.firstChild();
            for (; ! n.isNull(); n = n.nextSibling()) {
                if (! n.isElement()) {
                    continue;
                }
                KoXmlElement el = n.toElement();
                CodeDef d;
                d.code = el.attribute(QStringLiteral("code"));
                d.separator = el.attribute(QStringLiteral("separator"));
                int lvl = el.attribute(QStringLiteral("level"), QStringLiteral("-1")).toInt();
                if (lvl >= 0) {
                    setLevelsDef(lvl, d);
                } else errorPlan<<"Invalid levels definition";
            }
        }
    }
    return true;
}

void WBSDefinition::saveXML(QDomElement &element)  const {
    QDomElement me = element.ownerDocument().createElement(QStringLiteral("wbs-definition"));
    element.appendChild(me);

    me.setAttribute(QStringLiteral("project-code"), m_projectCode);
    me.setAttribute(QStringLiteral("project-separator"), m_projectSeparator);
    me.setAttribute(QStringLiteral("levels-enabled"), QString::number(m_levelsEnabled));
    if (! m_levelsDef.isEmpty()) {
        QDomElement ld = element.ownerDocument().createElement(QStringLiteral("levels"));
        me.appendChild(ld);
        QMap<int, CodeDef>::ConstIterator it;
        for (it = m_levelsDef.constBegin(); it != m_levelsDef.constEnd(); ++it) {
            QDomElement l = element.ownerDocument().createElement(QStringLiteral("level"));
            ld.appendChild(l);
            l.setAttribute(QStringLiteral("level"), QString::number(it.key()));
            l.setAttribute(QStringLiteral("code"), it.value().code);
            l.setAttribute(QStringLiteral("separator"), it.value().separator);
        }
    }
    QDomElement cd = element.ownerDocument().createElement(QStringLiteral("default"));
    me.appendChild(cd);
    cd.setAttribute(QStringLiteral("code"), m_defaultDef.code);
    cd.setAttribute(QStringLiteral("separator"), m_defaultDef.separator);
}

} //namespace KPlato
