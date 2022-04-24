/***************************************************************************
  qgspointlightsfromlayersettings.cpp
  --------------------------------------
  Date                 : November 2018
  Copyright            : (C) 2018 by Martin Dobias
  Email                : wonder dot sk at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgspointlightsfromlayersettings.h"
#include "qgssymbollayerutils.h"
#include "qgs3dmapsettings.h"
#include "qgsvectorlayer.h"
#include "qgsexpressioncontext.h"
#include "qgsexpressioncontextutils.h"
#include "qgs3dutils.h"

#include <QDomDocument>

#include <Qt3DCore/QEntity>
#include <Qt3DRender/QPointLight>
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DExtras/QSphereMesh>

QList<Qt3DCore::QEntity *> QgsPointLightsFromLayerSettings::createEntities( const Qgs3DMapSettings &map, Qt3DCore::QEntity *parent ) const
{
  QList<Qt3DCore::QEntity *> res;
  if ( !mSourceLayer )
    return res;

  QgsExpressionContext exprContext( Qgs3DUtils::globalProjectLayerExpressionContext( mSourceLayer.get() ) );
  exprContext.setFields( mSourceLayer->fields() );

  mDataDefinedProperties.prepare( exprContext );

  QgsFeature feature;
  QgsFeatureRequest req;
  req.setDestinationCrs( map.crs(), map.transformContext() );
  req.setExpressionContext( exprContext );
  req.setSubsetOfAttributes( mDataDefinedProperties.referencedFields(), mSourceLayer->fields() );
  req.setLimit( 50 ); // reasonable?

  QgsFeatureIterator it = mSourceLayer->getFeatures( req );
  while ( it.nextFeature( feature ) )
  {
    if ( feature.geometry().isEmpty() )
      continue;

    const QgsPoint *point = qgsgeometry_cast< const QgsPoint * >( feature.geometry().constGet()->simplifiedTypeRef() );
    if ( !point )
      continue;

    exprContext.setFeature( feature );

    Qt3DCore::QEntity *lightEntity = new Qt3DCore::QEntity( parent );
    Qt3DCore::QTransform *lightTransform = new Qt3DCore::QTransform;
    lightTransform->setTranslation( map.mapToWorldCoordinates( QgsVector3D( point->x(), point->y(), point->z() ) ).toVector3D() );
    Qt3DRender::QPointLight *light = new Qt3DRender::QPointLight;

    QColor evalColor = color();
    if ( mDataDefinedProperties.isActive( Property::Color ) )
    {
      evalColor = mDataDefinedProperties.valueAsColor( Property::Color, exprContext, evalColor );
    }
    light->setColor( evalColor );

    double evalIntensity = intensity();
    if ( mDataDefinedProperties.isActive( Property::Intensity ) )
    {
      evalIntensity = mDataDefinedProperties.valueAsDouble( Property::Intensity, exprContext, evalIntensity );
    }
    light->setIntensity( evalIntensity );

    light->setConstantAttenuation( constantAttenuation() );
    light->setLinearAttenuation( linearAttenuation() );
    light->setQuadraticAttenuation( quadraticAttenuation() );

    lightEntity->addComponent( light );
    lightEntity->addComponent( lightTransform );

    res << lightEntity;

    if ( map.showLightSourceOrigins() )
    {
      Qt3DCore::QEntity *originEntity = new Qt3DCore::QEntity( parent );

      Qt3DCore::QTransform *trLightOriginCenter = new Qt3DCore::QTransform;
      trLightOriginCenter->setTranslation( lightTransform->translation() );
      originEntity->addComponent( trLightOriginCenter );

      Qt3DExtras::QPhongMaterial *materialLightOriginCenter = new Qt3DExtras::QPhongMaterial;
      materialLightOriginCenter->setAmbient( evalColor );
      originEntity->addComponent( materialLightOriginCenter );

      Qt3DExtras::QSphereMesh *rendererLightOriginCenter = new Qt3DExtras::QSphereMesh;
      rendererLightOriginCenter->setRadius( 0.1 );
      originEntity->addComponent( rendererLightOriginCenter );

      originEntity->setEnabled( true );

      res << originEntity;
    }
  }

  return res;
}

QDomElement QgsPointLightsFromLayerSettings::writeXml( QDomDocument &doc, const QgsReadWriteContext &context ) const
{
  QDomElement elemLight = doc.createElement( QStringLiteral( "point-light-from-layer" ) );
  elemLight.setAttribute( QStringLiteral( "color" ), QgsSymbolLayerUtils::encodeColor( mColor ) );
  elemLight.setAttribute( QStringLiteral( "intensity" ), mIntensity );
  elemLight.setAttribute( QStringLiteral( "attenuation-0" ), mConstantAttenuation );
  elemLight.setAttribute( QStringLiteral( "attenuation-1" ), mLinearAttenuation );
  elemLight.setAttribute( QStringLiteral( "attenuation-2" ), mQuadraticAttenuation );

  if ( mSourceLayer )
  {
    elemLight.setAttribute( QStringLiteral( "sourceLayer" ), mSourceLayer.layerId );
  }
  else
  {
    elemLight.setAttribute( QStringLiteral( "sourceLayer" ), QString() );
  }

  writeCommonProperties( elemLight, doc, context );

  return elemLight;
}

void QgsPointLightsFromLayerSettings::readXml( const QDomElement &elem, const QgsReadWriteContext &context )
{
  mColor = QgsSymbolLayerUtils::decodeColor( elem.attribute( QStringLiteral( "color" ) ) );
  mIntensity = elem.attribute( QStringLiteral( "intensity" ) ).toFloat();
  mConstantAttenuation = elem.attribute( QStringLiteral( "attenuation-0" ) ).toDouble();
  mLinearAttenuation = elem.attribute( QStringLiteral( "attenuation-1" ) ).toDouble();
  mQuadraticAttenuation = elem.attribute( QStringLiteral( "attenuation-2" ) ).toDouble();

  const QString layerId = elem.attribute( QStringLiteral( "sourceLayer" ) );
  mSourceLayer = QgsVectorLayerRef( layerId );

  readCommonProperties( elem, context );
}

void QgsPointLightsFromLayerSettings::resolveReferences( const QgsProject &project )
{
  mSourceLayer.resolve( &project );
}

void QgsPointLightsFromLayerSettings::setSourceLayer( QgsVectorLayer *layer )
{
  if ( layer == mSourceLayer.get() )
  {
    return;
  }

  mSourceLayer.setLayer( layer );
}

bool QgsPointLightsFromLayerSettings::operator==( const QgsPointLightsFromLayerSettings &other )
{
  return mSourceLayer == other.mSourceLayer &&
         mColor == other.mColor && mIntensity == other.mIntensity &&
         mConstantAttenuation == other.mConstantAttenuation && mLinearAttenuation == other.mLinearAttenuation &&
         mQuadraticAttenuation == other.mQuadraticAttenuation;
}
