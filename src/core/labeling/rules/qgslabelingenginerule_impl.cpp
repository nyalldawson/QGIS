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

}

bool QgsLabelingEngineRuleMinimumDistanceLabelToFeature::modifyProblem()
{

}

void QgsLabelingEngineRuleMinimumDistanceLabelToFeature::writeXml( QDomDocument &doc, QDomElement &element, const QgsReadWriteContext &context ) const
{

}

void QgsLabelingEngineRuleMinimumDistanceLabelToFeature::readXml( const QDomElement &element, const QgsReadWriteContext &context )
{

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

}

bool QgsLabelingEngineRuleMinimumDistanceLabelToLabel::modifyProblem()
{

}

void QgsLabelingEngineRuleMinimumDistanceLabelToLabel::writeXml( QDomDocument &doc, QDomElement &element, const QgsReadWriteContext &context ) const
{

}

void QgsLabelingEngineRuleMinimumDistanceLabelToLabel::readXml( const QDomElement &element, const QgsReadWriteContext &context )
{

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

}

bool QgsLabelingEngineRuleMaximumDistanceLabelToFeature::modifyProblem()
{

}

void QgsLabelingEngineRuleMaximumDistanceLabelToFeature::writeXml( QDomDocument &doc, QDomElement &element, const QgsReadWriteContext &context ) const
{

}

void QgsLabelingEngineRuleMaximumDistanceLabelToFeature::readXml( const QDomElement &element, const QgsReadWriteContext &context )
{

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

}

bool QgsLabelingEngineRuleAvoidLabelOverlapWithFeature::modifyProblem()
{

}

void QgsLabelingEngineRuleAvoidLabelOverlapWithFeature::writeXml( QDomDocument &doc, QDomElement &element, const QgsReadWriteContext &context ) const
{

}

void QgsLabelingEngineRuleAvoidLabelOverlapWithFeature::readXml( const QDomElement &element, const QgsReadWriteContext &context )
{

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
