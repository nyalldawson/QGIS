/***************************************************************************
    qgslinedisplacementrenderer.h
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

#include "qgslinedisplacementrenderer.h"

#include "qgssymbol.h"
#include "qgssymbollayerutils.h"

#include "qgsfeature.h"
#include "qgsvectorlayer.h"
#include "qgsstyleentityvisitor.h"
#include "qgsunittypes.h"

#include <QDomDocument>
#include <QDomElement>

QgsLineDisplacementRenderer::QgsLineDisplacementRenderer( QgsFeatureRenderer *subRenderer )
  : QgsLineDistanceRenderer( QStringLiteral( "lineDisplacement" ) )
{
  if ( subRenderer )
  {
    mRenderer.reset( subRenderer );
  }
}

QgsLineDisplacementRenderer *QgsLineDisplacementRenderer::clone() const
{
  std::unique_ptr< QgsLineDisplacementRenderer > r = std::make_unique< QgsLineDisplacementRenderer >( nullptr );
  if ( mRenderer )
    r->setEmbeddedRenderer( mRenderer->clone() );

  r->setTolerance( mTolerance );
  r->setToleranceUnit( mToleranceUnit );
  r->setToleranceMapUnitScale( mToleranceMapUnitScale );
  r->setAngleThreshold( mAngleThreshold );

  copyRendererData( r.get() );
  return r.release();
}

bool QgsLineDisplacementRenderer::accept( QgsStyleEntityVisitorInterface *visitor ) const
{
  if ( !QgsLineDistanceRenderer::accept( visitor ) )
    return false;

  return true;
}

void QgsLineDisplacementRenderer::startRender( QgsRenderContext &context, const QgsFields &fields )
{
  QgsLineDistanceRenderer::startRender( context, fields );
}

void QgsLineDisplacementRenderer::stopRender( QgsRenderContext &context )
{
  QgsLineDistanceRenderer::stopRender( context );
}

QgsFeatureRenderer *QgsLineDisplacementRenderer::create( QDomElement &element, const QgsReadWriteContext &context )
{
  QgsLineDisplacementRenderer *r = new QgsLineDisplacementRenderer( nullptr );
  //look for an embedded renderer <renderer-v2>
  QDomElement embeddedRendererElem = element.firstChildElement( QStringLiteral( "renderer-v2" ) );
  if ( !embeddedRendererElem.isNull() )
  {
    QgsFeatureRenderer *renderer = QgsFeatureRenderer::load( embeddedRendererElem, context );
    r->setEmbeddedRenderer( renderer );
  }

  r->setTolerance( element.attribute( QStringLiteral( "tolerance" ), QStringLiteral( "0.00001" ) ).toDouble() );
  r->setToleranceUnit( QgsUnitTypes::decodeRenderUnit( element.attribute( QStringLiteral( "toleranceUnit" ), QStringLiteral( "MapUnit" ) ) ) );
  r->setToleranceMapUnitScale( QgsSymbolLayerUtils::decodeMapUnitScale( element.attribute( QStringLiteral( "toleranceUnitScale" ) ) ) );
  r->setAngleThreshold( element.attribute( QStringLiteral( "angleThreshold" ), QStringLiteral( "10" ) ).toDouble() );

  return r;
}

QDomElement QgsLineDisplacementRenderer::save( QDomDocument &doc, const QgsReadWriteContext &context )
{
  // clazy:skip

  QDomElement rendererElem = doc.createElement( RENDERER_TAG_NAME );
  rendererElem.setAttribute( QStringLiteral( "type" ), QStringLiteral( "lineDisplacement" ) );

  rendererElem.setAttribute( QStringLiteral( "tolerance" ), QString::number( mTolerance ) );
  rendererElem.setAttribute( QStringLiteral( "toleranceUnit" ), QgsUnitTypes::encodeUnit( mToleranceUnit ) );
  rendererElem.setAttribute( QStringLiteral( "toleranceUnitScale" ), QgsSymbolLayerUtils::encodeMapUnitScale( mToleranceMapUnitScale ) );
  rendererElem.setAttribute( QStringLiteral( "angleThreshold" ), mAngleThreshold );

  if ( mRenderer )
  {
    QDomElement embeddedRendererElem = mRenderer->save( doc, context );
    rendererElem.appendChild( embeddedRendererElem );
  }

  saveRendererData( doc, rendererElem, context );

  return rendererElem;
}

QSet<QString> QgsLineDisplacementRenderer::usedAttributes( const QgsRenderContext &context ) const
{
  return QgsLineDistanceRenderer::usedAttributes( context );
}

QgsLineDisplacementRenderer *QgsLineDisplacementRenderer::convertFromRenderer( const QgsFeatureRenderer *renderer )
{
  if ( renderer->type() == QLatin1String( "lineDisplacement" ) )
  {
    return dynamic_cast<QgsLineDisplacementRenderer *>( renderer->clone() );
  }

  if ( renderer->type() == QLatin1String( "singleSymbol" ) ||
       renderer->type() == QLatin1String( "categorizedSymbol" ) ||
       renderer->type() == QLatin1String( "graduatedSymbol" ) ||
       renderer->type() == QLatin1String( "RuleRenderer" ) )
  {
    std::unique_ptr< QgsLineDisplacementRenderer > res = std::make_unique< QgsLineDisplacementRenderer >( renderer->clone() );
    renderer->copyRendererData( res.get() );
    return res.release();
  }
  return nullptr;
}

void QgsLineDisplacementRenderer::drawGroups( QgsRenderContext &context, const QVector<QgsFeature> &features, const QHash<QgsFeatureId, QList<int> > &featureIdToSegments,
    const QHash< int, QList< int> > &segmentGroups,
    const QHash< int, SplitSegment> &splitSegments
                                            ) const
{

}

