/***************************************************************************
                         qgsrasterlayerprofilegenerator.cpp
                         ---------------
    begin                : March 2022
    copyright            : (C) 2022 by Nyall Dawson
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
#include "qgsrasterlayerprofilegenerator.h"
#include "qgsprofilerequest.h"
#include "qgscurve.h"
#include "qgsrasterlayer.h"
#include "qgsrasterlayerelevationproperties.h"
#include "qgsrasteriterator.h"
#include "qgsgeometryengine.h"
#include "qgsgeos.h"
#include "qgslinesymbol.h"
#include "qgsprofilepoint.h"
#include "qgsfillsymbol.h"
#include "qgsthreadingutils.h"
#include "qgsrasterrenderer.h"

#include <QPolygonF>
#include <QThread>

//
// QgsRasterLayerProfileResults
//

QString QgsRasterLayerProfileResults::type() const
{
  return QStringLiteral( "raster" );
}

QVector<QgsProfileIdentifyResults> QgsRasterLayerProfileResults::identify( const QgsProfilePoint &point, const QgsProfileIdentifyContext &context )
{
  switch ( mMode )
  {
    case Qgis::RasterElevationMode::FixedElevationRange:
    {
      // TODO -- consider an index if performance is an issue
      std::optional< QgsProfileIdentifyResults > result;

      double prevDistance = std::numeric_limits< double >::max();
      double prevElevation = 0;
      for ( auto it = mDistanceToHeightMap.constBegin(); it != mDistanceToHeightMap.constEnd(); ++it )
      {
        // find segment which corresponds to the given distance along curve
        if ( it != mDistanceToHeightMap.constBegin() && prevDistance <= point.distance() && it.key() >= point.distance() )
        {
          const double dx = it.key() - prevDistance;
          const double dy = it.value() - prevElevation;
          const double snappedZ = ( dy / dx ) * ( point.distance() - prevDistance ) + prevElevation;

          if ( std::fabs( point.elevation() - snappedZ ) > context.maximumSurfaceElevationDelta )
            return {};

          result = QgsProfileIdentifyResults( mLayer,
          {
            QVariantMap(
            {
              {QStringLiteral( "distance" ),  point.distance() },
              {QStringLiteral( "elevation" ), snappedZ },
              {QStringLiteral( "value" ), mDistanceToValueMap.value( it.key() )}
            } )
          } );
          break;
        }

        prevDistance = it.key();
        prevElevation = it.value();
      }
      if ( result.has_value() )
        return {result.value()};
      else
        return {};
    }

    case Qgis::RasterElevationMode::RepresentsElevationSurface:
    {
      const QVector<QgsProfileIdentifyResults> noLayerResults = QgsAbstractProfileSurfaceResults::identify( point, context );

      // we have to make a new list, with the correct layer reference set
      QVector<QgsProfileIdentifyResults> res;
      res.reserve( noLayerResults.size() );
      for ( const QgsProfileIdentifyResults &result : noLayerResults )
      {
        res.append( QgsProfileIdentifyResults( mLayer, result.results() ) );
      }
      return res;
    }
  }
  BUILTIN_UNREACHABLE
}

void QgsRasterLayerProfileResults::renderResults( QgsProfileRenderContext &context )
{
  switch ( mMode )
  {
    case Qgis::RasterElevationMode::FixedElevationRange:
    {
      QPainter *painter = context.renderContext().painter();
      if ( !painter )
        return;

      const QgsScopedQPainterState painterState( painter );

      painter->setBrush( Qt::NoBrush );
      painter->setPen( Qt::NoPen );

      const double minDistance = context.distanceRange().lower();
      const double maxDistance = context.distanceRange().upper();
      double minZ = context.elevationRange().lower();
      double maxZ = context.elevationRange().upper();

      const QRectF visibleRegion( minDistance, minZ, maxDistance - minDistance, maxZ - minZ );
      QPainterPath clipPath;
      clipPath.addPolygon( context.worldTransform().map( visibleRegion ) );
      painter->setClipPath( clipPath, Qt::ClipOperation::IntersectClip );

      double prevDistance = std::numeric_limits< double >::quiet_NaN();
      for ( auto pointIt = mDistanceToHeightMap.constBegin(); pointIt != mDistanceToHeightMap.constEnd(); ++pointIt )
      {
        if ( std::isnan( pointIt.value() ) )
        {

        }

        if ( !std::isnan( prevDistance ) )
        {
          const QColor currentColor = mDistanceToColorMap.value( pointIt.key() );
          QPolygonF currentPolygon;
          currentPolygon.append( context.worldTransform().map( QPointF( prevDistance, mFixedRange.lower() ) ) );
          currentPolygon.append( context.worldTransform().map( QPointF( pointIt.key(), mFixedRange.lower() ) ) );
          currentPolygon.append( context.worldTransform().map( QPointF( pointIt.key(), mFixedRange.upper() ) ) );
          currentPolygon.append( context.worldTransform().map( QPointF( prevDistance, mFixedRange.upper() ) ) );
          currentPolygon.append( currentPolygon.at( 0 ) );
          painter->setBrush( QBrush( currentColor ) );
          painter->setPen( QPen( currentColor ) );
          painter->drawPolygon( currentPolygon );
        }
        prevDistance = pointIt.key();
      }
      break;
    }

    case Qgis::RasterElevationMode::RepresentsElevationSurface:
      QgsAbstractProfileSurfaceResults::renderResults( context );
      break;
  }
}



//
// QgsRasterLayerProfileGenerator
//

QgsRasterLayerProfileGenerator::QgsRasterLayerProfileGenerator( QgsRasterLayer *layer, const QgsProfileRequest &request )
  : QgsAbstractProfileSurfaceGenerator( request )
  , mId( layer->id() )
  , mFeedback( std::make_unique< QgsRasterBlockFeedback >() )
  , mProfileCurve( request.profileCurve() ? request.profileCurve()->clone() : nullptr )
  , mSourceCrs( layer->crs() )
  , mTargetCrs( request.crs() )
  , mTransformContext( request.transformContext() )
  , mLayer( layer )
  , mRasterUnitsPerPixelX( layer->rasterUnitsPerPixelX() )
  , mRasterUnitsPerPixelY( layer->rasterUnitsPerPixelY() )
  , mStepDistance( request.stepDistance() )
{
  mRasterProvider.reset( layer->dataProvider()->clone() );
  mRasterProvider->moveToThread( nullptr );

  const QgsRasterLayerElevationProperties *elevationProperties = qgis::down_cast< QgsRasterLayerElevationProperties * >( layer->elevationProperties() );

  mMode = qgis::down_cast< QgsRasterLayerElevationProperties * >( layer->elevationProperties() )->mode();
  mSymbology = elevationProperties->profileSymbology();
  mElevationLimit = elevationProperties->elevationLimit();
  if ( elevationProperties->profileLineSymbol() )
    mLineSymbol.reset( elevationProperties->profileLineSymbol()->clone() );
  if ( elevationProperties->profileFillSymbol() )
    mFillSymbol.reset( elevationProperties->profileFillSymbol()->clone() );

  mFixedRange = elevationProperties->fixedRange();

  switch ( mMode )
  {
    case Qgis::RasterElevationMode::FixedElevationRange:
    {
      mRasterRenderer.reset( layer->renderer()->clone() );
      const QList<int> rendererBands = mRasterRenderer->usesBands();
      if ( rendererBands.size() == 1 )
      {
        mBand = rendererBands.at( 0 );
      }
      break;
    }

    case Qgis::RasterElevationMode::RepresentsElevationSurface:
      mBand = elevationProperties->bandNumber();
      mOffset = layer->elevationProperties()->zOffset();
      mScale = layer->elevationProperties()->zScale();
      break;
  }
}

QString QgsRasterLayerProfileGenerator::sourceId() const
{
  return mId;
}

Qgis::ProfileGeneratorFlags QgsRasterLayerProfileGenerator::flags() const
{
  return Qgis::ProfileGeneratorFlag::RespectsDistanceRange | Qgis::ProfileGeneratorFlag::RespectsMaximumErrorMapUnit;
}

QgsRasterLayerProfileGenerator::~QgsRasterLayerProfileGenerator() = default;

bool QgsRasterLayerProfileGenerator::generateProfile( const QgsProfileGenerationContext &context )
{
  if ( !mProfileCurve || mFeedback->isCanceled() )
    return false;

  QgsScopedAssignObjectToCurrentThread assignProviderToCurrentThread( mRasterProvider.get() );
  if ( mRasterRenderer )
    mRasterRenderer->setInput( mRasterProvider.get() );

  const double startDistanceOffset = std::max( !context.distanceRange().isInfinite() ? context.distanceRange().lower() : 0, 0.0 );
  const double endDistance = context.distanceRange().upper();

  std::unique_ptr< QgsCurve > trimmedCurve;
  QgsCurve *sourceCurve = nullptr;
  if ( startDistanceOffset > 0 || endDistance < mProfileCurve->length() )
  {
    trimmedCurve.reset( mProfileCurve->curveSubstring( startDistanceOffset, endDistance ) );
    sourceCurve = trimmedCurve.get();
  }
  else
  {
    sourceCurve = mProfileCurve.get();
  }

  // we need to transform the profile curve to the raster's CRS
  std::unique_ptr< QgsCurve > transformedCurve( sourceCurve->clone() );
  const QgsCoordinateTransform rasterToTargetTransform( mSourceCrs, mTargetCrs, mTransformContext );
  try
  {
    transformedCurve->transform( rasterToTargetTransform, Qgis::TransformDirection::Reverse );
  }
  catch ( QgsCsException & )
  {
    QgsDebugError( QStringLiteral( "Error transforming profile line to raster CRS" ) );
    return false;
  }

  if ( mFeedback->isCanceled() )
    return false;

  const QgsRectangle profileCurveBoundingBox = transformedCurve->boundingBox();
  if ( !profileCurveBoundingBox.intersects( mRasterProvider->extent() ) )
    return false;

  if ( mFeedback->isCanceled() )
    return false;

  mResults = std::make_unique< QgsRasterLayerProfileResults >();
  mResults->mLayer = mLayer;
  mResults->mId = mId;
  mResults->mMode = mMode;
  mResults->mFixedRange = mFixedRange;
  mResults->copyPropertiesFromGenerator( this );

  std::unique_ptr< QgsGeometryEngine > curveEngine( QgsGeometry::createGeometryEngine( transformedCurve.get() ) );
  curveEngine->prepareGeometry();

  if ( mFeedback->isCanceled() )
    return false;

  double stepDistance = mStepDistance;
  if ( !std::isnan( context.maximumErrorMapUnits() ) )
  {
    // convert the maximum error in curve units to a step distance
    // TODO -- there's no point in this being << pixel size!
    if ( std::isnan( stepDistance ) || context.maximumErrorMapUnits() > stepDistance )
    {
      stepDistance = context.maximumErrorMapUnits();
    }
  }

  QSet< QgsPointXY > profilePoints;
  if ( !std::isnan( stepDistance ) )
  {
    // if specific step distance specified, use this to generate points along the curve
    QgsGeometry densifiedCurve( sourceCurve->clone() );
    densifiedCurve = densifiedCurve.densifyByDistance( stepDistance );
    densifiedCurve.transform( rasterToTargetTransform, Qgis::TransformDirection::Reverse );
    profilePoints.reserve( densifiedCurve.constGet()->nCoordinates() );
    for ( auto it = densifiedCurve.vertices_begin(); it != densifiedCurve.vertices_end(); ++it )
    {
      profilePoints.insert( *it );
    }
  }

  if ( mFeedback->isCanceled() )
    return false;

  // calculate the portion of the raster which actually covers the curve
  int subRegionWidth = 0;
  int subRegionHeight = 0;
  int subRegionLeft = 0;
  int subRegionTop = 0;
  QgsRectangle rasterSubRegion = mRasterProvider->xSize() > 0 && mRasterProvider->ySize() > 0 ?
                                 QgsRasterIterator::subRegion(
                                   mRasterProvider->extent(),
                                   mRasterProvider->xSize(),
                                   mRasterProvider->ySize(),
                                   transformedCurve->boundingBox(),
                                   subRegionWidth,
                                   subRegionHeight,
                                   subRegionLeft,
                                   subRegionTop ) : transformedCurve->boundingBox();

  const bool zeroXYSize = mRasterProvider->xSize() == 0 || mRasterProvider->ySize() == 0;
  if ( zeroXYSize )
  {
    const double curveLengthInPixels = sourceCurve->length() / context.mapUnitsPerDistancePixel();
    const double conversionFactor = curveLengthInPixels / transformedCurve->length();
    subRegionWidth = rasterSubRegion.width() * conversionFactor;
    subRegionHeight = rasterSubRegion.height() * conversionFactor;

    // ensure we fetch at least 1 pixel wide/high blocks, otherwise exactly vertical/horizontal profile lines will result in zero size blocks
    // see https://github.com/qgis/QGIS/issues/51196
    if ( subRegionWidth == 0 )
    {
      subRegionWidth = 1;
      rasterSubRegion.setXMaximum( rasterSubRegion.xMinimum() + 1 / conversionFactor );
    }
    if ( subRegionHeight == 0 )
    {
      subRegionHeight = 1;
      rasterSubRegion.setYMaximum( rasterSubRegion.yMinimum() + 1 / conversionFactor );
    }
  }

  // iterate over the raster blocks, throwing away any which don't intersect the profile curve
  QgsRasterIterator it( mRasterProvider.get() );
  // we use smaller tile sizes vs the default, as we will be skipping over tiles which don't intersect the curve at all,
  // and we expect that to be the VAST majority of the tiles in the raster.
  // => Smaller tile sizes = more regions we can shortcut over = less pixels to iterate over = faster runtime
  it.setMaximumTileHeight( 64 );
  it.setMaximumTileWidth( 64 );

  it.startRasterRead( mBand, subRegionWidth, subRegionHeight, rasterSubRegion );

  const double halfPixelSizeX = mRasterUnitsPerPixelX / 2.0;
  const double halfPixelSizeY = mRasterUnitsPerPixelY / 2.0;
  int blockColumns = 0;
  int blockRows = 0;
  int blockTopLeftColumn = 0;
  int blockTopLeftRow = 0;
  QgsRectangle blockExtent;

  while ( it.next( mBand, blockColumns, blockRows, blockTopLeftColumn, blockTopLeftRow, blockExtent ) )
  {
    if ( mFeedback->isCanceled() )
      return false;

    const QgsGeometry blockExtentGeom = QgsGeometry::fromRect( blockExtent );
    if ( !curveEngine->intersects( blockExtentGeom.constGet() ) )
      continue;

    std::unique_ptr< QgsRasterBlock > block( mRasterProvider->block( mBand, blockExtent, blockColumns, blockRows, mFeedback.get() ) );
    if ( mFeedback->isCanceled() )
      return false;

    if ( !block )
      continue;

    std::unique_ptr< QgsRasterBlock > renderedBlock;
    if ( mRasterRenderer )
    {
      renderedBlock.reset( mRasterRenderer->block( mBand, blockExtent, blockColumns, blockRows, mFeedback.get() ) );
    }

    bool isNoData = false;

    // there's two potential code paths we use here, depending on if we want to sample at every pixel, or if we only want to
    // sample at specific points
    if ( !std::isnan( stepDistance ) )
    {
      auto it = profilePoints.begin();
      while ( it != profilePoints.end() )
      {
        if ( mFeedback->isCanceled() )
          return false;

        // convert point to a pixel and sample, if it's in this block
        if ( blockExtent.contains( *it ) )
        {
          int row, col;
          if ( zeroXYSize )
          {
            row = std::clamp( static_cast< int >( std::round( ( blockExtent.yMaximum() - it->y() ) * blockRows / blockExtent.height() ) ), 0, blockRows - 1 );
            col = std::clamp( static_cast< int >( std::round( ( it->x() - blockExtent.xMinimum() ) * blockColumns / blockExtent.width() ) ), 0, blockColumns - 1 );
          }
          else
          {
            row = std::clamp( static_cast< int >( std::round( ( blockExtent.yMaximum() - it->y() ) / mRasterUnitsPerPixelY ) ), 0, blockRows - 1 );
            col = std::clamp( static_cast< int >( std::round( ( it->x() - blockExtent.xMinimum() ) / mRasterUnitsPerPixelX ) ),  0, blockColumns - 1 );
          }
          const double val = block->valueAndNoData( row, col, isNoData );
          double z = 0;
          QColor pixelColor;
          if ( !isNoData )
          {
            switch ( mMode )
            {
              case Qgis::RasterElevationMode::FixedElevationRange:
              {
                z = mFixedRange.lower();
                pixelColor = renderedBlock->color( row, col );
                break;
              }

              case Qgis::RasterElevationMode::RepresentsElevationSurface:
                z = val * mScale + mOffset;
                break;
            }
          }
          else
          {
            z = std::numeric_limits<double>::quiet_NaN();
          }

          QgsPoint pixel( it->x(), it->y(), z, val );
          try
          {
            pixel.transform( rasterToTargetTransform );
          }
          catch ( QgsCsException & )
          {
            continue;
          }
          mResults->mRawPoints.append( pixel );

          switch ( mMode )
          {
            case Qgis::RasterElevationMode::FixedElevationRange:
            {
              mResults->mRenderedColors.append( pixelColor );
              break;
            }

            case Qgis::RasterElevationMode::RepresentsElevationSurface:
              break;
          }

          it = profilePoints.erase( it );
        }
        else
        {
          it++;
        }
      }

      if ( profilePoints.isEmpty() )
        break; // all done!
    }
    else
    {
      double currentY = blockExtent.yMaximum() - 0.5 * mRasterUnitsPerPixelY;
      for ( int row = 0; row < blockRows; ++row )
      {
        if ( mFeedback->isCanceled() )
          return false;

        double currentX = blockExtent.xMinimum() + 0.5 * mRasterUnitsPerPixelX;
        for ( int col = 0; col < blockColumns; ++col, currentX += mRasterUnitsPerPixelX )
        {
          const double val = block->valueAndNoData( row, col, isNoData );

          // does pixel intersect curve?
          QgsGeometry pixelRectGeometry = QgsGeometry::fromRect( QgsRectangle( currentX - halfPixelSizeX,
                                          currentY - halfPixelSizeY,
                                          currentX + halfPixelSizeX,
                                          currentY + halfPixelSizeY ) );
          if ( !curveEngine->intersects( pixelRectGeometry.constGet() ) )
            continue;

          double z = 0;
          QColor pixelColor;
          switch ( mMode )
          {
            case Qgis::RasterElevationMode::FixedElevationRange:
              z = mFixedRange.lower();
              pixelColor = renderedBlock->color( row, col );
              break;

            case Qgis::RasterElevationMode::RepresentsElevationSurface:
              z = val * mScale + mOffset;
              break;
          }

          QgsPoint pixel( currentX, currentY, isNoData ? std::numeric_limits<double>::quiet_NaN() : z, val );
          try
          {
            pixel.transform( rasterToTargetTransform );
          }
          catch ( QgsCsException & )
          {
            continue;
          }
          mResults->mRawPoints.append( pixel );

          switch ( mMode )
          {
            case Qgis::RasterElevationMode::FixedElevationRange:
            {
              mResults->mRenderedColors.append( pixelColor );
              break;
            }

            case Qgis::RasterElevationMode::RepresentsElevationSurface:
              break;
          }
        }
        currentY -= mRasterUnitsPerPixelY;
      }
    }
  }

  if ( mFeedback->isCanceled() )
    return false;

  // convert x/y values back to distance/height values
  QgsGeos originalCurveGeos( sourceCurve );
  originalCurveGeos.prepareGeometry();
  QString lastError;
  int pixelIndex = 0;
  for ( const QgsPoint &pixel : std::as_const( mResults->mRawPoints ) )
  {
    if ( mFeedback->isCanceled() )
      return false;

    const double distance = originalCurveGeos.lineLocatePoint( pixel, &lastError ) + startDistanceOffset;

    if ( !std::isnan( pixel.z() ) )
    {
      mResults->minZ = std::min( pixel.z(), mResults->minZ );
      mResults->maxZ = std::max( pixel.z(), mResults->maxZ );
    }
    mResults->mDistanceToHeightMap.insert( distance, pixel.z() );

    switch ( mMode )
    {
      case Qgis::RasterElevationMode::FixedElevationRange:
        mResults->mDistanceToValueMap.insert( distance, pixel.m() );
        mResults->mDistanceToColorMap.insert( distance, mResults->mRenderedColors.at( pixelIndex ) );
        break;

      case Qgis::RasterElevationMode::RepresentsElevationSurface:
        break;
    }

    pixelIndex++;
  }

  return true;
}

QgsAbstractProfileResults *QgsRasterLayerProfileGenerator::takeResults()
{
  return mResults.release();
}

QgsFeedback *QgsRasterLayerProfileGenerator::feedback() const
{
  return mFeedback.get();
}
