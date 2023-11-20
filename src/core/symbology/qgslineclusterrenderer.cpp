/***************************************************************************
                              qgslineclusterrenderer.h
                              -------------------------
  begin                : November 2023
  copyright            : (C) 2023 by Nyall Dawson
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

#include "qgslineclusterrenderer.h"
#include "qgssymbollayerutils.h"
#include "qgsstyleentityvisitor.h"
#include "qgslinesymbol.h"
#include "qgsunittypes.h"

#include <cmath>

QgsLineClusterRenderer::QgsLineClusterRenderer()
  : QgsLineDistanceRenderer( QStringLiteral( "lineCluster" ) )
{
  mClusterSymbol = std::make_unique< QgsLineSymbol >();
}

QgsLineClusterRenderer *QgsLineClusterRenderer::clone() const
{
  std::unique_ptr< QgsLineClusterRenderer > r = std::make_unique< QgsLineClusterRenderer >();
  if ( mRenderer )
    r->setEmbeddedRenderer( mRenderer->clone() );
  r->setTolerance( mTolerance );
  r->setToleranceUnit( mToleranceUnit );
  r->setToleranceMapUnitScale( mToleranceMapUnitScale );
  r->setAngleThreshold( mAngleThreshold );
  if ( mClusterSymbol )
  {
    r->setClusterSymbol( mClusterSymbol->clone() );
  }
  copyRendererData( r.get() );
  return r.release();
}

void QgsLineClusterRenderer::startRender( QgsRenderContext &context, const QgsFields &fields )
{
  if ( mClusterSymbol )
  {
    mClusterSymbol->startRender( context, fields );
  }
  QgsLineDistanceRenderer::startRender( context, fields );
}

void QgsLineClusterRenderer::stopRender( QgsRenderContext &context )
{
  QgsLineDistanceRenderer::stopRender( context );
  if ( mClusterSymbol )
  {
    mClusterSymbol->stopRender( context );
  }
}

QgsFeatureRenderer *QgsLineClusterRenderer::create( QDomElement &symbologyElem, const QgsReadWriteContext &context )
{
  std::unique_ptr< QgsLineClusterRenderer > r = std::make_unique< QgsLineClusterRenderer >();
  r->setTolerance( symbologyElem.attribute( QStringLiteral( "tolerance" ), QStringLiteral( "3" ) ).toDouble() );
  r->setToleranceUnit( QgsUnitTypes::decodeRenderUnit( symbologyElem.attribute( QStringLiteral( "toleranceUnit" ), QStringLiteral( "MM" ) ) ) );
  r->setToleranceMapUnitScale( QgsSymbolLayerUtils::decodeMapUnitScale( symbologyElem.attribute( QStringLiteral( "toleranceUnitScale" ) ) ) );
  r->setAngleThreshold( symbologyElem.attribute( QStringLiteral( "angleThreshold" ), QStringLiteral( "10" ) ).toDouble() );

  //look for an embedded renderer <renderer-v2>
  QDomElement embeddedRendererElem = symbologyElem.firstChildElement( QStringLiteral( "renderer-v2" ) );
  if ( !embeddedRendererElem.isNull() )
  {
    r->setEmbeddedRenderer( QgsFeatureRenderer::load( embeddedRendererElem, context ) );
  }

  //cluster symbol
  const QDomElement lineSymbolElem = symbologyElem.firstChildElement( QStringLiteral( "symbol" ) );
  if ( !lineSymbolElem.isNull() )
  {
    r->setClusterSymbol( QgsSymbolLayerUtils::loadSymbol<QgsLineSymbol>( lineSymbolElem, context ) );
  }
  return r.release();
}

QgsLineSymbol *QgsLineClusterRenderer::clusterSymbol()
{
  return mClusterSymbol.get();
}

QDomElement QgsLineClusterRenderer::save( QDomDocument &doc, const QgsReadWriteContext &context )
{
  QDomElement rendererElement = doc.createElement( RENDERER_TAG_NAME );
  rendererElement.setAttribute( QStringLiteral( "type" ), QStringLiteral( "lineCluster" ) );
  rendererElement.setAttribute( QStringLiteral( "tolerance" ), QString::number( mTolerance ) );
  rendererElement.setAttribute( QStringLiteral( "toleranceUnit" ), QgsUnitTypes::encodeUnit( mToleranceUnit ) );
  rendererElement.setAttribute( QStringLiteral( "toleranceUnitScale" ), QgsSymbolLayerUtils::encodeMapUnitScale( mToleranceMapUnitScale ) );
  rendererElement.setAttribute( QStringLiteral( "angleThreshold" ), mAngleThreshold );

  if ( mRenderer )
  {
    const QDomElement embeddedRendererElem = mRenderer->save( doc, context );
    rendererElement.appendChild( embeddedRendererElem );
  }
  if ( mClusterSymbol )
  {
    const QDomElement centerSymbolElem = QgsSymbolLayerUtils::saveSymbol( QStringLiteral( "clusterSymbol" ), mClusterSymbol.get(), doc, context );
    rendererElement.appendChild( centerSymbolElem );
  }

  saveRendererData( doc, rendererElement, context );

  return rendererElement;
}

QSet<QString> QgsLineClusterRenderer::usedAttributes( const QgsRenderContext &context ) const
{
  QSet<QString> attr = QgsLineDistanceRenderer::usedAttributes( context );
  if ( mClusterSymbol )
    attr.unite( mClusterSymbol->usedAttributes( context ) );
  return attr;
}

bool QgsLineClusterRenderer::accept( QgsStyleEntityVisitorInterface *visitor ) const
{
  if ( !QgsLineDistanceRenderer::accept( visitor ) )
    return false;

  if ( mClusterSymbol )
  {
    QgsStyleSymbolEntity entity( mClusterSymbol.get() );
    if ( !visitor->visit( QgsStyleEntityVisitorInterface::StyleLeaf( &entity, QStringLiteral( "cluster" ), QObject::tr( "Cluster Symbol" ) ) ) )
      return false;
  }

  return true;
}

void QgsLineClusterRenderer::setClusterSymbol( QgsLineSymbol *symbol )
{
  mClusterSymbol.reset( symbol );
}

QgsLineClusterRenderer *QgsLineClusterRenderer::convertFromRenderer( const QgsFeatureRenderer *renderer )
{
  if ( renderer->type() == QLatin1String( "lineCluster" ) )
  {
    return dynamic_cast<QgsLineClusterRenderer *>( renderer->clone() );
  }
  else if ( renderer->type() == QLatin1String( "singleSymbol" ) ||
            renderer->type() == QLatin1String( "categorizedSymbol" ) ||
            renderer->type() == QLatin1String( "graduatedSymbol" ) ||
            renderer->type() == QLatin1String( "RuleRenderer" ) )
  {
    std::unique_ptr< QgsLineClusterRenderer > lineRenderer = std::make_unique< QgsLineClusterRenderer >();
    lineRenderer->setEmbeddedRenderer( renderer->clone() );
    renderer->copyRendererData( lineRenderer.get() );
    return lineRenderer.release();
  }
#if 0
  else if ( renderer->type() == QLatin1String( "pointDisplacement" ) )
  {
    QgsPointClusterRenderer *pointRenderer = new QgsPointClusterRenderer();
    const QgsPointDisplacementRenderer *displacementRenderer = static_cast< const QgsPointDisplacementRenderer * >( renderer );
    if ( displacementRenderer->embeddedRenderer() )
      pointRenderer->setEmbeddedRenderer( displacementRenderer->embeddedRenderer()->clone() );
    pointRenderer->setTolerance( displacementRenderer->tolerance() );
    pointRenderer->setToleranceUnit( displacementRenderer->toleranceUnit() );
    pointRenderer->setToleranceMapUnitScale( displacementRenderer->toleranceMapUnitScale() );
    if ( const_cast< QgsPointDisplacementRenderer * >( displacementRenderer )->centerSymbol() )
      pointRenderer->setClusterSymbol( const_cast< QgsPointDisplacementRenderer * >( displacementRenderer )->centerSymbol()->clone() );
    renderer->copyRendererData( pointRenderer );
    return pointRenderer;
  }
#endif
  else
  {
    return nullptr;
  }
}

void QgsLineClusterRenderer::drawGroups( QgsRenderContext &context, const QVector<QgsFeature> &features, const QHash<QgsFeatureId, QList<int> > &featureIdToSegments, const QHash<int, QList<int> > &segmentGroups, const QHash<int, SplitSegment> &splitSegments ) const
{
  // process segment groups to generate clustered segments
  QHash< int, QVector< double > > processedGroups;
  QHash< int, QSet< int > > reversedSegments;
  for ( auto it = segmentGroups.constBegin(); it != segmentGroups.constEnd(); ++it )
  {
    if ( it.value().length() > 1 )
    {
      auto valueIt = it.value().constBegin();
      const SplitSegment &firstSegment = splitSegments.value( *valueIt++ );

      // calculate centroid of clustered segment start/end
      double sumStartPointX = firstSegment.x1;
      double sumStartPointY = firstSegment.y1;
      double sumEndPointX = firstSegment.x2;
      double sumEndPointY = firstSegment.y2;

      // flip segment directions if needed so that all segments in group are the same direction;

      for ( ; valueIt != it.value().constEnd(); ++valueIt )
      {
        const SplitSegment &thisSegment = splitSegments.value( *valueIt );
        if ( ( std::pow( thisSegment.x1 - firstSegment.x1, 2 ) + std::pow( thisSegment.y1 - firstSegment.y1, 2 ) )
             < ( std::pow( thisSegment.x1 - firstSegment.x2, 2 ) + std::pow( thisSegment.y1 - firstSegment.y2, 2 ) ) )
        {
          sumStartPointX += thisSegment.x1;
          sumStartPointY += thisSegment.y1;
          sumEndPointX += thisSegment.x2;
          sumEndPointY += thisSegment.y2;
        }
        else
        {
          sumStartPointX += thisSegment.x2;
          sumStartPointY += thisSegment.y2;
          sumEndPointX += thisSegment.x1;
          sumEndPointY += thisSegment.y1;

          reversedSegments[it.key()].insert( thisSegment.indexInGroup );
        }
      }

      const int size = it.value().size();
      const double averageStartPointX = sumStartPointX / size;
      const double averageStartPointY = sumStartPointY / size;
      const double averageEndPointX = sumEndPointX / size;
      const double averageEndPointY = sumEndPointY / size;

      processedGroups[it.key()] = { averageStartPointX, averageStartPointY, averageEndPointX, averageEndPointY };
    }
  }

  // concatenate segments back to linestrings
  struct CollapsedSegment
  {
    int indexInGeometry;
    int indexInSplit;
    QVector< double > segment;
    int groupSize;
  };

  QHash< QgsFeatureId, QVector<CollapsedSegment> > collapsedSegments;

  for ( auto it = splitSegments.constBegin(); it != splitSegments.constEnd(); ++ it )
  {
    CollapsedSegment collapsedSegment;
    collapsedSegment.indexInGeometry = it->indexInGeometry;
    collapsedSegment.indexInSplit = it->indexInSplit;

    auto processedGroup = processedGroups.constFind( it.value().segmentGroup );
    if ( processedGroup != processedGroups.constEnd() )
    {
      auto reversedIt = reversedSegments.constFind( it.value().segmentGroup );
      collapsedSegment.segment = processedGroups[it.value().segmentGroup];
      if ( reversedIt != reversedSegments.constEnd() && reversedIt->contains( it.value().indexInGroup ) )
      {
        // we need to reverse the segment to match the original line segment direction
        collapsedSegment.segment =
        {
          collapsedSegment.segment[2], collapsedSegment.segment[3],
          collapsedSegment.segment[0], collapsedSegment.segment[1]
        };
      }
      collapsedSegment.groupSize = segmentGroups.value( it.value().segmentGroup ).size();
    }
    else
    {
      collapsedSegment.groupSize = 1;
      collapsedSegment.segment = { it.value().x1, it.value().y1, it.value().x2, it.value().y2 };
    }

    collapsedSegments[it->feature.id()].append( collapsedSegment );
  }

  for ( const QgsFeature &feature : features )
  {
    context.expressionContext().setFeature( feature );
    if ( QgsLineSymbol *symbol = firstSymbolForFeature( feature, context ) )
    {
      QVector< CollapsedSegment > segments = collapsedSegments.value( feature.id() );
      std::sort( segments.begin(), segments.end(), []( const CollapsedSegment & s1, const CollapsedSegment & s2 )
      {
        return s1.indexInGeometry < s2.indexInGeometry
               || ( s1.indexInGeometry == s2.indexInGeometry && s1.indexInSplit < s2.indexInSplit );
      } );

      double prevEndX = segments.at( 0 ).segment[0];
      double prevEndY = segments.at( 0 ).segment[1];

      for ( int i = 0; i < segments.size(); ++ i )
      {
        QgsLineSymbol *thisSymbol = nullptr;
        const CollapsedSegment &segment = segments[i];
        const double thisStartX = prevEndX;
        const double thisStartY = prevEndY;
        double thisEndX = segment.segment[2];
        double thisEndY = segment.segment[3];
        if ( i < segments.size() - 1 )
        {
          const CollapsedSegment &nextSegment = segments[i + 1];
          if ( nextSegment.groupSize > segment.groupSize )
          {
            thisEndX = nextSegment.segment[0];
            thisEndY = nextSegment.segment[1];
          }
        }

        if ( segment.groupSize > 1 )
        {
          thisSymbol = mClusterSymbol.get();
        }
        else
        {
          thisSymbol = symbol;
        }

        thisSymbol->renderPolyline( QPolygonF( QVector<QPointF> { QPointF( thisStartX, thisStartY ), QPointF( thisEndX, thisEndY )} ),
                                    &feature,
                                    context );

        prevEndX = thisEndX;
        prevEndY = thisEndY;
      }
    }
  }

#if 0
  if ( group.size() > 1 )
  {

    mClusterSymbol->renderLine( centerPoint, &( group.at( 0 ).feature ), context, -1, false );

  }
  else
  {

    //single isolated symbol, draw it untouched
    QgsMarkerSymbol *symbol = group.at( 0 ).symbol();
    symbol->startRender( context );
    symbol->renderPoint( centerPoint, &( group.at( 0 ).feature ), context, -1, group.at( 0 ).isSelected );
    symbol->stopRender( context );

  }
}
#endif

}
