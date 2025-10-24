/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2004-2006 David Faure <faure@kde.org>
   SPDX-FileCopyrightText: 2007-2008 Thorsten Zachmann <zachmann@kde.org>
   SPDX-FileCopyrightText: 2009 Inge Wallin <inge@lysator.liu.se>
   SPDX-FileCopyrightText: 2010 KO GmbH <jos.van.den.oever@kogmbh.com>
   SPDX-FileCopyrightText: 2010 Jarosław Staniek <staniek@kde.org>
   SPDX-FileCopyrightText: 2011 Pierre Ducroquet <pinaraf@pinaraf.info>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KOGENSTYLE_H
#define KOGENSTYLE_H

#include <QList>
#include <QMap>
#include <QString>
#include "koodf_export.h"

class QTextLength;
class KoGenStyles;
class KoXmlWriter;

/**
 * A generic style, i.e. basically a collection of properties and a name.
 * Instances of KoGenStyle can either be held in the KoGenStyles collection,
 * or created (e.g. on the stack) and given to KoGenStyles::insert().
 *
 * @author David Faure <faure@kde.org>
 */
class KOODF_EXPORT KoGenStyle
{
public:
    /**
     * Possible values for the "type" argument of the KoGenStyle constructor.
     * @note If there is still something missing, add it here so that it is possible to use the same
     *       saving code in all applications.
     */
    enum Type {
        PageLayoutStyle,             ///< style:page-layout as in odf 14.3 Page Layout
        TextStyle,                   ///< style:style from family "text" as in odf 14.8.1 Text Styles
                                     ///<  (office:styles)
        TextAutoStyle,               ///< style:style from family "text" as in odf 14.8.1 Text Styles
                                     ///<  (office:automatic-styles)
        ParagraphStyle,              ///< style:style from family "paragraph" as in odf 14.1 Style Element
                                     ///<  (office:styles)
        ParagraphAutoStyle,          ///< style:style from family "paragraph" as in odf 14.1 Style Element
                                     ///<  (office:automatic-styles)
        SectionStyle,                ///< style:style from family "section" as in odf 14.8.3 Section Styles
                                     ///<  (office:styles)
        SectionAutoStyle,            ///< style:style from family "section" as in odf 14.8.3 Section Styles
                                     ///<  (office:automatic-styles)
        RubyStyle,                   ///< style:style from family "ruby" as in odf 14.8.4 Ruby Style
                                     ///<  (office:styles)
        RubyAutoStyle,               ///< style:style from family "ruby" as in odf 14.8.4 Ruby Style
                                     ///<  (office:automatic-styles)
        TableStyle,                  ///< style:style from family "table" as in odf 14.12 Table Formatting
                                     ///<  Properties (office:styles)
        TableAutoStyle,              ///< style:style from family "table" as in odf 14.12 Table Formatting Properties
                                     ///<  (office:automatic-styles)
        TableColumnStyle,            ///< style:style from family "table-column" as in odf 15.9 Column Formatting
                                     ///<  Properties (office:styles)
        TableColumnAutoStyle,        ///< style:style from family "table-column" as in odf 15.9 Column Formatting
                                     ///<  Properties (office:automatic-styles)
        TableRowStyle,               ///< style:style from family "table-row" as in odf 15.10 Table Row Formatting
                                     ///<  Properties (office:styles)
        TableRowAutoStyle,           ///< style:style from family "table-row" as in odf 15.10 Table Row Formatting
                                     ///<  Properties (office:automatic-styles)
        TableCellStyle,              ///< style:style from family "table-cell" as in odf 15.11 Table Cell Formatting
                                     ///<  Properties (office:styles)
        TableCellAutoStyle,          ///< style:style from family "table-cell" as in odf 15.11 Table Cell Formatting
                                     ///<  Properties (office:automatic-styles)
        GraphicStyle,                ///< style:style from family "graphic" as in 14.13.1 Graphic and Presentation
                                     ///<  Styles (office:automatic-styles)
        GraphicAutoStyle,            ///< style:style from family "graphic" as in 14.13.1 Graphic and Presentation
                                     ///<  Styles (office:automatic-styles)
        PresentationStyle,           ///< style:style from family "presentation" as in 14.13.1 Graphic and
                                     ///<  Presentation Styles (office:styles)
        PresentationAutoStyle,       ///< style:style from family "presentation" as in 14.13.1 Graphic and
                                     ///<  Presentation Styles (office:automatic-styles)
        DrawingPageStyle,            ///< style:style from family "drawing-page" as in odf 14.13.2 Drawing Page Style
                                     ///<  (office:styles)
        DrawingPageAutoStyle,        ///< style:style from family "drawing-page" as in odf 14.13.2 Drawing Page Style
                                     ///<  (office:automatic-styles)
        ChartStyle,                  ///< style:style from family "chart" as in odf 14.16 Chart Styles
                                     ///<  (office:styles)
        ChartAutoStyle,              ///< style:style from family "chart" as in odf 14.16 Chart Styles
                                     ///<  (office:automatic-styles)

        ListStyle,                   ///< text:list-style as in odf 14.10 List Style (office:styles)
        ListAutoStyle,               ///< text:list-style as in odf 14.10 List Style (office:automatic-styles)
        NumericNumberStyle,          ///< number:number-style as in odf 14.7.1 Number Style
        NumericDateStyle,            ///< number:date-style as in odf 14.7.4 Date Style
        NumericTimeStyle,            ///< number:time-style as in odf 14.7.5 Time Style
        NumericFractionStyle,        ///< number:number-style as in odf 14.7.1 Number Style
        NumericPercentageStyle,      ///< number:percentage-style as in odf 14.7.3 Percentage Style
        NumericScientificStyle,      ///< number:number-style as in odf 14.7.1 Number Style
        NumericCurrencyStyle,        ///< number:currency-style as in odf 14.7.2 Currency Style
        NumericTextStyle,            ///< number:text-style 14.7.7 Text Style
                                     ///<  @note unused
        HatchStyle,                  ///< draw:hatch as in odf 14.14.3 Hatch (office:styles)
        StrokeDashStyle,             ///< draw:stroke-dash as in odf 14.14.7 Stroke Dash (office:styles)
        GradientStyle,               ///< draw:gradient as in odf 14.14.1 Gradient (office:styles)
        LinearGradientStyle,         ///< svg:linearGradient as in odf 14.14.2 SVG Gradients (office:styles)
        RadialGradientStyle,         ///< svg:radialGradient as in odf 14.14.2 SVG Gradients (office:styles)
        ConicalGradientStyle,        ///< calligra:conicalGradient calligra extension for conical gradients
        FillImageStyle,              ///< draw:fill-image as in odf 14.14.4 Fill Image (office:styles)
        NumericBooleanStyle,         ///< number:boolean 14.7.6 Boolean Style
                                     ///<  @note unused
        OpacityStyle,                ///< draw:opacity as in odf 14.14.5 Opacity Gradient
                                     ///<  @note unused
        MarkerStyle,                 ///< draw:marker as in odf 14.14.6 Marker
        PresentationPageLayoutStyle, ///< style:presentation-page-layout as in odf 14.15 Presentation Page Layouts
        OutlineLevelStyle,           ///< text:outline-style as in odf 1.2 section 16.34
        //   TODO differently
        MasterPageStyle,             ///< style:master-page as in odf 14.4 14.4 Master Pages (office:master-styles)
        // style:default-style as in odf 14.2 Default Styles
        // 14.5 Table Templates
        /// @internal @note always update when adding values to this enum
        LastStyle = MasterPageStyle
    };

    /**
     * Start the definition of a new style. Its name will be set later by KoGenStyles::insert(),
     * but first you must define its properties and attributes.
     *
     * @param type this is a hook for the application to categorize styles
     * See the Style* enum. Ignored when writing out the style.
     *
     * @param familyName The value for style:family, e.g. text, paragraph, graphic etc.
     * The family is for style:style elements only; number styles and list styles don't have one.
     *
     * @param parentName If set, name of the parent style from which this one inherits.
     */
    explicit KoGenStyle(Type type = PageLayoutStyle, const char *familyName = nullptr,
                        const QString &parentName = QString());
    ~KoGenStyle();

    /**
     * setAutoStyleInStylesDotXml(true) marks a given automatic style as being needed in styles.xml.
     * For instance styles used by headers and footers need to go there, since
     * they are saved in styles.xml, and styles.xml must be independent from content.xml.
     *
     * The application should use KoGenStyles::styles(type, true) in order to retrieve
     * those styles and save them separately.
     */
    void setAutoStyleInStylesDotXml(bool b) {
        m_autoStyleInStylesDotXml = b;
    }
    /// @return the value passed to setAutoStyleInStylesDotXml; false by default
    bool autoStyleInStylesDotXml() const {
        return m_autoStyleInStylesDotXml;
    }

    /**
     * setDefaultStyle(true) marks a given style as being the default style.
     * This means we expect that you will call writeStyle(...,"style:default-style"),
     * and its name will be omitted in the output.
     */
    void setDefaultStyle(bool b) {
        m_defaultStyle = b;
    }
    /// @return the value passed to setDefaultStyle; false by default
    bool isDefaultStyle() const {
        return m_defaultStyle;
    }

    /// Return the type of this style, as set in the constructor
    Type type() const {
        return m_type;
    }

    /// Return the family name
    const char* familyName() const {
        return m_familyName.data();
    }

    /// Sets the name of style's parent.
    void setParentName(const QString &name) {
        m_parentName = name;
    }

    /// Return the name of style's parent, if set
    QString parentName() const {
        return m_parentName;
    }

    /**
     *  @brief The types of properties
     *
     *  Simple styles only write one foo-properties tag, in which case they can just use DefaultType.
     *  However a given style might want to write several kinds of properties, in which case it would
     *  need to use other property types than the default one.
     *
     *  For instance this style:
     *  @code
     *  <style:style style:family="chart">
     *    <style:chart-properties .../>
     *    <style:graphic-properties .../>
     *    <style:text-properties .../>
     *  </style:style>
     *  @endcode
     *  would use DefaultType for chart-properties (and would pass "style:chart-properties" to writeStyle(),
     *  and would use GraphicType and TextType.
     */
    enum PropertyType {
        /**
         *  DefaultType depends on family: e.g. paragraph-properties if family=paragraph
         *  or on the type of style (e.g. page-layout -> page-layout-properties).
         *  (In fact that tag name is the one passed to writeStyle)
         */
        DefaultType,
        /// TextType is always text-properties.
        TextType,
        /// ParagraphType is always paragraph-properties.
        ParagraphType,
        /// GraphicType is always graphic-properties.
        GraphicType,
        /// SectionType is always section-properties.
        SectionType,
        /// RubyType is always ruby-properties.
        RubyType,
        /// TableType is always table-properties.
        TableType,
        /// TableColumnType is always table-column-properties
        TableColumnType,
        /// TableRowType is always table-row-properties.
        TableRowType,
        /// TableCellType is always for table-cell-properties.
        TableCellType,
        /// PresentationType is always for presentation-properties.
        PresentationType,
        /// DrawingPageType is always for drawing-page-properties.
        DrawingPageType,
        /// ChartType is always for chart-properties.
        ChartType,
        Reserved1, ///< @internal for binary compatible extensions
        /// For elements that are children of the style itself, not any of the properties
        StyleChildElement,
        /// @internal @note always update when adding values to this enum
        LastPropertyType = StyleChildElement
    };

    /// Add a property to the style. Passing DefaultType as property type uses a style-type specific property type.
    void addProperty(const QString &propName, const QString &propValue, PropertyType type = DefaultType) {
        if (type == DefaultType) {
            type = m_propertyType;
        }
        m_properties[type].insert(propName, propValue);
    }
    /// Overloaded version of addProperty that takes a char*, usually for "..."
    void addProperty(const QString &propName, const char *propValue, PropertyType type = DefaultType) {
        if (type == DefaultType) {
            type = m_propertyType;
        }
        m_properties[type].insert(propName, QString::fromUtf8(propValue));
    }
    /// Overloaded version of addProperty that takes a char*, usually for "..."
    void addProperty(const char *propName, const char *propValue, PropertyType type = DefaultType) {
        const auto name = QLatin1String(propName);
        if (type == DefaultType) {
            type = m_propertyType;
        }
        m_properties[type].insert(name, QString::fromUtf8(propValue));
    }
    /// Overloaded version of addProperty that takes a char*, usually for "..."
    void addProperty(const char *propName, const QString &propValue, PropertyType type = DefaultType) {
        const auto name = QLatin1String(propName);
        if (type == DefaultType) {
            type = m_propertyType;
        }
        m_properties[type].insert(name, propValue);
    }
    /// Overloaded version of addProperty that converts an int to a string
    void addProperty(const QString &propName, int propValue, PropertyType type = DefaultType) {
        if (type == DefaultType) {
            type = m_propertyType;
        }
        m_properties[type].insert(propName, QString::number(propValue));
    }
    /// Overloaded version of addProperty that converts a bool to a string (false/true)
    void addProperty(const QString &propName, bool propValue, PropertyType type = DefaultType) {
        if (type == DefaultType) {
            type = m_propertyType;
        }
        m_properties[type].insert(propName, propValue ? QStringLiteral("true") : QStringLiteral("false"));
    }

    /**
     *  Add a property which represents a distance, measured in pt
     *  The number is written out with the highest possible precision
     *  (unlike QString::number and setNum, which default to 6 digits),
     *  and the unit name ("pt") is appended to it.
     */
    void addPropertyPt(const QString &propName, qreal propValue, PropertyType type = DefaultType);
    void addPropertyPt(const char *propName, qreal propValue, PropertyType type = DefaultType);

    /**
     *  Add a property which represents a length, measured in pt, or in percent
     *  The number is written out with the highest possible precision
     *  (unlike QString::number and setNum, which default to 6 digits) or as integer (for percents),
     *  and the unit name ("pt" or "%") is appended to it.
     */
    void addPropertyLength(const QString &propName, const QTextLength &propValue, PropertyType type = DefaultType);

    /**
     *  Remove a property from the style.  Passing DefaultType as property type
     *  uses a style-type specific property type.
     */
    void removeProperty(const QString &propName, PropertyType type = DefaultType) {
        if (type == DefaultType) {
            type = m_propertyType;
        }
        m_properties[type].remove(propName);
    }

    /**
     *  Remove properties of defined type from the style.  Passing DefaultType
     *  as property type uses a style-type specific property type.
     */
    void removeAllProperties(PropertyType type = DefaultType) {
        if (type == DefaultType) {
            type = m_propertyType;
        }
        m_properties[type].clear();
    }

    /**
     *  Add an attribute to the style
     *  The difference between property and attributes is a bit oasis-format-specific:
     *  attributes are for the style element itself, and properties are in the style:properties child element
     */
    void addAttribute(const QString &attrName, const QString& attrValue) {
        m_attributes.insert(attrName, attrValue);
    }
    /// Overloaded version of addAttribute that takes a char*, usually for "..."
    void addAttribute(const QString &attrName, const char* attrValue) {
        m_attributes.insert(attrName, QString::fromUtf8(attrValue));
    }
    /// Overloaded version of addAttribute that takes a char*, usually for "..."
    void addAttribute(const char *attrName, const char* attrValue) {
        m_attributes.insert(QLatin1String(attrName), QString::fromUtf8(attrValue));
    }
    /// Overloaded version of addAttribute that takes a char*, usually for "..."
    void addAttribute(const char *attrName, const QString &attrValue) {
        m_attributes.insert(QLatin1String(attrName), attrValue);
    }
    /// Overloaded version of addAttribute that converts an int to a string
    void addAttribute(const QString &attrName, int attrValue) {
        m_attributes.insert(attrName, QString::number(attrValue));
    }
    /// Overloaded version of addAttribute that converts an int to a string
    void addAttribute(const char *attrName, int attrValue) {
        m_attributes.insert(QLatin1String(attrName), QString::number(attrValue));
    }

    /// Overloaded version of addAttribute that converts a bool to a string
    void addAttribute(const QString &attrName, bool attrValue) {
        m_attributes.insert(attrName, attrValue ? QStringLiteral("true") : QStringLiteral("false"));
    }
    /// Overloaded version of addAttribute that converts a bool to a string
    void addAttribute(const char *attrName, bool attrValue) {
        addAttribute(attrName, attrValue ? "true" : "false");
    }

    /**
     *  Add an attribute which represents a distance, measured in pt
     *  The number is written out with the highest possible precision
     *  (unlike QString::number and setNum, which default to 6 digits),
     *  and the unit name ("pt") is appended to it.
     */
    void addAttributePt(const QString &attrName, qreal attrValue);
    void addAttributePt(const char *attrName, qreal attrValue);

    /**
     * Add an attribute that represents a percentage value as defined in ODF
     */
    void addAttributePercent(const QString &attrName, qreal value);
    void addAttributePercent(const char *attrName, qreal value);

    /**
     * Add an attribute that represents a percentage value as defined in ODF
     */
    void addAttributePercent(const QString &attrName, int value);
    void addAttributePercent(const char* attrName, int value);

    /**
     *  Remove an attribute from the style.
     */
    void removeAttribute(const QString &attrName) {
        m_attributes.remove(attrName);
    }
    void removeAttribute(const char *attrName) {
        m_attributes.remove(QLatin1String(attrName));
    }


    /**
     * @brief Add a child element to the style properties.
     *
     * What is meant here is that the contents of the QString
     * will be written out literally. This means you should use
     * KoXmlWriter to generate it:
     * @code
     * QBuffer buffer;
     * buffer.open(QIODevice::WriteOnly);
     * KoXmlWriter elementWriter(&buffer);  // TODO pass indentation level
     * elementWriter.startElement("...");
     * ...
     * elementWriter.endElement();
     * QString elementContents = QString::fromUtf8(buffer.buffer(), buffer.buffer().size());
     * gs.addChildElement("...", elementContents);
     * @endcode
     *
     * The value of @p elementName is only used to set the order on how the child elements are written out.
     */
    void addChildElement(const QString &elementName, const QString& elementContents, PropertyType type = DefaultType) {
        if (type == DefaultType) {
            type = m_propertyType;
        }
        m_childProperties[type].insert(elementName, elementContents);
    }
    void addChildElement(const char *elementName, const QString& elementContents, PropertyType type = DefaultType) {
        addChildElement(QLatin1String(elementName), elementContents, type);
    }

    /**
     * Same like \a addChildElement above but with QByteArray to explicit convert from QByteArray
     * to QString using utf8 to prevent a dirty pitfall.
     */
    void addChildElement(const QString &elementName, const QByteArray& elementContents, PropertyType type = DefaultType) {
        if (type == DefaultType) {
            type = m_propertyType;
        }
        m_childProperties[type].insert(elementName, QString::fromUtf8(elementContents));
    }

    /**
     * Same like \a addChildElement above but adds a child style which is not child of any of the properties
     * The value of @p elementName is only used to set the order on how the child elements are written out.
     */
    void addStyleChildElement(const QString &elementName, const QString& elementContents) {
        m_properties[StyleChildElement].insert(elementName, elementContents);
    }

    /**
     * Same like \a addStyleChildElement above but with QByteArray to explicit convert from QByteArray
     * to QString using utf8 to prevent a dirty pitfall.
     * The value of @p elementName is only used to set the order on how the child elements are written out.
     */
    void addStyleChildElement(const QString &elementName, const QByteArray& elementContents) {
        m_properties[StyleChildElement].insert(elementName, QString::fromUtf8(elementContents));
    }

    /**
     * @brief Add a style:map to the style.
     * @param styleMap the attributes for the map, associated as (name,value).
     */
    void addStyleMap(const QMultiMap<QString, QString> &styleMap);

    /**
     * @return true if the style has no attributes, no properties, no style map etc.
     * This can be used by applications which do not save all attributes unconditionally,
     * but only those that differ from the parent. But note that KoGenStyles::insert() can't find this out...
     */
    bool isEmpty() const;

    /**
     *  Write the definition of this style to @p writer, using the OASIS format.
     *  @param writer the KoXmlWriter in which @p elementName will be created and filled in
     *  @param styles the styles collection, used to look up the parent style
     *  @param elementName the name of the XML element, e.g. "style:style". Don't forget to
     *  pass style:default-style if isDefaultStyle().
     *  @param name must come from the collection. It will be ignored if isDefaultStyle() is true.
     *  @param propertiesElementName the name of the XML element with the style properties,
     *  e.g. "style:text-properties". Can be 0 in special cases where there should be no such item,
     *  in which case the attributes and elements are added under the style itself.
     *  @param closeElement set it to false to be able to add more child elements to the style element
     *  @param drawElement set it to true to add "draw:name" (used for gradient/hatch style) otherwise add "style:name"
     */
    void writeStyle(KoXmlWriter *writer, const KoGenStyles &styles, const char *elementName, const QString &name,
                    const char *propertiesElementName, bool closeElement = true, bool drawElement = false) const;

    void writeStyle(KoXmlWriter *writer, const KoGenStyles &styles, const char *elementName, const char *name,
                    const char *propertiesElementName, bool closeElement = true, bool drawElement = false) const;

    /**
     *  Write the definition of these style properties to @p writer, using the OASIS format.
     *  @param writer the KoXmlWriter in which @p elementName will be created and filled in
     *  @param type the type of properties to write
     *  @param parentStyle the parent to this style
     */
    void writeStyleProperties(KoXmlWriter *writer, PropertyType type,
                              const KoGenStyle *parentStyle = nullptr) const;

    /**
     *  QMap requires a complete sorting order.
     *  Another solution would have been a qdict and a key() here, a la KoTextFormat,
     *  but the key was difficult to generate.
     *  Solutions with only a hash value (not representative of the whole data)
     *  require us to write a hashtable by hand....
     */
    bool operator<(const KoGenStyle &other) const;

    /// Not needed for QMap, but can still be useful
    bool operator==(const KoGenStyle &other) const;

    /**
     * Returns a property of this style. In principal this class is meant to be write-only, but
     * some exceptional cases having read-support as well is very useful.  Passing DefaultType
     * as property type uses a style-type specific property type.
     */
    QString property(const QString &propName, PropertyType type = DefaultType) const {
        if (type == DefaultType) {
            type = m_propertyType;
        }
        const QMultiMap<QString, QString>::const_iterator it = m_properties[type].constFind(propName);
        if (it != m_properties[type].constEnd())
            return it.value();
        return QString();
    }

    /**
     * Returns a property of this style. In principal this class is meant to be write-only, but
     * some exceptional cases having read-support as well is very useful.  Passing DefaultType
     * as property type uses a style-type specific property type.
     */
    QString childProperty(const QString &propName, PropertyType type = DefaultType) const {
        if (type == DefaultType) {
            type = m_propertyType;
        }
        const QMultiMap<QString, QString>::const_iterator it = m_childProperties[type].constFind(propName);
        if (it != m_childProperties[type].constEnd())
            return it.value();
        return QString();
    }

    /// Returns an attribute of this style. In principal this class is meant to be write-only, but some exceptional cases having read-support as well is very useful.
    QString attribute(const QString &propName) const {
        const QMultiMap<QString, QString>::const_iterator it = m_attributes.constFind(propName);
        if (it != m_attributes.constEnd())
            return it.value();
        return QString();
    }
    /// Returns an attribute of this style. In principal this class is meant to be write-only, but some exceptional cases having read-support as well is very useful.
    QString attribute(const char *propName) const {
        return attribute(QLatin1String(propName));
    }

    /**
     * Copies properties of defined type from a style to another style.
     * This is needed in rare cases where two styles have properties of different types
     * and we want to merge them to one style.
     */
    static void copyPropertiesFromStyle(const KoGenStyle &sourceStyle, KoGenStyle &targetStyle, PropertyType type = DefaultType);

private:
#ifndef NDEBUG
    void printDebug() const;
#endif

private:
    // Note that the copy constructor and assignment operator are allowed.
    // Better not use pointers below!
    // TODO turn this into a QSharedData class
    PropertyType m_propertyType;
    Type m_type;
    QByteArray m_familyName;
    QString m_parentName;
    /// We use QMaps since they provide automatic sorting on the key (important for unicity!)
    typedef QMultiMap<QString, QString> StyleMap;
    StyleMap m_properties[LastPropertyType+1];
    StyleMap m_childProperties[LastPropertyType+1];
    StyleMap m_attributes;
    QList<StyleMap> m_maps; // we can't really sort the maps between themselves...

    bool m_autoStyleInStylesDotXml;
    bool m_defaultStyle;
    short m_unused2;

    // For insert()
    friend class KoGenStyles;
};

#endif /* KOGENSTYLE_H */
