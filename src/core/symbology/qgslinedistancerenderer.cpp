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
#include "qgslinesymbol.h"
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

  QgsLineSymbol *symbol = firstSymbolForFeature( feature, context );

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

  QList< int > &featureToSegments = mFeatureIdToSegments[feature.id()];

  const double tolerance = context.convertToPainterUnits( mTolerance, mToleranceUnit, mToleranceMapUnitScale );

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
    context.mapToPixel().transformInPlace( thisSegmentData.x2, thisSegmentData.y2 );
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
      context.mapToPixel().transformInPlace( thisSegmentData.x2, thisSegmentData.y2 );
      thisSegmentData.segmentIndex = n - 1;

      // rectangle will be normalized automatically, we don't need to check min/max coordinates here
      const QgsRectangle segmentRect = QgsRectangle( thisSegmentData.x1,
                                       thisSegmentData.y1,
                                       thisSegmentData.x2,
                                       thisSegmentData.y2 );


      const double featureLineAngle = QgsGeometryUtilsBase::lineAngle(
                                        thisSegmentData.x1, thisSegmentData.y1, thisSegmentData.x2, thisSegmentData.y2
                                      );

      const double segmentLength = std::sqrt( std::pow( thisSegmentData.x1 - thisSegmentData.x2, 2 )
                                              + std::pow( thisSegmentData.y1 - thisSegmentData.y2, 2 ) );

      const QgsRectangle searchRect = segmentRect.buffered( tolerance );

      const QList<QgsFeatureId> candidateSegments = mSegmentIndex->intersects( searchRect );

      QList< int > matchedExistingSegmentsForThisFeature;

      for ( const QgsFeatureId candidate : candidateSegments )
      {
        if ( context.renderingStopped() )
          break;

        const SegmentData &candidateSegment = mSegmentData[candidate];
        // ignore all segments from same source feature!
        if ( feature.id() == candidateSegment.feature.id() )
          continue;

        const double candidateLineAngle = QgsGeometryUtilsBase::lineAngle(
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
        double nearestPointOnSegmentX1;
        double nearestPointOnSegmentY1;
        ( void )QgsGeometryUtilsBase::sqrDistToLine( candidateSegment.x1, candidateSegment.y1,
            thisSegmentData.x1, thisSegmentData.y1,
            thisSegmentData.x2, thisSegmentData.y2, nearestPointOnSegmentX1, nearestPointOnSegmentY1, 0 );
        double nearestPointOnSegmentX2;
        double nearestPointOnSegmentY2;
        ( void )QgsGeometryUtilsBase::sqrDistToLine( candidateSegment.x2, candidateSegment.y2,
            thisSegmentData.x1, thisSegmentData.y1,
            thisSegmentData.x2, thisSegmentData.y2, nearestPointOnSegmentX2, nearestPointOnSegmentY2, 0 );

        if ( qgsDoubleNear( nearestPointOnSegmentX1, nearestPointOnSegmentX2 ) && qgsDoubleNear( nearestPointOnSegmentY1, nearestPointOnSegmentY2 ) )
        {
          // lines are parallel which come close, but don't actually overlap at all
          // i.e. ------ ----------
          continue;
        }

        double nearestPointOnCandidateX1;
        double nearestPointOnCandidateY1;
        double nearestPointOnCandidateX2;
        double nearestPointOnCandidateY2;

        // project closest points from segment back onto candidate
        const double dist3 = QgsGeometryUtilsBase::sqrDistToLine( nearestPointOnSegmentX1, nearestPointOnSegmentY1,
                             candidateSegment.x1, candidateSegment.y1,
                             candidateSegment.x2, candidateSegment.y2, nearestPointOnCandidateX1, nearestPointOnCandidateY1, 0 );
        const double dist4 = QgsGeometryUtilsBase::sqrDistToLine( nearestPointOnSegmentX2, nearestPointOnSegmentY2,
                             candidateSegment.x1, candidateSegment.y1,
                             candidateSegment.x2, candidateSegment.y2, nearestPointOnCandidateX2, nearestPointOnCandidateY2, 0 );
        if ( qgsDoubleNear( nearestPointOnCandidateX1, nearestPointOnCandidateX2 ) && qgsDoubleNear( nearestPointOnCandidateY1, nearestPointOnCandidateY2 ) )
        {
          continue;
        }

        if ( std::max( dist3, dist4 ) > tolerance * tolerance )
          continue;


        // calculate split points:
        // 1. the distance along the SEGMENT at which the overlap starts/ends
        const double segmentSplitDistance1 = std::sqrt( std::pow( thisSegmentData.x1 - nearestPointOnSegmentX1, 2 ) + std::pow( thisSegmentData.y1 - nearestPointOnSegmentY1, 2 ) );
        const double segmentSplitDistance2 = std::sqrt( std::pow( thisSegmentData.x1 - nearestPointOnSegmentX2, 2 ) + std::pow( thisSegmentData.y1 - nearestPointOnSegmentY2, 2 ) );
        // 2. the distance along the MATCHED CANDIDATE at which the overlap starts/ends
        const double candidateSplitDistance1 = std::sqrt( std::pow( candidateSegment.x1 - nearestPointOnCandidateX1, 2 ) + std::pow( candidateSegment.y1 - nearestPointOnCandidateY1, 2 ) );
        const double candidateSplitDistance2 = std::sqrt( std::pow( candidateSegment.x1 - nearestPointOnCandidateX2, 2 ) + std::pow( candidateSegment.y1 - nearestPointOnCandidateY2, 2 ) );

        const double candidateLength = std::sqrt( std::pow( candidateSegment.x1 - candidateSegment.x2, 2 )
                                       + std::pow( candidateSegment.y1 - candidateSegment.y2, 2 ) );

        if ( !qgsDoubleNear( candidateSplitDistance1, 0 ) && !qgsDoubleNear( candidateSplitDistance1, candidateLength )
             && !qgsDoubleNear( candidateSplitDistance2, 0 ) && !qgsDoubleNear( candidateSplitDistance2, candidateLength )
             && !qgsDoubleNear( candidateSplitDistance1, candidateSplitDistance2 ) )
        {
          // CASE 1:
          // The CURRENT segment falls within the middle of the existing CANDIDATE segment.
          // We have two valid split points, so we make three new segments from candidate and replace existing
          const double splitPoint1x = candidateSplitDistance1 < candidateSplitDistance2 ? nearestPointOnCandidateX1 : nearestPointOnCandidateX2;
          const double splitPoint1y = candidateSplitDistance1 < candidateSplitDistance2 ? nearestPointOnCandidateY1 : nearestPointOnCandidateY2;
          const double splitPoint2x = candidateSplitDistance1 < candidateSplitDistance2 ? nearestPointOnCandidateX2 : nearestPointOnCandidateX1;
          const double splitPoint2y = candidateSplitDistance1 < candidateSplitDistance2 ? nearestPointOnCandidateY2 : nearestPointOnCandidateY1;

          // do a weighted shift of the vertices in the overlapping part of the line
          SegmentData newSegment2;
          newSegment2.weight = candidateSegment.weight + 1;
          // TODO -- do we need to check if nearest point on segment 1 or 2 is the closest to the split point???
          newSegment2.x1 = ( ( splitPoint1x * candidateSegment.weight ) + nearestPointOnSegmentX1 ) / newSegment2.weight;
          newSegment2.y1 = ( ( splitPoint1y * candidateSegment.weight ) + nearestPointOnSegmentY1 ) / newSegment2.weight;
          newSegment2.x2 = ( ( splitPoint2x * candidateSegment.weight ) + nearestPointOnSegmentX2 ) / newSegment2.weight;
          newSegment2.y2 = ( ( splitPoint2y * candidateSegment.weight ) + nearestPointOnSegmentY2 ) / newSegment2.weight;

          // these are the non-overlapping ends of the line -- their vertices need
          // to match those we calculated for the overlapping part of the line
          SegmentData newSegment1;
          newSegment1.weight = candidateSegment.weight + 1;
          newSegment1.x1 = candidateSegment.x1;
          newSegment1.y1 = candidateSegment.y1;
          newSegment1.x2 = newSegment2.x1;
          newSegment1.y2 = newSegment2.y1;

          SegmentData newSegment3;
          newSegment3.weight = candidateSegment.weight + 1;
          newSegment3.x1 = newSegment2.x2;
          newSegment3.y1 = newSegment2.y2;
          newSegment3.x2 = candidateSegment.x2;
          newSegment3.y2 = candidateSegment.y2;

          mSegmentData.append( newSegment1 );
          mSegmentData.append( newSegment2 );
          mSegmentData.append( newSegment3 );

          // remove old segment from spatial index
          mSegmentIndex->deleteFeature( candidate, QgsRectangle( candidateSegment.x1,
                                        candidateSegment.y1,
                                        candidateSegment.x2,
                                        candidateSegment.y2 ) );

          // update lookup tables for features using the old segment
          auto it = mSegmentToFeatureId.find( candidate );
          QList< QgsFeatureId > featureIds;
          if ( it != mSegmentToFeatureId.end() )
          {
            featureIds = it.value();
            mSegmentToFeatureId.erase( it );
          }
          for ( const QgsFeatureId id : std::as_const( featureIds ) )
          {
            mFeatureIdToSegments[id].removeAll( candidate );
            mFeatureIdToSegments[id].append( mSegmentId );
            mFeatureIdToSegments[id].append( mSegmentId + 1 );
            mFeatureIdToSegments[id].append( mSegmentId + 2 );
          }
          mSegmentToFeatureId.insert( mSegmentId, featureIds );
          mSegmentToFeatureId.insert( mSegmentId + 1, featureIds );
          mSegmentToFeatureId.insert( mSegmentId + 2, featureIds );

          // only the MIDDLE overlapping new segment is in the current feature
          matchedExistingSegmentsForThisFeature.append( mSegmentId + 1 );

          // update spatial index with new segments
          mSegmentIndex->addFeature( mSegmentId, QgsRectangle( newSegment1.x1,
                                     newSegment1.y1,
                                     newSegment1.x2,
                                     newSegment1.y2 ) );
          mSegmentIndex->addFeature( mSegmentId + 1, QgsRectangle( newSegment2.x1,
                                     newSegment2.y1,
                                     newSegment2.x2,
                                     newSegment2.y2 ) );
          mSegmentIndex->addFeature( mSegmentId + 2, QgsRectangle( newSegment3.x1,
                                     newSegment3.y1,
                                     newSegment3.x2,
                                     newSegment3.y2 ) );

          mSegmentId += 3;
        }
        else if ( !qgsDoubleNear( segmentSplitDistance1, 0 ) && !qgsDoubleNear( segmentSplitDistance1, segmentLength )
                  && !qgsDoubleNear( segmentSplitDistance2, 0 ) && !qgsDoubleNear( segmentSplitDistance2, segmentLength )
                  && !qgsDoubleNear( segmentSplitDistance1, segmentSplitDistance2 ) )
        {
          // CASE 2:
          // The CANDIDATE segment falls within the middle of the CURRENT segment.
          // We have two valid split points, so we extract the overlapping middle segment from the current segment and replace existing
          const double splitPoint1x = segmentSplitDistance1 < segmentSplitDistance2 ? nearestPointOnSegmentX1 : nearestPointOnSegmentX2;
          const double splitPoint1y = segmentSplitDistance1 < segmentSplitDistance2 ? nearestPointOnSegmentY1 : nearestPointOnSegmentY2;
          const double splitPoint2x = segmentSplitDistance1 < segmentSplitDistance2 ? nearestPointOnSegmentX2 : nearestPointOnSegmentX1;
          const double splitPoint2y = segmentSplitDistance1 < segmentSplitDistance2 ? nearestPointOnSegmentY2 : nearestPointOnSegmentY1;

          // do a weighted shift of the vertices in the overlapping part of the line
          SegmentData newSegment;
          newSegment.weight = candidateSegment.weight + 1;
          // TODO -- do we need to check if nearest point on candidate 1 or 2 is the closest to the split point???
          newSegment.x1 = ( ( splitPoint1x * candidateSegment.weight ) + nearestPointOnCandidateX1 ) / newSegment.weight;
          newSegment.y1 = ( ( splitPoint1y * candidateSegment.weight ) + nearestPointOnCandidateY1 ) / newSegment.weight;
          newSegment.x2 = ( ( splitPoint2x * candidateSegment.weight ) + nearestPointOnCandidateX2 ) / newSegment.weight;
          newSegment.y2 = ( ( splitPoint2y * candidateSegment.weight ) + nearestPointOnCandidateY2 ) / newSegment.weight;

          mSegmentData.append( newSegment );

          // remove old segment from spatial index
          mSegmentIndex->deleteFeature( candidate, QgsRectangle( candidateSegment.x1,
                                        candidateSegment.y1,
                                        candidateSegment.x2,
                                        candidateSegment.y2 ) );

          // update lookup tables for features using the old segment
          auto it = mSegmentToFeatureId.find( candidate );
          QList< QgsFeatureId > featureIds;
          if ( it != mSegmentToFeatureId.end() )
          {
            featureIds = it.value();
            mSegmentToFeatureId.erase( it );
          }
          for ( const QgsFeatureId id : std::as_const( featureIds ) )
          {
            mFeatureIdToSegments[id].removeAll( candidate );
            mFeatureIdToSegments[id].append( mSegmentId );
          }
          mSegmentToFeatureId.insert( mSegmentId, featureIds );

          matchedExistingSegmentsForThisFeature.append( mSegmentId );

          // update spatial index with shifted segment
          mSegmentIndex->addFeature( mSegmentId, QgsRectangle( newSegment.x1,
                                     newSegment.y1,
                                     newSegment.x2,
                                     newSegment.y2 ) );

          mSegmentId++;
        }
        else
        {
          // TODO CASE 3/4, partially overlapping lines

        }
      }

      // find what's left over now...
      for ( int segmentId : std::as_const( matchedExistingSegmentsForThisFeature ) )
      {
        mSegmentToFeatureId[segmentId].append( feature.id() );
      }
      mFeatureIdToSegments[feature.id()].append( matchedExistingSegmentsForThisFeature );


      mSegmentData.push_back( thisSegmentData );
      featureToSegments.append( mSegmentId );
      mSegmentToFeatureId[mSegmentId].append( feature.id() );

      mSegmentIndex->addFeature( mSegmentId, segmentRect );
      mSegmentId++;
    }
  }

  mQueuedFeatures << GroupedFeature( feature, symbol, selected );

  return true;
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
  mQueuedFeatures.clear();
  mSegmentData.clear();
  mSegmentIndex = std::make_unique< QgsSpatialIndex >();
}

void QgsLineDistanceRenderer::stopRender( QgsRenderContext &context )
{
  QgsFeatureRenderer::stopRender( context );

  if ( !context.renderingStopped() )
  {
#if 0
    QgsSpatialIndex segmentGroupIndex;
    QHash< int, QList< int> > segmentGroups;

    const double tolerance = context.convertToPainterUnits( mTolerance, mToleranceUnit, mToleranceMapUnitScale );

    int _id = -1;

    // make a const copy for iteration, we'll be modifying the original as we go
    const QVector< SegmentData > segmentData = mSegmentData;
    for ( const SegmentData &segmentData : segmentData )
    {
      _id++;
      if ( context.renderingStopped() )
        break;

      const double featureLineAngle = QgsGeometryUtilsBase::lineAngle(
                                        segmentData.x1, segmentData.y1, segmentData.x2, segmentData.y2
                                      );

      const double segmentLength = std::sqrt( std::pow( segmentData.x1 - segmentData.x2, 2 )
                                              + std::pow( segmentData.y1 - segmentData.y2, 2 ) );

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

        const double candidateLineAngle = QgsGeometryUtilsBase::lineAngle(
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
        double nearestPointOnSegmentX1;
        double nearestPointOnSegmentY1;
        ( void )QgsGeometryUtilsBase::sqrDistToLine( candidateSegment.x1, candidateSegment.y1,
            segmentData.x1, segmentData.y1,
            segmentData.x2, segmentData.y2, nearestPointOnSegmentX1, nearestPointOnSegmentY1, 0 );
        double nearestPointOnSegmentX2;
        double nearestPointOnSegmentY2;
        ( void )QgsGeometryUtilsBase::sqrDistToLine( candidateSegment.x2, candidateSegment.y2,
            segmentData.x1, segmentData.y1,
            segmentData.x2, segmentData.y2, nearestPointOnSegmentX2, nearestPointOnSegmentY2, 0 );

        if ( qgsDoubleNear( nearestPointOnSegmentX1, nearestPointOnSegmentX2 ) && qgsDoubleNear( nearestPointOnSegmentY1, nearestPointOnSegmentY2 ) )
        {
          // lines are parallel which come close, but don't actually overlap at all
          // i.e. ------ ----------
          continue;
        }

        double nearestPointOnCandidateX1;
        double nearestPointOnCandidateY1;
        double nearestPointOnCandidateX2;
        double nearestPointOnCandidateY2;

        // project closest points from segment back onto candidate
        const double dist3 = QgsGeometryUtilsBase::sqrDistToLine( nearestPointOnSegmentX1, nearestPointOnSegmentY1,
                             candidateSegment.x1, candidateSegment.y1,
                             candidateSegment.x2, candidateSegment.y2, nearestPointOnCandidateX1, nearestPointOnCandidateY1, 0 );
        const double dist4 = QgsGeometryUtilsBase::sqrDistToLine( nearestPointOnSegmentX2, nearestPointOnSegmentY2,
                             candidateSegment.x1, candidateSegment.y1,
                             candidateSegment.x2, candidateSegment.y2, nearestPointOnCandidateX2, nearestPointOnCandidateY2, 0 );
        if ( qgsDoubleNear( nearestPointOnCandidateX1, nearestPointOnCandidateX2 ) && qgsDoubleNear( nearestPointOnCandidateY1, nearestPointOnCandidateY2 ) )
        {
          continue;
        }

        if ( std::max( dist3, dist4 ) > tolerance * tolerance )
          continue;

        // calculate split points:
        // 1. the distance along the SEGMENT at which the overlap starts/ends
        const double segmentSplitDistance1 = std::sqrt( std::pow( segmentData.x1 - nearestPointOnSegmentX1, 2 ) + std::pow( segmentData.y1 - nearestPointOnSegmentY1, 2 ) );
        const double segmentSplitDistance2 = std::sqrt( std::pow( segmentData.x1 - nearestPointOnSegmentX2, 2 ) + std::pow( segmentData.y1 - nearestPointOnSegmentY2, 2 ) );
        // 2. the distance along the MATCHED CANDIDATE at which the overlap starts/ends
        const double candidateSplitDistance1 = std::sqrt( std::pow( candidateSegment.x1 - nearestPointOnCandidateX1, 2 ) + std::pow( candidateSegment.y1 - nearestPointOnCandidateY1, 2 ) );
        const double candidateSplitDistance2 = std::sqrt( std::pow( candidateSegment.x1 - nearestPointOnCandidateX2, 2 ) + std::pow( candidateSegment.y1 - nearestPointOnCandidateY2, 2 ) );

        const double candidateLength = std::sqrt( std::pow( candidateSegment.x1 - candidateSegment.x2, 2 )
                                       + std::pow( candidateSegment.y1 - candidateSegment.y2, 2 ) );

        if ( !qgsDoubleNear( candidateSplitDistance1, 0 ) && !qgsDoubleNear( candidateSplitDistance1, candidateLength )
             && !qgsDoubleNear( candidateSplitDistance2, 0 ) && !qgsDoubleNear( candidateSplitDistance2, candidateLength )
             && !qgsDoubleNear( candidateSplitDistance1, candidateSplitDistance2 ) )
        {
          // CASE 1:
          // The CURRENT segment falls within the middle of the CANDIDATE segment.
          // We have two valid split points, so we make three new segments from candidate and replace existing
          const double splitPoint1x = candidateSplitDistance1 < candidateSplitDistance2 ? nearestPointOnCandidateX1 : nearestPointOnCandidateX2;
          const double splitPoint1y = candidateSplitDistance1 < candidateSplitDistance2 ? nearestPointOnCandidateY1 : nearestPointOnCandidateY2;
          const double splitPoint2x = candidateSplitDistance1 < candidateSplitDistance2 ? nearestPointOnCandidateX2 : nearestPointOnCandidateX1;
          const double splitPoint2y = candidateSplitDistance1 < candidateSplitDistance2 ? nearestPointOnCandidateY2 : nearestPointOnCandidateY1;

          // do a weighted shift of the vertices in the overlapping part of the line
          SegmentData newSegment2;
          newSegment2.weight = candidateSegment.weight + 1;
          // TODO -- do we need to check if nearest point on segment 1 or 2 is the closest to the split point???
          newSegment2.x1 = ( ( splitPoint1x * candidateSegment.weight ) + nearestPointOnSegmentX1 ) / newSegment2.weight;
          newSegment2.y1 = ( ( splitPoint1y * candidateSegment.weight ) + nearestPointOnSegmentY1 ) / newSegment2.weight;
          newSegment2.x2 = ( ( splitPoint2x * candidateSegment.weight ) + nearestPointOnSegmentX2 ) / newSegment2.weight;
          newSegment2.y2 = ( ( splitPoint2y * candidateSegment.weight ) + nearestPointOnSegmentY2 ) / newSegment2.weight;

          // these are the non-overlapping ends of the line -- their vertices need
          // to match those we calculated for the overlapping part of the line
          SegmentData newSegment1;
          newSegment1.weight = candidateSegment.weight + 1;
          newSegment1.x1 = candidateSegment.x1;
          newSegment1.y1 = candidateSegment.y1;
          newSegment1.x2 = newSegment2.x1;
          newSegment1.y2 = newSegment2.y1;

          SegmentData newSegment3;
          newSegment3.weight = candidateSegment.weight + 1;
          newSegment3.x1 = newSegment2.x2;
          newSegment3.y1 = newSegment2.y2;
          newSegment3.x2 = candidateSegment.x2;
          newSegment3.y2 = candidateSegment.y2;

          mSegmentData.append( newSegment1 );
          mSegmentData.append( newSegment2 );
          mSegmentData.append( newSegment3 );

          // remove old segment from spatial index
          mSegmentIndex->deleteFeature( candidate, QgsRectangle( candidateSegment.x1,
                                        candidateSegment.y1,
                                        candidateSegment.x2,
                                        candidateSegment.y2 ) );

          // update lookup tables for features using the old segment
          auto it = mSegmentToFeatureId.find( candidate );
          QList< QgsFeatureId > featureIds;
          if ( it != mSegmentToFeatureId.end() )
          {
            featureIds = it.value();
            mSegmentToFeatureId.erase( it );
          }
          for ( const QgsFeatureId id : std::as_const( featureIds ) )
          {
            mFeatureIdToSegments[id].removeAll( candidate );
            mFeatureIdToSegments[id].append( mSegmentId );
            mFeatureIdToSegments[id].append( mSegmentId + 1 );
            mFeatureIdToSegments[id].append( mSegmentId + 2 );
          }
          // only the MIDDLE overlapping new segment is in the current feature
          mSegmentToFeatureId.insert( mSegmentId, featureIds );
          mSegmentToFeatureId.insert( mSegmentId + 2, featureIds );

          featureIds.append( segmentData.feature.id() );
          mSegmentToFeatureId.remove( _id );
          mSegmentToFeatureId.insert( mSegmentId + 1, featureIds );
          mFeatureIdToSegments[segmentData.feature.id()].removeAll( _id );
          mFeatureIdToSegments[segmentData.feature.id()].append( mSegmentId + 1 );

          // update spatial index with new segments
          mSegmentIndex->addFeature( mSegmentId, QgsRectangle( newSegment1.x1,
                                     newSegment1.y1,
                                     newSegment1.x2,
                                     newSegment1.y2 ) );
          mSegmentIndex->addFeature( mSegmentId + 1, QgsRectangle( newSegment2.x1,
                                     newSegment2.y1,
                                     newSegment2.x2,
                                     newSegment2.y2 ) );
          mSegmentIndex->addFeature( mSegmentId + 2, QgsRectangle( newSegment3.x1,
                                     newSegment3.y1,
                                     newSegment3.x2,
                                     newSegment3.y2 ) );

          mSegmentId += 3;
        }
        else if ( !qgsDoubleNear( segmentSplitDistance1, 0 ) && !qgsDoubleNear( segmentSplitDistance1, segmentLength )
                  && !qgsDoubleNear( segmentSplitDistance2, 0 ) && !qgsDoubleNear( segmentSplitDistance2, segmentLength )
                  && !qgsDoubleNear( segmentSplitDistance1, segmentSplitDistance2 ) )
        {
          // CASE 2:
          // The CANDIDATE segment falls within the middle of the CURRENT segment.
          // We have two valid split points, so we make three new segments from the current segment and replace existing
          const double splitPoint1x = segmentSplitDistance1 < segmentSplitDistance2 ? nearestPointOnSegmentX1 : nearestPointOnSegmentX2;
          const double splitPoint1y = segmentSplitDistance1 < segmentSplitDistance2 ? nearestPointOnSegmentY1 : nearestPointOnSegmentY2;
          const double splitPoint2x = segmentSplitDistance1 < segmentSplitDistance2 ? nearestPointOnSegmentX2 : nearestPointOnSegmentX1;
          const double splitPoint2y = segmentSplitDistance1 < segmentSplitDistance2 ? nearestPointOnSegmentY2 : nearestPointOnSegmentY1;

          // do a weighted shift of the vertices in the overlapping part of the line
          SegmentData newSegment2;
          newSegment2.weight = candidateSegment.weight + 1;
          // TODO -- do we need to check if nearest point on candidate 1 or 2 is the closest to the split point???
          newSegment2.x1 = ( ( splitPoint1x * candidateSegment.weight ) + nearestPointOnCandidateX1 ) / newSegment2.weight;
          newSegment2.y1 = ( ( splitPoint1y * candidateSegment.weight ) + nearestPointOnCandidateY1 ) / newSegment2.weight;
          newSegment2.x2 = ( ( splitPoint2x * candidateSegment.weight ) + nearestPointOnCandidateX2 ) / newSegment2.weight;
          newSegment2.y2 = ( ( splitPoint2y * candidateSegment.weight ) + nearestPointOnCandidateY2 ) / newSegment2.weight;

          // these are the non-overlapping ends of the line -- their vertices need
          // to match those we calculated for the overlapping part of the line
          SegmentData newSegment1;
          newSegment1.weight = candidateSegment.weight + 1;
          newSegment1.x1 = candidateSegment.x1;
          newSegment1.y1 = candidateSegment.y1;
          newSegment1.x2 = newSegment2.x1;
          newSegment1.y2 = newSegment2.y1;

          SegmentData newSegment3;
          newSegment3.weight = candidateSegment.weight + 1;
          newSegment3.x1 = newSegment2.x2;
          newSegment3.y1 = newSegment2.y2;
          newSegment3.x2 = candidateSegment.x2;
          newSegment3.y2 = candidateSegment.y2;

          mSegmentData.append( newSegment1 );
          mSegmentData.append( newSegment2 );
          mSegmentData.append( newSegment3 );

          // remove old segment from spatial index
          mSegmentIndex->deleteFeature( _id, QgsRectangle( segmentData.x1,
                                        segmentData.y1,
                                        segmentData.x2,
                                        segmentData.y2 ) );

          // update lookup tables for features using the old segment
          auto it = mSegmentToFeatureId.find( candidate );
          QList< QgsFeatureId > featureIds;
          if ( it != mSegmentToFeatureId.end() )
          {
            featureIds = it.value();
            mSegmentToFeatureId.erase( it );
          }
          for ( const QgsFeatureId id : std::as_const( featureIds ) )
          {
            mFeatureIdToSegments[id].removeAll( candidate );
            // only the MIDDLE overlapping new segment is in the candidate
            mFeatureIdToSegments[id].append( mSegmentId + 1 );
          }
          featureIds.append( segmentData.feature.id() );
          mSegmentToFeatureId.insert( mSegmentId + 1, featureIds );

          mSegmentToFeatureId.remove( _id );
          mSegmentToFeatureId.insert( mSegmentId, {segmentData.feature.id() } );
          mSegmentToFeatureId.insert( mSegmentId + 2, {segmentData.feature.id() } );

          mFeatureIdToSegments[segmentData.feature.id()].removeAll( _id );
          mFeatureIdToSegments[segmentData.feature.id()].append( mSegmentId );
          mFeatureIdToSegments[segmentData.feature.id()].append( mSegmentId + 1 );
          mFeatureIdToSegments[segmentData.feature.id()].append( mSegmentId + 2 );

          // update spatial index with new segments
          mSegmentIndex->addFeature( mSegmentId, QgsRectangle( newSegment1.x1,
                                     newSegment1.y1,
                                     newSegment1.x2,
                                     newSegment1.y2 ) );
          mSegmentIndex->addFeature( mSegmentId + 1, QgsRectangle( newSegment2.x1,
                                     newSegment2.y1,
                                     newSegment2.x2,
                                     newSegment2.y2 ) );
          mSegmentIndex->addFeature( mSegmentId + 2, QgsRectangle( newSegment3.x1,
                                     newSegment3.y1,
                                     newSegment3.x2,
                                     newSegment3.y2 ) );

          mSegmentId += 3;
        }

        // TODO -- even if start/end point coincide with start/end of candidate, we need to shift the
        // candidate points

        // TODO -- only two or one split segments on candidate

        // TODO -- node new segment and add

        // TODO -- this should all be happening in renderFeature now?


        //      splitPoints.insert( split1 );
//        splitPoints.insert( split2 );

        outSegments.insert( candidate );
      }
    }
#endif
    if ( !context.renderingStopped() )
    {
      drawGroups( context, mQueuedFeatures, mFeatureIdToSegments, {}, {} );
    }
  }

  mSegmentId = 0;
  mSegmentData.clear();
  mQueuedFeatures.clear();
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

QgsExpressionContextScope *QgsLineDistanceRenderer::createGroupScope() const
{
  QgsExpressionContextScope *clusterScope = new QgsExpressionContextScope();
#if 0
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
#endif
  return clusterScope;
}

QgsLineSymbol *QgsLineDistanceRenderer::firstSymbolForFeature( const QgsFeature &feature, QgsRenderContext &context ) const
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

  return dynamic_cast< QgsLineSymbol * >( symbolList.at( 0 ) );
}

QgsLineDistanceRenderer::GroupedFeature::GroupedFeature( const QgsFeature &feature, QgsLineSymbol *symbol, bool isSelected )
  : feature( feature )
  , isSelected( isSelected )
  , mSymbol( symbol )
{}

QgsLineDistanceRenderer::GroupedFeature::~GroupedFeature() = default;
