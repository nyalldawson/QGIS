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

bool QgsLabelingEngineRuleMinimumDistanceLabelToFeature::modifyProblem()
{

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

bool QgsLabelingEngineRuleMinimumDistanceLabelToLabel::modifyProblem()
{

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

bool QgsLabelingEngineRuleMaximumDistanceLabelToFeature::modifyProblem()
{

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

QgsLabelingEngineRuleAvoidLabelOverlapWithFeature *QgsLabelingEngineRuleAvoidLabelOverlapWithFeature::clone() const
{
  return new QgsLabelingEngineRuleAvoidLabelOverlapWithFeature( *this );
}

QString QgsLabelingEngineRuleAvoidLabelOverlapWithFeature::id() const
{
  return QStringLiteral( "avoidLabelOverlapWithFeature" );
}

bool QgsLabelingEngineRuleAvoidLabelOverlapWithFeature::prepare( QgsRenderContext & )
{
  return true;
}

bool QgsLabelingEngineRuleAvoidLabelOverlapWithFeature::modifyProblem()
{

}

void QgsLabelingEngineRuleAvoidLabelOverlapWithFeature::writeXml( QDomDocument &, QDomElement &element, const QgsReadWriteContext & ) const
{
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

void QgsLabelingEngineRuleAvoidLabelOverlapWithFeature::readXml( const QDomElement &element, const QgsReadWriteContext & )
{
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

void QgsLabelingEngineRuleAvoidLabelOverlapWithFeature::resolveReferences( const QgsProject *project )
{
  mLabeledLayer.resolve( project );
  mTargetLayer.resolve( project );
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
