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

void QgsLineClusterRenderer::drawGroup( QPointF centerPoint, QgsRenderContext &context, const ClusteredGroup &group ) const
{
  if ( group.size() > 1 )
  {
#if 0
    mClusterSymbol->renderLine( centerPoint, &( group.at( 0 ).feature ), context, -1, false );
#endif
  }
  else
  {
#if 0
    //single isolated symbol, draw it untouched
    QgsMarkerSymbol *symbol = group.at( 0 ).symbol();
    symbol->startRender( context );
    symbol->renderPoint( centerPoint, &( group.at( 0 ).feature ), context, -1, group.at( 0 ).isSelected );
    symbol->stopRender( context );
#endif
  }
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
  r->setAngleThreshold( symbologyElem.attribute( QStringLiteral( "angleTolerance" ), QStringLiteral( "10" ) ).toDouble() );

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
