/***************************************************************************
                          qgspointlightsfromlayersettings.h
                          ---------------
    begin                : April 2022
    copyright            : (C) 2022 by Nyall Dawson
    email                : nyall dot dawson at gmail dot com
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSPOINTLIGHTSFROMLAYERSETTINGS_H
#define QGSPOINTLIGHTSFROMLAYERSETTINGS_H

#include "qgis_3d.h"

#include "qgsvector3d.h"
#include "qgslightsource.h"
#include "qgsvectorlayerref.h"
#include <QColor>

class QDomDocument;
class QDomElement;
class QgsVectorLayer;

/**
 * \ingroup 3d
 * \brief A light source where point lights are created corresponding to each feature in a point vector layer.
 *
 * \since QGIS 3.26
 */
class _3D_EXPORT QgsPointLightsFromLayerSettings : public QgsLightSource
{
  public:
    //! Construct a point light with default values
    QgsPointLightsFromLayerSettings() = default;

    QList<Qt3DCore::QEntity *> createEntities( const Qgs3DMapSettings &map, Qt3DCore::QEntity *parent ) const override SIP_SKIP;
    QDomElement writeXml( QDomDocument &doc, const QgsReadWriteContext &context = QgsReadWriteContext() ) const override;
    void readXml( const QDomElement &elem, const QgsReadWriteContext &context = QgsReadWriteContext() ) override;
    void resolveReferences( const QgsProject &project ) override;

    /**
     * Returns the source layer used for the light positions.
     * \see setSourceLayer()
     */
    QgsVectorLayer *sourceLayer() const { return mSourceLayer.get(); }

    /**
     * Sets the source \a layer to use for the light positions.
     *
     * The \a layer argument must be a point vector layer.
     *
     * \see sourceLayer()
     */
    void setSourceLayer( QgsVectorLayer *layer );

    //! Returns color of the light
    QColor color() const { return mColor; }
    //! Sets color of the light
    void setColor( const QColor &color ) { mColor = color; }

    //! Returns intensity of the light
    float intensity() const { return mIntensity; }
    //! Sets intensity of the light
    void setIntensity( float intensity ) { mIntensity = intensity; }

    //! Returns constant attenuation (A_0)
    float constantAttenuation() const { return mConstantAttenuation; }
    //! Sets constant attenuation (A_0)
    void setConstantAttenuation( float value ) { mConstantAttenuation = value; }

    //! Returns linear attenuation (A_1)
    float linearAttenuation() const { return mLinearAttenuation; }
    //! Sets linear attenuation (A_1)
    void setLinearAttenuation( float value ) { mLinearAttenuation = value; }

    //! Returns quadratic attenuation (A_2)
    float quadraticAttenuation() const { return mQuadraticAttenuation; }
    //! Sets quadratic attenuation (A_2)
    void setQuadraticAttenuation( float value ) { mQuadraticAttenuation = value; }

    // TODO c++20 - replace with = default
    bool operator==( const QgsPointLightsFromLayerSettings &other );

  private:
    QgsVectorLayerRef mSourceLayer;

    QColor mColor = Qt::white;
    float mIntensity = 1.0;
    float mConstantAttenuation = 1.0f;
    float mLinearAttenuation = 0.0f;
    float mQuadraticAttenuation = 0.0f;
};

#endif // QGSPOINTLIGHTSFROMLAYERSETTINGS_H
