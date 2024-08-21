/***************************************************************************
    qgslinearreferencingsymbollayer.h
    ---------------------
    begin                : August 2024
    copyright            : (C) 2024 by Nyall Dawson
    email                : nyall dot dawson at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgslinearreferencingsymbollayer.h"
#include "qgsrendercontext.h"
#include "qgstextrenderer.h"
#include "qgslinestring.h"
#include "qgsmarkersymbol.h"
#include <QPainter>
#include <QPen>

QgsLinearReferencingSymbolLayer::QgsLinearReferencingSymbolLayer()
  : QgsLineSymbolLayer()
{
  mMarkerSymbol.reset( QgsMarkerSymbol::createSimple( {} ) );
}

QgsLinearReferencingSymbolLayer::~QgsLinearReferencingSymbolLayer() = default;

QgsSymbolLayer *QgsLinearReferencingSymbolLayer::create( const QVariantMap &properties )
{
  return new QgsLinearReferencingSymbolLayer();
}

QgsLinearReferencingSymbolLayer *QgsLinearReferencingSymbolLayer::clone() const
{
  std::unique_ptr< QgsLinearReferencingSymbolLayer > res = std::make_unique< QgsLinearReferencingSymbolLayer >();
  res->mMarkerSymbol.reset( mMarkerSymbol ? mMarkerSymbol->clone() : nullptr );

  return res.release();
}

QVariantMap QgsLinearReferencingSymbolLayer::properties() const
{
  return {};
}

QString QgsLinearReferencingSymbolLayer::layerType() const
{
  return QStringLiteral( "LinearReferencing" );
}

Qgis::SymbolLayerFlags QgsLinearReferencingSymbolLayer::flags() const
{
  return Qgis::SymbolLayerFlag::DisableFeatureClipping;
}

QgsSymbol *QgsLinearReferencingSymbolLayer::subSymbol()
{
  return mMarkerSymbol.get();
}

bool QgsLinearReferencingSymbolLayer::setSubSymbol( QgsSymbol *symbol )
{
  if ( symbol && symbol->type() == Qgis::SymbolType::Marker )
  {
    mMarkerSymbol.reset( qgis::down_cast<QgsMarkerSymbol *>( symbol ) );
    return true;
  }
  delete symbol;
  return false;
}

void QgsLinearReferencingSymbolLayer::startRender( QgsSymbolRenderContext &context )
{
  if ( mMarkerSymbol )
  {
    mMarkerSymbol->setRenderHints( mMarkerSymbol->renderHints() | Qgis::SymbolRenderHint::IsSymbolLayerSubSymbol );
    mMarkerSymbol->startRender( context.renderContext(), context.fields() );
  }
}

void QgsLinearReferencingSymbolLayer::stopRender( QgsSymbolRenderContext &context )
{
  if ( mMarkerSymbol )
  {
    mMarkerSymbol->stopRender( context.renderContext() );
  }
}


void visitPointsByRegularDistance( const QgsLineString *line, const double distance, const std::function<bool ( double, double, double, double, double, double, double, double, double, double, double, double )> &visitPoint )
{
  if ( distance < 0 )
    return;

  double distanceTraversed = 0;
  const int totalPoints = line->numPoints();
  if ( totalPoints == 0 )
    return;

  const double *x = line->xData();
  const double *y = line->yData();
  const double *z = line->is3D() ? line->zData() : nullptr;
  const double *m = line->isMeasure() ? line->mData() : nullptr;

  double prevX = *x++;
  double prevY = *y++;
  double prevZ = z ? *z++ : 0.0;
  double prevM = m ? *m++ : 0.0;

  if ( qgsDoubleNear( distance, 0.0 ) )
  {
    visitPoint( prevX, prevY, prevZ, prevM, prevX, prevY, prevZ, prevM, prevX, prevY, prevZ, prevM );
    return;
  }

  double pZ = std::numeric_limits<double>::quiet_NaN();
  double pM = std::numeric_limits<double>::quiet_NaN();
  double nextPointDistance = distance;
  for ( int i = 1; i < totalPoints; ++i )
  {
    double thisX = *x++;
    double thisY = *y++;
    double thisZ = z ? *z++ : 0.0;
    double thisM = m ? *m++ : 0.0;

    const double segmentLength = QgsGeometryUtilsBase::distance2D( thisX, thisY, prevX, prevY );
    while ( nextPointDistance < distanceTraversed + segmentLength || qgsDoubleNear( nextPointDistance, distanceTraversed + segmentLength ) )
    {
      // point falls on this segment - truncate to segment length if qgsDoubleNear test was actually > segment length
      const double distanceToPoint = std::min( nextPointDistance - distanceTraversed, segmentLength );
      double pX, pY;
      QgsGeometryUtilsBase::pointOnLineWithDistance( prevX, prevY, thisX, thisY, distanceToPoint, pX, pY,
          z ? &prevZ : nullptr, z ? &thisZ : nullptr, z ? &pZ : nullptr,
          m ? &prevM : nullptr, m ? &thisM : nullptr, m ? &pM : nullptr );

      if ( !visitPoint( pX, pY, pZ, pM, prevX, prevY, prevZ, prevM, thisX, thisY, thisZ, thisM ) )
        return;

      nextPointDistance += distance;
    }

    distanceTraversed += segmentLength;
    prevX = thisX;
    prevY = thisY;
    prevZ = thisZ;
    prevM = thisM;
  }
}

void QgsLinearReferencingSymbolLayer::renderPolyline( const QPolygonF &points, QgsSymbolRenderContext &context )
{
  QPainter *p = context.renderContext().painter();
  if ( !p )
  {
    return;
  }

  const QgsAbstractGeometry *geometry = context.renderContext().geometry();

  p->setBrush( Qt::NoBrush );
  QPen pen( QColor( 255, 0, 0 ) );
  p->setPen( pen );
  QPainterPath path;
  path.addPolygon( points );
  p->drawPath( path );

  double distance = 0.5;

  // TODO if NOT a original geometry, convert points to linestring and scale distance to painter units..

  if ( const QgsLineString *line = qgsgeometry_cast< const QgsLineString * >( geometry ) )
  {
    double currentDistance = 0;
    visitPointsByRegularDistance( line, distance, [&context, &currentDistance, distance, this]( double x, double y, double z, double m,
                                  double startSegmentX, double startSegmentY, double startSegmentZ, double startSegmentM,
                                  double endSegmentX, double endSegmentY, double endSegmentZ, double endSegmentM
                                                                                              ) -> bool
    {
      currentDistance += distance;

      // need to convert layer point to painter paint
      QPointF pt;
      if ( context.renderContext().coordinateTransform().isValid() )
      {
        context.renderContext().coordinateTransform().transformInPlace( x, y, z );
        pt = QPointF( x, y );

      }
      else
      {
        pt = QPointF( x, y );
      }

      context.renderContext().mapToPixel().transformInPlace( pt.rx(), pt.ry() );
      if ( mMarkerSymbol )
        mMarkerSymbol->renderPoint( pt, context.feature(), context.renderContext() );

      QgsTextRenderer::drawText( pt, 0, Qgis::TextHorizontalAlignment::Left, { QString::number( currentDistance ) }, context.renderContext(), mTextFormat );

      return true;
    } );

  }
}


