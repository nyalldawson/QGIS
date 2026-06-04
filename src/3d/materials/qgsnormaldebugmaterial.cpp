/***************************************************************************
    qgshighlqgsnormaldebugmaterialightmaterial.cpp
    ---------------------
  Date                 : June 2026
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

#include "qgsnormaldebugmaterial.h"

#include "qgs3dutils.h"

#include <QString>
#include <Qt3DRender/QEffect>
#include <Qt3DRender/QGraphicsApiFilter>
#include <Qt3DRender/QParameter>
#include <Qt3DRender/QRenderPass>
#include <Qt3DRender/QShaderProgram>
#include <Qt3DRender/QTechnique>
#include <Qt3DRender/QTexture>

using namespace Qt::StringLiterals;

///@cond PRIVATE

QgsNormalDebugMaterial::QgsNormalDebugMaterial( QNode *parent )
  : QgsMaterial( parent )
{
  auto shader = new Qt3DRender::QShaderProgram( this );

  // gl_VertexID is 0 for start and 1 for tip
  // gl_InstanceID is the index of the source vertex being processed
  shader->setVertexShaderCode( Qt3DRender::QShaderProgram::loadSource( QUrl( u"qrc:/shaders/normals.vert"_s ) ) );
  shader->setFragmentShaderCode( Qt3DRender::QShaderProgram::loadSource( QUrl( u"qrc:/shaders/singlecolor.frag"_s ) ) );

  auto pass = new Qt3DRender::QRenderPass( this );
  pass->setShaderProgram( shader );

  auto technique = new Qt3DRender::QTechnique( this );
  technique->addRenderPass( pass );

  technique->graphicsApiFilter()->setApi( Qt3DRender::QGraphicsApiFilter::OpenGL );
  technique->graphicsApiFilter()->setProfile( Qt3DRender::QGraphicsApiFilter::CoreProfile );
  technique->graphicsApiFilter()->setMajorVersion( 3 );
  technique->graphicsApiFilter()->setMinorVersion( 3 );

  auto effect = new Qt3DRender::QEffect( this );
  effect->addTechnique( technique );
  setEffect( effect );

  mNormalLength = new Qt3DRender::QParameter( u"normalLength"_s, 1.0f, this );
  addParameter( mNormalLength );

  mColor = new Qt3DRender::QParameter( u"color"_s, Qgs3DUtils::srgbToLinear( QColor( 255, 0, 0 ) ), this );
  addParameter( mColor );
}

void QgsNormalDebugMaterial::setNormalLength( float length )
{
  mNormalLength->setValue( length );
}

void QgsNormalDebugMaterial::setNormalColor( const QColor &color )
{
  mColor->setValue( Qgs3DUtils::srgbToLinear( color ) );
}

///@endcond PRIVATE
