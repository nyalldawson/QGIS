/***************************************************************************
    qgslinedistancerenderer.cpp
    ---------------------
    begin                : November 2023
    copyright            : (C) 2023 by Nyall Dawson
    email                : nyall dot dawson at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgslinedistancerenderer.h"

#include "qgssymbol.h"
#include "qgssymbollayerutils.h"

#include "qgsfeature.h"
#include "qgsvectorlayer.h"
#include "qgsstyleentityvisitor.h"
#include "qgsspatialindex.h"
#include "qgsmarkersymbol.h"
#include "qgsmultipoint.h"
#include "qgsexpressioncontextutils.h"

#include <QDomDocument>
#include <QDomElement>

QgsLineDistanceRenderer::QgsLineDistanceRenderer( const QString &rendererName )
  : QgsFeatureRenderer( rendererName )
{
  mRenderer.reset( QgsFeatureRenderer::defaultRenderer( Qgis::GeometryType::Line ) );
}

QgsLineDistanceRenderer::~QgsLineDistanceRenderer() = default;

void QgsLineDistanceRenderer::toSld( QDomDocument &doc, QDomElement &element, const QVariantMap &props ) const
{
  mRenderer->toSld( doc, element, props );
}

bool QgsLineDistanceRenderer::renderFeature( const QgsFeature &feature, QgsRenderContext &context, int layer, bool selected, bool drawVertexMarker )
{
  Q_UNUSED( drawVertexMarker )
  Q_UNUSED( context )
  Q_UNUSED( layer )

  /*
  * IMPORTANT: This algorithm is ported to Python in the processing "Points Displacement" algorithm.
  * Please port any changes/improvements to that algorithm too!
  */

  //check if there is already a point at that position
  if ( !feature.hasGeometry() )
    return false;

  QgsMarkerSymbol *symbol = firstSymbolForFeature( feature, context );

  //if the feature has no symbol (e.g., no matching rule in a rule-based renderer), skip it
  if ( !symbol )
    return false;

  //point position in screen coords
  QgsGeometry geom = feature.geometry();
  const Qgis::WkbType geomType = geom.wkbType();
  if ( QgsWkbTypes::geometryType( geomType ) != Qgis::GeometryType::Point )
  {
    //can only render point type
    return false;
  }

  const QgsCoordinateTransform xform = context.coordinateTransform();
  QgsFeature transformedFeature = feature;
  if ( xform.isValid() )
  {
    geom.transform( xform );
    transformedFeature.setGeometry( geom );
  }

  const double searchDistance = context.convertToMapUnits( mTolerance, mToleranceUnit, mToleranceMapUnitScale );

  const QgsGeometry transformedGeometry = transformedFeature.geometry();
  for ( auto partIt = transformedGeometry.const_parts_begin(); partIt != transformedGeometry.const_parts_end(); ++partIt )
  {
    const QgsPoint *point = qgsgeometry_cast< const QgsPoint * >( *partIt );
    // create a new feature which is JUST this point, no other parts from the multi-point
    QgsFeature pointFeature = transformedFeature;
    pointFeature.setGeometry( QgsGeometry( point->clone() ) );
    const QList<QgsFeatureId> intersectList = mSpatialIndex->intersects( searchRect( point, searchDistance ) );
    if ( intersectList.empty() )
    {
      mSpatialIndex->addFeature( pointFeature );
      // create new group
      ClusteredGroup newGroup;
      newGroup << GroupedFeature( pointFeature, symbol->clone(), selected );
      mClusteredGroups.push_back( newGroup );
      // add to group index
      mGroupIndex.insert( pointFeature.id(), mClusteredGroups.count() - 1 );
      mGroupLocations.insert( pointFeature.id(), QgsPointXY( *point ) );
    }
    else
    {
      // find group with closest location to this point (may be more than one within search tolerance)
      QgsFeatureId minDistFeatureId = intersectList.at( 0 );
      double minDist = mGroupLocations.value( minDistFeatureId ).distance( point->x(), point->y() );
      for ( int i = 1; i < intersectList.count(); ++i )
      {
        const QgsFeatureId candidateId = intersectList.at( i );
        const double newDist = mGroupLocations.value( candidateId ).distance( point->x(), point->y() );
        if ( newDist < minDist )
        {
          minDist = newDist;
          minDistFeatureId = candidateId;
        }
      }

      const int groupIdx = mGroupIndex[ minDistFeatureId ];
      ClusteredGroup &group = mClusteredGroups[groupIdx];

      // calculate new centroid of group
      const QgsPointXY oldCenter = mGroupLocations.value( minDistFeatureId );
      mGroupLocations[ minDistFeatureId ] = QgsPointXY( ( oldCenter.x() * group.size() + point->x() ) / ( group.size() + 1.0 ),
                                            ( oldCenter.y() * group.size() + point->y() ) / ( group.size() + 1.0 ) );

      // add to a group
      group << GroupedFeature( pointFeature, symbol->clone(), selected );
      // add to group index
      mGroupIndex.insert( pointFeature.id(), groupIdx );
    }
  }

  return true;
}

void QgsLineDistanceRenderer::drawGroup( const ClusteredGroup &group, QgsRenderContext &context ) const
{
  //calculate centroid of all points, this will be center of group
  QgsMultiPoint *groupMultiPoint = new QgsMultiPoint();
  const auto constGroup = group;
  for ( const GroupedFeature &f : constGroup )
  {
    groupMultiPoint->addGeometry( f.feature.geometry().constGet()->clone() );
  }
  const QgsGeometry groupGeom( groupMultiPoint );
  const QgsGeometry centroid = groupGeom.centroid();
  QPointF pt = centroid.asQPointF();
  context.mapToPixel().transformInPlace( pt.rx(), pt.ry() );

  const QgsExpressionContextScopePopper scopePopper( context.expressionContext(), createGroupScope( group ) );
  drawGroup( pt, context, group );
}

void QgsLineDistanceRenderer::setEmbeddedRenderer( QgsFeatureRenderer *r )
{
  mRenderer.reset( r );
}

const QgsFeatureRenderer *QgsLineDistanceRenderer::embeddedRenderer() const
{
  return mRenderer.get();
}

void QgsLineDistanceRenderer::setLegendSymbolItem( const QString &key, QgsSymbol *symbol )
{
  if ( !mRenderer )
    return;

  mRenderer->setLegendSymbolItem( key, symbol );
}

bool QgsLineDistanceRenderer::legendSymbolItemsCheckable() const
{
  if ( !mRenderer )
    return false;

  return mRenderer->legendSymbolItemsCheckable();
}

bool QgsLineDistanceRenderer::legendSymbolItemChecked( const QString &key )
{
  if ( !mRenderer )
    return false;

  return mRenderer->legendSymbolItemChecked( key );
}

void QgsLineDistanceRenderer::checkLegendSymbolItem( const QString &key, bool state )
{
  if ( !mRenderer )
    return;

  mRenderer->checkLegendSymbolItem( key, state );
}

QString QgsLineDistanceRenderer::filter( const QgsFields &fields )
{
  if ( !mRenderer )
    return QgsFeatureRenderer::filter( fields );
  else
    return mRenderer->filter( fields );
}

bool QgsLineDistanceRenderer::accept( QgsStyleEntityVisitorInterface *visitor ) const
{
  if ( mRenderer )
    if ( !mRenderer->accept( visitor ) )
      return false;

  return true;
}

QSet<QString> QgsLineDistanceRenderer::usedAttributes( const QgsRenderContext &context ) const
{
  QSet<QString> attributeList;
  if ( mRenderer )
  {
    attributeList += mRenderer->usedAttributes( context );
  }
  return attributeList;
}

bool QgsLineDistanceRenderer::filterNeedsGeometry() const
{
  return mRenderer ? mRenderer->filterNeedsGeometry() : false;
}

QgsFeatureRenderer::Capabilities QgsLineDistanceRenderer::capabilities()
{
  if ( !mRenderer )
  {
    return Capabilities();
  }
  return mRenderer->capabilities();
}

QgsSymbolList QgsLineDistanceRenderer::symbols( QgsRenderContext &context ) const
{
  if ( !mRenderer )
  {
    return QgsSymbolList();
  }
  return mRenderer->symbols( context );
}

QgsSymbol *QgsLineDistanceRenderer::symbolForFeature( const QgsFeature &feature, QgsRenderContext &context ) const
{
  if ( !mRenderer )
  {
    return nullptr;
  }
  return mRenderer->symbolForFeature( feature, context );
}

QgsSymbol *QgsLineDistanceRenderer::originalSymbolForFeature( const QgsFeature &feature, QgsRenderContext &context ) const
{
  if ( !mRenderer )
    return nullptr;
  return mRenderer->originalSymbolForFeature( feature, context );
}

QgsSymbolList QgsLineDistanceRenderer::symbolsForFeature( const QgsFeature &feature, QgsRenderContext &context ) const
{
  if ( !mRenderer )
  {
    return QgsSymbolList();
  }
  return mRenderer->symbolsForFeature( feature, context );
}

QgsSymbolList QgsLineDistanceRenderer::originalSymbolsForFeature( const QgsFeature &feature, QgsRenderContext &context ) const
{
  if ( !mRenderer )
    return QgsSymbolList();
  return mRenderer->originalSymbolsForFeature( feature, context );
}

QSet< QString > QgsLineDistanceRenderer::legendKeysForFeature( const QgsFeature &feature, QgsRenderContext &context ) const
{
  if ( !mRenderer )
    return QSet< QString >() << QString();
  return mRenderer->legendKeysForFeature( feature, context );
}

QString QgsLineDistanceRenderer::legendKeyToExpression( const QString &key, QgsVectorLayer *layer, bool &ok ) const
{
  ok = false;
  if ( !mRenderer )
    return QString();
  return mRenderer->legendKeyToExpression( key, layer, ok );
}

bool QgsLineDistanceRenderer::willRenderFeature( const QgsFeature &feature, QgsRenderContext &context ) const
{
  if ( !mRenderer )
  {
    return false;
  }
  return mRenderer->willRenderFeature( feature, context );
}


void QgsLineDistanceRenderer::startRender( QgsRenderContext &context, const QgsFields &fields )
{
  QgsFeatureRenderer::startRender( context, fields );

  mRenderer->startRender( context, fields );

  mClusteredGroups.clear();
  mGroupIndex.clear();
  mGroupLocations.clear();
  mSpatialIndex = std::make_unique< QgsSpatialIndex >();
}

void QgsLineDistanceRenderer::stopRender( QgsRenderContext &context )
{
  QgsFeatureRenderer::stopRender( context );

  //printInfoDisplacementGroups(); //just for debugging

  if ( !context.renderingStopped() )
  {
    const auto constMClusteredGroups = mClusteredGroups;
    for ( const ClusteredGroup &group : constMClusteredGroups )
    {
      drawGroup( group, context );
    }
  }

  mClusteredGroups.clear();
  mGroupIndex.clear();
  mGroupLocations.clear();
  mSpatialIndex.reset();

  mRenderer->stopRender( context );
}

QgsLegendSymbolList QgsLineDistanceRenderer::legendSymbolItems() const
{
  if ( mRenderer )
  {
    return mRenderer->legendSymbolItems();
  }
  return QgsLegendSymbolList();
}

QgsRectangle QgsLineDistanceRenderer::searchRect( const QgsPoint *p, double distance ) const
{
  return QgsRectangle( p->x() - distance, p->y() - distance, p->x() + distance, p->y() + distance );
}

void QgsLineDistanceRenderer::printGroupInfo() const
{
#ifdef QGISDEBUG
  const int nGroups = mClusteredGroups.size();
  QgsDebugMsgLevel( "number of displacement groups:" + QString::number( nGroups ), 3 );
  for ( int i = 0; i < nGroups; ++i )
  {
    QgsDebugMsgLevel( "***************displacement group " + QString::number( i ), 3 );
    const auto constAt = mClusteredGroups.at( i );
    for ( const GroupedFeature &feature : constAt )
    {
      QgsDebugMsgLevel( FID_TO_STRING( feature.feature.id() ), 3 );
    }
  }
#endif
}

QgsExpressionContextScope *QgsLineDistanceRenderer::createGroupScope( const ClusteredGroup &group ) const
{
  QgsExpressionContextScope *clusterScope = new QgsExpressionContextScope();
  if ( group.size() > 1 )
  {
    //scan through symbols to check color, e.g., if all clustered symbols are same color
    QColor groupColor;
    ClusteredGroup::const_iterator groupIt = group.constBegin();
    for ( ; groupIt != group.constEnd(); ++groupIt )
    {
      if ( !groupIt->symbol() )
        continue;

      if ( !groupColor.isValid() )
      {
        groupColor = groupIt->symbol()->color();
      }
      else
      {
        if ( groupColor != groupIt->symbol()->color() )
        {
          groupColor = QColor();
          break;
        }
      }
    }

    if ( groupColor.isValid() )
    {
      clusterScope->addVariable( QgsExpressionContextScope::StaticVariable( QgsExpressionContext::EXPR_CLUSTER_COLOR, QgsSymbolLayerUtils::encodeColor( groupColor ), true ) );
    }
    else
    {
      //mixed colors
      clusterScope->addVariable( QgsExpressionContextScope::StaticVariable( QgsExpressionContext::EXPR_CLUSTER_COLOR, QVariant(), true ) );
    }

    clusterScope->addVariable( QgsExpressionContextScope::StaticVariable( QgsExpressionContext::EXPR_CLUSTER_SIZE, group.size(), true ) );
  }
  if ( !group.empty() )
  {
    // data defined properties may require a feature in the expression context, so just use first feature in group
    clusterScope->setFeature( group.at( 0 ).feature );
  }
  return clusterScope;
}

QgsMarkerSymbol *QgsLineDistanceRenderer::firstSymbolForFeature( const QgsFeature &feature, QgsRenderContext &context )
{
  if ( !mRenderer )
  {
    return nullptr;
  }

  const QgsSymbolList symbolList = mRenderer->symbolsForFeature( feature, context );
  if ( symbolList.isEmpty() )
  {
    return nullptr;
  }

  return dynamic_cast< QgsMarkerSymbol * >( symbolList.at( 0 ) );
}

QgsLineDistanceRenderer::GroupedFeature::GroupedFeature( const QgsFeature &feature, QgsMarkerSymbol *symbol, bool isSelected )
  : feature( feature )
  , isSelected( isSelected )
  , mSymbol( symbol )
{}

QgsLineDistanceRenderer::GroupedFeature::~GroupedFeature() = default;
