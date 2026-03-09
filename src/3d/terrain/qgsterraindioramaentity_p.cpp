/***************************************************************************
  qgsterraindioramaentity_p.cpp
  --------------------------------------
  Date                 : March 2026
  Copyright            : (C) 2026 by Nyall Dawson
  Email                : nyall dot dawson at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsterraindioramaentity_p.h"

#include "qgs3dmapsettings.h"
#include "qgsgeotransform.h"
#include "qgsphongmaterialsettings.h"

#include <Qt3DCore/QAttribute>
#include <Qt3DCore/QBuffer>
#include <Qt3DCore/QGeometry>
#include <Qt3DRender/QCullFace>
#include <Qt3DRender/QEffect>
#include <Qt3DRender/QGeometryRenderer>
#include <Qt3DRender/QRenderPass>
#include <Qt3DRender/QTechnique>

#include "moc_qgsterraindioramaentity_p.cpp"

///@cond PRIVATE

QgsTerrainDioramaEntity::QgsTerrainDioramaEntity( const Qgs3DMapSettings &mapSettings, Qt3DCore::QNode *parent )
  : Qt3DCore::QEntity( parent )
{
  buildGeometry( mapSettings );
}

void QgsTerrainDioramaEntity::buildGeometry( const Qgs3DMapSettings &mapSettings )
{
  const QgsRectangle extent = mapSettings.extent();
  const float baseZ = static_cast<float>( mapSettings.dioramaHeight() );

  // Build the bottom quad in local coordinates relative to the extent's
  // south-west corner, using a GeoTransform for the offset.
  const double xMin = extent.xMinimum();
  const double yMin = extent.yMinimum();
  const float w = static_cast<float>( extent.width() );
  const float h = static_cast<float>( extent.height() );
  const QgsVector3D origin = mapSettings.origin();

  struct Vertex
  {
      float pos[3];
      float normal[3];
  };

  Vertex v;
  v.normal[0] = 0;
  v.normal[1] = 0;
  v.normal[2] = -1;

  QVector<Vertex> vertices;
  vertices.reserve( 6 );

  // Triangle 1
  v.pos[0] = 0;
  v.pos[1] = h;
  v.pos[2] = baseZ;
  vertices.append( v );
  v.pos[0] = 0;
  v.pos[1] = 0;
  v.pos[2] = baseZ;
  vertices.append( v );
  v.pos[0] = w;
  v.pos[1] = 0;
  v.pos[2] = baseZ;
  vertices.append( v );

  // Triangle 2
  v.pos[0] = 0;
  v.pos[1] = h;
  v.pos[2] = baseZ;
  vertices.append( v );
  v.pos[0] = w;
  v.pos[1] = 0;
  v.pos[2] = baseZ;
  vertices.append( v );
  v.pos[0] = w;
  v.pos[1] = h;
  v.pos[2] = baseZ;
  vertices.append( v );

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
  addComponent( renderer );

  // Default material
  QgsPhongMaterialSettings materialSettings;
  materialSettings.setDiffuse( QColor( 160, 160, 160 ) );
  materialSettings.setAmbient( QColor( 80, 80, 80 ) );
  materialSettings.setSpecular( QColor( 30, 30, 30 ) );
  materialSettings.setShininess( 1.0 );

  QgsMaterialContext materialContext;
  materialContext.setIsSelected( false );
  QgsMaterial *material = materialSettings.toMaterial( QgsMaterialSettingsRenderingTechnique::Triangles, materialContext );

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

  addComponent( material );

  QgsGeoTransform *transform = new QgsGeoTransform;
  transform->setGeoTranslation( QgsVector3D( xMin, yMin, 0 ) );
  transform->setOrigin( origin );
  addComponent( transform );
}

/// @endcond
