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
#include "qgspointlightsettings.h"
#include "qgsterrainprovider.h"
#include "qgstiledscenelayer.h"
#include "qgstiledscenelayer3drenderer.h"
#include "qgschunkboundsentity_p.h"
#include "qgs3dwiredmesh_p.h"
#include "qgsgoochmaterialsettings.h"
#include "qgsphongmaterialsettings.h"
#include "qgsdirectionallightsettings.h"

#include <QScreen>

#include <Qt3DExtras/QGoochMaterial>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <Qt3DRender/QAttribute>
#include <Qt3DRender/QGeometry>
typedef Qt3DRender::QAttribute Qt3DQAttribute;
typedef Qt3DRender::QGeometry Qt3DQGeometry;
typedef Qt3DRender::QBuffer Qt3DQBuffer;
#else
#include <Qt3DCore/QAttribute>
#include <Qt3DCore/QGeometry>
typedef Qt3DCore::QAttribute Qt3DQAttribute;
typedef Qt3DCore::QGeometry Qt3DQGeometry;
typedef Qt3DCore::QBuffer Qt3DQBuffer;
#endif

class Mesh : public Qt3DRender::QGeometryRenderer
{

public:


    Mesh( Qt3DCore::QNode *parent = nullptr )
        : Qt3DRender::QGeometryRenderer( parent )
        , mPositionAttribute( new Qt3DQAttribute( this ) )
        , mNormalAttribute( new Qt3DQAttribute( this ) )
        , mIndexAttribute( new Qt3DQAttribute( this ) )
        , mVertexBuffer( new Qt3DQBuffer( this ) )
        , mNormalBuffer( new Qt3DQBuffer( this ) )
        , mIndexBuffer( new Qt3DQBuffer( this ) )
    {
        mPositionAttribute->setAttributeType( Qt3DQAttribute::VertexAttribute );
        mPositionAttribute->setBuffer( mVertexBuffer );
        mPositionAttribute->setVertexBaseType( Qt3DQAttribute::Float );
        mPositionAttribute->setVertexSize( 3 );
        mPositionAttribute->setByteStride( 3 * sizeof( float ) );
        mPositionAttribute->setByteOffset( 0 );
        mPositionAttribute->setName( Qt3DQAttribute::defaultPositionAttributeName() );

        mNormalAttribute->setAttributeType( Qt3DQAttribute::VertexAttribute );
        mNormalAttribute->setBuffer( mNormalBuffer );
        mNormalAttribute->setVertexBaseType( Qt3DQAttribute::Float );
        mNormalAttribute->setVertexSize( 3 );
        mNormalAttribute->setByteStride( 3 * sizeof( float ) );
        mNormalAttribute->setByteOffset( 0 );
        mNormalAttribute->setName( Qt3DQAttribute::defaultNormalAttributeName() );

        mIndexAttribute->setAttributeType(Qt3DQAttribute::IndexAttribute);
        mIndexAttribute->setVertexBaseType(Qt3DQAttribute::UnsignedInt);
          mIndexAttribute->setByteStride( sizeof( uint ) );
          mIndexAttribute->setByteOffset( 0 );
        mIndexAttribute->setBuffer(mIndexBuffer);

        mGeom = new Qt3DQGeometry( this );
        mGeom->addAttribute( mPositionAttribute );
        mGeom->addAttribute( mIndexAttribute );
        mGeom->addAttribute( mNormalAttribute );

        setInstanceCount( 1 );
        setIndexOffset( 0 );
        setFirstInstance( 0 );
        setPrimitiveType( Qt3DRender::QGeometryRenderer::Triangles );
        //setPrimitiveType( Qt3DRender::QGeometryRenderer::Lines);
        setGeometry( mGeom );


        const double radius = 50;
        constexpr int numberOfSubDivisions = 3;
        constexpr double rootTwoOverThree = M_SQRT2 / 3.0;
        constexpr double oneThird = 1.0 / 3.0;
        constexpr double rootSixOverThree = 2.449489742783178 / 3.0;

        QList<QVector3D> positions;
        QList<QVector3D> normals;
        QList<int> indices;
        positions.append( QVector3D( 0, 0, 1) * radius );
        positions.append( QVector3D( 0, 2 * rootTwoOverThree, -oneThird ) * radius );
        positions.append( QVector3D( -rootSixOverThree, -rootTwoOverThree, -oneThird ) * radius );
        positions.append( QVector3D( rootSixOverThree, -rootTwoOverThree, -oneThird ) * radius );


        normals.append( QVector3D( 0, 0, 1).normalized() );
        normals.append( QVector3D( 0, 2 * rootTwoOverThree, -oneThird ).normalized() );
        normals.append( QVector3D( -rootSixOverThree, -rootTwoOverThree, -oneThird ).normalized() );
        normals.append( QVector3D( rootSixOverThree, -rootTwoOverThree, -oneThird ).normalized() );


        subdivide( positions, normals, indices, {0,1,2}, numberOfSubDivisions, radius );
        subdivide( positions, normals, indices, {0,2,3}, numberOfSubDivisions, radius );
        subdivide( positions, normals, indices, {0,3,1}, numberOfSubDivisions, radius );
        subdivide( positions, normals, indices, {1,3,2}, numberOfSubDivisions, radius );

        setVertices( positions, normals,  indices );
    }

    void subdivide( QList<QVector3D> & positions, QList<QVector3D> & normals, QList<int> &indices, QList< int > triangle, int level, double radius )
    {
        if ( level > 0 )
        {
            QVector3D p1 = QVector3D(( positions[triangle[0]] + positions[triangle[1]] ) / 2).normalized();
            QVector3D p2 = QVector3D(( positions[triangle[1]] + positions[triangle[2]] ) / 2).normalized();
            QVector3D p3 = QVector3D(( positions[triangle[2]] + positions[triangle[0]] ) / 2).normalized();
            positions.append( p1 * radius );
            positions.append( p2* radius );
            positions.append( p3 * radius );

            normals.append( p1 );
            normals.append( p2 );
            normals.append( p3 );

            int index01 = positions.size() - 3;
            int index12 = positions.size() - 2;
            int index20 = positions.size() - 1;
            level--;

            subdivide( positions, normals, indices, {triangle[0], index01, index20}, level, radius );
            subdivide( positions, normals, indices, {index01, triangle[1], index12}, level, radius );
            subdivide( positions, normals, indices, {index01, index12, index20}, level, radius );
            subdivide( positions, normals, indices, {index20, index12, triangle[2]}, level, radius );
        }
        else
        {
#if 1
            indices.append( triangle[0]);
            indices.append( triangle[1]);
            indices.append( triangle[2]);

            const QVector3D t0 = positions[triangle[0]];
            const QVector3D t1 = positions[triangle[1]];
            const QVector3D t2 = positions[triangle[2]];

            const QVector3D center = (t0+t1+t2);

            QVector3D m( center.x() / ( radius * radius ), center.y()/ ( radius * radius ), center.z()/ ( radius * radius ) );





#else
            indices.append( triangle[0]);
            indices.append( triangle[1]);

            indices.append( triangle[1]);
            indices.append( triangle[2]);

            indices.append( triangle[2]);
            indices.append( triangle[0]);
#endif
        }
    }

    void setVertices( const QList<QVector3D> &vertices, const QList<QVector3D> &normals, const QList<int> &indices )
    {
        QByteArray vertexBufferData;
        vertexBufferData.resize( static_cast<int>( static_cast<long>( vertices.size() ) * 3 * sizeof( float ) ) );

        QByteArray indexBufferData;
        indexBufferData.resize( static_cast<int>( static_cast<long>( indices.size() ) * sizeof( uint ) ) );

        float *rawVertexArray = reinterpret_cast<float *>( vertexBufferData.data() );
        int idx = 0;

        uint *rawIndexArray = reinterpret_cast<uint *>( indexBufferData.data() );

        for ( const QVector3D &v : std::as_const( vertices ) )
        {
            rawVertexArray[idx++] = v.x();
            rawVertexArray[idx++] = v.y();
            rawVertexArray[idx++] = v.z();
        }

        idx = 0;
        for ( int index : indices )
        {
            rawIndexArray[idx++] = index;
        }

        mVertexBuffer->setData( vertexBufferData );
        //setVertexCount( vertices.count() );
        mPositionAttribute->setCount( vertices.count() );

        mIndexBuffer->setData( indexBufferData );
        mIndexAttribute->setCount(indices.size() );

        QByteArray normalBufferData;
        normalBufferData.resize( static_cast<int>( static_cast<long>( normals.size() ) * 3 * sizeof( float ) ) );

        float *rawNormalArray = reinterpret_cast<float *>( normalBufferData.data() );
        idx = 0;

        for ( const QVector3D &v : std::as_const( normals ) )
        {
            rawNormalArray[idx++] = v.x();
            rawNormalArray[idx++] = v.y();
            rawNormalArray[idx++] = v.z();
        }
        mNormalBuffer->setData( normalBufferData );
        //setVertexCount( vertices.count() );
        mNormalAttribute->setCount( normals.count() );
    }

private:
    Qt3DRender::QGeometry *mGeom = nullptr;
    Qt3DRender::QAttribute *mPositionAttribute = nullptr;
    Qt3DRender::QAttribute *mIndexAttribute = nullptr;
    Qt3DRender::QAttribute *mNormalAttribute = nullptr;
    Qt3DRender::QBuffer *mVertexBuffer = nullptr;
    Qt3DRender::QBuffer *mNormalBuffer = nullptr;
      Qt3DRender::QBuffer *mIndexBuffer = nullptr;
};

class TetrahedronEntity : public Qt3DCore::QEntity
{

public:
    TetrahedronEntity( Qt3DCore::QNode *parent = nullptr );

private:
    Mesh *mMesh = nullptr;
};

TetrahedronEntity::TetrahedronEntity( Qt3DCore::QNode *parent )
    : Qt3DCore::QEntity( parent )
{

    mMesh = new Mesh;
    addComponent( mMesh );

    QgsPhongMaterialSettings settings;

    Qt3DRender::QMaterial *bboxesMaterial = settings.toMaterial( QgsMaterialSettingsRenderingTechnique::Triangles,
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
  map->setBackgroundColor( QColor( 255,255,255 ) );

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

  TetrahedronEntity* tetrahedron = new TetrahedronEntity();
  //chunkBoundsEntity->setBoxes( {QgsAABB( -50, -50, -50, 50, 50, 50 ) });
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
