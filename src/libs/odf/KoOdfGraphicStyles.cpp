/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2004-2006 David Faure <faure@kde.org>
   SPDX-FileCopyrightText: 2007 Jan Hambrecht <jaham@gmx.net>
   SPDX-FileCopyrightText: 2007-2008, 2010-2011 Thorsten Zachmann <zachmann@kde.org>
   SPDX-FileCopyrightText: 2011 Lukáš Tvrdý <lukas.tvrdy@ixonos.com>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "KoOdfGraphicStyles.h"

#include <QBrush>
#include <QBuffer>
#include <QPen>

#include <OdfDebug.h>

#include <KoGenStyles.h>
#include <KoStyleStack.h>
#include <KoUnit.h>
#include <KoXmlNS.h>
#include <KoXmlWriter.h>
#include <QRegularExpression>

#include "KoOdfStylesReader.h"

void KoOdfGraphicStyles::saveOdfFillStyle(KoGenStyle &styleFill, KoGenStyles& mainStyles, const QBrush & brush)
{
    KoGenStyle::Type type = styleFill.type();
    KoGenStyle::PropertyType propertyType = (type == KoGenStyle::GraphicStyle || type == KoGenStyle::GraphicAutoStyle ||
                                             type == KoGenStyle::DrawingPageStyle || type == KoGenStyle::DrawingPageAutoStyle)
                                            ? KoGenStyle::DefaultType : KoGenStyle::GraphicType;
    switch (brush.style()) {
    case Qt::Dense1Pattern:
        styleFill.addProperty("draw:opacity", QStringLiteral("6%"), propertyType);
        styleFill.addProperty("draw:fill", "solid", propertyType);
        styleFill.addProperty("draw:fill-color", brush.color().name(), propertyType);
        break;
    case Qt::Dense2Pattern:
        styleFill.addProperty("draw:opacity", QStringLiteral("12%"), propertyType);
        styleFill.addProperty("draw:fill", "solid", propertyType);
        styleFill.addProperty("draw:fill-color", brush.color().name(), propertyType);
        break;
    case Qt::Dense3Pattern:
        styleFill.addProperty("draw:opacity", QStringLiteral("37%"), propertyType);
        styleFill.addProperty("draw:fill", "solid", propertyType);
        styleFill.addProperty("draw:fill-color", brush.color().name(), propertyType);
        break;
    case Qt::Dense4Pattern:
        styleFill.addProperty("draw:opacity", QStringLiteral("50%"), propertyType);
        styleFill.addProperty("draw:fill", "solid", propertyType);
        styleFill.addProperty("draw:fill-color", brush.color().name(), propertyType);
        break;
    case Qt::Dense5Pattern:
        styleFill.addProperty("draw:opacity", QStringLiteral("63%"), propertyType);
        styleFill.addProperty("draw:fill", "solid", propertyType);
        styleFill.addProperty("draw:fill-color", brush.color().name(), propertyType);
        break;
    case Qt::Dense6Pattern:
        styleFill.addProperty("draw:opacity", QStringLiteral("88%"), propertyType);
        styleFill.addProperty("draw:fill", "solid", propertyType);
        styleFill.addProperty("draw:fill-color", brush.color().name(), propertyType);
        break;
    case Qt::Dense7Pattern:
        styleFill.addProperty("draw:opacity", QStringLiteral("94%"), propertyType);
        styleFill.addProperty("draw:fill", "solid", propertyType);
        styleFill.addProperty("draw:fill-color", brush.color().name(), propertyType);
        break;
    case Qt::LinearGradientPattern:
    case Qt::RadialGradientPattern:
    case Qt::ConicalGradientPattern:
        styleFill.addProperty("draw:fill", "gradient", propertyType);
        styleFill.addProperty("draw:fill-gradient-name", saveOdfGradientStyle(mainStyles, brush), propertyType);
        break;
    case Qt::HorPattern:
    case Qt::VerPattern:
    case Qt::CrossPattern:
    case Qt::BDiagPattern:
    case Qt::FDiagPattern:
    case Qt::DiagCrossPattern:
        styleFill.addProperty("draw:fill", "hatch", propertyType);
        styleFill.addProperty("draw:fill-hatch-name", saveOdfHatchStyle(mainStyles, brush), propertyType);
        break;
    case Qt::SolidPattern:
        styleFill.addProperty("draw:fill", "solid", propertyType);
        styleFill.addProperty("draw:fill-color", brush.color().name(), propertyType);
        if (! brush.isOpaque())
            styleFill.addProperty("draw:opacity", QStringLiteral("%1%").arg(brush.color().alphaF() * 100.0), propertyType);
        break;
    case Qt::NoBrush:
    default:
        styleFill.addProperty("draw:fill", "none", propertyType);
        break;
    }
}

void KoOdfGraphicStyles::saveOdfStrokeStyle(KoGenStyle &styleStroke, KoGenStyles &mainStyles, const QPen &pen)
{
    // TODO implement all possibilities
    switch (pen.style()) {
    case Qt::NoPen:
        styleStroke.addProperty("draw:stroke", "none", KoGenStyle::GraphicType);
        return;
    case Qt::SolidLine:
        styleStroke.addProperty("draw:stroke", "solid", KoGenStyle::GraphicType);
        break;
    default: { // must be a dashed line
        styleStroke.addProperty("draw:stroke", "dash", KoGenStyle::GraphicType);
        // save stroke dash (14.14.7) which is severely limited, but still
        KoGenStyle dashStyle(KoGenStyle::StrokeDashStyle);
        dashStyle.addAttribute("draw:style", "rect");
        QVector<qreal> dashes = pen.dashPattern();
        dashStyle.addAttribute("draw:dots1", static_cast<int>(1));
        dashStyle.addAttributePt("draw:dots1-length", dashes[0]*pen.widthF());
        dashStyle.addAttributePt("draw:distance", dashes[1]*pen.widthF());
        if (dashes.size() > 2) {
            dashStyle.addAttribute("draw:dots2", static_cast<int>(1));
            dashStyle.addAttributePt("draw:dots2-length", dashes[2]*pen.widthF());
        }
        QString dashStyleName = mainStyles.insert(dashStyle, QStringLiteral("dash"));
        styleStroke.addProperty("draw:stroke-dash", dashStyleName, KoGenStyle::GraphicType);
        break;
    }
    }

    if (pen.brush().gradient()) {
        styleStroke.addProperty("calligra:stroke-gradient", saveOdfGradientStyle(mainStyles, pen.brush()), KoGenStyle::GraphicType);
    }
    else {
        styleStroke.addProperty("svg:stroke-color", pen.color().name(), KoGenStyle::GraphicType);
        styleStroke.addProperty("svg:stroke-opacity", QStringLiteral("%1").arg(pen.color().alphaF()), KoGenStyle::GraphicType);
    }
    styleStroke.addPropertyPt("svg:stroke-width", pen.widthF(), KoGenStyle::GraphicType);

    switch (pen.joinStyle()) {
    case Qt::MiterJoin:
        styleStroke.addProperty("draw:stroke-linejoin", "miter", KoGenStyle::GraphicType);
        break;
    case Qt::BevelJoin:
        styleStroke.addProperty("draw:stroke-linejoin", "bevel", KoGenStyle::GraphicType);
        break;
    case Qt::RoundJoin:
        styleStroke.addProperty("draw:stroke-linejoin", "round", KoGenStyle::GraphicType);
        break;
    default:
        styleStroke.addProperty("draw:stroke-linejoin", "miter", KoGenStyle::GraphicType);
        styleStroke.addProperty("calligra:stroke-miterlimit", QStringLiteral("%1").arg(pen.miterLimit()), KoGenStyle::GraphicType);
        break;
    }
    switch (pen.capStyle()) {
    case Qt::RoundCap:
        styleStroke.addProperty("svg:stroke-linecap", "round", KoGenStyle::GraphicType);
        break;
    case Qt::SquareCap:
        styleStroke.addProperty("svg:stroke-linecap", "square", KoGenStyle::GraphicType);
        break;
    default:
        styleStroke.addProperty("svg:stroke-linecap", "butt", KoGenStyle::GraphicType);
        break;
    }
}

QString KoOdfGraphicStyles::saveOdfHatchStyle(KoGenStyles& mainStyles, const QBrush &brush)
{
    KoGenStyle hatchStyle(KoGenStyle::HatchStyle /*no family name*/);
    hatchStyle.addAttribute("draw:color", brush.color().name());
    //hatchStyle.addAttribute("draw:distance", m_distance); not implemented into Stage
    switch (brush.style()) {
    case Qt::HorPattern:
        hatchStyle.addAttribute("draw:style", "single");
        hatchStyle.addAttribute("draw:rotation", 0);
        break;
    case Qt::BDiagPattern:
        hatchStyle.addAttribute("draw:style", "single");
        hatchStyle.addAttribute("draw:rotation", 450);
        break;
    case Qt::VerPattern:
        hatchStyle.addAttribute("draw:style", "single");
        hatchStyle.addAttribute("draw:rotation", 900);
        break;
    case Qt::FDiagPattern:
        hatchStyle.addAttribute("draw:style", "single");
        hatchStyle.addAttribute("draw:rotation", 1350);
        break;
    case Qt::CrossPattern:
        hatchStyle.addAttribute("draw:style", "double");
        hatchStyle.addAttribute("draw:rotation", 0);
        break;
    case Qt::DiagCrossPattern:
        hatchStyle.addAttribute("draw:style", "double");
        hatchStyle.addAttribute("draw:rotation", 450);
        break;
    default:
        break;
    }

    return mainStyles.insert(hatchStyle, QStringLiteral("hatch"));
}

QString KoOdfGraphicStyles::saveOdfGradientStyle(KoGenStyles &mainStyles, const QBrush &brush)
{
    KoGenStyle gradientStyle;
    if (brush.style() == Qt::RadialGradientPattern) {
        const QRadialGradient *gradient = static_cast<const QRadialGradient*>(brush.gradient());
        gradientStyle = KoGenStyle(KoGenStyle::RadialGradientStyle /*no family name*/);
        gradientStyle.addAttributePercent("svg:cx", gradient->center().x() * 100);
        gradientStyle.addAttributePercent("svg:cy", gradient->center().y() * 100);
        gradientStyle.addAttributePercent("svg:r",  gradient->radius() * 100);
        gradientStyle.addAttributePercent("svg:fx", gradient->focalPoint().x() * 100);
        gradientStyle.addAttributePercent("svg:fy", gradient->focalPoint().y() * 100);
    } else if (brush.style() == Qt::LinearGradientPattern) {
        const QLinearGradient *gradient = static_cast<const QLinearGradient*>(brush.gradient());
        gradientStyle = KoGenStyle(KoGenStyle::LinearGradientStyle /*no family name*/);
        gradientStyle.addAttributePercent("svg:x1", gradient->start().x() * 100);
        gradientStyle.addAttributePercent("svg:y1", gradient->start().y() * 100);
        gradientStyle.addAttributePercent("svg:x2", gradient->finalStop().x() * 100);
        gradientStyle.addAttributePercent("svg:y2", gradient->finalStop().y() * 100);
    } else if (brush.style() == Qt::ConicalGradientPattern) {
        const QConicalGradient * gradient = static_cast<const QConicalGradient*>(brush.gradient());
        gradientStyle = KoGenStyle(KoGenStyle::ConicalGradientStyle /*no family name*/);
        gradientStyle.addAttributePercent("svg:cx", gradient->center().x() * 100);
        gradientStyle.addAttributePercent("svg:cy", gradient->center().y() * 100);
        gradientStyle.addAttribute("draw:angle", QStringLiteral("%1").arg(gradient->angle()));
    }
    const QGradient * gradient = brush.gradient();
    if (gradient->spread() == QGradient::RepeatSpread)
        gradientStyle.addAttribute("svg:spreadMethod", "repeat");
    else if (gradient->spread() == QGradient::ReflectSpread)
        gradientStyle.addAttribute("svg:spreadMethod", "reflect");
    else
        gradientStyle.addAttribute("svg:spreadMethod", "pad");

    if (! brush.transform().isIdentity()) {
        gradientStyle.addAttribute("svg:gradientTransform", saveTransformation(brush.transform()));
    }

    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    KoXmlWriter elementWriter(&buffer);    // TODO pass indentation level

    // save stops
    const QGradientStops stops = gradient->stops();
    for (const QGradientStop & stop : stops) {
        elementWriter.startElement("svg:stop");
        elementWriter.addAttribute("svg:offset", QStringLiteral("%1").arg(stop.first));
        elementWriter.addAttribute("svg:stop-color", stop.second.name());
        if (stop.second.alphaF() < 1.0)
            elementWriter.addAttribute("svg:stop-opacity", QStringLiteral("%1").arg(stop.second.alphaF()));
        elementWriter.endElement();
    }

    QString elementContents = QString::fromUtf8(buffer.buffer().constData(), buffer.buffer().size());
    gradientStyle.addChildElement(QStringLiteral("svg:stop"), elementContents);

    return mainStyles.insert(gradientStyle, QStringLiteral("gradient"));
}

QBrush KoOdfGraphicStyles::loadOdfGradientStyle(const KoStyleStack &styleStack, const KoOdfStylesReader & stylesReader, const QSizeF &size)
{
    QString styleName = styleStack.property(KoXmlNS::draw, "fill-gradient-name");
    return loadOdfGradientStyleByName(stylesReader, styleName, size);
}

qreal percent(const KoXmlElement &element, const QString &ns, const char *type, const char *defaultValue, qreal absolute)
{
    qreal tmp = 0.0;
    QString value = element.attributeNS(ns, type, defaultValue);
    if (value.indexOf(QLatin1Char('%')) > -1) { // percent value
        tmp = (qreal)(value.remove(QLatin1Char('%')).toDouble()) / 100.0;
    }
    else { // fixed value
        tmp = KoUnit::parseValue(value) / absolute;
        // The following is done so that we get the same data as when we save/load.
        // This is needed that we get the same values due to rounding differences
        // of absolute and relative values.
        QString value = QStringLiteral("%1").arg(tmp * 100.0);
        tmp = value.toDouble() / 100;
    }

    return tmp;
}

QBrush KoOdfGraphicStyles::loadOdfGradientStyleByName(const KoOdfStylesReader &stylesReader, const QString &styleName, const QSizeF &size)
{
    KoXmlElement* e = stylesReader.drawStyles("gradient").value(styleName);
    if (! e)
        return QBrush();

    QGradient * gradient = nullptr;
    QTransform transform;

    if (e->namespaceURI() == KoXmlNS::draw && e->localName() == QStringLiteral("gradient")) {
        // FIXME seems like oo renders the gradient start stop color at the center of the
        // radial gradient, and the start color at the radius of the radial gradient
        // whereas it is not mentioned in the spec how it should be rendered
        // note that svg defines that exactly as the opposite as oo does
        // so what should we do?
        QString type = e->attributeNS(KoXmlNS::draw, "style", QString());
        if (type == QStringLiteral("radial")) {
            // Zagge: at the moment the only objectBoundingBox is supported:
            // 18.539 svg:gradientUnits
            // See §13.2.2 and §13.2.3 of [SVG].
            // The default value for this attribute is objectBoundingBox.
            // The only value of the svg:gradientUnits attribute is objectBoundingBox.

            qreal cx = KoUnit::parseValue(e->attributeNS(KoXmlNS::draw, "cx", QString()).remove(QLatin1Char('%')));
            qreal cy = KoUnit::parseValue(e->attributeNS(KoXmlNS::draw, "cy", QString()).remove(QLatin1Char('%')));
            gradient = new QRadialGradient(QPointF(cx * 0.01, cy * 0.01), sqrt(0.5));
            gradient->setCoordinateMode(QGradient::ObjectBoundingMode);
        } else if (type == QStringLiteral("linear") || type == QStringLiteral("axial")) {
            QLinearGradient * lg = new QLinearGradient();
            lg->setCoordinateMode(QGradient::ObjectBoundingMode);
            // Dividing by 10 here because OOo saves as degree * 10
            qreal angle = 90 + e->attributeNS(KoXmlNS::draw, "angle", QString::number(0.0)).toDouble() / 10;
            qreal radius = sqrt(0.5);

            qreal sx = cos(angle * M_PI / 180) * radius;
            qreal sy = sin(angle * M_PI / 180) * radius;
            lg->setStart(QPointF(0.5 + sx, 0.5 - sy));
            lg->setFinalStop(QPointF(0.5 - sx, 0.5 + sy));
            gradient = lg;
        } else
            return QBrush();

        qreal border = 0.01 * e->attributeNS(KoXmlNS::draw, "border", QString::number(0.0)).remove(QLatin1Char('%')).toDouble();
        QGradientStops stops;
        if (type != QStringLiteral("axial")) {
            // In case of radial gradients the colors are reversed, because OOo saves them as the opposite of the SVG direction
            // see bug 137639
            QGradientStop start;
            start.first = (type != QStringLiteral("radial")) ? border : 1.0 - border;
            start.second = QColor(e->attributeNS(KoXmlNS::draw, "start-color", QString()));
            start.second.setAlphaF(0.01 * e->attributeNS(KoXmlNS::draw, "start-intensity", QString::number(100.0)).remove(QLatin1Char('%')).toDouble());

            QGradientStop end;
            end.first = (type != QStringLiteral("radial")) ? 1.0 : 0.0;
            end.second = QColor(e->attributeNS(KoXmlNS::draw, "end-color", QString()));
            end.second.setAlphaF(0.01 * e->attributeNS(KoXmlNS::draw, "end-intensity", QString::number(100.0)).remove(QLatin1Char('%')).toDouble());

            stops << start << end;
        } else {
            QGradientStop start;
            start.first = 0.5 * border;
            start.second = QColor(e->attributeNS(KoXmlNS::draw, "end-color", QString()));
            start.second.setAlphaF(0.01 * e->attributeNS(KoXmlNS::draw, "end-intensity", QString::number(100.0)).remove(QLatin1Char('%')).toDouble());

            QGradientStop middle;
            middle.first = 0.5;
            middle.second = QColor(e->attributeNS(KoXmlNS::draw, "start-color", QString()));
            middle.second.setAlphaF(0.01 * e->attributeNS(KoXmlNS::draw, "start-intensity", QString::number(100.0)).remove(QLatin1Char('%')).toDouble());

            QGradientStop end;
            end.first = 1.0 - 0.5 * border;
            end.second = QColor(e->attributeNS(KoXmlNS::draw, "end-color", QString()));
            end.second.setAlphaF(0.01 * e->attributeNS(KoXmlNS::draw, "end-intensity", QString::number(100.0)).remove(QLatin1Char('%')).toDouble());

            stops << start << middle << end;
        }

        gradient->setStops(stops);
    } else if (e->namespaceURI() == KoXmlNS::svg) {
        if (e->localName() == QStringLiteral("linearGradient")) {
            QPointF start, stop;
            start.setX(percent(*e, KoXmlNS::svg, "x1", "0%", size.width()));
            start.setY(percent(*e, KoXmlNS::svg, "y1", "0%", size.height()));
            stop.setX(percent(*e, KoXmlNS::svg, "x2", "100%", size.width()));
            stop.setY(percent(*e, KoXmlNS::svg, "y2", "100%", size.height()));
            gradient = new QLinearGradient(start, stop);
        } else if (e->localName() == QStringLiteral("radialGradient")) {
            QPointF center, focalPoint;
            center.setX(percent(*e, KoXmlNS::svg, "cx", "50%", size.width()));
            center.setY(percent(*e, KoXmlNS::svg, "cy", "50%", size.height()));
            qreal r = percent(*e, KoXmlNS::svg, "r", "50%", sqrt(size.width() * size.width() + size.height() * size.height()));
            focalPoint.setX(percent(*e, KoXmlNS::svg, "fx", nullptr, size.width()));
            focalPoint.setY(percent(*e, KoXmlNS::svg, "fy", nullptr, size.height()));
            gradient = new QRadialGradient(center, r, focalPoint);
        }
        if (! gradient)
            return QBrush();

        gradient->setCoordinateMode(QGradient::ObjectBoundingMode);

        QString strSpread(e->attributeNS(KoXmlNS::svg, "spreadMethod", "pad"));
        if (strSpread == QStringLiteral("repeat"))
            gradient->setSpread(QGradient::RepeatSpread);
        else if (strSpread == QStringLiteral("reflect"))
            gradient->setSpread(QGradient::ReflectSpread);
        else
            gradient->setSpread(QGradient::PadSpread);

        if (e->hasAttributeNS(KoXmlNS::svg, "gradientTransform"))
            transform = loadTransformation(e->attributeNS(KoXmlNS::svg, "gradientTransform", QString()));

        QGradientStops stops;

        // load stops
        KoXmlElement colorstop;
        forEachElement(colorstop, (*e)) {
            if (colorstop.namespaceURI() == KoXmlNS::svg && colorstop.localName() == QStringLiteral("stop")) {
                QGradientStop stop;
                stop.second = QColor(colorstop.attributeNS(KoXmlNS::svg, "stop-color", QString()));
                stop.second.setAlphaF(colorstop.attributeNS(KoXmlNS::svg, "stop-opacity", "1.0").toDouble());
                stop.first = colorstop.attributeNS(KoXmlNS::svg, "offset", "0.0").toDouble();
                stops.append(stop);
            }
        }
        gradient->setStops(stops);
    } else if (e->namespaceURI() == KoXmlNS::calligra) {
        if (e->localName() == QStringLiteral("conicalGradient")) {
            QPointF center;
            center.setX(percent(*e, KoXmlNS::svg, "cx", "50%", size.width()));
            center.setY(percent(*e, KoXmlNS::svg, "cy", "50%", size.height()));
            qreal angle = KoUnit::parseValue(e->attributeNS(KoXmlNS::draw, "angle", QString()));
            gradient = new QConicalGradient(center, angle);
            gradient->setCoordinateMode(QGradient::ObjectBoundingMode);

            QString strSpread(e->attributeNS(KoXmlNS::svg, "spreadMethod", "pad"));
            if (strSpread == QStringLiteral("repeat"))
                gradient->setSpread(QGradient::RepeatSpread);
            else if (strSpread == QStringLiteral("reflect"))
                gradient->setSpread(QGradient::ReflectSpread);
            else
                gradient->setSpread(QGradient::PadSpread);

            if (e->hasAttributeNS(KoXmlNS::svg, "gradientTransform"))
                transform = loadTransformation(e->attributeNS(KoXmlNS::svg, "gradientTransform", QString()));

            QGradientStops stops;

            // load stops
            KoXmlElement colorstop;
            forEachElement(colorstop, (*e)) {
                if (colorstop.namespaceURI() == KoXmlNS::svg && colorstop.localName() == QStringLiteral("stop")) {
                    QGradientStop stop;
                    stop.second = QColor(colorstop.attributeNS(KoXmlNS::svg, "stop-color", QString()));
                    stop.second.setAlphaF(colorstop.attributeNS(KoXmlNS::svg, "stop-opacity", "1.0").toDouble());
                    stop.first = colorstop.attributeNS(KoXmlNS::svg, "offset", "0.0").toDouble();
                    stops.append(stop);
                }
            }
            gradient->setStops(stops);
        }
    }

    if (! gradient)
        return QBrush();

    QBrush resultBrush(*gradient);
    resultBrush.setTransform(transform);

    delete gradient;
    return resultBrush;
}

QBrush KoOdfGraphicStyles::loadOdfFillStyle(const KoStyleStack &styleStack, const QString & fill,  const KoOdfStylesReader & stylesReader)
{
    QBrush tmpBrush; // default brush for "none" is a Qt::NoBrush

    if (fill == QStringLiteral("solid")) {
        tmpBrush.setStyle(Qt::SolidPattern);
        if (styleStack.hasProperty(KoXmlNS::draw, "fill-color"))
            tmpBrush.setColor(styleStack.property(KoXmlNS::draw, "fill-color"));
        if (styleStack.hasProperty(KoXmlNS::draw, "opacity")) {
            QString opacity = styleStack.property(KoXmlNS::draw, "opacity");
            if (! opacity.isEmpty() && opacity.right(1) == QLatin1Char('%')) {
                float percent = opacity.left(opacity.length() - 1).toFloat();
                QColor color = tmpBrush.color();
                color.setAlphaF(percent / 100.0);
                tmpBrush.setColor(color);
            }
        }
        //TODO
        if (styleStack.hasProperty(KoXmlNS::draw, "transparency")) {
            QString transparency = styleStack.property(KoXmlNS::draw, "transparency");
            if (transparency == QStringLiteral("94%")) {
                tmpBrush.setStyle(Qt::Dense1Pattern);
            } else if (transparency == QStringLiteral("88%")) {
                tmpBrush.setStyle(Qt::Dense2Pattern);
            } else if (transparency == QStringLiteral("63%")) {
                tmpBrush.setStyle(Qt::Dense3Pattern);

            } else if (transparency == QStringLiteral("50%")) {
                tmpBrush.setStyle(Qt::Dense4Pattern);

            } else if (transparency == QStringLiteral("37%")) {
                tmpBrush.setStyle(Qt::Dense5Pattern);

            } else if (transparency == QStringLiteral("12%")) {
                tmpBrush.setStyle(Qt::Dense6Pattern);

            } else if (transparency == QStringLiteral("6%")) {
                tmpBrush.setStyle(Qt::Dense7Pattern);

            } else
                debugOdf << " transparency is not defined into Stage :" << transparency;
        }
    } else if (fill == QStringLiteral("hatch")) {
        QString style = styleStack.property(KoXmlNS::draw, "fill-hatch-name");
        debugOdf << " hatch style is  :" << style;

        //type not defined by default
        //try to use style.
        KoXmlElement* draw = stylesReader.drawStyles("hatch").value(style);
        if (draw) {
            debugOdf << "We have a style";
            int angle = 0;
            if (draw->hasAttributeNS(KoXmlNS::draw, "rotation")) {
                angle = (draw->attributeNS(KoXmlNS::draw, "rotation", QString()).toInt()) / 10;
                debugOdf << "angle :" << angle;
            }
            if (draw->hasAttributeNS(KoXmlNS::draw, "color")) {
                //debugOdf<<" draw:color :"<<draw->attributeNS(KoXmlNS::draw,"color", QString());
                tmpBrush.setColor(draw->attributeNS(KoXmlNS::draw, "color", QString()));
            }
            if (draw->hasAttributeNS(KoXmlNS::draw, "distance")) {
                //todo implement it into Stage
            }
            if (draw->hasAttributeNS(KoXmlNS::draw, "display-name")) {
                //todo implement it into Stage
            }
            if (draw->hasAttributeNS(KoXmlNS::draw, "style")) {
                //todo implement it into Stage
                QString styleHash = draw->attributeNS(KoXmlNS::draw, "style", QString());
                if (styleHash == QStringLiteral("single")) {
                    switch (angle) {
                    case 0:
                    case 180:
                        tmpBrush.setStyle(Qt::HorPattern);
                        break;
                    case 45:
                    case 225:
                        tmpBrush.setStyle(Qt::BDiagPattern);
                        break;
                    case 90:
                    case 270:
                        tmpBrush.setStyle(Qt::VerPattern);
                        break;
                    case 135:
                    case 315:
                        tmpBrush.setStyle(Qt::FDiagPattern);
                        break;
                    default:
                        //todo fixme when we will have a kopaint
                        debugOdf << " draw:rotation 'angle' :" << angle;
                        break;
                    }
                } else if (styleHash == QStringLiteral("double")) {
                    switch (angle) {
                    case 0:
                    case 180:
                    case 90:
                    case 270:
                        tmpBrush.setStyle(Qt::CrossPattern);
                        break;
                    case 45:
                    case 135:
                    case 225:
                    case 315:
                        tmpBrush.setStyle(Qt::DiagCrossPattern);
                        break;
                    default:
                        //todo fixme when we will have a kopaint
                        debugOdf << " draw:rotation 'angle' :" << angle;
                        break;
                    }

                } else if (styleHash == QStringLiteral("triple")) {
                    debugOdf << " it is not implemented :(";
                }
            }
        }
    }

    return tmpBrush;
}

static qreal parseDashEntrySize(QString& attr, qreal penWidth, qreal defaultValue = 0.0){
    qreal result = defaultValue;
    if (attr.endsWith(QLatin1Char('%'))) {
        bool ok;
        const int percent = attr.remove(QLatin1Char('%')).toInt(&ok);
        if (ok && percent >= 0) {
            result = percent / 100.0;
        }
    } else {
        result = KoUnit::parseValue(attr) / penWidth;
    }
    return result;
}

QPen KoOdfGraphicStyles::loadOdfStrokeStyle(const KoStyleStack &styleStack, const QString & stroke, const KoOdfStylesReader & stylesReader)
{
    QPen tmpPen(Qt::NoPen); // default pen for "none" is a Qt::NoPen

    if (stroke == QStringLiteral("solid") || stroke == QStringLiteral("dash")) {
        // If solid or dash is set then we assume that the color is black and the penWidth
        // is zero till defined otherwise with the following attributes.
        tmpPen = QPen();

        if (styleStack.hasProperty(KoXmlNS::svg, "stroke-color"))
            tmpPen.setColor(styleStack.property(KoXmlNS::svg, "stroke-color"));
        if (styleStack.hasProperty(KoXmlNS::svg, "stroke-opacity")) {
            QColor color = tmpPen.color();
            QString opacity = styleStack.property(KoXmlNS::svg, "stroke-opacity");
            if (opacity.endsWith(QLatin1Char('%')))
                color.setAlphaF(0.01 * opacity.remove(QLatin1Char('%')).toDouble());
            else
                color.setAlphaF(opacity.toDouble());
            tmpPen.setColor(color);
        }
        if (styleStack.hasProperty(KoXmlNS::svg, "stroke-width"))
            tmpPen.setWidthF(KoUnit::parseValue(styleStack.property(KoXmlNS::svg, "stroke-width")));
        if (styleStack.hasProperty(KoXmlNS::draw, "stroke-linejoin")) {
            QString join = styleStack.property(KoXmlNS::draw, "stroke-linejoin");
            if (join == QStringLiteral("bevel"))
                tmpPen.setJoinStyle(Qt::BevelJoin);
            else if (join == QStringLiteral("round"))
                tmpPen.setJoinStyle(Qt::RoundJoin);
            else {
                tmpPen.setJoinStyle(Qt::MiterJoin);
                if (styleStack.hasProperty(KoXmlNS::calligra, "stroke-miterlimit")) {
                    QString miterLimit = styleStack.property(KoXmlNS::calligra, "stroke-miterlimit");
                    tmpPen.setMiterLimit(miterLimit.toDouble());
                }
            }
        }
        if (styleStack.hasProperty(KoXmlNS::svg, "stroke-linecap")) {
            const QString cap = styleStack.property(KoXmlNS::svg, "stroke-linecap");
            if (cap == QStringLiteral("round"))
                tmpPen.setCapStyle(Qt::RoundCap);
            else if (cap == QStringLiteral("square"))
                tmpPen.setCapStyle(Qt::SquareCap);
            else
                tmpPen.setCapStyle(Qt::FlatCap);
        } else {
            // default as per svg specification
            tmpPen.setCapStyle(Qt::FlatCap);
        }

        if (stroke == QStringLiteral("dash") && styleStack.hasProperty(KoXmlNS::draw, "stroke-dash")) {
            QString dashStyleName = styleStack.property(KoXmlNS::draw, "stroke-dash");

            // set width to 1 in case it is 0 as dividing by 0 gives infinity
            qreal width = tmpPen.widthF();
            if (width == 0) {
                width = 1;
            }

            KoXmlElement * dashElement = stylesReader.drawStyles("stroke-dash").value(dashStyleName);
            if (dashElement) {
                QVector<qreal> dashes;
                if (dashElement->hasAttributeNS(KoXmlNS::draw, "dots1")) {
                    QString distance(dashElement->attributeNS(KoXmlNS::draw, "distance", QString()));
                    qreal space = parseDashEntrySize(distance, width, 0.0);

                    QString dots1Length(dashElement->attributeNS(KoXmlNS::draw, "dots1-length", QString()));
                    qreal dot1Length = parseDashEntrySize(dots1Length,width,1.0);

                    bool ok;
                    int dots1 = dashElement->attributeNS(KoXmlNS::draw, "dots1").toInt(&ok);
                    if (!ok) {
                        dots1 = 1;
                    }

                    for (int i = 0; i < dots1; i++) {
                        dashes.append(dot1Length);
                        dashes.append(space);
                    }

                    if (dashElement->hasAttributeNS(KoXmlNS::draw, "dots2")) {
                        QString dots2Length(dashElement->attributeNS(KoXmlNS::draw, "dots2-length", QString()));
                        qreal dot2Length = parseDashEntrySize(dots2Length,width,1.0);

                        int dots2 = dashElement->attributeNS(KoXmlNS::draw, "dots2").toInt(&ok);
                        if (!ok) {
                            dots2 = 1;
                        }

                        for (int i = 0; i < dots2; i++) {
                            dashes.append(dot2Length);
                            dashes.append(space);
                        }
                    }
                    tmpPen.setDashPattern(dashes);
                }
            }
        }
    }

    return tmpPen;
}

QTransform KoOdfGraphicStyles::loadTransformation(const QString &transformation)
{
    QTransform transform;

    // Split string for handling 1 transform statement at a time
    QStringList subtransforms = transformation.split(QLatin1Char(')'), Qt::SkipEmptyParts);
    QStringList::ConstIterator it = subtransforms.constBegin();
    QStringList::ConstIterator end = subtransforms.constEnd();
    for (; it != end; ++it) {
        QStringList subtransform = (*it).split(QLatin1Char('('), Qt::SkipEmptyParts);

        subtransform[0] = subtransform[0].trimmed().toLower();
        subtransform[1] = subtransform[1].simplified();
        QRegularExpression reg(QStringLiteral("[,(]"));
        QStringList params = subtransform[1].split(reg, Qt::SkipEmptyParts);

        if (subtransform[0].startsWith(QLatin1Char(';')) || subtransform[0].startsWith(QLatin1Char(',')))
            subtransform[0] = subtransform[0].right(subtransform[0].length() - 1);

        if (subtransform[0] == QStringLiteral("rotate")) {
            // TODO find out what oo2 really does when rotating, it seems severely broken
            if (params.count() == 3) {
                qreal x = KoUnit::parseValue(params[1]);
                qreal y = KoUnit::parseValue(params[2]);

                transform.translate(x, y);
                // oo2 rotates by radians
                transform.rotate(params[0].toDouble()*180.0 / M_PI);
                transform.translate(-x, -y);
            } else {
                // oo2 rotates by radians
                transform.rotate(params[0].toDouble()*180.0 / M_PI);
            }
        } else if (subtransform[0] == QStringLiteral("translate")) {
            if (params.count() == 2) {
                qreal x = KoUnit::parseValue(params[0]);
                qreal y = KoUnit::parseValue(params[1]);
                transform.translate(x, y);
            } else   // Spec : if only one param given, assume 2nd param to be 0
                transform.translate(KoUnit::parseValue(params[0]) , 0);
        } else if (subtransform[0] == QStringLiteral("scale")) {
            if (params.count() == 2)
                transform.scale(params[0].toDouble(), params[1].toDouble());
            else    // Spec : if only one param given, assume uniform scaling
                transform.scale(params[0].toDouble(), params[0].toDouble());
        } else if (subtransform[0] == QStringLiteral("skewx"))
            transform.shear(tan(params[0].toDouble()), 0.0F);
        else if (subtransform[0] == QStringLiteral("skewy"))
            transform.shear(tan(params[0].toDouble()), 0.0F);
        else if (subtransform[0] == QStringLiteral("matrix")) {
            if (params.count() >= 6) {
                transform.setMatrix(params[0].toDouble(), params[1].toDouble(), 0,
                    params[2].toDouble(), params[3].toDouble(), 0,
                    KoUnit::parseValue(params[4]), KoUnit::parseValue(params[5]), 1);
            }
        }
    }

    return transform;
}

QString KoOdfGraphicStyles::saveTransformation(const QTransform &transformation, bool appendTranslateUnit)
{
    QString transform;
    if (appendTranslateUnit)
        transform = QStringLiteral("matrix(%1 %2 %3 %4 %5pt %6pt)")
                    .arg(transformation.m11()).arg(transformation.m12())
                    .arg(transformation.m21()).arg(transformation.m22())
                    .arg(transformation.dx()) .arg(transformation.dy());
    else
        transform = QStringLiteral("matrix(%1 %2 %3 %4 %5 %6)")
                    .arg(transformation.m11()).arg(transformation.m12())
                    .arg(transformation.m21()).arg(transformation.m22())
                    .arg(transformation.dx()) .arg(transformation.dy());

    return transform;
}

