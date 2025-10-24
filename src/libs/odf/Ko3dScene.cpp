/* This file is part of the KDE project
 *
 * SPDX-FileCopyrightText: 2012 Inge Wallin <inge@lysator.liu.se>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// Own
// clazy:excludeall=qstring-arg
#include "Ko3dScene.h"

#include "OdfDebug.h"

// Calligra
#include <KoXmlReader.h>
#include <KoXmlNS.h>
#include <KoXmlWriter.h>


static QVector3D odfToVector3D(const QString &string);



// ----------------------------------------------------------------
//                         Ko3dScene::Private


class Q_DECL_HIDDEN Ko3dScene::Private
{
public:
    Private() {};
    ~Private() {};

    // Camera attributes
    QVector3D vrp;          // Camera origin
    QVector3D vpn;          // Camera direction
    QVector3D vup;          // Up direction
    Projection projection;
    QString distance;
    QString focalLength;
    QString shadowSlant;

    // Rendering attributes
    Shademode shadeMode;
    QColor ambientColor;
    bool lightingMode; // True: enable lights, false: disable lights
    QString transform;

    // Lightsources (these are children of the element, not attributes)
    QVector<Lightsource> lights;
};


// ----------------------------------------------------------------
//                         Lightsource


Ko3dScene::Lightsource::Lightsource()
    : m_enabled(false)
    , m_specular(false)
{
}

Ko3dScene::Lightsource::~Lightsource()
{
}


// getters
QColor Ko3dScene::Lightsource::diffuseColor() const { return m_diffuseColor; }
QVector3D Ko3dScene::Lightsource::direction() const { return m_direction; }
bool Ko3dScene::Lightsource::enabled() const { return m_enabled; }
bool Ko3dScene::Lightsource::specular() const { return m_specular; }

// setters
void
Ko3dScene::Lightsource::setDiffuseColor(const QColor &color)
{
    m_diffuseColor = color;
}

void
Ko3dScene::Lightsource::setDirection(const QVector3D &direction)
{
    m_direction = direction;
}

void
Ko3dScene::Lightsource::setEnabled(const bool enabled)
{
    m_enabled = enabled;
}

void
Ko3dScene::Lightsource::setSpecular(const bool specular)
{
    m_specular = specular;
}


bool Ko3dScene::Lightsource::loadOdf(const KoXmlElement &lightElement)
{
    m_diffuseColor = QColor(lightElement.attributeNS(KoXmlNS::dr3d, "diffuse-color", "#ffffff"));
    QString direction = lightElement.attributeNS(KoXmlNS::dr3d, "direction");
    m_direction = odfToVector3D(direction);
    m_enabled = (lightElement.attributeNS(KoXmlNS::dr3d, "enabled") == QStringLiteral("true"));
    m_specular = (lightElement.attributeNS(KoXmlNS::dr3d, "specular") == QStringLiteral("true"));

    return true;
}

void Ko3dScene::Lightsource::saveOdf(KoXmlWriter &writer) const
{
    writer.startElement("dr3d:light");

    writer.addAttribute("dr3d:diffuse-color", m_diffuseColor.name());
    writer.addAttribute("dr3d:direction", (QStringLiteral("(%1 %2 %3)")
                                           .arg(m_direction.x(), 0, 'f', 11)
                                           .arg(m_direction.y(), 0, 'f', 11)
                                           .arg(m_direction.z(), 0, 'f', 11)));
    writer.addAttribute("dr3d:enabled", m_enabled);
    writer.addAttribute("dr3d:specular", m_specular);

    writer.endElement(); // dr3d:light
}


// ----------------------------------------------------------------
//                         Ko3dScene


Ko3dScene::Ko3dScene()
    : d(new Private())
{
}

Ko3dScene::~Ko3dScene()
{
    delete d;
}


// getters
QVector3D Ko3dScene::vrp() const { return d->vrp; }
QVector3D Ko3dScene::vpn() const { return d->vpn; }
QVector3D Ko3dScene::vup() const { return d->vup; }
Ko3dScene::Projection  Ko3dScene::projection() const { return d->projection; }
QString Ko3dScene::distance() const { return d->distance; }
QString Ko3dScene::focalLength() const { return d->focalLength; }
QString Ko3dScene::shadowSlant() const { return d->shadowSlant; }
Ko3dScene::Shademode   Ko3dScene::shadeMode() const { return d->shadeMode; }
QColor Ko3dScene::ambientColor() const { return d->ambientColor; }
bool Ko3dScene::lightingMode() const { return d->lightingMode; }
QString Ko3dScene::transform() const { return d->transform; }

    // setters
void Ko3dScene::setVrp(const QVector3D &vrp) { d->vrp = vrp; }
void Ko3dScene::setVpn(const QVector3D &vpn) { d->vpn = vpn; }
void Ko3dScene::setVup(const QVector3D &vup) { d->vup = vup; }
void Ko3dScene::setProjection(Projection projection) { d->projection = projection; }
void Ko3dScene::setDistance(const QString &distance) { d->distance = distance; }
void Ko3dScene::setFocalLength(const QString &focalLength) { d->focalLength = focalLength; }
void Ko3dScene::setShadowSlant(const QString &shadowSlant) { d->shadowSlant = shadowSlant; }
void Ko3dScene::setShadeMode(Shademode shadeMode) { d->shadeMode = shadeMode; }
void Ko3dScene::setAmbientColor(const QColor &ambientColor) { d->ambientColor = ambientColor; }
void Ko3dScene::setLightingMode(bool lightingMode) { d->lightingMode = lightingMode; }
void Ko3dScene::setTransform(const QString &transform) { d->transform = transform; }


bool Ko3dScene::loadOdf(const KoXmlElement &sceneElement)
{
    QString dummy;

    // Check if there is a 3d scene at all in this element. We
    // approximate that by checking if there are any camera parameters.
    if (!sceneElement.hasAttributeNS(KoXmlNS::dr3d, "vrp")
        && !sceneElement.hasAttributeNS(KoXmlNS::dr3d, "vpn")
        && !sceneElement.hasAttributeNS(KoXmlNS::dr3d, "vup"))
    {
        return false;
    }

    // 1. Load the scene attributes.

    // Camera attributes
    dummy = sceneElement.attributeNS(KoXmlNS::dr3d, "vrp");
    d->vrp = odfToVector3D(dummy);
    dummy = sceneElement.attributeNS(KoXmlNS::dr3d, "vpn");
    d->vpn = odfToVector3D(dummy);
    dummy = sceneElement.attributeNS(KoXmlNS::dr3d, "vup", "(0.0 0.0 1.0)");
    d->vup = odfToVector3D(dummy);

    dummy = sceneElement.attributeNS(KoXmlNS::dr3d, "projection", "perspective");
    if (dummy == QStringLiteral("parallel")) {
        d->projection = Parallel;
    }
    else {
        d->projection = Perspective;
    }

    d->distance = sceneElement.attributeNS(KoXmlNS::dr3d, "distance");
    d->focalLength = sceneElement.attributeNS(KoXmlNS::dr3d, "focal-length");
    d->shadowSlant = sceneElement.attributeNS(KoXmlNS::dr3d, "shadow-slant");
    d->ambientColor = QColor(sceneElement.attributeNS(KoXmlNS::dr3d, "ambient-color", "#888888"));

    // Rendering attributes
    dummy = sceneElement.attributeNS(KoXmlNS::dr3d, "shade-mode", "gouraud");
    if (dummy == QStringLiteral("flat")) {
        d->shadeMode = Flat;
    }
    else if (dummy == QStringLiteral("phong")) {
        d->shadeMode = Phong;
    }
    else if (dummy == QStringLiteral("draft")) {
        d->shadeMode = Draft;
    }
    else {
        d->shadeMode = Gouraud;
    }

    d->lightingMode = (sceneElement.attributeNS(KoXmlNS::dr3d, "lighting-mode") == QStringLiteral("true"));
    d->transform = sceneElement.attributeNS(KoXmlNS::dr3d, "transform");

    // 2. Load the light sources.

    // From the ODF 1.1 spec section 9.4.1:
    KoXmlElement elem;
    forEachElement(elem, sceneElement) {
        if (elem.localName() == QStringLiteral("light") && elem.namespaceURI() == KoXmlNS::dr3d) {
            Lightsource  light;
            light.loadOdf(elem);
            d->lights.append(light);
        }
    }

    //debugOdf << "Lights:" << d->lights.size();

    return true;
}

void Ko3dScene::saveOdfAttributes(KoXmlWriter &writer) const
{
    // 1. Write scene attributes
    // Camera attributes
    writer.addAttribute("dr3d:vrp", (QStringLiteral("(%1 %2 %3)")
                                     .arg(d->vrp.x(), 0, 'f', 11)
                                     .arg(d->vrp.y(), 0, 'f', 11)
                                     .arg(d->vrp.z(), 0, 'f', 11)));
    writer.addAttribute("dr3d:vpn", (QStringLiteral("(%1 %2 %3)")
                                     .arg(d->vpn.x(), 0, 'f', 11)
                                     .arg(d->vpn.y(), 0, 'f', 11)
                                     .arg(d->vpn.z(), 0, 'f', 11)));
    writer.addAttribute("dr3d:vup", (QStringLiteral("(%1 %2 %3)")
                                     .arg(d->vup.x(), 0, 'f', 11)
                                     .arg(d->vup.y(), 0, 'f', 11)
                                     .arg(d->vup.z(), 0, 'f', 11)));

    writer.addAttribute("dr3d:projection", (d->projection == Parallel) ? "parallel" : "perspective");

    writer.addAttribute("dr3d:distance", d->distance);
    writer.addAttribute("dr3d:focal-length", d->focalLength);
    writer.addAttribute("dr3d:shadow-slant", d->shadowSlant);
    writer.addAttribute("dr3d:ambient-color", d->ambientColor.name());

    // Rendering attributes
    switch (d->shadeMode) {
    case Flat:
        writer.addAttribute("dr3d:shade-mode", "flat");
        break;
    case Phong:
        writer.addAttribute("dr3d:shade-mode", "phong");
        break;
    case Draft:
        writer.addAttribute("dr3d:shade-mode", "draft");
        break;
    case Gouraud:
    default:
        writer.addAttribute("dr3d:shade-mode", "gouraud");
        break;
    }

    writer.addAttribute("dr3d:lighting-mode", d->lightingMode);
    writer.addAttribute("dr3d:transform", d->transform);
}


void Ko3dScene::saveOdfChildren(KoXmlWriter &writer) const
{
    // Write light sources.
    for (const Lightsource &light : std::as_const(d->lights)) {
        light.saveOdf(writer);
    }
}


// ----------------------------------------------------------------
//                         Public functions


KOODF_EXPORT Ko3dScene *load3dScene(const KoXmlElement &element)
{
    Ko3dScene *scene = new Ko3dScene();

    if (scene->loadOdf(element)) {
        return scene;
    }

    delete scene;
    return nullptr;
}


// ----------------------------------------------------------------
//                         Static functions


QVector3D odfToVector3D(const QString &string)
{
    // The string comes into this function in the form "(0 3.5 0.3)".
    QStringList elements = string.mid(1, string.size() - 2).split(QLatin1Char(' '), Qt::SkipEmptyParts);
    if (elements.size() == 3) {
        return QVector3D(elements[0].toDouble(), elements[1].toDouble(), elements[2].toDouble());
    }
    else {
        return QVector3D(0, 0, 1);
    }
}
