/***************************************************************************
                         qgsalgorithmxyztiles.cpp
                         ---------------------
    begin                : August 2023
    copyright            : (C) 2023 by Alexander Bruy
    email                : alexander dot bruy at gmail dot com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsalgorithmxyztiles.h"

#include "qgsexpressioncontextutils.h"
#include "qgslayertree.h"
#include "qgslayertreelayer.h"
#include "qgsmaplayerutils.h"
#include "qgsprovidermetadata.h"

#include <QBuffer>
#include <QString>

using namespace Qt::StringLiterals;

///@cond PRIVATE

int tile2tms( const int y, const int zoom )
{
  double n = std::pow( 2, zoom );
  return ( int ) std::floor( n - y - 1 );
}

int lon2tileX( const double lon, const int z )
{
  return ( int ) ( std::floor( ( lon + 180.0 ) / 360.0 * ( 1 << z ) ) );
}

int lat2tileY( const double lat, const int z )
{
  double latRad = lat * M_PI / 180.0;
  return ( int ) ( std::floor( ( 1.0 - std::asinh( std::tan( latRad ) ) / M_PI ) / 2.0 * ( 1 << z ) ) );
}

double tileX2lon( const int x, const int z )
{
  return x / ( double ) ( 1 << z ) * 360.0 - 180;
}

double tileY2lat( const int y, const int z )
{
  double n = M_PI - 2.0 * M_PI * y / ( double ) ( 1 << z );
  return 180.0 / M_PI * std::atan( 0.5 * ( std::exp( n ) - std::exp( -n ) ) );
}

QList<MetaTile> getMetatiles( const QgsRectangle extent, const int zoom, long long &tileCount, const int tileSize )
{
  int minX = lon2tileX( extent.xMinimum(), zoom );
  int minY = lat2tileY( extent.yMaximum(), zoom );
  int maxX = lon2tileX( extent.xMaximum(), zoom );
  int maxY = lat2tileY( extent.yMinimum(), zoom );
  tileCount = static_cast<long long>( maxX - minX + 1 ) * static_cast<long long>( maxY - minY + 1 );

  int i = 0;
  QMap<QString, MetaTile> tiles;
  for ( int x = minX; x <= maxX; x++ )
  {
    int j = 0;
    for ( int y = minY; y <= maxY; y++ )
    {
      QString key = u"%1:%2"_s.arg( ( int ) ( i / tileSize ) ).arg( ( int ) ( j / tileSize ) );
      MetaTile tile = tiles.value( key, MetaTile() );
      tile.addTile( i % tileSize, j % tileSize, Tile( x, y, zoom ) );
      tiles.insert( key, tile );
      j++;
    }
    i++;
  }
  return tiles.values();
}

////

QString QgsXyzTilesBaseAlgorithm::group() const
{
  return QObject::tr( "Raster tools" );
}

QString QgsXyzTilesBaseAlgorithm::groupId() const
{
  return u"rastertools"_s;
}

Qgis::ProcessingAlgorithmFlags QgsXyzTilesBaseAlgorithm::flags() const
{
  return QgsProcessingAlgorithm::flags() | Qgis::ProcessingAlgorithmFlag::RequiresProject;
}

void QgsXyzTilesBaseAlgorithm::createCommonParameters()
{
  addParameter( new QgsProcessingParameterExtent( u"EXTENT"_s, QObject::tr( "Extent" ) ) );
  addParameter( new QgsProcessingParameterNumber( u"ZOOM_MIN"_s, QObject::tr( "Minimum zoom" ), Qgis::ProcessingNumberParameterType::Integer, 12, false, 0, 25 ) );
  addParameter( new QgsProcessingParameterNumber( u"ZOOM_MAX"_s, QObject::tr( "Maximum zoom" ), Qgis::ProcessingNumberParameterType::Integer, 12, false, 0, 25 ) );
  addParameter( new QgsProcessingParameterNumber( u"DPI"_s, QObject::tr( "DPI" ), Qgis::ProcessingNumberParameterType::Integer, 96, false, 48, 600 ) );
  addParameter( new QgsProcessingParameterColor( u"BACKGROUND_COLOR"_s, QObject::tr( "Background color" ), QColor( Qt::transparent ), true, true ) );
  addParameter( new QgsProcessingParameterBoolean( u"ANTIALIAS"_s, QObject::tr( "Enable antialiasing" ), true ) );

  QStringList formatOptions = { u"PNG"_s, u"JPG"_s };
  if ( supportsWebP() )
  {
    formatOptions.append( u"WEBP"_s );
  }
  addParameter( new QgsProcessingParameterEnum( u"TILE_FORMAT"_s, QObject::tr( "Tile format" ), formatOptions, false, 0 ) );
  addParameter( new QgsProcessingParameterNumber( u"QUALITY"_s, QObject::tr( "Quality (JPG only)" ), Qgis::ProcessingNumberParameterType::Integer, 75, false, 1, 100 ) );
  addParameter( new QgsProcessingParameterNumber( u"METATILESIZE"_s, QObject::tr( "Metatile size" ), Qgis::ProcessingNumberParameterType::Integer, 4, false, 1, 20 ) );
}

bool QgsXyzTilesBaseAlgorithm::prepareAlgorithm( const QVariantMap &parameters, QgsProcessingContext &context, QgsProcessingFeedback *feedback )
{
  Q_UNUSED( feedback );

  QgsProject *project = context.project();

  const QList<QgsLayerTreeLayer *> projectLayers = project->layerTreeRoot()->findLayers();
  QSet<QString> visibleLayers;
  for ( const QgsLayerTreeLayer *layer : projectLayers )
  {
    if ( layer->isVisible() )
    {
      visibleLayers << layer->layer()->id();
    }
  }

  QList<QgsMapLayer *> renderLayers = project->layerTreeRoot()->layerOrder();
  for ( QgsMapLayer *layer : renderLayers )
  {
    if ( visibleLayers.contains( layer->id() ) )
    {
      QgsMapLayer *clonedLayer = layer->clone();
      clonedLayer->moveToThread( nullptr );
      mLayers << clonedLayer;
    }
  }

  QgsRectangle extent = parameterAsExtent( parameters, u"EXTENT"_s, context );
  QgsCoordinateReferenceSystem extentCrs = parameterAsExtentCrs( parameters, u"EXTENT"_s, context );
  QgsCoordinateTransform ct( extentCrs, project->crs(), context.transformContext() );
  ct.setBallparkTransformsAreAppropriate( true );
  try
  {
    mExtent = ct.transformBoundingBox( extent );
  }
  catch ( QgsCsException & )
  {
    throw QgsProcessingException( QObject::tr( "Could not transform the extent into the project CRS" ) );
  }

  mMinZoom = parameterAsInt( parameters, u"ZOOM_MIN"_s, context );
  mMaxZoom = parameterAsInt( parameters, u"ZOOM_MAX"_s, context );
  if ( mMaxZoom < mMinZoom )
  {
    throw QgsProcessingException( QObject::tr( "Maximum zoom (%1) must be ≥ minimum zoom (%2)" ).arg( mMaxZoom ).arg( mMinZoom ) );
  }
  mDpi = parameterAsInt( parameters, u"DPI"_s, context );
  mBackgroundColor = parameterAsColor( parameters, u"BACKGROUND_COLOR"_s, context );
  mAntialias = parameterAsBool( parameters, u"ANTIALIAS"_s, context );
  switch ( parameterAsEnum( parameters, u"TILE_FORMAT"_s, context ) )
  {
    case 0:
      mTileFormat = u"PNG"_s;
      break;
    case 1:
      mTileFormat = u"JPG"_s;
      break;
    case 2:
      mTileFormat = u"WEBP"_s;
      break;
    default:
      mTileFormat = u"PNG"_s;
      break;
  }

  mJpgQuality = mTileFormat != "PNG"_L1 ? parameterAsInt( parameters, u"QUALITY"_s, context ) : -1;
  mMetaTileSize = parameterAsInt( parameters, u"METATILESIZE"_s, context );
  mThreadsNumber = context.maximumThreads();
  mTransformContext = context.transformContext();
  mEllipsoid = context.ellipsoid();
  mFeedback = feedback;

  QgsCoordinateTransform src2Wgs = QgsCoordinateTransform( project->crs(), QgsCoordinateReferenceSystem( "EPSG:4326" ), context.transformContext() );
  src2Wgs.setBallparkTransformsAreAppropriate( true );
  try
  {
    mWgs84Extent = src2Wgs.transformBoundingBox( mExtent );
  }
  catch ( QgsCsException & )
  {
    throw QgsProcessingException( QObject::tr( "Could not transform the extent into WGS84" ) );
  }

  if ( parameters.contains( u"TILE_WIDTH"_s ) )
  {
    mTileWidth = parameterAsInt( parameters, u"TILE_WIDTH"_s, context );
  }

  if ( parameters.contains( u"TILE_HEIGHT"_s ) )
  {
    mTileHeight = parameterAsInt( parameters, u"TILE_HEIGHT"_s, context );
  }

  if ( ( mTileFormat != "PNG"_L1 && mTileFormat != "WEBP"_L1 ) && mBackgroundColor.alpha() != 255 )
  {
    feedback->pushWarning( QObject::tr( "Background color setting ignored, the JPG format only supports fully opaque colors" ) );
  }

  mScaleMethod = project->scaleMethod();

  return true;
}

void QgsXyzTilesBaseAlgorithm::checkLayersUsagePolicy( QgsProcessingFeedback *feedback )
{
  if ( mTotalMetaTiles > MAXIMUM_OPENSTREETMAP_TILES_FETCH )
  {
    for ( QgsMapLayer *layer : std::as_const( mLayers ) )
    {
      if ( QgsMapLayerUtils::isOpenStreetMapLayer( layer ) )
      {
        // Prevent bulk downloading of tiles from openstreetmap.org as per OSMF tile usage policy
        feedback->pushFormattedMessage(
          QObject::tr( "Layer %1 will be skipped as the algorithm leads to bulk downloading behavior which is prohibited by the %2OpenStreetMap Foundation tile usage policy%3" )
            .arg( layer->name(), u"<a href=\"https://operations.osmfoundation.org/policies/tiles/\">"_s, u"</a>"_s ),
          QObject::tr( "Layer %1 will be skipped as the algorithm leads to bulk downloading behavior which is prohibited by the %2OpenStreetMap Foundation tile usage policy%3" )
            .arg( layer->name(), QString(), QString() )
        );
        mLayers.removeAll( layer );
        delete layer;
      }
    }
  }
}

void QgsXyzTilesBaseAlgorithm::startJobs()
{
  QgsCoordinateReferenceSystem mercatorCrs = QgsCoordinateReferenceSystem( "EPSG:3857" );
  QgsCoordinateTransform wgsToMercator = QgsCoordinateTransform( QgsCoordinateReferenceSystem( "EPSG:4326" ), mercatorCrs, mTransformContext );
  wgsToMercator.setBallparkTransformsAreAppropriate( true );

  while ( mRendererJobs.size() < mThreadsNumber && !mMetaTiles.empty() )
  {
    MetaTile metaTile = mMetaTiles.takeFirst();

    QgsMapSettings settings;
    try
    {
      settings.setExtent( wgsToMercator.transformBoundingBox( metaTile.extent() ) );
    }
    catch ( QgsCsException & )
    {
      continue;
    }
    settings.setRendererUsage( Qgis::RendererUsage::Export );
    settings.setOutputImageFormat( QImage::Format_ARGB32_Premultiplied );
    settings.setTransformContext( mTransformContext );
    settings.setEllipsoid( mEllipsoid );
    settings.setDestinationCrs( mercatorCrs );
    settings.setLayers( mLayers );
    settings.setOutputDpi( mDpi );
    settings.setFlag( Qgis::MapSettingsFlag::Antialiasing, mAntialias );
    settings.setScaleMethod( mScaleMethod );
    if ( mTileFormat == "PNG"_L1 || mTileFormat == "WEBP"_L1 || mBackgroundColor.alpha() == 255 )
    {
      settings.setBackgroundColor( mBackgroundColor );
    }
    QSize size( mTileWidth * metaTile.rows, mTileHeight * metaTile.cols );
    settings.setOutputSize( size );

    QgsLabelingEngineSettings labelingSettings = settings.labelingEngineSettings();
    labelingSettings.setFlag( Qgis::LabelingFlag::UsePartialCandidates, false );
    settings.setLabelingEngineSettings( labelingSettings );

    QgsExpressionContext exprContext = settings.expressionContext();
    exprContext.appendScope( QgsExpressionContextUtils::mapSettingsScope( settings ) );
    settings.setExpressionContext( exprContext );

    QgsMapRendererSequentialJob *job = new QgsMapRendererSequentialJob( settings );
    mRendererJobs.insert( job, metaTile );
    QObject::connect( job, &QgsMapRendererJob::finished, mJobOwner, [this, job]() { processMetaTile( job ); } );
    job->start();
  }
}

// Native XYZ tiles (directory) algorithm

QString QgsXyzTilesDirectoryAlgorithm::name() const
{
  return u"tilesxyzdirectory"_s;
}

QString QgsXyzTilesDirectoryAlgorithm::displayName() const
{
  return QObject::tr( "Generate XYZ tiles (Directory)" );
}

QStringList QgsXyzTilesDirectoryAlgorithm::tags() const
{
  return QObject::tr( "tiles,xyz,tms,directory" ).split( ',' );
}

QString QgsXyzTilesDirectoryAlgorithm::shortHelpString() const
{
  return QObject::tr( "Generates XYZ tiles of map canvas content and saves them as individual images in a directory." );
}

QgsXyzTilesDirectoryAlgorithm *QgsXyzTilesDirectoryAlgorithm::createInstance() const
{
  return new QgsXyzTilesDirectoryAlgorithm();
}

void QgsXyzTilesDirectoryAlgorithm::initAlgorithm( const QVariantMap & )
{
  createCommonParameters();
  addParameter( new QgsProcessingParameterNumber( u"TILE_WIDTH"_s, QObject::tr( "Tile width" ), Qgis::ProcessingNumberParameterType::Integer, 256, false, 1, 4096 ) );
  addParameter( new QgsProcessingParameterNumber( u"TILE_HEIGHT"_s, QObject::tr( "Tile height" ), Qgis::ProcessingNumberParameterType::Integer, 256, false, 1, 4096 ) );
  addParameter( new QgsProcessingParameterBoolean( u"TMS_CONVENTION"_s, QObject::tr( "Use inverted tile Y axis (TMS convention)" ), false ) );

  auto titleParam = std::make_unique<QgsProcessingParameterString>( u"HTML_TITLE"_s, QObject::tr( "Leaflet HTML output title" ), QVariant(), false, true );
  titleParam->setFlags( titleParam->flags() | Qgis::ProcessingParameterFlag::Advanced );
  addParameter( titleParam.release() );
  auto attributionParam = std::make_unique<QgsProcessingParameterString>( u"HTML_ATTRIBUTION"_s, QObject::tr( "Leaflet HTML output attribution" ), QVariant(), false, true );
  attributionParam->setFlags( attributionParam->flags() | Qgis::ProcessingParameterFlag::Advanced );
  addParameter( attributionParam.release() );
  auto osmParam = std::make_unique<QgsProcessingParameterBoolean>( u"HTML_OSM"_s, QObject::tr( "Include OpenStreetMap basemap in Leaflet HTML output" ), false );
  osmParam->setFlags( osmParam->flags() | Qgis::ProcessingParameterFlag::Advanced );
  addParameter( osmParam.release() );

  addParameter( new QgsProcessingParameterFolderDestination( u"OUTPUT_DIRECTORY"_s, QObject::tr( "Output directory" ) ) );
  addParameter( new QgsProcessingParameterFileDestination( u"OUTPUT_HTML"_s, QObject::tr( "Output html (Leaflet)" ), QObject::tr( "HTML files (*.html)" ), QVariant(), true ) );
}

QVariantMap QgsXyzTilesDirectoryAlgorithm::processAlgorithm( const QVariantMap &parameters, QgsProcessingContext &context, QgsProcessingFeedback *feedback )
{
  const bool tms = parameterAsBoolean( parameters, u"TMS_CONVENTION"_s, context );
  const QString title = parameterAsString( parameters, u"HTML_TITLE"_s, context );
  const QString attribution = parameterAsString( parameters, u"HTML_ATTRIBUTION"_s, context );
  const bool useOsm = parameterAsBoolean( parameters, u"HTML_OSM"_s, context );
  QString outputDir = parameterAsString( parameters, u"OUTPUT_DIRECTORY"_s, context );
  const QString outputHtml = parameterAsString( parameters, u"OUTPUT_HTML"_s, context );

  mOutputDir = outputDir;
  mTms = tms;

  long long totalTiles = 0;
  mTotalMetaTiles = 0;
  for ( int z = mMinZoom; z <= mMaxZoom; z++ )
  {
    if ( feedback->isCanceled() )
      break;

    long long tileCount = 0;
    mMetaTiles += getMetatiles( mWgs84Extent, z, tileCount, mMetaTileSize );
    feedback->pushInfo( QObject::tr( "%1 metatiles (%2 tiles) will be created for zoom level %3" ).arg( mMetaTiles.size() - mTotalMetaTiles ).arg( tileCount ).arg( z ) );
    mTotalMetaTiles = mMetaTiles.size();
    totalTiles += tileCount;
  }
  if ( mTotalMetaTiles == 0 )
  {
    throw QgsProcessingException( QObject::tr( "No metatiles will be created -- please check the extent and zoom limits" ) );
  }

  feedback->pushInfo( QObject::tr( "A total of %1 metatiles (%2 tiles) will be created" ).arg( mTotalMetaTiles ).arg( totalTiles ) );

  checkLayersUsagePolicy( feedback );

  for ( QgsMapLayer *layer : std::as_const( mLayers ) )
  {
    layer->moveToThread( QThread::currentThread() );
  }
  mJobOwner.reset( new QObject() );

  QEventLoop loop;
  // cppcheck-suppress danglingLifetime
  mEventLoop = &loop;
  startJobs();
  loop.exec();

  qDeleteAll( mLayers );
  mLayers.clear();

  QVariantMap results;
  results.insert( u"OUTPUT_DIRECTORY"_s, outputDir );

  if ( !outputHtml.isEmpty() )
  {
    QString osm = QStringLiteral(
                    "var osm_layer = L.tileLayer('https://tile.openstreetmap.org/{z}/{x}/{y}.png',"
                    "{minZoom: %1, maxZoom: %2, attribution: '&copy; <a href=\"https://www.openstreetmap.org/copyright\">OpenStreetMap</a> contributors'}).addTo(map);"
    )
                    .arg( mMinZoom )
                    .arg( mMaxZoom );

    QString addOsm = useOsm ? osm : QString();
    QString tmsConvention = tms ? u"true"_s : u"false"_s;
    QString attr = attribution.isEmpty() ? u"Created by QGIS"_s : attribution;
    QString tileSource = u"'file:///%1/{z}/{x}/{y}.%2'"_s.arg( outputDir.replace( "\\", "/" ).toHtmlEscaped() ).arg( mTileFormat.toLower() );

    QString html = QStringLiteral(
                     "<!DOCTYPE html><html><head><title>%1</title><meta charset=\"utf-8\"/>"
                     "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
                     "<link rel=\"stylesheet\" href=\"https://unpkg.com/leaflet@1.9.4/dist/leaflet.css\""
                     "integrity=\"sha384-sHL9NAb7lN7rfvG5lfHpm643Xkcjzp4jFvuavGOndn6pjVqS6ny56CAt3nsEVT4H\""
                     "crossorigin=\"\"/>"
                     "<script src=\"https://unpkg.com/leaflet@1.9.4/dist/leaflet.js\""
                     "integrity=\"sha384-cxOPjt7s7Iz04uaHJceBmS+qpjv2JkIHNVcuOrM+YHwZOmJGBXI00mdUXEq65HTH\""
                     "crossorigin=\"\"></script>"
                     "<style type=\"text/css\">body {margin: 0;padding: 0;} html, body, #map{width: 100%;height: 100%;}</style></head>"
                     "<body><div id=\"map\"></div><script>"
                     "var map = L.map('map', {attributionControl: false}).setView([%2, %3], %4);"
                     "L.control.attribution({prefix: false}).addTo(map);"
                     "%5"
                     "var tilesource_layer = L.tileLayer(%6, {minZoom: %7, maxZoom: %8, tms: %9, attribution: '%10'}).addTo(map);"
                     "</script></body></html>"
    )
                     .arg( title.isEmpty() ? u"Leaflet preview"_s : title )
                     .arg( mWgs84Extent.center().y() )
                     .arg( mWgs84Extent.center().x() )
                     .arg( ( mMaxZoom + mMinZoom ) / 2 )
                     .arg( addOsm )
                     .arg( tileSource )
                     .arg( mMinZoom )
                     .arg( mMaxZoom )
                     .arg( tmsConvention )
                     .arg( attr );

    QFile htmlFile( outputHtml );
    if ( !htmlFile.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
    {
      throw QgsProcessingException( QObject::tr( "Could not open html file %1" ).arg( outputHtml ) );
    }
    QTextStream fout( &htmlFile );
    fout << html;

    results.insert( u"OUTPUT_HTML"_s, outputHtml );
  }

  return results;
}

void QgsXyzTilesDirectoryAlgorithm::processMetaTile( QgsMapRendererSequentialJob *job )
{
  MetaTile metaTile = mRendererJobs.value( job );
  QImage img = job->renderedImage();

  QMap<QPair<int, int>, Tile>::const_iterator it = metaTile.tiles.constBegin();
  while ( it != metaTile.tiles.constEnd() )
  {
    QPair<int, int> tm = it.key();
    Tile tile = it.value();
    QImage tileImage = img.copy( mTileWidth * tm.first, mTileHeight * tm.second, mTileWidth, mTileHeight );
    QDir tileDir( u"%1/%2/%3"_s.arg( mOutputDir ).arg( tile.z ).arg( tile.x ) );
    tileDir.mkpath( tileDir.absolutePath() );
    int y = tile.y;
    if ( mTms )
    {
      y = tile2tms( y, tile.z );
    }
    tileImage.save( u"%1/%2.%3"_s.arg( tileDir.absolutePath() ).arg( y ).arg( mTileFormat.toLower() ), mTileFormat.toStdString().c_str(), mJpgQuality );
    ++it;
  }

  mRendererJobs.remove( job );
  job->deleteLater();

  mFeedback->setProgress( 100.0 * ( mProcessedMetaTiles++ ) / mTotalMetaTiles );

  if ( mFeedback->isCanceled() )
  {
    while ( mRendererJobs.size() > 0 )
    {
      QgsMapRendererSequentialJob *j = mRendererJobs.firstKey();
      j->cancel();
      mRendererJobs.remove( j );
      j->deleteLater();
    }
    mRendererJobs.clear();
    if ( mEventLoop )
    {
      mEventLoop->exit();
    }
    return;
  }

  if ( mMetaTiles.size() > 0 )
  {
    startJobs();
  }
  else if ( mMetaTiles.size() == 0 && mRendererJobs.size() == 0 )
  {
    if ( mEventLoop )
    {
      mEventLoop->exit();
    }
  }
}

// Native XYZ tiles (MBTiles) algorithm

QString QgsXyzTilesMbtilesAlgorithm::name() const
{
  return u"tilesxyzmbtiles"_s;
}

QString QgsXyzTilesMbtilesAlgorithm::displayName() const
{
  return QObject::tr( "Generate XYZ tiles (MBTiles)" );
}

QStringList QgsXyzTilesMbtilesAlgorithm::tags() const
{
  return QObject::tr( "tiles,xyz,tms,mbtiles" ).split( ',' );
}

QString QgsXyzTilesMbtilesAlgorithm::shortHelpString() const
{
  return QObject::tr( "Generates XYZ tiles of map canvas content and saves them as an MBTiles file." );
}

QgsXyzTilesMbtilesAlgorithm *QgsXyzTilesMbtilesAlgorithm::createInstance() const
{
  return new QgsXyzTilesMbtilesAlgorithm();
}

void QgsXyzTilesMbtilesAlgorithm::initAlgorithm( const QVariantMap & )
{
  createCommonParameters();
  addParameter( new QgsProcessingParameterFileDestination( u"OUTPUT_FILE"_s, QObject::tr( "Output" ), QObject::tr( "MBTiles files (*.mbtiles *.MBTILES)" ) ) );
}

QVariantMap QgsXyzTilesMbtilesAlgorithm::processAlgorithm( const QVariantMap &parameters, QgsProcessingContext &context, QgsProcessingFeedback *feedback )
{
  const QString outputFile = parameterAsString( parameters, u"OUTPUT_FILE"_s, context );

  mMbtilesWriter = std::make_unique<QgsMbTiles>( outputFile );
  if ( !mMbtilesWriter->create() )
  {
    throw QgsProcessingException( QObject::tr( "Failed to create MBTiles file %1" ).arg( outputFile ) );
  }
  mMbtilesWriter->setMetadataValue( "format", mTileFormat.toLower() );
  mMbtilesWriter->setMetadataValue( "name", QFileInfo( outputFile ).baseName() );
  mMbtilesWriter->setMetadataValue( "description", QFileInfo( outputFile ).baseName() );
  mMbtilesWriter->setMetadataValue( "version", u"1.1"_s );
  mMbtilesWriter->setMetadataValue( "type", u"overlay"_s );
  mMbtilesWriter->setMetadataValue( "minzoom", QString::number( mMinZoom ) );
  mMbtilesWriter->setMetadataValue( "maxzoom", QString::number( mMaxZoom ) );
  QString boundsStr = QString( "%1,%2,%3,%4" ).arg( mWgs84Extent.xMinimum() ).arg( mWgs84Extent.yMinimum() ).arg( mWgs84Extent.xMaximum() ).arg( mWgs84Extent.yMaximum() );
  mMbtilesWriter->setMetadataValue( "bounds", boundsStr );

  long long totalTiles = 0;
  mTotalMetaTiles = 0;
  for ( int z = mMinZoom; z <= mMaxZoom; z++ )
  {
    if ( feedback->isCanceled() )
      break;

    long long tileCount = 0;
    mMetaTiles += getMetatiles( mWgs84Extent, z, tileCount, mMetaTileSize );
    feedback->pushInfo( QObject::tr( "%1 metatiles (%2 tiles) will be created for zoom level %3" ).arg( mMetaTiles.size() - mTotalMetaTiles ).arg( tileCount ).arg( z ) );
    mTotalMetaTiles = mMetaTiles.size();
    totalTiles += tileCount;
  }
  if ( mTotalMetaTiles == 0 )
  {
    throw QgsProcessingException( QObject::tr( "No metatiles will be created -- please check the extent and zoom limits" ) );
  }

  feedback->pushInfo( QObject::tr( "A total of %1 metatiles (%2 tiles) will be created" ).arg( mTotalMetaTiles ).arg( totalTiles ) );

  checkLayersUsagePolicy( feedback );

  for ( QgsMapLayer *layer : std::as_const( mLayers ) )
  {
    layer->moveToThread( QThread::currentThread() );
  }
  mJobOwner.reset( new QObject() );

  QEventLoop loop;
  // cppcheck-suppress danglingLifetime
  mEventLoop = &loop;
  startJobs();
  loop.exec();

  qDeleteAll( mLayers );
  mLayers.clear();

  QVariantMap results;
  results.insert( u"OUTPUT_FILE"_s, outputFile );
  return results;
}

void QgsXyzTilesMbtilesAlgorithm::processMetaTile( QgsMapRendererSequentialJob *job )
{
  MetaTile metaTile = mRendererJobs.value( job );
  QImage img = job->renderedImage();

  QMap<QPair<int, int>, Tile>::const_iterator it = metaTile.tiles.constBegin();
  while ( it != metaTile.tiles.constEnd() )
  {
    QPair<int, int> tm = it.key();
    Tile tile = it.value();
    QImage tileImage = img.copy( mTileWidth * tm.first, mTileHeight * tm.second, mTileWidth, mTileHeight );
    QByteArray ba;
    QBuffer buffer( &ba );
    buffer.open( QIODevice::WriteOnly );
    tileImage.save( &buffer, mTileFormat.toStdString().c_str(), mJpgQuality );
    mMbtilesWriter->setTileData( tile.z, tile.x, tile2tms( tile.y, tile.z ), ba );
    ++it;
  }

  mRendererJobs.remove( job );
  job->deleteLater();

  mFeedback->setProgress( 100.0 * ( mProcessedMetaTiles++ ) / mTotalMetaTiles );

  if ( mFeedback->isCanceled() )
  {
    while ( mRendererJobs.size() > 0 )
    {
      QgsMapRendererSequentialJob *j = mRendererJobs.firstKey();
      j->cancel();
      mRendererJobs.remove( j );
      j->deleteLater();
    }
    mRendererJobs.clear();
    if ( mEventLoop )
    {
      mEventLoop->exit();
    }
    return;
  }

  if ( mMetaTiles.size() > 0 )
  {
    startJobs();
  }
  else if ( mMetaTiles.size() == 0 && mRendererJobs.size() == 0 )
  {
    if ( mEventLoop )
    {
      mEventLoop->exit();
    }
  }
}


//
// QgsGeoPackageTiles
//

QgsGeoPackageTiles::QgsGeoPackageTiles( const QString &filename )
  : mFilename( filename )
{}

QgsGeoPackageTiles::~QgsGeoPackageTiles()
{
  close();
}

bool QgsGeoPackageTiles::create( const QgsRectangle &mercExtent, int minZoom, int maxZoom, int tileWidth, int tileHeight )
{
  if ( sqlite3_open( mFilename.toUtf8().constData(), &mDb ) != SQLITE_OK )
    return false;

  sqlite3_exec( mDb, "PRAGMA application_id = 1196444487;", nullptr, nullptr, nullptr );
  sqlite3_exec( mDb, "PRAGMA user_version = 10200;", nullptr, nullptr, nullptr );
  sqlite3_exec( mDb, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr );

  const char *schemaSql = "CREATE TABLE gpkg_spatial_ref_sys ("
                          "  srs_name TEXT NOT NULL, srs_id INTEGER NOT NULL PRIMARY KEY,"
                          "  organization TEXT NOT NULL, organization_coordsys_id INTEGER NOT NULL,"
                          "  definition TEXT NOT NULL, description TEXT"
                          ");"
                          "CREATE TABLE gpkg_contents ("
                          "  table_name TEXT NOT NULL PRIMARY KEY, data_type TEXT NOT NULL,"
                          "  identifier TEXT UNIQUE, description TEXT DEFAULT '',"
                          "  last_change DATETIME NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ', 'now')),"
                          "  min_x DOUBLE, min_y DOUBLE, max_x DOUBLE, max_y DOUBLE, srs_id INTEGER"
                          ");"
                          "CREATE TABLE gpkg_tile_matrix_set ("
                          "  table_name TEXT NOT NULL PRIMARY KEY, srs_id INTEGER NOT NULL,"
                          "  min_x DOUBLE NOT NULL, min_y DOUBLE NOT NULL,"
                          "  max_x DOUBLE NOT NULL, max_y DOUBLE NOT NULL"
                          ");"
                          "CREATE TABLE gpkg_tile_matrix ("
                          "  table_name TEXT NOT NULL, zoom_level INTEGER NOT NULL,"
                          "  matrix_width INTEGER NOT NULL, matrix_height INTEGER NOT NULL,"
                          "  tile_width INTEGER NOT NULL, tile_height INTEGER NOT NULL,"
                          "  pixel_x_size DOUBLE NOT NULL, pixel_y_size DOUBLE NOT NULL,"
                          "  CONSTRAINT pk_ttm PRIMARY KEY (table_name, zoom_level)"
                          ");"
                          "CREATE TABLE tiles ("
                          "  id INTEGER PRIMARY KEY AUTOINCREMENT, zoom_level INTEGER NOT NULL,"
                          "  tile_column INTEGER NOT NULL, tile_row INTEGER NOT NULL,"
                          "  tile_data BLOB NOT NULL,"
                          "  CONSTRAINT uk_tiles UNIQUE (zoom_level, tile_column, tile_row)"
                          ");";

  if ( sqlite3_exec( mDb, schemaSql, nullptr, nullptr, nullptr ) != SQLITE_OK )
    return false;

  // Insert standard GeoPackage SRS entries
  const char *srsSql
    = "INSERT INTO gpkg_spatial_ref_sys VALUES "
      "('Undefined cartesian', -1, 'NONE', -1, 'undefined', 'undefined')," // #spellok
      "('Undefined geographic', 0, 'NONE', 0, 'undefined', 'undefined'),"
      "('WGS 84', 4326, 'EPSG', 4326, 'GEOGCS[\"WGS 84\",DATUM[\"WGS_1984\",SPHEROID[\"WGS 84\",6378137,298.257223563]],PRIMEM[\"Greenwich\",0],UNIT[\"degree\",0.0174532925199433]]', 'WGS 84'),"
      "('WGS 84 / Pseudo-Mercator', 3857, 'EPSG', 3857, 'PROJCS[\"WGS 84 / Pseudo-Mercator\",GEOGCS[\"WGS 84\",DATUM[\"WGS_1984\",SPHEROID[\"WGS "
      "84\",6378137,298.257223563]],PRIMEM[\"Greenwich\",0],UNIT[\"degree\",0.0174532925199433]],PROJECTION[\"Mercator_1SP\"],PARAMETER[\"central_meridian\",0],PARAMETER[\"scale_factor\",1],"
      "PARAMETER[\"false_easting\",0],PARAMETER[\"false_northing\",0],UNIT[\"metre\",1]]', 'EPSG:3857');";
  sqlite3_exec( mDb, srsSql, nullptr, nullptr, nullptr );

  const double mercMax = 20037508.342789244;
  const double mercMin = -mercMax;
  const double worldWidth = 2.0 * mercMax;

  // Use Mercator meters for gpkg_contents extents
  QString contentsSql = QString(
                          "INSERT INTO gpkg_contents (table_name, data_type, identifier, min_x, min_y, max_x, max_y, srs_id) "
                          "VALUES ('tiles', 'tiles', 'tiles', %1, %2, %3, %4, 3857);"
  )
                          .arg( mercExtent.xMinimum(), 0, 'f', 4 )
                          .arg( mercExtent.yMinimum(), 0, 'f', 4 )
                          .arg( mercExtent.xMaximum(), 0, 'f', 4 )
                          .arg( mercExtent.yMaximum(), 0, 'f', 4 );
  sqlite3_exec( mDb, contentsSql.toUtf8().constData(), nullptr, nullptr, nullptr );

  QString tmsSql
    = QString( "INSERT INTO gpkg_tile_matrix_set VALUES ('tiles', 3857, %1, %2, %3, %4);" ).arg( mercMin, 0, 'f', 4 ).arg( mercMin, 0, 'f', 4 ).arg( mercMax, 0, 'f', 4 ).arg( mercMax, 0, 'f', 4 );
  sqlite3_exec( mDb, tmsSql.toUtf8().constData(), nullptr, nullptr, nullptr );

  for ( int z = minZoom; z <= maxZoom; ++z )
  {
    long long matrixDim = 1LL << z;
    double pixelXSize = worldWidth / ( matrixDim * tileWidth );
    double pixelYSize = worldWidth / ( matrixDim * tileHeight );

    QString matrixSql = QString( "INSERT INTO gpkg_tile_matrix VALUES ('tiles', %1, %2, %3, %4, %5, %6, %7);" )
                          .arg( z )
                          .arg( matrixDim )
                          .arg( matrixDim )
                          .arg( tileWidth )
                          .arg( tileHeight )
                          .arg( pixelXSize, 0, 'g', 12 )
                          .arg( pixelYSize, 0, 'g', 12 );

    sqlite3_exec( mDb, matrixSql.toUtf8().constData(), nullptr, nullptr, nullptr );
  }

  const char *insertQuery = "INSERT INTO tiles (zoom_level, tile_column, tile_row, tile_data) VALUES (?, ?, ?, ?);";
  return sqlite3_prepare_v2( mDb, insertQuery, -1, &mInsertStmt, nullptr ) == SQLITE_OK;
}

bool QgsGeoPackageTiles::setTileData( int zoom, int column, int row, const QByteArray &data )
{
  if ( !mInsertStmt )
    return false;

  sqlite3_reset( mInsertStmt );
  sqlite3_bind_int( mInsertStmt, 1, zoom );
  sqlite3_bind_int( mInsertStmt, 2, column );
  sqlite3_bind_int( mInsertStmt, 3, row ); // Standard top-left origin (no TMS inversion)
  sqlite3_bind_blob( mInsertStmt, 4, data.constData(), data.size(), SQLITE_TRANSIENT );

  return sqlite3_step( mInsertStmt ) == SQLITE_DONE;
}

bool QgsGeoPackageTiles::close()
{
  if ( mInsertStmt )
  {
    sqlite3_finalize( mInsertStmt );
    mInsertStmt = nullptr;
  }
  if ( mDb )
  {
    sqlite3_exec( mDb, "COMMIT;", nullptr, nullptr, nullptr );
    sqlite3_close( mDb );
    mDb = nullptr;
  }
  return true;
}


//
// QgsXyzTilesGpkgAlgorithm
//

QString QgsXyzTilesGpkgAlgorithm::name() const
{
  return u"tilesxyzgpkg"_s;
}

QString QgsXyzTilesGpkgAlgorithm::displayName() const
{
  return QObject::tr( "Generate XYZ tiles (GeoPackage)" );
}

QStringList QgsXyzTilesGpkgAlgorithm::tags() const
{
  return QObject::tr( "tiles,xyz,geopackage,gpkg,raster" ).split( ',' );
}

QString QgsXyzTilesGpkgAlgorithm::shortHelpString() const
{
  return QObject::tr( "Generates XYZ raster tiles of map canvas content and saves them into an OGC GeoPackage file." );
}

QgsXyzTilesGpkgAlgorithm *QgsXyzTilesGpkgAlgorithm::createInstance() const
{
  return new QgsXyzTilesGpkgAlgorithm();
}

void QgsXyzTilesGpkgAlgorithm::initAlgorithm( const QVariantMap & )
{
  createCommonParameters();
  addParameter( new QgsProcessingParameterNumber( u"TILE_WIDTH"_s, QObject::tr( "Tile width" ), Qgis::ProcessingNumberParameterType::Integer, 256, false, 1, 4096 ) );
  addParameter( new QgsProcessingParameterNumber( u"TILE_HEIGHT"_s, QObject::tr( "Tile height" ), Qgis::ProcessingNumberParameterType::Integer, 256, false, 1, 4096 ) );

  addParameter( new QgsProcessingParameterFileDestination( u"OUTPUT_FILE"_s, QObject::tr( "Output file" ), QObject::tr( "GeoPackage files (*.gpkg *.GPKG)" ) ) );
}

QVariantMap QgsXyzTilesGpkgAlgorithm::processAlgorithm( const QVariantMap &parameters, QgsProcessingContext &context, QgsProcessingFeedback *feedback )
{
  // TODO -- delete if exists
  const QString outputFile = parameterAsString( parameters, u"OUTPUT_FILE"_s, context );

  QgsRectangle mercatorExtent;
  try
  {
    QgsCoordinateTransform extentTransform = QgsCoordinateTransform( QgsCoordinateReferenceSystem( "EPSG:4326" ), QgsCoordinateReferenceSystem( "EPSG:3857" ), mTransformContext );
    extentTransform.setBallparkTransformsAreAppropriate( true );
    mercatorExtent = extentTransform.transformBoundingBox( mWgs84Extent );
  }
  catch ( QgsCsException & )
  {
    throw QgsProcessingException( QObject::tr( "Could not transform the extent into web mercator" ) );
  }

  mGpkgWriter = std::make_unique<QgsGeoPackageTiles>( outputFile );
  if ( !mGpkgWriter->create( mercatorExtent, mMinZoom, mMaxZoom, mTileWidth, mTileHeight ) )
  {
    throw QgsProcessingException( QObject::tr( "Failed to create GeoPackage file %1" ).arg( outputFile ) );
  }

  long long totalTiles = 0;
  mTotalMetaTiles = 0;
  for ( int z = mMinZoom; z <= mMaxZoom; z++ )
  {
    if ( feedback->isCanceled() )
      break;

    long long tileCount = 0;
    mMetaTiles += getMetatiles( mWgs84Extent, z, tileCount, mMetaTileSize );
    feedback->pushInfo( QObject::tr( "%1 metatiles (%2 tiles) will be created for zoom level %3" ).arg( mMetaTiles.size() - mTotalMetaTiles ).arg( tileCount ).arg( z ) );
    mTotalMetaTiles = mMetaTiles.size();
    totalTiles += tileCount;
  }
  if ( mTotalMetaTiles == 0 )
  {
    throw QgsProcessingException( QObject::tr( "No metatiles will be created -- please check the extent and zoom limits" ) );
  }
  feedback->pushInfo( QObject::tr( "A total of %1 metatiles (%2 tiles) will be created" ).arg( mTotalMetaTiles ).arg( totalTiles ) );

  checkLayersUsagePolicy( feedback );

  for ( QgsMapLayer *layer : std::as_const( mLayers ) )
  {
    layer->moveToThread( QThread::currentThread() );
  }
  mJobOwner.reset( new QObject() );

  QEventLoop loop;
  mEventLoop = &loop;
  startJobs();
  loop.exec();
  mGpkgWriter->close();

  qDeleteAll( mLayers );
  mLayers.clear();

  QVariantMap results;
  results.insert( u"OUTPUT_FILE"_s, outputFile );
  return results;
}

void QgsXyzTilesGpkgAlgorithm::processMetaTile( QgsMapRendererSequentialJob *job )
{
  MetaTile metaTile = mRendererJobs.value( job );
  QImage img = job->renderedImage();

  QMap<QPair<int, int>, Tile>::const_iterator it = metaTile.tiles.constBegin();
  while ( it != metaTile.tiles.constEnd() )
  {
    QPair<int, int> tm = it.key();
    Tile tile = it.value();
    QImage tileImage = img.copy( mTileWidth * tm.first, mTileHeight * tm.second, mTileWidth, mTileHeight );

    QByteArray ba;
    QBuffer buffer( &ba );
    buffer.open( QIODevice::WriteOnly );
    tileImage.save( &buffer, mTileFormat.toStdString().c_str(), mJpgQuality );

    // GeoPackage uses tile.y directly (Top-Left origin)
    mGpkgWriter->setTileData( tile.z, tile.x, tile.y, ba );
    ++it;
  }

  mRendererJobs.remove( job );
  job->deleteLater();

  mFeedback->setProgress( 100.0 * ( mProcessedMetaTiles++ ) / mTotalMetaTiles );

  if ( mFeedback->isCanceled() )
  {
    while ( mRendererJobs.size() > 0 )
    {
      QgsMapRendererSequentialJob *j = mRendererJobs.firstKey();
      j->cancel();
      mRendererJobs.remove( j );
      j->deleteLater();
    }
    mRendererJobs.clear();
    if ( mEventLoop )
    {
      mEventLoop->exit();
    }
    return;
  }

  if ( mMetaTiles.size() > 0 )
  {
    startJobs();
  }
  else if ( mMetaTiles.size() == 0 && mRendererJobs.size() == 0 )
  {
    if ( mEventLoop )
    {
      mEventLoop->exit();
    }
  }
}

///@endcond
