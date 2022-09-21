/***************************************************************************
                             qgsgdalutils.cpp
                             ----------------
    begin                : September 2018
    copyright            : (C) 2018 Even Rouault
    email                : even.rouault at spatialys.com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsgdalutils.h"
#include "qgslogger.h"
#include "qgsnetworkaccessmanager.h"
#include "qgssettings.h"
#include "qgscoordinatereferencesystem.h"
#include "qgsrasterblock.h"

#define CPL_SUPRESS_CPLUSPLUS  //#spellok
#include "gdal.h"
#include "gdalwarper.h"
#include "cpl_string.h"
#include "qgsapplication.h"

#include <QNetworkProxy>
#include <QString>
#include <QImage>
#include <QFileInfo>
#include <mutex>

bool QgsGdalUtils::supportsRasterCreate( GDALDriverH driver )
{
  const QString driverShortName = GDALGetDriverShortName( driver );
  if ( driverShortName == QLatin1String( "SQLite" ) )
  {
    // it supports Create() but only for vector side
    return false;
  }
  char **driverMetadata = GDALGetMetadata( driver, nullptr );
  return  CSLFetchBoolean( driverMetadata, GDAL_DCAP_CREATE, false ) &&
          CSLFetchBoolean( driverMetadata, GDAL_DCAP_RASTER, false );
}

gdal::dataset_unique_ptr QgsGdalUtils::createSingleBandMemoryDataset( GDALDataType dataType, const QgsRectangle &extent, int width, int height, const QgsCoordinateReferenceSystem &crs )
{
  return createMultiBandMemoryDataset( dataType, 1, extent, width, height, crs );
}

gdal::dataset_unique_ptr QgsGdalUtils::createMultiBandMemoryDataset( GDALDataType dataType, int bands, const QgsRectangle &extent, int width, int height, const QgsCoordinateReferenceSystem &crs )
{
  GDALDriverH hDriverMem = GDALGetDriverByName( "MEM" );
  if ( !hDriverMem )
  {
    return gdal::dataset_unique_ptr();
  }

  gdal::dataset_unique_ptr hSrcDS( GDALCreate( hDriverMem, "", width, height, bands, dataType, nullptr ) );

  const double cellSizeX = extent.width() / width;
  const double cellSizeY = extent.height() / height;
  double geoTransform[6];
  geoTransform[0] = extent.xMinimum();
  geoTransform[1] = cellSizeX;
  geoTransform[2] = 0;
  geoTransform[3] = extent.yMinimum() + ( cellSizeY * height );
  geoTransform[4] = 0;
  geoTransform[5] = -cellSizeY;

  GDALSetProjection( hSrcDS.get(), crs.toWkt( QgsCoordinateReferenceSystem::WKT_PREFERRED_GDAL ).toLatin1().constData() );
  GDALSetGeoTransform( hSrcDS.get(), geoTransform );
  return hSrcDS;
}

gdal::dataset_unique_ptr QgsGdalUtils::createSingleBandTiffDataset( const QString &filename, GDALDataType dataType, const QgsRectangle &extent, int width, int height, const QgsCoordinateReferenceSystem &crs )
{
  const double cellSizeX = extent.width() / width;
  const double cellSizeY = extent.height() / height;
  double geoTransform[6];
  geoTransform[0] = extent.xMinimum();
  geoTransform[1] = cellSizeX;
  geoTransform[2] = 0;
  geoTransform[3] = extent.yMinimum() + ( cellSizeY * height );
  geoTransform[4] = 0;
  geoTransform[5] = -cellSizeY;

  GDALDriverH hDriver = GDALGetDriverByName( "GTiff" );
  if ( !hDriver )
  {
    return gdal::dataset_unique_ptr();
  }

  // Create the output file.
  gdal::dataset_unique_ptr hDstDS( GDALCreate( hDriver, filename.toLocal8Bit().constData(), width, height, 1, dataType, nullptr ) );
  if ( !hDstDS )
  {
    return gdal::dataset_unique_ptr();
  }

  // Write out the projection definition.
  GDALSetProjection( hDstDS.get(), crs.toWkt( QgsCoordinateReferenceSystem::WKT_PREFERRED_GDAL ).toLatin1().constData() );
  GDALSetGeoTransform( hDstDS.get(), geoTransform );
  return hDstDS;
}

gdal::dataset_unique_ptr QgsGdalUtils::imageToMemoryDataset( const QImage &image )
{
  if ( image.isNull() )
    return nullptr;

  const QRgb *rgb = reinterpret_cast<const QRgb *>( image.constBits() );
  GDALDriverH hDriverMem = GDALGetDriverByName( "MEM" );
  if ( !hDriverMem )
  {
    return nullptr;
  }
  gdal::dataset_unique_ptr hSrcDS( GDALCreate( hDriverMem, "",  image.width(), image.height(), 0, GDT_Byte, nullptr ) );

  char **papszOptions = QgsGdalUtils::papszFromStringList( QStringList()
                        << QStringLiteral( "PIXELOFFSET=%1" ).arg( sizeof( QRgb ) )
                        << QStringLiteral( "LINEOFFSET=%1" ).arg( image.bytesPerLine() )
                        << QStringLiteral( "DATAPOINTER=%1" ).arg( reinterpret_cast< qulonglong >( rgb ) + 2 ) );
  GDALAddBand( hSrcDS.get(), GDT_Byte, papszOptions );
  CSLDestroy( papszOptions );

  papszOptions = QgsGdalUtils::papszFromStringList( QStringList()
                 << QStringLiteral( "PIXELOFFSET=%1" ).arg( sizeof( QRgb ) )
                 << QStringLiteral( "LINEOFFSET=%1" ).arg( image.bytesPerLine() )
                 << QStringLiteral( "DATAPOINTER=%1" ).arg( reinterpret_cast< qulonglong >( rgb ) + 1 ) );
  GDALAddBand( hSrcDS.get(), GDT_Byte, papszOptions );
  CSLDestroy( papszOptions );

  papszOptions = QgsGdalUtils::papszFromStringList( QStringList()
                 << QStringLiteral( "PIXELOFFSET=%1" ).arg( sizeof( QRgb ) )
                 << QStringLiteral( "LINEOFFSET=%1" ).arg( image.bytesPerLine() )
                 << QStringLiteral( "DATAPOINTER=%1" ).arg( reinterpret_cast< qulonglong >( rgb ) ) );
  GDALAddBand( hSrcDS.get(), GDT_Byte, papszOptions );
  CSLDestroy( papszOptions );

  papszOptions = QgsGdalUtils::papszFromStringList( QStringList()
                 << QStringLiteral( "PIXELOFFSET=%1" ).arg( sizeof( QRgb ) )
                 << QStringLiteral( "LINEOFFSET=%1" ).arg( image.bytesPerLine() )
                 << QStringLiteral( "DATAPOINTER=%1" ).arg( reinterpret_cast< qulonglong >( rgb ) + 3 ) );
  GDALAddBand( hSrcDS.get(), GDT_Byte, papszOptions );
  CSLDestroy( papszOptions );

  return hSrcDS;
}

gdal::dataset_unique_ptr QgsGdalUtils::blockToSingleBandMemoryDataset( int pixelWidth, int pixelHeight, const QgsRectangle &extent, void *block,  GDALDataType dataType )
{
  if ( !block )
    return nullptr;

  GDALDriverH hDriverMem = GDALGetDriverByName( "MEM" );
  if ( !hDriverMem )
    return nullptr;

  const double cellSizeX = extent.width() / pixelWidth;
  const double cellSizeY = extent.height() / pixelHeight;
  double geoTransform[6];
  geoTransform[0] = extent.xMinimum();
  geoTransform[1] = cellSizeX;
  geoTransform[2] = 0;
  geoTransform[3] = extent.yMinimum() + ( cellSizeY * pixelHeight );
  geoTransform[4] = 0;
  geoTransform[5] = -cellSizeY;

  gdal::dataset_unique_ptr hDstDS( GDALCreate( hDriverMem, "", pixelWidth, pixelHeight, 0, dataType, nullptr ) );

  int dataTypeSize = GDALGetDataTypeSizeBytes( dataType );
  char **papszOptions = QgsGdalUtils::papszFromStringList( QStringList()
                        << QStringLiteral( "PIXELOFFSET=%1" ).arg( dataTypeSize )
                        << QStringLiteral( "LINEOFFSET=%1" ).arg( pixelWidth * dataTypeSize )
                        << QStringLiteral( "DATAPOINTER=%1" ).arg( reinterpret_cast< qulonglong >( block ) ) );
  GDALAddBand( hDstDS.get(), dataType, papszOptions );
  CSLDestroy( papszOptions );

  GDALSetGeoTransform( hDstDS.get(), geoTransform );

  return hDstDS;
}

bool QgsGdalUtils::resampleSingleBandRaster( GDALDatasetH hSrcDS, GDALDatasetH hDstDS, GDALResampleAlg resampleAlg, const char *pszCoordinateOperation )
{
  gdal::warp_options_unique_ptr psWarpOptions( GDALCreateWarpOptions() );
  psWarpOptions->hSrcDS = hSrcDS;
  psWarpOptions->hDstDS = hDstDS;

  psWarpOptions->nBandCount = 1;
  psWarpOptions->panSrcBands = reinterpret_cast< int * >( CPLMalloc( sizeof( int ) * 1 ) );
  psWarpOptions->panDstBands = reinterpret_cast< int * >( CPLMalloc( sizeof( int ) * 1 ) );
  psWarpOptions->panSrcBands[0] = 1;
  psWarpOptions->panDstBands[0] = 1;

  psWarpOptions->eResampleAlg = resampleAlg;

  // Establish reprojection transformer.
  char **papszOptions = nullptr;
  if ( pszCoordinateOperation != nullptr )
    papszOptions = CSLSetNameValue( papszOptions, "COORDINATE_OPERATION", pszCoordinateOperation );
  psWarpOptions->pTransformerArg = GDALCreateGenImgProjTransformer2( hSrcDS, hDstDS, papszOptions );
  CSLDestroy( papszOptions );

  if ( ! psWarpOptions->pTransformerArg )
  {
    return false;
  }

  psWarpOptions->pfnTransformer = GDALGenImgProjTransform;

  // Initialize and execute the warp operation.
  GDALWarpOperation oOperation;
  oOperation.Initialize( psWarpOptions.get() );

  const bool retVal { oOperation.ChunkAndWarpImage( 0, 0, GDALGetRasterXSize( hDstDS ), GDALGetRasterYSize( hDstDS ) ) == CE_None };
  GDALDestroyGenImgProjTransformer( psWarpOptions->pTransformerArg );
  return retVal;
}

QImage QgsGdalUtils::resampleImage( const QImage &image, QSize outputSize, GDALRIOResampleAlg resampleAlg )
{
  const gdal::dataset_unique_ptr srcDS = QgsGdalUtils::imageToMemoryDataset( image );
  if ( !srcDS )
    return QImage();

  GDALRasterIOExtraArg extra;
  INIT_RASTERIO_EXTRA_ARG( extra );
  extra.eResampleAlg = resampleAlg;

  QImage res( outputSize, image.format() );
  if ( res.isNull() )
    return QImage();

  GByte *rgb = reinterpret_cast<GByte *>( res.bits() );

  CPLErr err = GDALRasterIOEx( GDALGetRasterBand( srcDS.get(), 1 ), GF_Read, 0, 0, image.width(), image.height(), rgb + 2, outputSize.width(),
                               outputSize.height(), GDT_Byte, sizeof( QRgb ), res.bytesPerLine(), &extra );
  if ( err != CE_None )
  {
    QgsDebugMsg( QStringLiteral( "failed to read red band" ) );
    return QImage();
  }

  err = GDALRasterIOEx( GDALGetRasterBand( srcDS.get(), 2 ), GF_Read, 0, 0, image.width(), image.height(), rgb + 1, outputSize.width(),
                        outputSize.height(), GDT_Byte, sizeof( QRgb ), res.bytesPerLine(), &extra );
  if ( err != CE_None )
  {
    QgsDebugMsg( QStringLiteral( "failed to read green band" ) );
    return QImage();
  }

  err = GDALRasterIOEx( GDALGetRasterBand( srcDS.get(), 3 ), GF_Read, 0, 0, image.width(), image.height(), rgb, outputSize.width(),
                        outputSize.height(), GDT_Byte, sizeof( QRgb ), res.bytesPerLine(), &extra );
  if ( err != CE_None )
  {
    QgsDebugMsg( QStringLiteral( "failed to read blue band" ) );
    return QImage();
  }

  err = GDALRasterIOEx( GDALGetRasterBand( srcDS.get(), 4 ), GF_Read, 0, 0, image.width(), image.height(), rgb + 3, outputSize.width(),
                        outputSize.height(), GDT_Byte, sizeof( QRgb ), res.bytesPerLine(), &extra );
  if ( err != CE_None )
  {
    QgsDebugMsg( QStringLiteral( "failed to read alpha band" ) );
    return QImage();
  }

  return res;
}

QString QgsGdalUtils::helpCreationOptionsFormat( const QString &format )
{
  QString message;
  GDALDriverH myGdalDriver = GDALGetDriverByName( format.toLocal8Bit().constData() );
  if ( myGdalDriver )
  {
    // first report details and help page
    char **GDALmetadata = GDALGetMetadata( myGdalDriver, nullptr );
    message += QLatin1String( "Format Details:\n" );
    message += QStringLiteral( "  Extension: %1\n" ).arg( CSLFetchNameValue( GDALmetadata, GDAL_DMD_EXTENSION ) );
    message += QStringLiteral( "  Short Name: %1" ).arg( GDALGetDriverShortName( myGdalDriver ) );
    message += QStringLiteral( "  /  Long Name: %1\n" ).arg( GDALGetDriverLongName( myGdalDriver ) );
    message += QStringLiteral( "  Help page:  http://www.gdal.org/%1\n\n" ).arg( CSLFetchNameValue( GDALmetadata, GDAL_DMD_HELPTOPIC ) );

    // next get creation options
    // need to serialize xml to get newlines, should we make the basic xml prettier?
    CPLXMLNode *psCOL = CPLParseXMLString( GDALGetMetadataItem( myGdalDriver,
                                           GDAL_DMD_CREATIONOPTIONLIST, "" ) );
    char *pszFormattedXML = CPLSerializeXMLTree( psCOL );
    if ( pszFormattedXML )
      message += QString( pszFormattedXML );
    if ( psCOL )
      CPLDestroyXMLNode( psCOL );
    if ( pszFormattedXML )
      CPLFree( pszFormattedXML );
  }
  return message;
}

char **QgsGdalUtils::papszFromStringList( const QStringList &list )
{
  char **papszRetList = nullptr;
  const auto constList = list;
  for ( const QString &elem : constList )
  {
    papszRetList = CSLAddString( papszRetList, elem.toLocal8Bit().constData() );
  }
  return papszRetList;
}

QString QgsGdalUtils::validateCreationOptionsFormat( const QStringList &createOptions, const QString &format )
{
  GDALDriverH myGdalDriver = GDALGetDriverByName( format.toLocal8Bit().constData() );
  if ( ! myGdalDriver )
    return QStringLiteral( "invalid GDAL driver" );

  char **papszOptions = papszFromStringList( createOptions );
  // get error string?
  const int ok = GDALValidateCreationOptions( myGdalDriver, papszOptions );
  CSLDestroy( papszOptions );

  if ( !ok )
    return QStringLiteral( "Failed GDALValidateCreationOptions() test" );
  return QString();
}

GDALDatasetH QgsGdalUtils::rpcAwareAutoCreateWarpedVrt(
  GDALDatasetH hSrcDS,
  const char *pszSrcWKT,
  const char *pszDstWKT,
  GDALResampleAlg eResampleAlg,
  double dfMaxError,
  const GDALWarpOptions *psOptionsIn )
{
  char **opts = nullptr;
  if ( GDALGetMetadata( hSrcDS, "RPC" ) )
  {
    // well-behaved RPC should have height offset a good value for RPC_HEIGHT
    const char *heightOffStr = GDALGetMetadataItem( hSrcDS, "HEIGHT_OFF", "RPC" );
    if ( heightOffStr )
      opts = CSLAddNameValue( opts, "RPC_HEIGHT", heightOffStr );
  }

  return GDALAutoCreateWarpedVRTEx( hSrcDS, pszSrcWKT, pszDstWKT, eResampleAlg, dfMaxError, psOptionsIn, opts );
}

void *QgsGdalUtils::rpcAwareCreateTransformer( GDALDatasetH hSrcDS, GDALDatasetH hDstDS, char **papszOptions )
{
  char **opts = CSLDuplicate( papszOptions );
  if ( GDALGetMetadata( hSrcDS, "RPC" ) )
  {
    // well-behaved RPC should have height offset a good value for RPC_HEIGHT
    const char *heightOffStr = GDALGetMetadataItem( hSrcDS, "HEIGHT_OFF", "RPC" );
    if ( heightOffStr )
      opts = CSLAddNameValue( opts, "RPC_HEIGHT", heightOffStr );
  }
  void *transformer = GDALCreateGenImgProjTransformer2( hSrcDS, hDstDS, opts );
  CSLDestroy( opts );
  return transformer;
}

#ifndef QT_NO_NETWORKPROXY
void QgsGdalUtils::setupProxy()
{
  // Check proxy configuration, they are application level but
  // instead of adding an API and complex signal/slot connections
  // given the limited cost of checking them on every provider instantiation
  // we can do it here so that new settings are applied whenever a new layer
  // is created.
  const QgsSettings settings;
  // Check that proxy is enabled
  if ( settings.value( QStringLiteral( "proxy/proxyEnabled" ), false ).toBool() )
  {
    // Get the first configured proxy
    QList<QNetworkProxy> proxies( QgsNetworkAccessManager::instance()->proxyFactory()->queryProxy( ) );
    if ( ! proxies.isEmpty() )
    {
      const QNetworkProxy proxy( proxies.first() );
      // TODO/FIXME: check excludes (the GDAL config options are global, we need a per-connection config option)
      //QStringList excludes;
      //excludes = settings.value( QStringLiteral( "proxy/proxyExcludedUrls" ), "" ).toStringList();

      const QString proxyHost( proxy.hostName() );
      const quint16 proxyPort( proxy.port() );

      const QString proxyUser( proxy.user() );
      const QString proxyPassword( proxy.password() );

      if ( ! proxyHost.isEmpty() )
      {
        QString connection( proxyHost );
        if ( proxyPort )
        {
          connection += ':' +  QString::number( proxyPort );
        }
        CPLSetConfigOption( "GDAL_HTTP_PROXY", connection.toUtf8() );
        if ( !  proxyUser.isEmpty( ) )
        {
          QString credentials( proxyUser );
          if ( !  proxyPassword.isEmpty( ) )
          {
            credentials += ':' + proxyPassword;
          }
          CPLSetConfigOption( "GDAL_HTTP_PROXYUSERPWD", credentials.toUtf8() );
        }
      }
    }
  }
}

bool QgsGdalUtils::pathIsCheapToOpen( const QString &path, int smallFileSizeLimit )
{
  const QFileInfo info( path );
  const long long size = info.size();

  // if size could not be determined, safest to flag path as expensive
  if ( size == 0 )
    return false;

  const QString suffix = info.suffix().toLower();
  static const QStringList sFileSizeDependentExtensions
  {
    QStringLiteral( "xlsx" ),
    QStringLiteral( "ods" ),
    QStringLiteral( "csv" )
  };
  if ( sFileSizeDependentExtensions.contains( suffix ) )
  {
    // path corresponds to a file type which is only cheap to open for small files
    return size < smallFileSizeLimit;
  }

  // treat all other formats as expensive.
  // TODO -- flag formats which only require a quick header parse as cheap
  return false;
}

QStringList QgsGdalUtils::multiLayerFileExtensions()
{
#if GDAL_VERSION_NUM >= GDAL_COMPUTE_VERSION(3,4,0)
  // get supported extensions
  static std::once_flag initialized;
  static QStringList SUPPORTED_DB_LAYERS_EXTENSIONS;
  std::call_once( initialized, [ = ]
  {
    // iterate through all of the supported drivers, adding the corresponding file extensions for
    // types which advertise multilayer support
    GDALDriverH driver = nullptr;

    QSet< QString > extensions;

    for ( int i = 0; i < GDALGetDriverCount(); ++i )
    {
      driver = GDALGetDriver( i );
      if ( !driver )
      {
        QgsLogger::warning( "unable to get driver " + QString::number( i ) );
        continue;
      }

      bool isMultiLayer = false;
      if ( QString( GDALGetMetadataItem( driver, GDAL_DCAP_RASTER, nullptr ) ) == QLatin1String( "YES" ) )
      {
        if ( GDALGetMetadataItem( driver, GDAL_DMD_SUBDATASETS, nullptr ) != nullptr )
        {
          isMultiLayer = true;
        }
      }
      if ( !isMultiLayer && QString( GDALGetMetadataItem( driver, GDAL_DCAP_VECTOR, nullptr ) ) == QLatin1String( "YES" ) )
      {
        if ( GDALGetMetadataItem( driver, GDAL_DCAP_MULTIPLE_VECTOR_LAYERS, nullptr ) != nullptr )
        {
          isMultiLayer = true;
        }
      }

      if ( !isMultiLayer )
        continue;

      const QString driverExtensions = GDALGetMetadataItem( driver, GDAL_DMD_EXTENSIONS, "" );
      if ( driverExtensions.isEmpty() )
        continue;

      const QStringList splitExtensions = driverExtensions.split( ' ', Qt::SkipEmptyParts );

      for ( const QString &ext : splitExtensions )
        extensions.insert( ext );
    }

    SUPPORTED_DB_LAYERS_EXTENSIONS = QStringList( extensions.constBegin(), extensions.constEnd() );
  } );
  return SUPPORTED_DB_LAYERS_EXTENSIONS;

#else
  static const QStringList SUPPORTED_DB_LAYERS_EXTENSIONS
  {
    QStringLiteral( "gpkg" ),
    QStringLiteral( "sqlite" ),
    QStringLiteral( "db" ),
    QStringLiteral( "gdb" ),
    QStringLiteral( "kml" ),
    QStringLiteral( "kmz" ),
    QStringLiteral( "osm" ),
    QStringLiteral( "mdb" ),
    QStringLiteral( "accdb" ),
    QStringLiteral( "xls" ),
    QStringLiteral( "xlsx" ),
    QStringLiteral( "ods" ),
    QStringLiteral( "gpx" ),
    QStringLiteral( "pdf" ),
    QStringLiteral( "pbf" ),
    QStringLiteral( "vrt" ),
    QStringLiteral( "nc" ),
    QStringLiteral( "shp.zip" ) };
  return SUPPORTED_DB_LAYERS_EXTENSIONS;
#endif
}

bool QgsGdalUtils::vrtMatchesLayerType( const QString &vrtPath, QgsMapLayerType type )
{
  CPLPushErrorHandler( CPLQuietErrorHandler );
  CPLErrorReset();
  GDALDriverH hDriver = nullptr;

  switch ( type )
  {
    case QgsMapLayerType::VectorLayer:
      hDriver = GDALIdentifyDriverEx( vrtPath.toUtf8().constData(), GDAL_OF_VECTOR, nullptr, nullptr );
      break;

    case QgsMapLayerType::RasterLayer:
      hDriver = GDALIdentifyDriverEx( vrtPath.toUtf8().constData(), GDAL_OF_RASTER, nullptr, nullptr );
      break;

    case QgsMapLayerType::PluginLayer:
    case QgsMapLayerType::MeshLayer:
    case QgsMapLayerType::VectorTileLayer:
    case QgsMapLayerType::AnnotationLayer:
    case QgsMapLayerType::PointCloudLayer:
    case QgsMapLayerType::GroupLayer:
      break;
  }

  CPLPopErrorHandler();
  return static_cast< bool >( hDriver );
}

bool QgsGdalUtils::parseNetCdfBandName( const QString &name, QDateTime &referenceTime, QgsUnitTypes::TemporalUnit &unit )
{
  const QStringList parts = name.split( ' ' );
  if ( parts.size() < 3 )
    return false;

  referenceTime = QDateTime::fromString( parts.at( 2 ) );
  if ( !referenceTime.isValid() )
    return false;

  if ( parts.at( 1 ).compare( QLatin1String( "since" ), Qt::CaseInsensitive ) != 0 )
    return false;

  const QString timeUnit = parts.at( 0 );
  if ( timeUnit.compare( QLatin1String( "millisec" ), Qt::CaseInsensitive ) == 0 ||
       timeUnit.compare( QLatin1String( "msec" ), Qt::CaseInsensitive ) == 0 ||
       timeUnit.compare( QLatin1String( "millisecs" ), Qt::CaseInsensitive ) == 0 ||
       timeUnit.compare( QLatin1String( "msecs" ), Qt::CaseInsensitive ) == 0 )
  {
    unit = QgsUnitTypes::TemporalMilliseconds;
    return true;
  }
  else if ( timeUnit.compare( QLatin1String( "second" ), Qt::CaseInsensitive ) == 0 ||
            timeUnit.compare( QLatin1String( "seconds" ), Qt::CaseInsensitive ) == 0 ||
            timeUnit.compare( QLatin1String( "sec" ), Qt::CaseInsensitive ) == 0 ||
            timeUnit.compare( QLatin1String( "secs" ), Qt::CaseInsensitive ) == 0 ||
            timeUnit.compare( QLatin1String( "s" ), Qt::CaseInsensitive ) == 0 ||
            timeUnit.compare( QLatin1String( "se" ), Qt::CaseInsensitive ) == 0 )
  {
    unit = QgsUnitTypes::TemporalSeconds;
    return true;
  }
  else if ( timeUnit.compare( QLatin1String( "minute" ), Qt::CaseInsensitive ) == 0 ||
            timeUnit.compare( QLatin1String( "minutes" ), Qt::CaseInsensitive ) == 0 ||
            timeUnit.compare( QLatin1String( "min" ), Qt::CaseInsensitive ) == 0 ||
            timeUnit.compare( QLatin1String( "mins" ), Qt::CaseInsensitive ) == 0 ||
            timeUnit.compare( QLatin1String( "mi" ), Qt::CaseInsensitive ) == 0 )
  {
    unit = QgsUnitTypes::TemporalMinutes;
    return true;
  }
  else if ( timeUnit.compare( QLatin1String( "hour" ), Qt::CaseInsensitive ) == 0 ||
            timeUnit.compare( QLatin1String( "hours" ), Qt::CaseInsensitive ) == 0 )
  {
    unit = QgsUnitTypes::TemporalHours;
    return true;
  }
  else if ( timeUnit.compare( QLatin1String( "day" ), Qt::CaseInsensitive ) == 0 ||
            timeUnit.compare( QLatin1String( "days" ), Qt::CaseInsensitive ) == 0 )
  {
    unit = QgsUnitTypes::TemporalDays;
    return true;
  }
  else if ( timeUnit.compare( QLatin1String( "week" ), Qt::CaseInsensitive ) == 0 ||
            timeUnit.compare( QLatin1String( "weeks" ), Qt::CaseInsensitive ) == 0 )
  {
    unit = QgsUnitTypes::TemporalWeeks;
    return true;
  }
  else if ( timeUnit.compare( QLatin1String( "month" ), Qt::CaseInsensitive ) == 0 ||
            timeUnit.compare( QLatin1String( "months" ), Qt::CaseInsensitive ) == 0 ||
            timeUnit.compare( QLatin1String( "mon" ), Qt::CaseInsensitive ) == 0 ||
            timeUnit.compare( QLatin1String( "mons" ), Qt::CaseInsensitive ) == 0 )
  {
    unit = QgsUnitTypes::TemporalMonths;
    return true;
  }
  else if ( timeUnit.compare( QLatin1String( "year" ), Qt::CaseInsensitive ) == 0 ||
            timeUnit.compare( QLatin1String( "years" ), Qt::CaseInsensitive ) == 0 ||
            timeUnit.compare( QLatin1String( "yr" ), Qt::CaseInsensitive ) == 0 ||
            timeUnit.compare( QLatin1String( "yrs" ), Qt::CaseInsensitive ) == 0 )
  {
    unit = QgsUnitTypes::TemporalYears;
    return true;
  }

  return false;
}

#endif
