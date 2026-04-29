/***************************************************************************
  qgsmetalroughmaterial.cpp
  --------------------------------------
  Date                 : December 2023
  Copyright            : (C) 2023 by Nyall Dawson
  Email                : nyall dot dawson at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsmetalroughmaterial.h"

#include <QString>
#include <Qt3DRender/QAbstractTexture>
#include <Qt3DRender/QGraphicsApiFilter>
#include <Qt3DRender/QParameter>
#include <Qt3DRender/QRenderPass>
#include <Qt3DRender/QShaderProgramBuilder>
#include <Qt3DRender/QTechnique>
#include <Qt3DRender/QTexture>

#include "moc_qgsmetalroughmaterial.cpp"

using namespace Qt::StringLiterals;

///@cond PRIVATE
QgsMetalRoughMaterial::QgsMetalRoughMaterial( QNode *parent )
  : QgsPBRMaterial( parent )
  , mMetalnessParameter( new Qt3DRender::QParameter( u"metalness"_s, 0.0f, this ) )
  , mMetalnessMapParameter( new Qt3DRender::QParameter( u"metalnessMap"_s, QVariant(), this ) )
{
  init();
}

QgsMetalRoughMaterial::~QgsMetalRoughMaterial() = default;

void QgsMetalRoughMaterial::setMetalness( float metalness )
{
  mMetalnessParameter->setValue( metalness );
  bool oldUsingMetalnessMap = mUsingMetalnessMap;

  mUsingMetalnessMap = false;
  if ( mPBREffect->parameters().contains( mMetalnessMapParameter ) )
    mPBREffect->removeParameter( mMetalnessMapParameter );
  mPBREffect->addParameter( mMetalnessParameter );

  if ( oldUsingMetalnessMap != mUsingMetalnessMap )
    updateFragmentShader();
}

void QgsMetalRoughMaterial::setMetalnessTexture( Qt3DRender::QAbstractTexture *metalness )
{
  mMetalnessMapParameter->setValue( QVariant::fromValue( metalness ) );
  bool oldUsingMetalnessMap = mUsingMetalnessMap;

  mUsingMetalnessMap = true;
  mPBREffect->addParameter( mMetalnessMapParameter );
  if ( mPBREffect->parameters().contains( mMetalnessParameter ) )
    mPBREffect->removeParameter( mMetalnessParameter );

  if ( oldUsingMetalnessMap != mUsingMetalnessMap )
    updateFragmentShader();
}

QStringList QgsMetalRoughMaterial::fragmentShaderDefines()
{
  QStringList defines = QgsPBRMaterial::fragmentShaderDefines();
  if ( mUsingMetalnessMap )
    defines << "METALNESS_MAP";
  return defines;
}

void QgsMetalRoughMaterial::init()
{
  initMaterial();
  mMetalnessMapParameter->setParent( mPBREffect );
  mPBREffect->addParameter( mMetalnessParameter );
}


///@endcond PRIVATE
