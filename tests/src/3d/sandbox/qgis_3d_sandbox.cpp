/***************************************************************************
                         qgis_3d_sandbox.cpp
                         --------------------
    begin                : October 2020
    copyright            : (C) 2020 by Martin Dobias
    email                : wonder dot sk at gmail dot com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QApplication>

#include "qgsapplication.h"
#include "qgs3d.h"
#include "qgslayertree.h"
#include "qgsmapsettings.h"
#include "qgspointcloudlayer.h"
#include "qgspointcloudlayer3drenderer.h"
#include "qgsproject.h"
#include "qgsflatterraingenerator.h"
#include "qgs3dmapscene.h"
#include "qgs3dmapsettings.h"
#include "qgs3dmapcanvas.h"
#include "qgsprojectelevationproperties.h"
#include "qgsprojectviewsettings.h"
#include "qgsterrainprovider.h"
#include "qgstiledscenelayer.h"
#include "qgstiledscenelayer3drenderer.h"
#include "qgschunkboundsentity_p.h"
#include "qgsphongmaterialsettings.h"
#include "qgsdirectionallightsettings.h"
#include "qgssimplelinematerialsettings.h"

#include <QScreen>

#include <Qt3DExtras/QGoochMaterial>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <Qt3DRender/QAttribute>
#include <Qt3DRender/QGeometry>
#include <Qt3DRender/QBuffer>
#include <Qt3DRender/QPolygonOffset>
typedef Qt3DRender::QAttribute Qt3DQAttribute;
typedef Qt3DRender::QGeometry Qt3DQGeometry;
typedef Qt3DRender::QBuffer Qt3DQBuffer;
#else
#include <Qt3DCore/QAttribute>
#include <Qt3DCore/QGeometry>
#include <Qt3DCore/QBuffer>
typedef Qt3DCore::QAttribute Qt3DQAttribute;
typedef Qt3DCore::QGeometry Qt3DQGeometry;
typedef Qt3DCore::QBuffer Qt3DQBuffer;
#endif

class GlobeMesh : public Qt3DRender::QGeometryRenderer
{

  public:

    enum class Algorithm
    {
      SubdivisionSurface,
      CubeMapTessellation
    };


    GlobeMesh( Algorithm algorithm, double radiusA, double radiusB, double radiusC, bool useLines, Qt3DCore::QNode *parent = nullptr )
      : Qt3DRender::QGeometryRenderer( parent )
      , mAlgorithm( algorithm )
      , mUseLines( useLines )
      , mRadius( QVector3D( radiusA * ( useLines ? 1.001 : 1 ),
                            radiusB * ( useLines ? 1.001 : 1 ),
                            radiusC * ( useLines ? 1.001 : 1 ) ) )
      , mPositionAttribute( new Qt3DQAttribute( this ) )
      , mNormalAttribute( new Qt3DQAttribute( this ) )
      , mIndexAttribute( new Qt3DQAttribute( this ) )
      , mVertexBuffer( new Qt3DQBuffer( this ) )
      , mIndexBuffer( new Qt3DQBuffer( this ) )
    {
      mOneOverRadiiSquared = QVector3D(
                               1.0 / ( mRadius.x() * mRadius.x() ),
                               1.0 / ( mRadius.y() * mRadius.y() ),
                               1.0 / ( mRadius.z() * mRadius.z() )
                             );


      mPositionAttribute->setAttributeType( Qt3DQAttribute::VertexAttribute );
      mPositionAttribute->setBuffer( mVertexBuffer );
      mPositionAttribute->setVertexBaseType( Qt3DQAttribute::Float );
      mPositionAttribute->setVertexSize( 3 );
      mPositionAttribute->setByteStride( 3 * sizeof( float ) * 2 );
      mPositionAttribute->setByteOffset( 0 );
      mPositionAttribute->setName( Qt3DQAttribute::defaultPositionAttributeName() );

      mNormalAttribute->setAttributeType( Qt3DQAttribute::VertexAttribute );
      mNormalAttribute->setBuffer( mVertexBuffer );
      mNormalAttribute->setVertexBaseType( Qt3DQAttribute::Float );
      mNormalAttribute->setVertexSize( 3 );
      mNormalAttribute->setByteStride( 3 * sizeof( float ) * 2 );
      mNormalAttribute->setByteOffset( 3 * sizeof( float ) );
      mNormalAttribute->setName( Qt3DQAttribute::defaultNormalAttributeName() );

      mIndexAttribute->setAttributeType( Qt3DQAttribute::IndexAttribute );
      mIndexAttribute->setVertexBaseType( Qt3DQAttribute::UnsignedInt );
      mIndexAttribute->setByteStride( sizeof( uint ) );
      mIndexAttribute->setByteOffset( 0 );
      mIndexAttribute->setBuffer( mIndexBuffer );

      mGeom = new Qt3DQGeometry( this );
      mGeom->addAttribute( mPositionAttribute );
      mGeom->addAttribute( mIndexAttribute );
      mGeom->addAttribute( mNormalAttribute );

      setInstanceCount( 1 );
      setIndexOffset( 0 );
      setFirstInstance( 0 );
      setPrimitiveType( mUseLines ? Qt3DRender::QGeometryRenderer::Lines
                        : Qt3DRender::QGeometryRenderer::Triangles );
      setGeometry( mGeom );


      QList<QVector3D> positions;
      QList<int> indices;

      switch ( mAlgorithm )
      {
        case Algorithm::SubdivisionSurface:
        {

          constexpr int numberOfSubDivisions = 5;
          constexpr double rootTwoOverThree = M_SQRT2 / 3.0;
          constexpr double oneThird = 1.0 / 3.0;
          constexpr double rootSixOverThree = 2.449489742783178 / 3.0;

          positions.append( QVector3D( 0, 0, 1 ) );
          positions.append( QVector3D( 0, 2 * rootTwoOverThree, -oneThird ) );
          positions.append( QVector3D( -rootSixOverThree, -rootTwoOverThree, -oneThird ) );
          positions.append( QVector3D( rootSixOverThree, -rootTwoOverThree, -oneThird ) );

          subdivide( positions, indices, {0, 1, 2}, numberOfSubDivisions );
          subdivide( positions, indices, {0, 2, 3}, numberOfSubDivisions );
          subdivide( positions, indices, {0, 3, 1}, numberOfSubDivisions );
          subdivide( positions, indices, {1, 3, 2}, numberOfSubDivisions );

          break;
        }

        case Algorithm::CubeMapTessellation:
        {
          positions.append( QVector3D( -1, 0, -1 ) );
          positions.append( QVector3D( 0, -1, -1 ) );
          positions.append( QVector3D( 1, 0, -1 ) );
          positions.append( QVector3D( 0, 1, -1 ) );
          positions.append( QVector3D( -1, 0, 1 ) );
          positions.append( QVector3D( 0, -1, 1 ) );
          positions.append( QVector3D( 1, 0, 1 ) );
          positions.append( QVector3D( 0, 1, 1 ) );


          constexpr int numberOfPartitions = 8; //CubeMapMesh.NumberOfPartitions;


          auto addEdgePositions = [&positions]( int i0, int i1 )
          {
            QVector< int > indices;
            indices.resize( 2 + ( numberOfPartitions - 1 ) );

            indices[0] = i0;
            indices[indices.size() - 1] = i1;

            QVector3D origin = positions[i0];
            QVector3D direction = positions[i1] - positions[i0];

            for ( int i = 1; i < numberOfPartitions; ++i )
            {
              double delta = i / ( double )numberOfPartitions;

              indices[i] = positions.size();
              positions.append( origin + ( delta * direction ) );
            }

            return indices;
          };

          auto addFaceTriangles = [this, &positions, &indices](
                                    const QVector< int > &leftBottomToTop,
                                    const QVector< int > &bottomLeftToRight,
                                    const QVector< int > &rightBottomToTop,
                                    const QVector< int > &topLeftToRight )
          {
            QVector3D origin = positions[bottomLeftToRight[0]];
            QVector3D x = positions[bottomLeftToRight[bottomLeftToRight.size() - 1]] - origin;
            QVector3D y = positions[topLeftToRight[0]] - origin;

            QVector<int> bottomIndicesBuffer;
            bottomIndicesBuffer.resize( numberOfPartitions + 1 );
            QVector<int> topIndicesBuffer;
            topIndicesBuffer.resize( numberOfPartitions + 1 );

            const QVector<int> *bottomIndices = &bottomLeftToRight;
            const QVector<int> *topIndices = &topIndicesBuffer;

            for ( int j = 1; j <= numberOfPartitions; ++j )
            {
              if ( j != numberOfPartitions )
              {
                if ( j != 1 )
                {
                  //
                  // This copy could be avoided by ping ponging buffers.
                  //
                  bottomIndicesBuffer = topIndicesBuffer; //.CopyTo(bottomIndicesBuffer, 0);
                  bottomIndices = &bottomIndicesBuffer;
                }

                topIndicesBuffer[0] = leftBottomToTop[j];
                topIndicesBuffer[numberOfPartitions] = rightBottomToTop[j];

                double deltaY = j / ( double )numberOfPartitions;
                QVector3D offsetY = deltaY * y;

                for ( int i = 1; i < numberOfPartitions; ++i )
                {
                  double deltaX = i / ( double )numberOfPartitions;
                  QVector3D offsetX = deltaX * x;

                  topIndicesBuffer[i] = positions.size();
                  positions.append( origin + offsetX + offsetY );
                }
              }
              else
              {
                if ( j != 1 )
                {
                  bottomIndices = &topIndicesBuffer;
                }
                topIndices = &topLeftToRight;
              }

              for ( int i = 0; i < numberOfPartitions; ++i )
              {
                if ( !mUseLines )
                {
                  indices.append(
                  {( *bottomIndices )[i], ( *bottomIndices )[i + 1], ( *topIndices )[i + 1]} );
                  indices.append(
                  {
                    ( *bottomIndices )[i], ( *topIndices )[i + 1], ( *topIndices )[i]} );
                }
                else
                {
                  indices.append(
                  {
                    ( *bottomIndices )[i], ( *bottomIndices )[i + 1],
                    ( *bottomIndices )[i + 1], ( *topIndices )[i + 1],
                    ( *topIndices )[i + 1], ( *bottomIndices )[i]} );
                  indices.append(
                  {
                    ( *bottomIndices )[i], ( *topIndices )[i + 1],
                    ( *topIndices )[i + 1], ( *topIndices )[i],
                    ( *topIndices )[i], ( *bottomIndices )[i]} );
                }
              }
            }
          };

          QVector< int > edge0to1 = addEdgePositions( 0, 1 );
          QVector< int > edge1to2 = addEdgePositions( 1, 2 );
          QVector< int > edge2to3 = addEdgePositions( 2, 3 );
          QVector< int > edge3to0 = addEdgePositions( 3, 0 );

          QVector< int > edge4to5 = addEdgePositions( 4, 5 );
          QVector< int > edge5to6 = addEdgePositions( 5, 6 );
          QVector< int > edge6to7 = addEdgePositions( 6, 7 );
          QVector< int > edge7to4 = addEdgePositions( 7, 4 );

          QVector< int > edge0to4 = addEdgePositions( 0, 4 );
          QVector< int > edge1to5 = addEdgePositions( 1, 5 );
          QVector< int > edge2to6 = addEdgePositions( 2, 6 );
          QVector< int > edge3to7 = addEdgePositions( 3, 7 );

          addFaceTriangles( edge0to4, edge0to1, edge1to5, edge4to5 ); // Q3 Face
          addFaceTriangles( edge1to5, edge1to2, edge2to6, edge5to6 ); // Q4 Face
          addFaceTriangles( edge2to6, edge2to3, edge3to7, edge6to7 ); // Q1 Face
          addFaceTriangles( edge3to7, edge3to0, edge0to4, edge7to4 ); // Q2 Face

          QVector< int > reversedEdge7to4 = edge7to4;
          std::reverse( reversedEdge7to4.begin(), reversedEdge7to4.end() );
          QVector< int > reversedEdge6to7 = edge6to7;
          std::reverse( reversedEdge6to7.begin(), reversedEdge6to7.end() );
          QVector< int > reversedEdge0to1 = edge0to1;
          std::reverse( reversedEdge0to1.begin(), reversedEdge0to1.end() );
          QVector< int > reversedEdge3to0 = edge3to0;
          std::reverse( reversedEdge3to0.begin(), reversedEdge3to0.end() );

          addFaceTriangles( reversedEdge7to4, edge4to5, edge5to6, reversedEdge6to7 ); // Plane z = 1
          addFaceTriangles( edge1to2, reversedEdge0to1, reversedEdge3to0, edge2to3 ); // Plane z = -1


          auto cubeToEllipsoid = [&positions]
          {
            for ( int i = 0; i < positions.size(); ++i )
            {
              positions[i] = positions[i].normalized();
            }
          };

          cubeToEllipsoid();

          break;
        }
      }

      setVertices( positions, indices );
    }

    void subdivide( QList<QVector3D> &positions, QList<int> &indices, QList< int > triangle, int level )
    {
      if ( level > 0 )
      {
        QVector3D p1 = QVector3D( ( positions[triangle[0]] + positions[triangle[1]] ) / 2 ).normalized();
        QVector3D p2 = QVector3D( ( positions[triangle[1]] + positions[triangle[2]] ) / 2 ).normalized();
        QVector3D p3 = QVector3D( ( positions[triangle[2]] + positions[triangle[0]] ) / 2 ).normalized();
        positions.append( p1 );
        positions.append( p2 );
        positions.append( p3 );

        int index01 = positions.size() - 3;
        int index12 = positions.size() - 2;
        int index20 = positions.size() - 1;
        level--;

        subdivide( positions, indices, {triangle[0], index01, index20}, level );
        subdivide( positions, indices, {index01, triangle[1], index12}, level );
        subdivide( positions, indices, {index01, index12, index20}, level );
        subdivide( positions, indices, {index20, index12, triangle[2]}, level );
      }
      else
      {

        if ( !mUseLines )
        {
          indices.append( triangle[0] );
          indices.append( triangle[1] );
          indices.append( triangle[2] );
        }
        else
        {
          indices.append( triangle[0] );
          indices.append( triangle[1] );

          indices.append( triangle[1] );
          indices.append( triangle[2] );

          indices.append( triangle[2] );
          indices.append( triangle[0] );
        }
      }
    }

    void setVertices( const QList<QVector3D> &vertices, const QList<int> &indices )
    {
      QByteArray vertexBufferData;
      vertexBufferData.resize( static_cast<int>( static_cast<long>( vertices.size() ) * 3 * sizeof( float ) * 2 ) );

      QByteArray indexBufferData;
      indexBufferData.resize( static_cast<int>( static_cast<long>( indices.size() ) * sizeof( uint ) ) );

      float *rawVertexArray = reinterpret_cast<float *>( vertexBufferData.data() );
      int idx = 0;

      uint *rawIndexArray = reinterpret_cast<uint *>( indexBufferData.data() );

      for ( int i = 0; i < vertices.size(); ++i )
      {
        const QVector3D &v = vertices[i];
        rawVertexArray[idx++] = v.x() * mRadius.x();
        rawVertexArray[idx++] = v.y() * mRadius.y();
        rawVertexArray[idx++] = v.z() * mRadius.z();

        const QVector3D normal = ( v * mOneOverRadiiSquared ).normalized();

        rawVertexArray[idx++] = normal.x();
        rawVertexArray[idx++] = normal.y();
        rawVertexArray[idx++] = normal.z();
      }

      idx = 0;
      for ( int index : indices )
      {
        rawIndexArray[idx++] = index;
      }

      mVertexBuffer->setData( vertexBufferData );
      mPositionAttribute->setCount( vertices.count() );
      mNormalAttribute->setCount( vertices.count() );

      mIndexBuffer->setData( indexBufferData );
      mIndexAttribute->setCount( indices.size() );
    }

  private:
    Algorithm mAlgorithm = Algorithm::SubdivisionSurface;
    bool mUseLines = false;
    QVector3D mRadius;
    QVector3D mOneOverRadiiSquared;
    Qt3DRender::QGeometry *mGeom = nullptr;
    Qt3DRender::QAttribute *mPositionAttribute = nullptr;
    Qt3DRender::QAttribute *mNormalAttribute = nullptr;
    Qt3DRender::QAttribute *mIndexAttribute = nullptr;

    Qt3DRender::QBuffer *mVertexBuffer = nullptr;
    Qt3DRender::QBuffer *mIndexBuffer = nullptr;
};

class GlobeEntity : public Qt3DCore::QEntity
{

  public:
    GlobeEntity( GlobeMesh::Algorithm algorithm, bool useLines, const QgsAbstractMaterialSettings &material, Qt3DCore::QNode *parent = nullptr );

  private:
    GlobeMesh *mMesh = nullptr;
};

GlobeEntity::GlobeEntity( GlobeMesh::Algorithm algorithm, bool useLines, const QgsAbstractMaterialSettings &material, Qt3DCore::QNode *parent )
  : Qt3DCore::QEntity( parent )
{
  mMesh = new GlobeMesh( algorithm, 50, 50, 45, useLines );
  addComponent( mMesh );

  Qt3DRender::QMaterial *bboxesMaterial = material.toMaterial( QgsMaterialSettingsRenderingTechnique::Triangles,
                                          QgsMaterialContext() );


  addComponent( bboxesMaterial );

}

void initCanvas3D( Qgs3DMapCanvas *canvas )
{
  QgsLayerTree *root = QgsProject::instance()->layerTreeRoot();
  const QList< QgsMapLayer * > visibleLayers = root->checkedLayers();

  QgsCoordinateReferenceSystem crs = QgsProject::instance()->crs();
  if ( crs.isGeographic() )
  {
    // we can't deal with non-projected CRS, so let's just pick something
    QgsProject::instance()->setCrs( QgsCoordinateReferenceSystem( QStringLiteral( "EPSG:3857" ) ) );
  }

  QgsMapSettings ms;
  ms.setDestinationCrs( QgsProject::instance()->crs() );
  ms.setLayers( visibleLayers );
  QgsRectangle fullExtent = QgsProject::instance()->viewSettings()->fullExtent();
  fullExtent = QgsRectangle( -50, -50, 50, 50 );

  Qgs3DMapSettings *map = new Qgs3DMapSettings;
  map->setCrs( QgsProject::instance()->crs() );
  map->setOrigin( QgsVector3D( fullExtent.center().x(), fullExtent.center().y(), 0 ) );
  map->setLayers( visibleLayers );
  map->setBackgroundColor( QColor( 255, 255, 255 ) );

  map->setExtent( fullExtent );

  Qgs3DAxisSettings axis;
  axis.setMode( Qgs3DAxisSettings::Mode::Crs );
  map->set3DAxisSettings( axis );

  map->setTransformContext( QgsProject::instance()->transformContext() );
  map->setPathResolver( QgsProject::instance()->pathResolver() );
  map->setMapThemeCollection( QgsProject::instance()->mapThemeCollection() );
  QObject::connect( QgsProject::instance(), &QgsProject::transformContextChanged, map, [map]
  {
    map->setTransformContext( QgsProject::instance()->transformContext() );
  } );

  QgsFlatTerrainGenerator *flatTerrain = new QgsFlatTerrainGenerator;
  flatTerrain->setCrs( map->crs() );
  //map->setTerrainGenerator( flatTerrain );
  map->setTerrainElevationOffset( QgsProject::instance()->elevationProperties()->terrainProvider()->offset() );

  QgsDirectionalLightSettings defaultPointLight;
  //defaultPointLight.setPosition( QgsVector3D( 0, 1000, 0 ) );
  //defaultPointLight.setConstantAttenuation( 0 );
  map->setLightSources( {defaultPointLight.clone() } );
  if ( QScreen *screen = QGuiApplication::primaryScreen() )
  {
    map->setOutputDpi( screen->physicalDotsPerInch() );
  }
  else
  {
    map->setOutputDpi( 96 );
  }

  canvas->setMapSettings( map );


  QgsPhongMaterialSettings phong;
  phong.setAmbient( QColor( 0, 100, 30 ) );
  phong.setDiffuse( QColor( 0, 200, 100 ) );
  GlobeEntity *tetrahedron = new GlobeEntity( GlobeMesh::Algorithm::CubeMapTessellation, false, phong );
  canvas->scene()->addEntity( tetrahedron );

  phong.setAmbient( QColor( 0, 0, 0 ) );
  phong.setDiffuse( QColor( 0, 0, 0 ) );
  tetrahedron = new GlobeEntity( GlobeMesh::Algorithm::CubeMapTessellation,  true, phong );
  canvas->scene()->addEntity( tetrahedron );

  QgsRectangle extent = fullExtent;
  extent.scale( 1.3 );
  const float dist = static_cast< float >( std::max( extent.width(), extent.height() ) );
  canvas->setViewFromTop( extent.center(), dist * 2, 0 );

  QObject::connect( canvas->scene(), &Qgs3DMapScene::totalPendingJobsCountChanged, canvas, [canvas]
  {
    qDebug() << "pending jobs:" << canvas->scene()->totalPendingJobsCount();
  } );

  qDebug() << "pending jobs:" << canvas->scene()->totalPendingJobsCount();
}

int main( int argc, char *argv[] )
{
  QgsApplication myApp( argc, argv, true, QString(), QStringLiteral( "desktop" ) );

  // init QGIS's paths - true means that all path will be inited from prefix
  QgsApplication::init();
  QgsApplication::initQgis();
  Qgs3D::initialize();

  /*
  if ( argc < 2 )
  {
    qDebug() << "need QGIS project file";
    return 1;
  }

  const QString projectFile = argv[1];
  const bool res = QgsProject::instance()->read( projectFile );
  if ( !res )
  {
    qDebug() << "can't open project file" << projectFile;
    return 1;
  }
  */

  // a hack to assign 3D renderer
  for ( QgsMapLayer *layer : QgsProject::instance()->layerTreeRoot()->checkedLayers() )
  {
    if ( QgsPointCloudLayer *pcLayer = qobject_cast<QgsPointCloudLayer *>( layer ) )
    {
      QgsPointCloudLayer3DRenderer *r = new QgsPointCloudLayer3DRenderer();
      r->setLayer( pcLayer );
      r->resolveReferences( *QgsProject::instance() );
      pcLayer->setRenderer3D( r );
    }

    if ( QgsTiledSceneLayer *tsLayer = qobject_cast<QgsTiledSceneLayer *>( layer ) )
    {
      QgsTiledSceneLayer3DRenderer *r = new QgsTiledSceneLayer3DRenderer();
      r->setLayer( tsLayer );
      r->resolveReferences( *QgsProject::instance() );
      tsLayer->setRenderer3D( r );
    }
  }

  Qgs3DMapCanvas *canvas = new Qgs3DMapCanvas;
  initCanvas3D( canvas );
  canvas->resize( 800, 600 );
  canvas->show();

  return myApp.exec();
}

