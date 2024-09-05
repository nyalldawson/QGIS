/***************************************************************************
    qgslabelingenginerule_impl.cpp
    ---------------------
  Date                 : August 2024
  Copyright            : (C) 2024 by Nyall Dawson
  Email                : nyall dot dawson at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgslabelingenginerule_impl.h"
#include "qgsunittypes.h"
#include "qgssymbollayerutils.h"
#include "labelposition.h"
#include "feature.h"
#include "qgsvectorlayerfeatureiterator.h"
#include "qgsthreadingutils.h"
#include "qgsspatialindex.h"
#include "qgsgeos.h"

//
// QgsLabelingEngineRuleMinimumDistanceLabelToFeature
//

QgsLabelingEngineRuleMinimumDistanceLabelToFeature *QgsLabelingEngineRuleMinimumDistanceLabelToFeature::clone() const
{
  return new QgsLabelingEngineRuleMinimumDistanceLabelToFeature( *this );
}

QString QgsLabelingEngineRuleMinimumDistanceLabelToFeature::id() const
{
  return QStringLiteral( "minimumDistanceLabelToFeature" );
}

bool QgsLabelingEngineRuleMinimumDistanceLabelToFeature::prepare( QgsRenderContext & )
{
  return true;
}

void QgsLabelingEngineRuleMinimumDistanceLabelToFeature::writeXml( QDomDocument &, QDomElement &element, const QgsReadWriteContext & ) const
{
  element.setAttribute( QStringLiteral( "distance" ), mDistance );
  element.setAttribute( QStringLiteral( "distanceUnit" ), QgsUnitTypes::encodeUnit( mDistanceUnit ) );
  element.setAttribute( QStringLiteral( "distanceUnitScale" ), QgsSymbolLayerUtils::encodeMapUnitScale( mDistanceUnitScale ) );
  element.setAttribute( QStringLiteral( "cost" ), mCost );

  if ( mLabeledLayer )
  {
    element.setAttribute( QStringLiteral( "labeledLayer" ), mLabeledLayer.layerId );
    element.setAttribute( QStringLiteral( "labeledLayerName" ), mLabeledLayer.name );
    element.setAttribute( QStringLiteral( "labeledLayerSource" ), mLabeledLayer.source );
    element.setAttribute( QStringLiteral( "labeledLayerProvider" ), mLabeledLayer.provider );
  }
  if ( mTargetLayer )
  {
    element.setAttribute( QStringLiteral( "targetLayer" ), mTargetLayer.layerId );
    element.setAttribute( QStringLiteral( "targetLayerName" ), mTargetLayer.name );
    element.setAttribute( QStringLiteral( "targetLayerSource" ), mTargetLayer.source );
    element.setAttribute( QStringLiteral( "targetLayerProvider" ), mTargetLayer.provider );
  }
}

void QgsLabelingEngineRuleMinimumDistanceLabelToFeature::readXml( const QDomElement &element, const QgsReadWriteContext & )
{
  mDistance = element.attribute( QStringLiteral( "distance" ), QStringLiteral( "0" ) ).toDouble();
  mDistanceUnit = QgsUnitTypes::decodeRenderUnit( element.attribute( QStringLiteral( "distanceUnit" ) ) );
  mDistanceUnitScale =  QgsSymbolLayerUtils::decodeMapUnitScale( element.attribute( QStringLiteral( "distanceUnitScale" ) ) );
  mCost = element.attribute( QStringLiteral( "cost" ), QStringLiteral( "0" ) ).toDouble();

  {
    const QString layerId = element.attribute( QStringLiteral( "labeledLayer" ) );
    const QString layerName = element.attribute( QStringLiteral( "labeledLayerName" ) );
    const QString layerSource = element.attribute( QStringLiteral( "labeledLayerSource" ) );
    const QString layerProvider = element.attribute( QStringLiteral( "labeledLayerProvider" ) );
    mLabeledLayer = QgsVectorLayerRef( layerId, layerName, layerSource, layerProvider );
  }
  {
    const QString layerId = element.attribute( QStringLiteral( "targetLayer" ) );
    const QString layerName = element.attribute( QStringLiteral( "targetLayerName" ) );
    const QString layerSource = element.attribute( QStringLiteral( "targetLayerSource" ) );
    const QString layerProvider = element.attribute( QStringLiteral( "targetLayerProvider" ) );
    mTargetLayer = QgsVectorLayerRef( layerId, layerName, layerSource, layerProvider );
  }
}

void QgsLabelingEngineRuleMinimumDistanceLabelToFeature::resolveReferences( const QgsProject *project )
{
  mLabeledLayer.resolve( project );
  mTargetLayer.resolve( project );
}

QgsVectorLayer *QgsLabelingEngineRuleMinimumDistanceLabelToFeature::labeledLayer()
{
  return mLabeledLayer.get();
}

void QgsLabelingEngineRuleMinimumDistanceLabelToFeature::setLabeledLayer( QgsVectorLayer *layer )
{
  mLabeledLayer = layer;
}

QgsVectorLayer *QgsLabelingEngineRuleMinimumDistanceLabelToFeature::targetLayer()
{
  return mTargetLayer.get();
}

void QgsLabelingEngineRuleMinimumDistanceLabelToFeature::setTargetLayer( QgsVectorLayer *layer )
{
  mTargetLayer = layer;
}

//
// QgsLabelingEngineRuleMinimumDistanceLabelToLabel
//

QgsLabelingEngineRuleMinimumDistanceLabelToLabel *QgsLabelingEngineRuleMinimumDistanceLabelToLabel::clone() const
{
  return new QgsLabelingEngineRuleMinimumDistanceLabelToLabel( *this );
}

QString QgsLabelingEngineRuleMinimumDistanceLabelToLabel::id() const
{
  return QStringLiteral( "minimumDistanceLabelToLabel" );
}

bool QgsLabelingEngineRuleMinimumDistanceLabelToLabel::prepare( QgsRenderContext & )
{
  return true;
}

void QgsLabelingEngineRuleMinimumDistanceLabelToLabel::writeXml( QDomDocument &, QDomElement &element, const QgsReadWriteContext & ) const
{
  element.setAttribute( QStringLiteral( "distance" ), mDistance );
  element.setAttribute( QStringLiteral( "distanceUnit" ), QgsUnitTypes::encodeUnit( mDistanceUnit ) );
  element.setAttribute( QStringLiteral( "distanceUnitScale" ), QgsSymbolLayerUtils::encodeMapUnitScale( mDistanceUnitScale ) );
  element.setAttribute( QStringLiteral( "cost" ), mCost );

  if ( mLabeledLayer )
  {
    element.setAttribute( QStringLiteral( "labeledLayer" ), mLabeledLayer.layerId );
    element.setAttribute( QStringLiteral( "labeledLayerName" ), mLabeledLayer.name );
    element.setAttribute( QStringLiteral( "labeledLayerSource" ), mLabeledLayer.source );
    element.setAttribute( QStringLiteral( "labeledLayerProvider" ), mLabeledLayer.provider );
  }
  if ( mTargetLayer )
  {
    element.setAttribute( QStringLiteral( "targetLayer" ), mTargetLayer.layerId );
    element.setAttribute( QStringLiteral( "targetLayerName" ), mTargetLayer.name );
    element.setAttribute( QStringLiteral( "targetLayerSource" ), mTargetLayer.source );
    element.setAttribute( QStringLiteral( "targetLayerProvider" ), mTargetLayer.provider );
  }
}

void QgsLabelingEngineRuleMinimumDistanceLabelToLabel::readXml( const QDomElement &element, const QgsReadWriteContext & )
{
  mDistance = element.attribute( QStringLiteral( "distance" ), QStringLiteral( "0" ) ).toDouble();
  mDistanceUnit = QgsUnitTypes::decodeRenderUnit( element.attribute( QStringLiteral( "distanceUnit" ) ) );
  mDistanceUnitScale =  QgsSymbolLayerUtils::decodeMapUnitScale( element.attribute( QStringLiteral( "distanceUnitScale" ) ) );
  mCost = element.attribute( QStringLiteral( "cost" ), QStringLiteral( "0" ) ).toDouble();

  {
    const QString layerId = element.attribute( QStringLiteral( "labeledLayer" ) );
    const QString layerName = element.attribute( QStringLiteral( "labeledLayerName" ) );
    const QString layerSource = element.attribute( QStringLiteral( "labeledLayerSource" ) );
    const QString layerProvider = element.attribute( QStringLiteral( "labeledLayerProvider" ) );
    mLabeledLayer = QgsVectorLayerRef( layerId, layerName, layerSource, layerProvider );
  }
  {
    const QString layerId = element.attribute( QStringLiteral( "targetLayer" ) );
    const QString layerName = element.attribute( QStringLiteral( "targetLayerName" ) );
    const QString layerSource = element.attribute( QStringLiteral( "targetLayerSource" ) );
    const QString layerProvider = element.attribute( QStringLiteral( "targetLayerProvider" ) );
    mTargetLayer = QgsVectorLayerRef( layerId, layerName, layerSource, layerProvider );
  }
}

void QgsLabelingEngineRuleMinimumDistanceLabelToLabel::resolveReferences( const QgsProject *project )
{
  mLabeledLayer.resolve( project );
  mTargetLayer.resolve( project );
}

bool QgsLabelingEngineRuleMinimumDistanceLabelToLabel::candidatesAreConflicting( const pal::LabelPosition *lp1, const pal::LabelPosition *lp2 ) const
{
  return false;
}

QgsVectorLayer *QgsLabelingEngineRuleMinimumDistanceLabelToLabel::labeledLayer()
{
  return mLabeledLayer.get();
}

void QgsLabelingEngineRuleMinimumDistanceLabelToLabel::setLabeledLayer( QgsVectorLayer *layer )
{
  mLabeledLayer = layer;
}

QgsVectorLayer *QgsLabelingEngineRuleMinimumDistanceLabelToLabel::targetLayer()
{
  return mTargetLayer.get();
}

void QgsLabelingEngineRuleMinimumDistanceLabelToLabel::setTargetLayer( QgsVectorLayer *layer )
{
  mTargetLayer = layer;
}

//
// QgsLabelingEngineRuleMaximumDistanceLabelToFeature
//

QgsLabelingEngineRuleMaximumDistanceLabelToFeature *QgsLabelingEngineRuleMaximumDistanceLabelToFeature::clone() const
{
  return new QgsLabelingEngineRuleMaximumDistanceLabelToFeature( *this );
}

QString QgsLabelingEngineRuleMaximumDistanceLabelToFeature::id() const
{
  return QStringLiteral( "maximumDistanceLabelToFeature" );
}

bool QgsLabelingEngineRuleMaximumDistanceLabelToFeature::prepare( QgsRenderContext & )
{
  return true;
}

void QgsLabelingEngineRuleMaximumDistanceLabelToFeature::writeXml( QDomDocument &, QDomElement &element, const QgsReadWriteContext & ) const
{
  element.setAttribute( QStringLiteral( "distance" ), mDistance );
  element.setAttribute( QStringLiteral( "distanceUnit" ), QgsUnitTypes::encodeUnit( mDistanceUnit ) );
  element.setAttribute( QStringLiteral( "distanceUnitScale" ), QgsSymbolLayerUtils::encodeMapUnitScale( mDistanceUnitScale ) );
  element.setAttribute( QStringLiteral( "cost" ), mCost );

  if ( mLabeledLayer )
  {
    element.setAttribute( QStringLiteral( "labeledLayer" ), mLabeledLayer.layerId );
    element.setAttribute( QStringLiteral( "labeledLayerName" ), mLabeledLayer.name );
    element.setAttribute( QStringLiteral( "labeledLayerSource" ), mLabeledLayer.source );
    element.setAttribute( QStringLiteral( "labeledLayerProvider" ), mLabeledLayer.provider );
  }
  if ( mTargetLayer )
  {
    element.setAttribute( QStringLiteral( "targetLayer" ), mTargetLayer.layerId );
    element.setAttribute( QStringLiteral( "targetLayerName" ), mTargetLayer.name );
    element.setAttribute( QStringLiteral( "targetLayerSource" ), mTargetLayer.source );
    element.setAttribute( QStringLiteral( "targetLayerProvider" ), mTargetLayer.provider );
  }
}

void QgsLabelingEngineRuleMaximumDistanceLabelToFeature::readXml( const QDomElement &element, const QgsReadWriteContext & )
{
  mDistance = element.attribute( QStringLiteral( "distance" ), QStringLiteral( "0" ) ).toDouble();
  mDistanceUnit = QgsUnitTypes::decodeRenderUnit( element.attribute( QStringLiteral( "distanceUnit" ) ) );
  mDistanceUnitScale =  QgsSymbolLayerUtils::decodeMapUnitScale( element.attribute( QStringLiteral( "distanceUnitScale" ) ) );
  mCost = element.attribute( QStringLiteral( "cost" ), QStringLiteral( "0" ) ).toDouble();

  {
    const QString layerId = element.attribute( QStringLiteral( "labeledLayer" ) );
    const QString layerName = element.attribute( QStringLiteral( "labeledLayerName" ) );
    const QString layerSource = element.attribute( QStringLiteral( "labeledLayerSource" ) );
    const QString layerProvider = element.attribute( QStringLiteral( "labeledLayerProvider" ) );
    mLabeledLayer = QgsVectorLayerRef( layerId, layerName, layerSource, layerProvider );
  }
  {
    const QString layerId = element.attribute( QStringLiteral( "targetLayer" ) );
    const QString layerName = element.attribute( QStringLiteral( "targetLayerName" ) );
    const QString layerSource = element.attribute( QStringLiteral( "targetLayerSource" ) );
    const QString layerProvider = element.attribute( QStringLiteral( "targetLayerProvider" ) );
    mTargetLayer = QgsVectorLayerRef( layerId, layerName, layerSource, layerProvider );
  }
}

void QgsLabelingEngineRuleMaximumDistanceLabelToFeature::resolveReferences( const QgsProject *project )
{
  mLabeledLayer.resolve( project );
  mTargetLayer.resolve( project );
}

QgsVectorLayer *QgsLabelingEngineRuleMaximumDistanceLabelToFeature::labeledLayer()
{
  return mLabeledLayer.get();
}

void QgsLabelingEngineRuleMaximumDistanceLabelToFeature::setLabeledLayer( QgsVectorLayer *layer )
{
  mLabeledLayer = layer;
}

QgsVectorLayer *QgsLabelingEngineRuleMaximumDistanceLabelToFeature::targetLayer()
{
  return mTargetLayer.get();
}

void QgsLabelingEngineRuleMaximumDistanceLabelToFeature::setTargetLayer( QgsVectorLayer *layer )
{
  mTargetLayer = layer;
}

//
// QgsLabelingEngineRuleAvoidLabelOverlapWithFeature
//

QgsLabelingEngineRuleAvoidLabelOverlapWithFeature::QgsLabelingEngineRuleAvoidLabelOverlapWithFeature() = default;
QgsLabelingEngineRuleAvoidLabelOverlapWithFeature::~QgsLabelingEngineRuleAvoidLabelOverlapWithFeature() = default;

QgsLabelingEngineRuleAvoidLabelOverlapWithFeature *QgsLabelingEngineRuleAvoidLabelOverlapWithFeature::clone() const
{
  std::unique_ptr< QgsLabelingEngineRuleAvoidLabelOverlapWithFeature> res = std::make_unique< QgsLabelingEngineRuleAvoidLabelOverlapWithFeature >();
  res->mLabeledLayer = mLabeledLayer;
  res->mTargetLayer = mTargetLayer;
  return res.release();
}

QString QgsLabelingEngineRuleAvoidLabelOverlapWithFeature::id() const
{
  return QStringLiteral( "avoidLabelOverlapWithFeature" );
}

bool QgsLabelingEngineRuleAvoidLabelOverlapWithFeature::prepare( QgsRenderContext & )
{
  if ( !mTargetLayer )
    return false;

  QGIS_CHECK_OTHER_QOBJECT_THREAD_ACCESS( mTargetLayer );
  mTargetLayerSource = std::make_unique< QgsVectorLayerFeatureSource >( mTargetLayer.get() );
  return true;
}

void QgsLabelingEngineRuleAvoidLabelOverlapWithFeature::writeXml( QDomDocument &, QDomElement &element, const QgsReadWriteContext & ) const
{
  if ( mLabeledLayer )
  {
    element.setAttribute( QStringLiteral( "labeledLayer" ), mLabeledLayer.layerId );
    element.setAttribute( QStringLiteral( "labeledLayerName" ), mLabeledLayer.name );
    element.setAttribute( QStringLiteral( "labeledLayerSource" ), mLabeledLayer.source );
    element.setAttribute( QStringLiteral( "labeledLayerProvider" ), mLabeledLayer.provider );
  }
  if ( mTargetLayer )
  {
    element.setAttribute( QStringLiteral( "targetLayer" ), mTargetLayer.layerId );
    element.setAttribute( QStringLiteral( "targetLayerName" ), mTargetLayer.name );
    element.setAttribute( QStringLiteral( "targetLayerSource" ), mTargetLayer.source );
    element.setAttribute( QStringLiteral( "targetLayerProvider" ), mTargetLayer.provider );
  }
}

void QgsLabelingEngineRuleAvoidLabelOverlapWithFeature::readXml( const QDomElement &element, const QgsReadWriteContext & )
{
  {
    const QString layerId = element.attribute( QStringLiteral( "labeledLayer" ) );
    const QString layerName = element.attribute( QStringLiteral( "labeledLayerName" ) );
    const QString layerSource = element.attribute( QStringLiteral( "labeledLayerSource" ) );
    const QString layerProvider = element.attribute( QStringLiteral( "labeledLayerProvider" ) );
    mLabeledLayer = QgsVectorLayerRef( layerId, layerName, layerSource, layerProvider );
  }
  {
    const QString layerId = element.attribute( QStringLiteral( "targetLayer" ) );
    const QString layerName = element.attribute( QStringLiteral( "targetLayerName" ) );
    const QString layerSource = element.attribute( QStringLiteral( "targetLayerSource" ) );
    const QString layerProvider = element.attribute( QStringLiteral( "targetLayerProvider" ) );
    mTargetLayer = QgsVectorLayerRef( layerId, layerName, layerSource, layerProvider );
  }
}

void QgsLabelingEngineRuleAvoidLabelOverlapWithFeature::resolveReferences( const QgsProject *project )
{
  mLabeledLayer.resolve( project );
  mTargetLayer.resolve( project );
}

bool QgsLabelingEngineRuleAvoidLabelOverlapWithFeature::candidateIsIllegal( const pal::LabelPosition *candidate, QgsLabelingEngineContext &context ) const
{
  if ( candidate->getFeaturePart()->feature()->provider()->layerId() != mLabeledLayer.layerId )
  {
    return false;
  }

  if ( !mTargetLayerSource )
    return false;

  if ( !mInitialized )
    const_cast< QgsLabelingEngineRuleAvoidLabelOverlapWithFeature * >( this )->initialize( context );

  const QList<QgsFeatureId> overlapCandidates = mIndex->intersects( candidate->boundingBox() );
  if ( overlapCandidates.empty() )
    return false;

  GEOSContextHandle_t geosctxt = QgsGeosContext::get();

  const GEOSPreparedGeometry *candidateGeos = candidate->preparedMultiPartGeom();
  for ( const QgsFeatureId overlapCandidateId : overlapCandidates )
  {
    if ( context.renderContext().feedback() && context.renderContext().feedback()->isCanceled() )
      break;

    try
    {
      geos::unique_ptr featureCandidate = QgsGeos::asGeos( mIndex->geometry( overlapCandidateId ).constGet() );
      if ( GEOSPreparedIntersects_r( geosctxt, candidateGeos, featureCandidate.get() ) == 1 )
        return true;
    }
    catch ( GEOSException &e )
    {
      QgsDebugError( QStringLiteral( "GEOS exception: %1" ).arg( e.what() ) );
    }
  }

  return false;
}

QgsVectorLayer *QgsLabelingEngineRuleAvoidLabelOverlapWithFeature::labeledLayer()
{
  return mLabeledLayer.get();
}

void QgsLabelingEngineRuleAvoidLabelOverlapWithFeature::setLabeledLayer( QgsVectorLayer *layer )
{
  mLabeledLayer = layer;
}

QgsVectorLayer *QgsLabelingEngineRuleAvoidLabelOverlapWithFeature::targetLayer()
{
  return mTargetLayer.get();
}

void QgsLabelingEngineRuleAvoidLabelOverlapWithFeature::setTargetLayer( QgsVectorLayer *layer )
{
  mTargetLayer = layer;
}

void QgsLabelingEngineRuleAvoidLabelOverlapWithFeature::initialize( QgsLabelingEngineContext &context )
{
  QgsFeatureRequest req;
  req.setDestinationCrs( context.renderContext().coordinateTransform().destinationCrs(), context.renderContext().transformContext() );
  req.setFilterRect( context.extent() );
  req.setNoAttributes();

  QgsFeatureIterator it = mTargetLayerSource->getFeatures( req );

  mIndex = std::make_unique< QgsSpatialIndex >( it, context.renderContext().feedback(), QgsSpatialIndex::Flag::FlagStoreFeatureGeometries );

  mInitialized = true;
}
