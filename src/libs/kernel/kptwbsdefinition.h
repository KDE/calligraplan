/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2005 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTWBSDEFINITION_H
#define KPTWBSDEFINITION_H

#include "plankernel_export.h"

#include <KoXmlReaderForward.h>

#include <QString>
#include <QMap>
#include <QPair>
#include <QList>

class QDomElement;


namespace KPlato
{

class XMLLoaderObject;
    
class PLANKERNEL_EXPORT WBSDefinition {

public:
    /// Construct a default definition
    WBSDefinition();
    /// Copy constructor
    WBSDefinition(const WBSDefinition &def);
    /// Destructor
    ~WBSDefinition();

    class PLANKERNEL_EXPORT CodeDef {
        public:
            CodeDef() {}
            CodeDef(const QString &c, const QString &s) { code = c; separator = s; }
            ~CodeDef() {}
            void clear() { code.clear(); separator.clear(); }
            bool isEmpty() { return code.isEmpty(); }
            QString code;
            QString separator;
    };
    
    WBSDefinition &operator=(const WBSDefinition &def);
    
    void clear();
    
    /// Return wbs string.
    QString wbs(uint index, int level) const;
    /// Return wbs code.
    QString code(uint index, int level) const;
    /// Return wbs separator.
    QString separator(int level) const;
    
    CodeDef &defaultDef() { return m_defaultDef; }
    void setDefaultDef(const CodeDef &def) { m_defaultDef = def; }
    
    bool isLevelsDefEnabled() const { return m_levelsEnabled; }
    bool level0Enabled() const;
    void setLevelsDefEnabled(bool on) { m_levelsEnabled = on; }
    void clearLevelsDef() { m_levelsDef.clear(); }
    QMap<int, CodeDef> levelsDef() const { return m_levelsDef; }
    void setLevelsDef(const QMap<int, CodeDef> &def);
    CodeDef levelsDef(int level) const;
    void setLevelsDef(int level, const CodeDef &def);
    void setLevelsDef(int level, const QString& c, const QString& s);
    
    QStringList codeList() const;
    int defaultCodeIndex() const;
    bool setDefaultCode(uint index);
    QString defaultSeparator() const { return m_defaultDef.separator; }
    void setDefaultSeparator(const QString& s);

    QString projectCode() const { return m_projectCode; }
    void setProjectCode(const QString &str) { m_projectCode = str; }
    QString projectSeparator() const { return m_projectSeparator; }
    void setProjectSeparator(const QString &str) { m_projectSeparator = str; }
    
    /// Load from document
    bool loadXML(KoXmlElement &element, XMLLoaderObject &status);
    /// Save to document
    void saveXML(QDomElement &element) const;
    
protected:
    QString code(const CodeDef &def, uint index) const;
    QString toRoman(int n, bool upper = false) const;
    
private:
    QString m_projectCode;
    QString m_projectSeparator;
    CodeDef m_defaultDef;
    bool m_levelsEnabled;
    QMap<int, CodeDef> m_levelsDef;

    QList<QPair<QString, QString> > m_codeLists;
};

} //namespace KPlato

#endif //WBSDEFINITION_H
