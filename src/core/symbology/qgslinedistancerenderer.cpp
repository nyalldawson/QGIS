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
  Q_UNUSED( layer )

  if ( !feature.hasGeometry() )
    return false;

  QgsMarkerSymbol *symbol = firstSymbolForFeature( feature, context );

  //if the feature has no symbol (e.g., no matching rule in a rule-based renderer), skip it
  if ( !symbol )
    return false;

  //get line geometry in screen coordinates
  QgsGeometry geom = feature.geometry();
  const Qgis::WkbType geomType = geom.wkbType();
  if ( QgsWkbTypes::geometryType( geomType ) != Qgis::GeometryType::Line )
  {
    //can only render line features
    return false;
  }

  const QgsCoordinateTransform xform = context.coordinateTransform();
  QgsFeature transformedFeature = feature;
  if ( xform.isValid() )
  {
    try
    {
      geom.transform( xform );
    }
    catch ( QgsCsException & )
    {
      QgsDebugError( QStringLiteral( "Cannot transform line to map CRS" ) );
      return false;
    }

    transformedFeature.setGeometry( geom );
  }

  for ( auto partIt = geom.const_parts_begin(); partIt != geom.const_parts_end(); ++partIt )
  {
    std::unique_ptr< QgsLineString > segmentizedPart;
    const QgsLineString *thisLine = qgsgeometry_cast< const QgsLineString * >( *partIt );
    if ( !thisLine )
    {
      segmentizedPart.reset( qgsgeometry_cast< QgsLineString * >( thisLine->segmentize() ) );
      thisLine = segmentizedPart.get();
    }

    const int numPoints = thisLine->numPoints();
    if ( numPoints < 2 )
      continue;

    const double *xData = thisLine->xData();
    const double *yData = thisLine->yData();

    SegmentData thisSegmentData;
    thisSegmentData.feature = feature;
    thisSegmentData.x2 = *xData++;
    thisSegmentData.y2 = *yData++;
    thisSegmentData.selected = selected;
    thisSegmentData.drawVertexMarker = drawVertexMarker;

    for ( int n = 1; n < numPoints; ++ n )
    {
      if ( context.renderingStopped() )
        break;

      thisSegmentData.x1 = thisSegmentData.x2;
      thisSegmentData.y1 = thisSegmentData.y2;
      thisSegmentData.x2 = *xData++;
      thisSegmentData.y2 = *yData++;
      thisSegmentData.segmentIndex = n - 1;

      // rectangle will be normalized automatically, we don't need to check min/max coordinates here
      const QgsRectangle segmentRect = QgsRectangle( thisSegmentData.x1,
                                       thisSegmentData.y1,
                                       thisSegmentData.x2,
                                       thisSegmentData.y2 );

      mSegmentData.push_back( thisSegmentData );

      mSegmentIndex->addFeature( mSegmentId, segmentRect );
      mSegmentId++;
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

  mSegmentId = 0;
  mSegmentData.clear();
  mSegmentIndex = std::make_unique< QgsSpatialIndex >();
}

void QgsLineDistanceRenderer::stopRender( QgsRenderContext &context )
{
  QgsFeatureRenderer::stopRender( context );

  if ( !context.renderingStopped() )
  {
    QgsSpatialIndex segmentGroupIndex;
    int segmentGroupIndexId = 0;
    QHash< int, int> segmentGroups;
    QHash< int, int> splitSegments;

    int splitSegmentId = 0;

    const double tolerance = context.convertToMapUnits( mTolerance, mToleranceUnit, mToleranceMapUnitScale );

    int _id = -1;
    for ( const SegmentData &segmentData : std::as_const( mSegmentData ) )
    {
      _id++;
      if ( context.renderingStopped() )
        break;

      const double featureLineAngle = QgsGeometryUtils::lineAngle(
                                        segmentData.x1, segmentData.y1, segmentData.x2, segmentData.y2
                                      );

      QSet< double > splitPoints;

      const QgsRectangle searchRect = QgsRectangle( segmentData.x1,
                                      segmentData.y1,
                                      segmentData.x2,
                                      segmentData.y2 ).buffered( tolerance );

      const QList<QgsFeatureId> candidateSegments = mSegmentIndex->intersects( searchRect );
      QSet< QgsFeatureId > outSegments;
      for ( const QgsFeatureId candidate : candidateSegments )
      {
        if ( candidate == _id )
          continue;

        const SegmentData &candidateSegment = mSegmentData[candidate];
        //  candidate_p1, candidate_p2, _, candidate_fid, _

        // actually ignoring all segments from same source feature!
        if ( segmentData.feature.id() == candidateSegment.feature.id() )
          continue;

        const double candidateLineAngle = QgsGeometryUtils::lineAngle(
                                            candidateSegment.x1,
                                            candidateSegment.y1,
                                            candidateSegment.x2,
                                            candidateSegment.y2 );

        const double deltaAngle = std::min(
                                    std::min(
                                      std::min(
                                        std::min(
                                          std::fabs( candidateLineAngle - featureLineAngle ),
                                          std::fabs( 2 * M_PI + candidateLineAngle - featureLineAngle ) ),
                                        std::fabs( candidateLineAngle - ( 2 * M_PI + featureLineAngle ) ) ),
                                      std::fabs( M_PI + candidateLineAngle - featureLineAngle ) ),
                                    std::fabs( candidateLineAngle - ( M_PI + featureLineAngle ) )
                                  );

        if ( ( deltaAngle * 180 / M_PI ) > mAngleThreshold )
          continue;

        // next step -- find overlapping portion of segment
        // take the start/end of the candidate, and find the nearest point on segment to those
        double ptX1;
        double ptY1;
        const double dist1 = QgsGeometryUtils::sqrDistToLine( candidateSegment.x1, candidateSegment.y1,
                             segmentData.x1, segmentData.y1,
                             segmentData.x2, segmentData.y2, ptX1, ptY1, 0 );
        double ptX2;
        double ptY2;
        const double dist2 = QgsGeometryUtils::sqrDistToLine( candidateSegment.x2, candidateSegment.y2,
                             segmentData.x1, segmentData.y1,
                             segmentData.x2, segmentData.y2, ptX2, ptY2, 0 );

        if ( qgsDoubleNear( ptX1, ptX2 ) && qgsDoubleNear( ptY1, ptY2 ) )
        {
          // lines are parallel which come close, but don't actually overlap at all
          // i.e. ------ ----------
          continue;
        }

        double minDistX;
        double minDistY;
        const double dist3 = QgsGeometryUtils::sqrDistToLine( segmentData.x1, segmentData.y1,
                             candidateSegment.x1, candidateSegment.y1,
                             candidateSegment.x2, candidateSegment.y2, minDistX, minDistY, 0 );
        const double dist4 = QgsGeometryUtils::sqrDistToLine( segmentData.x2, segmentData.y2,
                             candidateSegment.x1, candidateSegment.y1,
                             candidateSegment.x2, candidateSegment.y2, minDistX, minDistY, 0 );

        const double minDistance = std::sqrt( std::min( std::min( std::min( dist1, dist2 ), dist3 ), dist4 ) );
        if ( minDistance > tolerance )
        {
          // lines are parallel, but minimum distance between them is larger than the tolerance
          continue;
        }

        // calculate split points - the distance along the segment at which the overlap starts/ends
        const double split1 = std::sqrt( std::pow( segmentData.x1 - ptX1, 2 ) + std::pow( segmentData.y1 - ptY1, 2 ) );
        splitPoints.insert( split1 );
        const double split2 = std::sqrt( std::pow( segmentData.x1 - ptX2, 2 ) + std::pow( segmentData.y1 - ptY2, 2 ) );
        splitPoints.insert( split2 );

        outSegments.insert( candidate );
      }

      splitPoints.remove( 0 );
      splitPoints.remove( std::sqrt( std::pow( segmentData.x1 - segmentData.x2, 2 ) + std::pow( segmentData.y1 - segmentData.y2, 2 ) ) );

      if ( !splitPoints.empty() )
      {
        split_points = [p1] + [QgsGeometryUtils.pointOnLineWithDistance( p1, p2, d ) for d in sorted( list( split_points ) )] + [p2];
        for ( i in range( len( split_points ) - 1 ) )
        {
          pp1 = split_points[i];
          pp2 = split_points[i + 1];

          centroid = QgsPointXY( 0.5 * ( pp1.x() + pp2.x() ), 0.5 * ( pp1.y() + pp2.y() ) );
          associated_group = segment_group_index.nearestNeighbor( centroid, maxDistance = tolerance );
          if ( associated_group )
          {
            // assigning to existing group
            split_segments[split_segment_id] = ( pp1, pp2, associated_group[0], len( segment_groups[associated_group[0]] ), fid, geometry_idx, i );

            segment_groups[associated_group[0]].append( split_segment_id );
          }
          else
          {
            // making a new group
            split_segments[split_segment_id] = ( pp1, pp2, segment_group_index_id, 0, fid, geometry_idx, i );
            segment_groups[segment_group_index_id] = [split_segment_id];
            segment_group_index.addFeature( segment_group_index_id, QgsRectangle( centroid, centroid ).buffered( 0.0001 ) );
            segment_group_index_id += 1;

            splitSegmentId++;
          }
        }
      }
      else
      {
        const QgsPointXY centroid( 0.5 * ( segmentData.x1 + segmentData.x2 ), 0.5 * ( segmentData.y1 + segmentData.y2 ) );
        associated_group = segment_group_index.nearestNeighbor( centroid, maxDistance = tolerance ) if out_segments else None;

      // TODO -- maybe if no out_segments we should store this elsewhere, since it will always be unchanged


      if ( associated_group )
        {
          // assigning to existing group
          split_segments[split_segment_id] = ( p1, p2, associated_group[0], len( segment_groups[associated_group[0]] ), fid, geometry_idx, 0 );

          segment_groups[associated_group[0]].append( split_segment_id );
        }
        else
        {
          // making a new group
          split_segments[split_segment_id] = ( p1, p2, segment_group_index_id, 0, fid, geometry_idx,  0 );
          segment_groups[segment_group_index_id] = [split_segment_id];
          segment_group_index.addFeature( segment_group_index_id, QgsRectangle( centroid, centroid ).buffered( 0.0001 ) );
          segment_group_index_id += 1;

          splitSegmentId++;
        }
      }
    }

  }

  mSegmentId = 0;
  mSegmentData.clear();
  mSegmentIndex.reset();

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
