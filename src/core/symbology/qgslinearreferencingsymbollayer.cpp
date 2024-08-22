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
#include "qgsnumericformatregistry.h"
#include "qgsapplication.h"
#include "qgsbasicnumericformat.h"

QgsLinearReferencingSymbolLayer::QgsLinearReferencingSymbolLayer()
  : QgsLineSymbolLayer()
{
  mMarkerSymbol.reset( QgsMarkerSymbol::createSimple( {} ) );
  mNumericFormat = std::make_unique< QgsBasicNumericFormat >();
}

QgsLinearReferencingSymbolLayer::~QgsLinearReferencingSymbolLayer() = default;

QgsSymbolLayer *QgsLinearReferencingSymbolLayer::create( const QVariantMap &properties )
{
  std::unique_ptr< QgsLinearReferencingSymbolLayer > res = std::make_unique< QgsLinearReferencingSymbolLayer >();
  bool ok = false;
  const double interval = properties.value( QStringLiteral( "interval" ) ).toDouble( &ok );
  if ( ok )
    res->setInterval( interval );
  const double skipMultiples = properties.value( QStringLiteral( "skip_multiples" ) ).toDouble( &ok );
  if ( ok )
    res->setSkipMultiplesOf( skipMultiples );

  // it's impossible to get the project's path resolver here :(
  // TODO QGIS 4.0 -- force use of QgsReadWriteContext in create methods
  QgsReadWriteContext rwContext;
  //rwContext.setPathResolver( QgsProject::instance()->pathResolver() );

  const QString textFormatXml = properties.value( QStringLiteral( "text_format" ) ).toString();
  if ( !textFormatXml.isEmpty() )
  {
    QDomDocument doc;
    QDomElement elem;
    doc.setContent( textFormatXml );
    elem = doc.documentElement();

    QgsTextFormat textFormat;
    textFormat.readXml( elem, rwContext );
    res->setTextFormat( textFormat );
  }

  const QString numericFormatXml = properties.value( QStringLiteral( "numeric_format" ) ).toString();
  if ( !numericFormatXml.isEmpty() )
  {
    QDomDocument doc;
    doc.setContent( numericFormatXml );
    res->setNumericFormat( QgsApplication::numericFormatRegistry()->createFromXml( doc.documentElement(), rwContext ) );
  }

  return res.release();
}

QgsLinearReferencingSymbolLayer *QgsLinearReferencingSymbolLayer::clone() const
{
  std::unique_ptr< QgsLinearReferencingSymbolLayer > res = std::make_unique< QgsLinearReferencingSymbolLayer >();
  res->setInterval( mInterval );
  res->setSkipMultiplesOf( mSkipMultiplesOf );

  res->mTextFormat = mTextFormat;
  res->mMarkerSymbol.reset( mMarkerSymbol ? mMarkerSymbol->clone() : nullptr );
  if ( mNumericFormat )
    res->mNumericFormat.reset( mNumericFormat->clone() );

  return res.release();
}

QVariantMap QgsLinearReferencingSymbolLayer::properties() const
{
  QDomDocument textFormatDoc;
  // it's impossible to get the project's path resolver here :(
  // TODO QGIS 4.0 -- force use of QgsReadWriteContext in properties methods
  QgsReadWriteContext rwContext;
  // rwContext.setPathResolver( QgsProject::instance()->pathResolver() );
  const QDomElement textElem = mTextFormat.writeXml( textFormatDoc, rwContext );
  textFormatDoc.appendChild( textElem );

  QDomDocument numericFormatDoc;
  QDomElement numericFormatElem = numericFormatDoc.createElement( QStringLiteral( "numericFormat" ) );
  mNumericFormat->writeXml( numericFormatElem, numericFormatDoc, rwContext );
  numericFormatDoc.appendChild( numericFormatElem );

  QVariantMap res
  {
    {
      QStringLiteral( "interval" ), mInterval
    },
    {
      QStringLiteral( "text_format" ), textFormatDoc.toString()
    },
    {
      QStringLiteral( "numeric_format" ), numericFormatDoc.toString()
    }
  };

  if ( mSkipMultiplesOf >= 0 )
  {
    res.insert( QStringLiteral( "skip_multiples" ), mSkipMultiplesOf );
  }

  return res;
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
  // TODO -- data defined
  double distance = mInterval;
  double skipMultiples = mSkipMultiplesOf;

  QgsNumericFormatContext numericContext;

  // TODO if NOT a original geometry, convert points to linestring and scale distance to painter units..

  if ( const QgsLineString *line = qgsgeometry_cast< const QgsLineString * >( geometry ) )
  {
    double currentDistance = 0;
    visitPointsByRegularDistance( line, distance, [&context, &currentDistance, &numericContext, distance, skipMultiples, this]( double x, double y, double z, double m,
                                  double startSegmentX, double startSegmentY, double startSegmentZ, double startSegmentM,
                                  double endSegmentX, double endSegmentY, double endSegmentZ, double endSegmentM
                                                                                                                              ) -> bool
    {
      currentDistance += distance;
      if ( skipMultiples > 0 && qgsDoubleNear( std::fmod( currentDistance,  skipMultiples ), 0 ) )
        return true;

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

      QgsTextRenderer::drawText( pt, 0, Qgis::TextHorizontalAlignment::Left, { mNumericFormat->formatDouble( currentDistance, numericContext ) }, context.renderContext(), mTextFormat );

      return true;
    } );

  }
}

QgsTextFormat QgsLinearReferencingSymbolLayer::textFormat() const
{
  return mTextFormat;
}

void QgsLinearReferencingSymbolLayer::setTextFormat( const QgsTextFormat &format )
{
  mTextFormat = format;
}

QgsNumericFormat *QgsLinearReferencingSymbolLayer::numericFormat() const
{
  return mNumericFormat.get();
}

void QgsLinearReferencingSymbolLayer::setNumericFormat( QgsNumericFormat *format )
{
  mNumericFormat.reset( format );
}

double QgsLinearReferencingSymbolLayer::interval() const
{
  return mInterval;
}

void QgsLinearReferencingSymbolLayer::setInterval( double interval )
{
  mInterval = interval;
}

double QgsLinearReferencingSymbolLayer::skipMultiplesOf() const
{
  return mSkipMultiplesOf;
}

void QgsLinearReferencingSymbolLayer::setSkipMultiplesOf( double skipMultiplesOf )
{
  mSkipMultiplesOf = skipMultiplesOf;
}
