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
  res->setPlacement( qgsEnumKeyToValue( properties.value( QStringLiteral( "placement" ) ).toString(), Qgis::LinearReferencingPlacement::Interval ) );
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
  if ( properties.contains( QStringLiteral( "average_angle_length" ) ) )
  {
    res->setAverageAngleLength( properties[QStringLiteral( "average_angle_length" )].toDouble() );
  }
  if ( properties.contains( QStringLiteral( "average_angle_unit" ) ) )
  {
    res->setAverageAngleUnit( QgsUnitTypes::decodeRenderUnit( properties[QStringLiteral( "average_angle_unit" )].toString() ) );
  }
  if ( properties.contains( ( QStringLiteral( "average_angle_map_unit_scale" ) ) ) )
  {
    res->setAverageAngleMapUnitScale( QgsSymbolLayerUtils::decodeMapUnitScale( properties[QStringLiteral( "average_angle_map_unit_scale" )].toString() ) );
  }

  return res.release();
}

QgsLinearReferencingSymbolLayer *QgsLinearReferencingSymbolLayer::clone() const
{
  std::unique_ptr< QgsLinearReferencingSymbolLayer > res = std::make_unique< QgsLinearReferencingSymbolLayer >();
  res->setPlacement( mPlacement );
  res->setInterval( mInterval );
  res->setSkipMultiplesOf( mSkipMultiplesOf );
  res->setRotateLabels( mRotateLabels );
  res->setLabelOffset( mLabelOffset );
  res->setLabelOffsetUnit( mLabelOffsetUnit );
  res->setLabelOffsetMapUnitScale( mLabelOffsetMapUnitScale );
  res->setShowMarker( mShowMarker );
  res->setAverageAngleLength( mAverageAngleLength );
  res->setAverageAngleUnit( mAverageAngleLengthUnit );
  res->setAverageAngleMapUnitScale( mAverageAngleLengthMapUnitScale );

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
      QStringLiteral( "placement" ), qgsEnumValueToKey( mPlacement )
    },
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
    {
      QStringLiteral( "average_angle_length" ), mAverageAngleLength
    },
    {
      QStringLiteral( "average_angle_unit" ), QgsUnitTypes::encodeUnit( mAverageAngleLengthUnit )
    },
    {
      QStringLiteral( "average_angle_map_unit_scale" ), QgsSymbolLayerUtils::encodeMapUnitScale( mAverageAngleLengthMapUnitScale )
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

void QgsLinearReferencingSymbolLayer::renderPolyline( const QPolygonF &points, QgsSymbolRenderContext &context )
{
  QPainter *p = context.renderContext().painter();
  if ( !p )
  {
    return;
  }

  // TODO -- data defined
  double skipMultiples = mSkipMultiplesOf;
  double labelOffsetX = mLabelOffset.x();
  double labelOffsetY = mLabelOffset.y();
  double averageAngle = mAverageAngleLength;

  const double labelOffsetPainterUnitsX = context.renderContext().convertToPainterUnits( labelOffsetX, mLabelOffsetUnit, mLabelOffsetMapUnitScale );
  const double labelOffsetPainterUnitsY = context.renderContext().convertToPainterUnits( labelOffsetY, mLabelOffsetUnit, mLabelOffsetMapUnitScale );
  const double averageAngleDistancePainterUnits = context.renderContext().convertToPainterUnits( averageAngle, mAverageAngleLengthUnit, mAverageAngleLengthMapUnitScale ) / 2;

  switch ( mPlacement )
  {
    case Qgis::LinearReferencingPlacement::Interval:
      renderPolylineInterval( points, context, skipMultiples, QPointF( labelOffsetPainterUnitsX, labelOffsetPainterUnitsY ), averageAngleDistancePainterUnits );
      break;

    case Qgis::LinearReferencingPlacement::Vertex:
      renderPolylineVertex( points, context, skipMultiples, QPointF( labelOffsetPainterUnitsX, labelOffsetPainterUnitsY ), averageAngleDistancePainterUnits );
      break;
  }
}

void visitPointsByRegularDistance( const QgsLineString *line, const QgsLineString *linePainterUnits, const double distance, const double averageAngleLengthPainterUnits, const std::function<bool ( double, double, double, double, double )> &visitPoint )
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

  const double *xPainterUnits = linePainterUnits->xData();
  const double *yPainterUnits = linePainterUnits->yData();

  double prevX = *x++;
  double prevY = *y++;
  double prevZ = z ? *z++ : 0.0;
  double prevM = m ? *m++ : 0.0;

  double prevXPainterUnits = *xPainterUnits++;
  double prevYPainterUnits = *yPainterUnits++;

  if ( qgsDoubleNear( distance, 0.0 ) )
  {
    visitPoint( prevX, prevY, prevZ, prevM, 0 );
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
    double thisXPainterUnits = *xPainterUnits++;
    double thisYPainterUnits = *yPainterUnits++;

    double angle = std::fmod( QgsGeometryUtilsBase::azimuth( prevXPainterUnits, prevYPainterUnits, thisXPainterUnits, thisYPainterUnits ) + 360, 360 );
    if ( angle > 90 && angle < 270 )
      angle += 180;

    const double segmentLength = QgsGeometryUtilsBase::distance2D( thisX, thisY, prevX, prevY );
    const double segmentLengthPainterUnits = QgsGeometryUtilsBase::distance2D( thisXPainterUnits, thisYPainterUnits, prevXPainterUnits, prevYPainterUnits );

    while ( nextPointDistance < distanceTraversed + segmentLength || qgsDoubleNear( nextPointDistance, distanceTraversed + segmentLength ) )
    {
      // point falls on this segment - truncate to segment length if qgsDoubleNear test was actually > segment length
      const double distanceToPoint = std::min( nextPointDistance - distanceTraversed, segmentLength );
      double pX, pY;
      QgsGeometryUtilsBase::pointOnLineWithDistance( prevX, prevY, thisX, thisY, distanceToPoint, pX, pY,
          z ? &prevZ : nullptr, z ? &thisZ : nullptr, z ? &pZ : nullptr,
          m ? &prevM : nullptr, m ? &thisM : nullptr, m ? &pM : nullptr );

      double calculatedAngle = angle;
      if ( averageAngleLengthPainterUnits > 0 )
      {
        const double targetPointFractionAlongSegment = distanceToPoint / segmentLength;
        const double targetPointDistanceAlongSegment = targetPointFractionAlongSegment * segmentLengthPainterUnits;

        // track forward by averageAngleLengthPainterUnits
        double painterDistRemaining = averageAngleLengthPainterUnits + targetPointDistanceAlongSegment;
        double startAverageSegmentX = prevXPainterUnits;
        double startAverageSegmentY = prevYPainterUnits;
        double endAverageSegmentX = thisXPainterUnits;
        double endAverageSegmentY = thisYPainterUnits;
        double averagingSegmentLengthPainterUnits = segmentLengthPainterUnits;
        const double *xAveragingData = xPainterUnits;
        const double *yAveragingData = yPainterUnits;

        int j = i;
        while ( painterDistRemaining > averagingSegmentLengthPainterUnits )
        {
          if ( j >= totalPoints - 1 )
            break;

          painterDistRemaining -= averagingSegmentLengthPainterUnits;
          startAverageSegmentX = endAverageSegmentX;
          startAverageSegmentY = endAverageSegmentY;

          endAverageSegmentX = *xAveragingData++;
          endAverageSegmentY = *yAveragingData++;
          j++;
          averagingSegmentLengthPainterUnits = QgsGeometryUtilsBase::distance2D( startAverageSegmentX, startAverageSegmentY, endAverageSegmentX, endAverageSegmentY );
        }
        // fits on this same segment
        double endAverageXPainterUnits;
        double endAverageYPainterUnits;
        if ( painterDistRemaining < averagingSegmentLengthPainterUnits )
        {
          QgsGeometryUtilsBase::pointOnLineWithDistance( startAverageSegmentX, startAverageSegmentY, endAverageSegmentX, endAverageSegmentY, painterDistRemaining, endAverageXPainterUnits, endAverageYPainterUnits,
              nullptr, nullptr, nullptr,
              nullptr, nullptr, nullptr );
        }
        else
        {
          endAverageXPainterUnits = endAverageSegmentX;
          endAverageYPainterUnits = endAverageSegmentY;
        }

        // also track back by averageAngleLengthPainterUnits
        j = i;
        painterDistRemaining = ( segmentLengthPainterUnits - targetPointDistanceAlongSegment ) + averageAngleLengthPainterUnits;
        startAverageSegmentX = thisXPainterUnits;
        startAverageSegmentY = thisYPainterUnits;
        endAverageSegmentX = prevXPainterUnits;
        endAverageSegmentY = prevYPainterUnits;
        averagingSegmentLengthPainterUnits = segmentLengthPainterUnits;
        xAveragingData = xPainterUnits - 2;
        yAveragingData = yPainterUnits - 2;
        while ( painterDistRemaining > averagingSegmentLengthPainterUnits )
        {
          if ( j < 1 )
            break;

          painterDistRemaining -= averagingSegmentLengthPainterUnits;
          startAverageSegmentX = endAverageSegmentX;
          startAverageSegmentY = endAverageSegmentY;

          endAverageSegmentX = *xAveragingData--;
          endAverageSegmentY = *yAveragingData--;
          j--;
          averagingSegmentLengthPainterUnits = QgsGeometryUtilsBase::distance2D( startAverageSegmentX, startAverageSegmentY, endAverageSegmentX, endAverageSegmentY );
        }
        // fits on this same segment
        double startAverageXPainterUnits;
        double startAverageYPainterUnits;
        if ( painterDistRemaining < averagingSegmentLengthPainterUnits )
        {
          QgsGeometryUtilsBase::pointOnLineWithDistance( startAverageSegmentX, startAverageSegmentY, endAverageSegmentX, endAverageSegmentY, painterDistRemaining, startAverageXPainterUnits, startAverageYPainterUnits,
              nullptr, nullptr, nullptr,
              nullptr, nullptr, nullptr );
        }
        else
        {
          startAverageXPainterUnits = endAverageSegmentX;
          startAverageYPainterUnits = endAverageSegmentY;
        }

        calculatedAngle = std::fmod( QgsGeometryUtilsBase::azimuth( startAverageXPainterUnits, startAverageYPainterUnits, endAverageXPainterUnits, endAverageYPainterUnits ) + 360, 360 );
        if ( calculatedAngle > 90 && calculatedAngle < 270 )
          calculatedAngle += 180;
      }

      if ( !visitPoint( pX, pY, pZ, pM, calculatedAngle ) )
        return;

      nextPointDistance += distance;
    }

    distanceTraversed += segmentLength;
    prevX = thisX;
    prevY = thisY;
    prevZ = thisZ;
    prevM = thisM;
    prevXPainterUnits = thisXPainterUnits;
    prevYPainterUnits = thisYPainterUnits;
  }
}

QPointF QgsLinearReferencingSymbolLayer::pointToPainter( QgsSymbolRenderContext &context, double x, double y, double z )
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
}

void QgsLinearReferencingSymbolLayer::renderPolylineInterval( const QPolygonF &points, QgsSymbolRenderContext &context, double skipMultiples, const QPointF &labelOffsetPainterUnits, double averageAngleLengthPainterUnits )
{
  const QgsAbstractGeometry *geometry = context.renderContext().geometry();

  double distance = mInterval;

  QgsNumericFormatContext numericContext;

  // TODO if NOT a original geometry, convert points to linestring and scale distance to painter units..

  if ( const QgsLineString *line = qgsgeometry_cast< const QgsLineString * >( geometry ) )
  {
    double currentDistance = 0;

    std::unique_ptr< QgsLineString > painterUnitsGeometry( line->clone() );
    if ( context.renderContext().coordinateTransform().isValid() )
    {
      painterUnitsGeometry->transform( context.renderContext().coordinateTransform() );
    }
    painterUnitsGeometry->transform( context.renderContext().mapToPixel().transform() );

    visitPointsByRegularDistance( line, painterUnitsGeometry.get(), distance, averageAngleLengthPainterUnits, [&context, &currentDistance, &numericContext, distance, skipMultiples,
                                            labelOffsetPainterUnits, this]( double x, double y, double z, double m, double angle ) -> bool
    {
      if ( context.renderContext().renderingStopped() )
        return false;

      currentDistance += distance;
      if ( skipMultiples > 0 && qgsDoubleNear( std::fmod( currentDistance,  skipMultiples ), 0 ) )
        return true;

      const QPointF pt = pointToPainter( context, x, y, z );


      if ( mMarkerSymbol && mShowMarker )
      {
        if ( mRotateLabels )
          mMarkerSymbol->setLineAngle( 90 - angle );
        mMarkerSymbol->renderPoint( pt, context.feature(), context.renderContext() );
      }

      const double angleRadians = ( mRotateLabels ? angle : 0 ) * M_PI / 180.0;
      const double dx = labelOffsetPainterUnits.x() * std::sin( angleRadians + M_PI_2 )
      + labelOffsetPainterUnits.y() * std::sin( angleRadians );
      const double dy = labelOffsetPainterUnits.x() * std::cos( angleRadians + M_PI_2 )
      + labelOffsetPainterUnits.y() * std::cos( angleRadians );

      QgsTextRenderer::drawText( QPointF( pt.x() + dx, pt.y() + dy ), angleRadians, Qgis::TextHorizontalAlignment::Left, { mNumericFormat->formatDouble( currentDistance, numericContext ) }, context.renderContext(), mTextFormat );

      return true;
    } );

  }
}

void QgsLinearReferencingSymbolLayer::renderPolylineVertex( const QPolygonF &points, QgsSymbolRenderContext &context, double skipMultiples, const QPointF &labelOffsetPainterUnits, double averageAngleLengthPainterUnits )
{
  const QgsAbstractGeometry *geometry = context.renderContext().geometry();

  QgsNumericFormatContext numericContext;

  // TODO if NOT a original geometry, convert points to linestring and scale distance to painter units..

  if ( const QgsLineString *line = qgsgeometry_cast< const QgsLineString * >( geometry ) )
  {
    const double *xData = line->xData();
    const double *yData = line->yData();
    const double *zData = line->is3D() ? line->zData() : nullptr;
    const double *mData = line->isMeasure() ? line->mData() : nullptr;
    const int size = line->numPoints();
    if ( size < 2 )
      return;

    double currentDistance = 0;
    double prevX = *xData++;
    double prevY = *yData++;
    double prevZ = zData ? *zData++ : 0;
    double prevM = mData ? *mData++ : 0;

    for ( int i = 0; i < size; ++i )
    {
      if ( context.renderContext().renderingStopped() )
        break;

      double thisX = *xData++;
      double thisY = *yData++;
      double thisZ = zData ? *zData++ : 0;
      double thisM = mData ? *mData++ : 0;

      const double thisSegmentLength = QgsGeometryUtilsBase::distance2D( prevX, prevY, thisX, thisY );
      currentDistance += thisSegmentLength;

      if ( skipMultiples > 0 && qgsDoubleNear( std::fmod( currentDistance,  skipMultiples ), 0 ) )
        continue;

      const QPointF pt = pointToPainter( context, thisX, thisY, thisZ );
      //const QPointF segmentStartPt = pointToPainter( context, startSegmentX, startSegmentY, startSegmentZ );
      //const QPointF segmentEndPt = pointToPainter( context, endSegmentX, endSegmentY, endSegmentZ );

      double angle = 0; // mRotateLabels ? std::fmod( QgsGeometryUtilsBase::azimuth( segmentStartPt.x(), segmentStartPt.y(), segmentEndPt.x(), segmentEndPt.y() ) + 360, 360 ) : 0;
      if ( angle > 90 && angle < 270 )
        angle += 180;

      if ( mMarkerSymbol && mShowMarker )
      {
        if ( mRotateLabels )
          mMarkerSymbol->setLineAngle( 90 - angle );
        mMarkerSymbol->renderPoint( pt, context.feature(), context.renderContext() );
      }

      const double angleRadians = angle * M_PI / 180.0;
      const double dx = labelOffsetPainterUnits.x() * std::sin( angleRadians + M_PI_2 )
                        + labelOffsetPainterUnits.y() * std::sin( angleRadians );
      const double dy = labelOffsetPainterUnits.x() * std::cos( angleRadians + M_PI_2 )
                        + labelOffsetPainterUnits.y() * std::cos( angleRadians );

      QgsTextRenderer::drawText( QPointF( pt.x() + dx, pt.y() + dy ), angleRadians, Qgis::TextHorizontalAlignment::Left, { mNumericFormat->formatDouble( currentDistance, numericContext ) }, context.renderContext(), mTextFormat );

      prevX = thisX;
      prevY = thisY;
      prevZ = thisZ;
      prevM = thisM;
    }

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

Qgis::LinearReferencingPlacement QgsLinearReferencingSymbolLayer::placement() const
{
  return mPlacement;
}

void QgsLinearReferencingSymbolLayer::setPlacement( Qgis::LinearReferencingPlacement placement )
{
  mPlacement = placement;
}

