/***************************************************************************
  qgsdemterraintileloader_p.cpp
  --------------------------------------
  Date                 : July 2017
  Copyright            : (C) 2017 by Martin Dobias
  Email                : wonder dot sk at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsdemterraintileloader_p.h"

#include <limits>

#include "qgs3dmapsettings.h"
#include "qgsabstractmaterialsettings.h"
#include "qgsabstractterrainsettings.h"
#include "qgschunknode.h"
#include "qgsdemterraingenerator.h"
#include "qgsdemterraintilegeometry_p.h"
#include "qgseventtracing.h"
#include "qgsgeotransform.h"
#include "qgsonlineterraingenerator.h"
#include "qgsphongmaterialsettings.h"
#include "qgsterrainentity.h"
#include "qgsterraingenerator.h"
#include "qgsterraintexturegenerator_p.h"
#include "qgsterraintileentity_p.h"

#include <QMutexLocker>
#include <QString>
#include <Qt3DCore/QTransform>
#include <Qt3DRender/QCullFace>
#include <Qt3DRender/QGeometryRenderer>
#include <Qt3DRender/QTechnique>

#include "moc_qgsdemterraintileloader_p.cpp"

using namespace Qt::StringLiterals;

///@cond PRIVATE

static void _heightMapMinMax( const QByteArray &heightMap, float &zMin, float &zMax )
{
  const float *zBits = ( const float * ) heightMap.constData();
  int zCount = heightMap.count() / sizeof( float );
  bool first = true;

  zMin = zMax = std::numeric_limits<float>::quiet_NaN();
  for ( int i = 0; i < zCount; ++i )
  {
    float z = zBits[i];
    if ( std::isnan( z ) )
      continue;
    if ( first )
    {
      zMin = zMax = z;
      first = false;
    }
    zMin = std::min( zMin, z );
    zMax = std::max( zMax, z );
  }
}


QgsDemTerrainTileLoader::QgsDemTerrainTileLoader( QgsTerrainEntity *terrain, QgsChunkNode *node, QgsTerrainGenerator *terrainGenerator )
  : QgsTerrainTileLoader( terrain, node )
  , mTerrainGenerator( terrainGenerator )
{}

void QgsDemTerrainTileLoader::start()
{
  QgsChunkNode *node = chunk();

  QgsDemHeightMapGenerator *heightMapGenerator = nullptr;
  if ( mTerrainGenerator->type() == QgsTerrainGenerator::Dem )
  {
    QgsDemTerrainGenerator *generator = static_cast<QgsDemTerrainGenerator *>( mTerrainGenerator );
    heightMapGenerator = generator->heightMapGenerator();
    mSkirtHeight = generator->skirtHeight();
  }
  else if ( mTerrainGenerator->type() == QgsTerrainGenerator::Online )
  {
    QgsOnlineTerrainGenerator *generator = static_cast<QgsOnlineTerrainGenerator *>( mTerrainGenerator );
    heightMapGenerator = generator->heightMapGenerator();
    mSkirtHeight = generator->skirtHeight();
  }
  else
    Q_ASSERT( false );

  // get heightmap asynchronously
  connect( heightMapGenerator, &QgsDemHeightMapGenerator::heightMapReady, this, &QgsDemTerrainTileLoader::onHeightMapReady );
  mHeightMapJobId = heightMapGenerator->render( node->tileId() );
  mResolution = heightMapGenerator->resolution();
}

Qt3DCore::QEntity *QgsDemTerrainTileLoader::createEntity( Qt3DCore::QEntity *parent )
{
  float zMin, zMax;
  _heightMapMinMax( mHeightMap, zMin, zMax );

  if ( std::isnan( zMin ) || std::isnan( zMax ) )
  {
    // no data available for this tile
    return nullptr;
  }

  Qgs3DMapSettings *map = terrain()->mapSettings();
  const QgsChunkNodeId nodeId = mNode->tileId();
  const QgsRectangle extent = map->terrainGenerator()->tilingScheme().tileToExtent( nodeId );
  const double side = extent.width();

  // work out which edges of this tile are internal edges and need skirts
  // to hide cracks between tiles, vs which are on the outer edges of the map
  // and don't need skirts
  const QgsRectangle rootExtent = map->terrainGenerator()->tilingScheme().tileToExtent( 0, 0, 0 );
  const double eps = side * 0.01;
  Qgis::TileEdges skirtEdges;
  skirtEdges.setFlag( Qgis::TileEdge::Left, !qgsDoubleNear( extent.xMinimum(), rootExtent.xMinimum(), eps ) );
  skirtEdges.setFlag( Qgis::TileEdge::Right, !qgsDoubleNear( extent.xMaximum(), rootExtent.xMaximum(), eps ) );
  skirtEdges.setFlag( Qgis::TileEdge::Top, !qgsDoubleNear( extent.yMaximum(), rootExtent.yMaximum(), eps ) );
  skirtEdges.setFlag( Qgis::TileEdge::Bottom, !qgsDoubleNear( extent.yMinimum(), rootExtent.yMinimum(), eps ) );

  QgsTerrainTileEntity *entity = new QgsTerrainTileEntity( nodeId );

  // create geometry renderer

  Qt3DRender::QGeometryRenderer *mesh = new Qt3DRender::QGeometryRenderer;
  mesh->setGeometry( new DemTerrainTileGeometry( mResolution, side, map->terrainSettings()->verticalScale(), mSkirtHeight, mHeightMap, skirtEdges, mesh ) );
  entity->addComponent( mesh ); // takes ownership if the component has no parent

  // create material

  createTextureComponent( entity, map->isTerrainShadingEnabled(), map->terrainShadingMaterial(), !map->layers().empty() );

  // create transform
  QgsGeoTransform *transform = new QgsGeoTransform;
  transform->setGeoTranslation( QgsVector3D( extent.xMinimum(), extent.yMinimum(), 0 ) );
  entity->addComponent( transform );

  double dioramaBaseHeight = std::numeric_limits< double >::max();
  // Create diorama wall geometry for outer edges of the map extent.
  // The outer edges are exactly those where skirts were suppressed.
  if ( map->isDioramaEnabled() && skirtEdges != Qgis::TileEdge::All )
  {
    createDioramaWalls( entity, skirtEdges, side, map->terrainSettings()->verticalScale(), static_cast<float>( map->dioramaHeight() ), *map->dioramaMaterial() );
    dioramaBaseHeight = map->dioramaHeight();
  }

  // clang-format off
  mNode->setExactBox3D(
          QgsBox3D( extent.xMinimum(), extent.yMinimum(), std::min( dioramaBaseHeight, zMin * map->terrainSettings()->verticalScale() ),
                    extent.xMinimum() + side, extent.yMinimum() + side, zMax * map->terrainSettings()->verticalScale() )
  );
  // clang-format on

  mNode->updateParentBoundingBoxesRecursively();

  entity->setParent( parent );
  return entity;
}

void QgsDemTerrainTileLoader::createDioramaWalls( QgsTerrainTileEntity *tileEntity, Qgis::TileEdges skirtEdges, double side, float vertScale, float baseZ, const QgsAbstractMaterialSettings &dioramaMaterial )
{
  // The heightmap is mResolution x mResolution floats.
  // Height at (i,j) = zData[j * mResolution + i], where:
  //   i goes 0..mResolution-1 (left to right, x = i * dx, dx = side/(mResolution-1))
  //   j goes 0..mResolution-1 (top to bottom in local coords, y = side - j * dy, dy = side/(mResolution-1))
  // In map CRS terms (with tile transform at xMin, yMin):
  //   j=0 -> local y=side -> map yMax (NORTH)
  //   j=mResolution-1 -> local y=0 -> map yMin (SOUTH)
  //   i=0 -> local x=0 -> map xMin (WEST)
  //   i=mResolution-1 -> local x=side -> map xMax (EAST)
  //
  // Outer edges (where we need diorama walls) are exactly the edges where
  // skirts were NOT applied. The skirtEdges bitmask tells us which edges
  // HAVE skirts, so outer edges are the complement.

  const float *zData = reinterpret_cast<const float *>( mHeightMap.constData() );
  const int res = mResolution;
  const float dx = static_cast<float>( side ) / static_cast<float>( res - 1 );
  const float dy = static_cast<float>( side ) / static_cast<float>( res - 1 );
  const float fSide = static_cast<float>( side );

  struct Vertex
  {
      float pos[3];
      float normal[3];
  };

  auto buildEdgeWall = [&]( const QVector<QVector3D> &edgePositions, const QVector3D &outwardNormal ) {
    if ( edgePositions.size() < 2 )
      return;

    QVector<Vertex> vertices;
    vertices.reserve( ( edgePositions.size() - 1 ) * 6 );

    for ( int k = 0; k < edgePositions.size() - 1; ++k )
    {
      const QVector3D &p0 = edgePositions[k];
      const QVector3D &p1 = edgePositions[k + 1];
      const QVector3D p2( p1.x(), p1.y(), baseZ );
      const QVector3D p3( p0.x(), p0.y(), baseZ );

      Vertex v;
      v.normal[0] = outwardNormal.x();
      v.normal[1] = outwardNormal.y();
      v.normal[2] = outwardNormal.z();

      // Triangle 1: p0, p1, p2
      v.pos[0] = p0.x();
      v.pos[1] = p0.y();
      v.pos[2] = p0.z();
      vertices.append( v );
      v.pos[0] = p1.x();
      v.pos[1] = p1.y();
      v.pos[2] = p1.z();
      vertices.append( v );
      v.pos[0] = p2.x();
      v.pos[1] = p2.y();
      v.pos[2] = p2.z();
      vertices.append( v );

      // Triangle 2: p0, p2, p3
      v.pos[0] = p0.x();
      v.pos[1] = p0.y();
      v.pos[2] = p0.z();
      vertices.append( v );
      v.pos[0] = p2.x();
      v.pos[1] = p2.y();
      v.pos[2] = p2.z();
      vertices.append( v );
      v.pos[0] = p3.x();
      v.pos[1] = p3.y();
      v.pos[2] = p3.z();
      vertices.append( v );
    }

    if ( vertices.isEmpty() )
      return;

    // Create a child entity parented to the tile entity. It will inherit
    // the tile's QgsGeoTransform, so coordinates are in tile-local space.
    Qt3DCore::QEntity *wallEntity = new Qt3DCore::QEntity( tileEntity );

    const int stride = 6 * sizeof( float );
    QByteArray bufferData;
    bufferData.resize( vertices.size() * stride );
    memcpy( bufferData.data(), vertices.constData(), bufferData.size() );

    Qt3DCore::QBuffer *buffer = new Qt3DCore::QBuffer();
    buffer->setData( bufferData );

    Qt3DCore::QAttribute *posAttr = new Qt3DCore::QAttribute();
    posAttr->setName( Qt3DCore::QAttribute::defaultPositionAttributeName() );
    posAttr->setVertexBaseType( Qt3DCore::QAttribute::Float );
    posAttr->setVertexSize( 3 );
    posAttr->setAttributeType( Qt3DCore::QAttribute::VertexAttribute );
    posAttr->setBuffer( buffer );
    posAttr->setByteStride( stride );
    posAttr->setByteOffset( 0 );
    posAttr->setCount( vertices.size() );

    Qt3DCore::QAttribute *normalAttr = new Qt3DCore::QAttribute();
    normalAttr->setName( Qt3DCore::QAttribute::defaultNormalAttributeName() );
    normalAttr->setVertexBaseType( Qt3DCore::QAttribute::Float );
    normalAttr->setVertexSize( 3 );
    normalAttr->setAttributeType( Qt3DCore::QAttribute::VertexAttribute );
    normalAttr->setBuffer( buffer );
    normalAttr->setByteStride( stride );
    normalAttr->setByteOffset( 3 * sizeof( float ) );
    normalAttr->setCount( vertices.size() );

    Qt3DCore::QGeometry *geometry = new Qt3DCore::QGeometry();
    geometry->addAttribute( posAttr );
    geometry->addAttribute( normalAttr );

    Qt3DRender::QGeometryRenderer *renderer = new Qt3DRender::QGeometryRenderer();
    renderer->setGeometry( geometry );
    renderer->setPrimitiveType( Qt3DRender::QGeometryRenderer::Triangles );
    renderer->setVertexCount( vertices.size() );
    wallEntity->addComponent( renderer );

    QgsMaterialContext materialContext;
    materialContext.setIsSelected( false );
    QgsMaterial *material = dioramaMaterial.toMaterial( QgsMaterialSettingsRenderingTechnique::Triangles, materialContext );

    // Disable backface culling
    const QVector<Qt3DRender::QTechnique *> techniques = material->effect()->techniques();
    for ( Qt3DRender::QTechnique *technique : techniques )
    {
      const QVector<Qt3DRender::QRenderPass *> passes = technique->renderPasses();
      for ( Qt3DRender::QRenderPass *pass : passes )
      {
        Qt3DRender::QCullFace *cullFace = new Qt3DRender::QCullFace;
        cullFace->setMode( Qt3DRender::QCullFace::NoCulling );
        pass->addRenderState( cullFace );
      }
    }

    wallEntity->addComponent( material );
  };

  // South edge (map yMin): j=res-1, local y=0.
  // Outer if skirt was suppressed on Bottom (SkirtEdgeBottom corresponds to j=res, the south skirt row).
  if ( !skirtEdges.testFlag( Qgis::TileEdge::Bottom ) )
  {
    QVector<QVector3D> edge;
    edge.reserve( res );
    const int j = res - 1;
    for ( int i = 0; i < res; ++i )
    {
      float z = zData[j * res + i];
      if ( std::isnan( z ) )
        z = 0;
      edge.append( QVector3D( static_cast<float>( i ) * dx, 0.0f, z * vertScale ) );
    }
    buildEdgeWall( edge, QVector3D( 0, -1, 0 ) );
  }

  // North edge (map yMax): j=0, local y=side.
  // Outer if skirt was suppressed on Top (SkirtEdgeTop corresponds to j=-1, the north skirt row).
  if ( !skirtEdges.testFlag( Qgis::TileEdge::Top ) )
  {
    QVector<QVector3D> edge;
    edge.reserve( res );
    const int j = 0;
    // Collect right-to-left so the strip faces outward (+Y).
    for ( int i = res - 1; i >= 0; --i )
    {
      float z = zData[j * res + i];
      if ( std::isnan( z ) )
        z = 0;
      edge.append( QVector3D( static_cast<float>( i ) * dx, fSide, z * vertScale ) );
    }
    buildEdgeWall( edge, QVector3D( 0, 1, 0 ) );
  }

  // West edge (map xMin): i=0, local x=0.
  // Outer if skirt was suppressed on Left.
  if ( !skirtEdges.testFlag( Qgis::TileEdge::Left ) )
  {
    QVector<QVector3D> edge;
    edge.reserve( res );
    const int i = 0;
    // j goes 0..res-1, local y goes from side to 0.
    // Collect south-to-north (j descending = y ascending) so strip faces outward (-X).
    for ( int j = res - 1; j >= 0; --j )
    {
      float z = zData[j * res + i];
      if ( std::isnan( z ) )
        z = 0;
      edge.append( QVector3D( 0.0f, fSide - static_cast<float>( j ) * dy, z * vertScale ) );
    }
    buildEdgeWall( edge, QVector3D( -1, 0, 0 ) );
  }

  // East edge (map xMax): i=res-1, local x=side.
  // Outer if skirt was suppressed on Right.
  if ( !skirtEdges.testFlag( Qgis::TileEdge::Right ) )
  {
    QVector<QVector3D> edge;
    edge.reserve( res );
    const int i = res - 1;
    // Collect north-to-south (j ascending = y descending) so strip faces outward (+X).
    for ( int j = 0; j < res; ++j )
    {
      float z = zData[j * res + i];
      if ( std::isnan( z ) )
        z = 0;
      edge.append( QVector3D( fSide, fSide - static_cast<float>( j ) * dy, z * vertScale ) );
    }
    buildEdgeWall( edge, QVector3D( 1, 0, 0 ) );
  }
}

void QgsDemTerrainTileLoader::onHeightMapReady( int jobId, const QByteArray &heightMap )
{
  if ( mHeightMapJobId == jobId )
  {
    this->mHeightMap = heightMap;
    mHeightMapJobId = -1;

    // continue loading - texture
    loadTexture();
  }
}


// ---------------------

#include "qgsrasterlayer.h"
#include "qgsrasterprojector.h"
#include <QtConcurrentRun>
#include <QFutureWatcher>
#include <memory>
#include "qgsterraindownloader.h"

QgsDemHeightMapGenerator::QgsDemHeightMapGenerator( QgsRasterLayer *dtm, const QgsTilingScheme &tilingScheme, int resolution, const QgsCoordinateTransformContext &transformContext )
  : mDtmExtent( dtm ? dtm->extent() : QgsRectangle() )
  , mClonedProvider( dtm ? qgis::down_cast<QgsRasterDataProvider *>( dtm->dataProvider()->clone() ) : nullptr )
  , mTilingScheme( tilingScheme )
  , mResolution( resolution )
  , mDownloader( dtm ? nullptr : new QgsTerrainDownloader( transformContext ) )
  , mTransformContext( transformContext )
{}

QgsDemHeightMapGenerator::~QgsDemHeightMapGenerator()
{}


static QByteArray _readDtmData( QgsRasterDataProvider *provider, const QgsRectangle &extent, int res, const QgsCoordinateReferenceSystem &destCrs, const QgsRectangle &clippingExtent )
{
  provider->moveToThread( QThread::currentThread() );

  QgsEventTracing::ScopedEvent e( u"3D"_s, u"DEM"_s );

  // TODO: use feedback object? (but GDAL currently does not support cancellation anyway)
  QgsRasterInterface *input = provider;
  std::unique_ptr<QgsRasterProjector> projector;
  if ( provider->crs() != destCrs )
  {
    projector = std::make_unique<QgsRasterProjector>();
    projector->setCrs( provider->crs(), destCrs, provider->transformContext() );
    projector->setInput( provider );
    input = projector.get();
  }
  std::unique_ptr<QgsRasterBlock> block( input->block( 1, extent, res, res ) );

  QByteArray data;
  if ( block )
  {
    block->convert( Qgis::DataType::Float32 ); // currently we expect just floats

    // set noData outside our clippingExtent
    const QRect subRect = QgsRasterBlock::subRect( extent, block->width(), block->height(), clippingExtent );
    if ( !block->hasNoDataValue() )
    {
      // QgsRasterBlock::setIsNoDataExcept() misbehaves without a defined no data value
      // see https://github.com/qgis/QGIS/issues/51285
      block->setNoDataValue( std::numeric_limits<float>::lowest() );
    }
    block->setIsNoDataExcept( subRect );

    data = block->data();
    data.detach(); // this should make a deep copy

    if ( block->hasNoData() )
    {
      // turn all no-data values into NaN in the output array
      float *floatData = reinterpret_cast<float *>( data.data() );
      Q_ASSERT( data.count() % sizeof( float ) == 0 );
      int count = data.count() / sizeof( float );
      for ( int i = 0; i < count; ++i )
      {
        if ( block->isNoData( i ) )
          floatData[i] = std::numeric_limits<float>::quiet_NaN();
      }
    }
  }

  delete provider;
  return data;
}

static QByteArray _readOnlineDtm( QgsTerrainDownloader *downloader, const QgsRectangle &extent, int res, const QgsCoordinateReferenceSystem &destCrs, const QgsCoordinateTransformContext &context )
{
  return downloader->getHeightMap( extent, res, destCrs, context );
}

int QgsDemHeightMapGenerator::render( const QgsChunkNodeId &nodeId )
{
  QgsEventTracing::addEvent( QgsEventTracing::AsyncBegin, u"3D"_s, u"DEM"_s, nodeId.text() );

  // extend the rect by half-pixel on each side? to get the values in "corners"
  QgsRectangle extent = mTilingScheme.tileToExtent( nodeId );
  float mapUnitsPerPixel = extent.width() / mResolution;
  extent.grow( mapUnitsPerPixel / 2 );
  // but make sure not to go beyond the root tile's full extent (returns invalid values)
  QgsRectangle rootTileExtent = mTilingScheme.tileToExtent( 0, 0, 0 );
  extent = extent.intersect( rootTileExtent );

  JobData jd;
  jd.jobId = ++mLastJobId;
  jd.tileId = nodeId;
  jd.extent = extent;
  jd.timer.start();
  QFutureWatcher<QByteArray> *fw = new QFutureWatcher<QByteArray>( nullptr );
  connect( fw, &QFutureWatcher<QByteArray>::finished, this, &QgsDemHeightMapGenerator::onFutureFinished );
  connect( fw, &QFutureWatcher<QByteArray>::finished, fw, &QObject::deleteLater );
  if ( mClonedProvider )
  {
    // make a clone of the data provider so it is safe to use in worker thread
    std::unique_ptr<QgsRasterDataProvider> clonedProviderClone( mClonedProvider->clone() );
    clonedProviderClone->moveToThread( nullptr );
    jd.future = QtConcurrent::run( _readDtmData, clonedProviderClone.release(), extent, mResolution, mTilingScheme.crs(), mTilingScheme.fullExtent() );
  }
  else
  {
    jd.future = QtConcurrent::run( _readOnlineDtm, mDownloader.get(), extent, mResolution, mTilingScheme.crs(), mTransformContext );
  }

  fw->setFuture( jd.future );

  mJobs.insert( fw, jd );

  return jd.jobId;
}

void QgsDemHeightMapGenerator::waitForFinished()
{
  for ( auto it = mJobs.keyBegin(); it != mJobs.keyEnd(); it++ )
  {
    QFutureWatcher<QByteArray> *fw = *it;
    disconnect( fw, &QFutureWatcher<QByteArray>::finished, this, &QgsDemHeightMapGenerator::onFutureFinished );
    disconnect( fw, &QFutureWatcher<QByteArray>::finished, fw, &QObject::deleteLater );
  }
  QVector<QFutureWatcher<QByteArray> *> toBeDeleted;
  for ( auto it = mJobs.keyBegin(); it != mJobs.keyEnd(); it++ )
  {
    QFutureWatcher<QByteArray> *fw = *it;
    fw->waitForFinished();
    JobData jobData = mJobs.value( fw );
    toBeDeleted.push_back( fw );

    QByteArray data = jobData.future.result();
    emit heightMapReady( jobData.jobId, data );
  }

  for ( QFutureWatcher<QByteArray> *fw : toBeDeleted )
  {
    mJobs.remove( fw );
    fw->deleteLater();
  }
}

void QgsDemHeightMapGenerator::lazyLoadDtmCoarseData( int res, const QgsRectangle &rect )
{
  QMutexLocker locker( &mLazyLoadDtmCoarseDataMutex );
  if ( !mDtmCoarseRasterBlock )
  {
    mDtmCoarseRasterBlock.reset( mClonedProvider->block( 1, rect, res, res ) );
  }
}

float QgsDemHeightMapGenerator::heightAt( double x, double y )
{
  if ( !mClonedProvider )
    return std::numeric_limits<float>::quiet_NaN(); // TODO: calculate heights for online DTM

  // TODO: this is quite a primitive implementation: better to use heightmaps currently in use
  int res = 1024;
  lazyLoadDtmCoarseData( res, mDtmExtent );

  int cellX = ( int ) ( ( x - mDtmExtent.xMinimum() ) / mDtmExtent.width() * res + .5f );
  int cellY = ( int ) ( ( mDtmExtent.yMaximum() - y ) / mDtmExtent.height() * res + .5f );
  cellX = std::clamp( cellX, 0, res - 1 );
  cellY = std::clamp( cellY, 0, res - 1 );

  bool isNoData = false;
  const double val = mDtmCoarseRasterBlock->valueAndNoData( cellY, cellX, isNoData );

  return isNoData ? std::numeric_limits<float>::quiet_NaN() : static_cast<float>( val );
}

void QgsDemHeightMapGenerator::onFutureFinished()
{
  QFutureWatcher<QByteArray> *fw = static_cast<QFutureWatcher<QByteArray> *>( sender() );
  Q_ASSERT( fw );
  Q_ASSERT( mJobs.contains( fw ) );
  JobData jobData = mJobs.value( fw );

  mJobs.remove( fw );
  fw->deleteLater();

  QgsEventTracing::addEvent( QgsEventTracing::AsyncEnd, u"3D"_s, u"DEM"_s, jobData.tileId.text() );

  QByteArray data = jobData.future.result();
  emit heightMapReady( jobData.jobId, data );
}

/// @endcond
