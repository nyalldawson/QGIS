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
#include "qgsgeometryutils.h"
#include "qgsunittypes.h"
#include "qgssymbollayerutils.h"

QgsLinearReferencingSymbolLayer::QgsLinearReferencingSymbolLayer()
  : QgsLineSymbolLayer()
{
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
  res->setRotateLabels( properties.value( QStringLiteral( "rotate" ), true ).toBool() );
  res->setShowMarker( properties.value( QStringLiteral( "show_marker" ), false ).toBool() );

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

  if ( properties.contains( QStringLiteral( "label_offset" ) ) )
  {
    res->setLabelOffset( QgsSymbolLayerUtils::decodePoint( properties[QStringLiteral( "label_offset" )].toString() ) );
  }
  if ( properties.contains( QStringLiteral( "label_offset_unit" ) ) )
  {
    res->setLabelOffsetUnit( QgsUnitTypes::decodeRenderUnit( properties[QStringLiteral( "label_offset_unit" )].toString() ) );
  }
  if ( properties.contains( ( QStringLiteral( "label_offset_map_unit_scale" ) ) ) )
  {
    res->setLabelOffsetMapUnitScale( QgsSymbolLayerUtils::decodeMapUnitScale( properties[QStringLiteral( "label_offset_map_unit_scale" )].toString() ) );
  }

  return res.release();
}

QgsLinearReferencingSymbolLayer *QgsLinearReferencingSymbolLayer::clone() const
{
  std::unique_ptr< QgsLinearReferencingSymbolLayer > res = std::make_unique< QgsLinearReferencingSymbolLayer >();
  res->setInterval( mInterval );
  res->setSkipMultiplesOf( mSkipMultiplesOf );
  res->setRotateLabels( mRotateLabels );
  res->setLabelOffset( mLabelOffset );
  res->setLabelOffsetUnit( mLabelOffsetUnit );
  res->setLabelOffsetMapUnitScale( mLabelOffsetMapUnitScale );
  res->setShowMarker( mShowMarker );

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
      QStringLiteral( "rotate" ), mRotateLabels
    },
    {
      QStringLiteral( "show_marker" ), mShowMarker
    },
    {
      QStringLiteral( "text_format" ), textFormatDoc.toString()
    },
    {
      QStringLiteral( "numeric_format" ), numericFormatDoc.toString()
    },
    {
      QStringLiteral( "label_offset" ), QgsSymbolLayerUtils::encodePoint( mLabelOffset )
    },
    {
      QStringLiteral( "label_offset_unit" ), QgsUnitTypes::encodeUnit( mLabelOffsetUnit )
    },
    {
      QStringLiteral( "label_offset_map_unit_scale" ), QgsSymbolLayerUtils::encodeMapUnitScale( mLabelOffsetMapUnitScale )
    },
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
  return mShowMarker ? mMarkerSymbol.get() : nullptr;
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
    Qgis::SymbolRenderHints hints = mMarkerSymbol->renderHints() | Qgis::SymbolRenderHint::IsSymbolLayerSubSymbol;
    if ( mRotateLabels )
      hints |= Qgis::SymbolRenderHint::DynamicRotation;
    mMarkerSymbol->setRenderHints( hints );

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
  double labelOffsetX = mLabelOffset.x();
  double labelOffsetY = mLabelOffset.y();

  const double labelOffsetPainterUnitsX = context.renderContext().convertToPainterUnits( labelOffsetX, mLabelOffsetUnit, mLabelOffsetMapUnitScale );
  const double labelOffsetPainterUnitsY = context.renderContext().convertToPainterUnits( labelOffsetY, mLabelOffsetUnit, mLabelOffsetMapUnitScale );

  QgsNumericFormatContext numericContext;

  // TODO if NOT a original geometry, convert points to linestring and scale distance to painter units..

  if ( const QgsLineString *line = qgsgeometry_cast< const QgsLineString * >( geometry ) )
  {
    auto pointToPainter = [&context]( double x, double y, double z ) -> QPointF
    {
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
      return pt;
    };
    double currentDistance = 0;
    visitPointsByRegularDistance( line, distance, [&context, &currentDistance, &numericContext, distance, skipMultiples, &pointToPainter,
                                            labelOffsetPainterUnitsX, labelOffsetPainterUnitsY, this]( double x, double y, double z, double m,
                                      double startSegmentX, double startSegmentY, double startSegmentZ, double startSegmentM,
                                      double endSegmentX, double endSegmentY, double endSegmentZ, double endSegmentM
                                                                                           ) -> bool
    {
      ( void )startSegmentM;
      ( void )endSegmentM;

      currentDistance += distance;
      if ( skipMultiples > 0 && qgsDoubleNear( std::fmod( currentDistance,  skipMultiples ), 0 ) )
        return true;

      const QPointF pt = pointToPainter( x, y, z );
      const QPointF segmentStartPt = pointToPainter( startSegmentX, startSegmentY, startSegmentZ );
      const QPointF segmentEndPt = pointToPainter( endSegmentX, endSegmentY, endSegmentZ );

      double angle = mRotateLabels ? std::fmod( QgsGeometryUtilsBase::azimuth( segmentStartPt.x(), segmentStartPt.y(), segmentEndPt.x(), segmentEndPt.y() ) + 360, 360 ) : 0;
      if ( angle > 90 && angle < 270 )
        angle += 180;

      if ( mMarkerSymbol && mShowMarker )
      {
        if ( mRotateLabels )
          mMarkerSymbol->setLineAngle( 90 - angle );
        mMarkerSymbol->renderPoint( pt, context.feature(), context.renderContext() );
      }

      const double angleRadians = angle *M_PI / 180.0;
      const double dx = labelOffsetPainterUnitsX * std::sin( angleRadians + M_PI_2 )
      + labelOffsetPainterUnitsY * std::sin( angleRadians );
      const double dy = labelOffsetPainterUnitsX * std::cos( angleRadians + M_PI_2 )
      + labelOffsetPainterUnitsY * std::cos( angleRadians );

      QgsTextRenderer::drawText( QPointF( pt.x() + dx, pt.y() + dy ), angleRadians, Qgis::TextHorizontalAlignment::Left, { mNumericFormat->formatDouble( currentDistance, numericContext ) }, context.renderContext(), mTextFormat );

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

bool QgsLinearReferencingSymbolLayer::showMarker() const
{
  return mShowMarker;
}

void QgsLinearReferencingSymbolLayer::setShowMarker( bool show )
{
  mShowMarker = show;
  if ( show && !mMarkerSymbol )
  {
    mMarkerSymbol.reset( QgsMarkerSymbol::createSimple( {} ) );
  }
}
