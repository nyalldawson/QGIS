/***************************************************************************
  qgspbrmaterial.cpp
  --------------------------------------
  Date                 : April 2026
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

#include "qgspbrmaterial.h"

#include "qgs3dutils.h"

#include <QString>
#include <Qt3DRender/QAbstractTexture>
#include <Qt3DRender/QGraphicsApiFilter>
#include <Qt3DRender/QParameter>
#include <Qt3DRender/QRenderPass>
#include <Qt3DRender/QShaderProgramBuilder>
#include <Qt3DRender/QTechnique>
#include <Qt3DRender/QTexture>

#include "moc_qgspbrmaterial.cpp"

using namespace Qt::StringLiterals;

///@cond PRIVATE
QgsPBRMaterial::QgsPBRMaterial( QNode *parent )
  : QgsMaterial( parent )
  , mPBREffect( new Qt3DRender::QEffect( this ) )
  , mBaseColorParameter( new Qt3DRender::QParameter( u"baseColor"_s, Qgs3DUtils::srgbToLinear( QColor( "grey" ) ), this ) )
  , mRoughnessParameter( new Qt3DRender::QParameter( u"roughness"_s, 0.0f, this ) )
  , mBaseColorMapParameter( new Qt3DRender::QParameter( u"baseColorMap"_s, QVariant(), this ) )
  , mRoughnessMapParameter( new Qt3DRender::QParameter( u"roughnessMap"_s, QVariant(), this ) )
  , mAmbientOcclusionMapParameter( new Qt3DRender::QParameter( u"ambientOcclusionMap"_s, QVariant(), this ) )
  , mNormalMapParameter( new Qt3DRender::QParameter( u"normalMap"_s, QVariant(), this ) )
  , mHeightMapParameter( new Qt3DRender::QParameter( u"heightMap"_s, QVariant(), this ) )
  , mParallaxScaleParameter( new Qt3DRender::QParameter( u"parallaxScale"_s, 0.1f, this ) )
  , mEmissionMapParameter( new Qt3DRender::QParameter( u"emissionMap"_s, QVariant(), this ) )
  , mEmissionFactorParameter( new Qt3DRender::QParameter( u"emissiveFactor"_s, 1.0f, this ) )
  , mTextureScaleParameter( new Qt3DRender::QParameter( u"texCoordScale"_s, 1.0f, this ) )
  , mTextureRotationParameter( new Qt3DRender::QParameter( u"texCoordRotation"_s, 0.0f, this ) )
  , mPBRGL3Technique( new Qt3DRender::QTechnique( this ) )
  , mPBRGL3RenderPass( new Qt3DRender::QRenderPass( this ) )
  , mPBRGL3Shader( new Qt3DRender::QShaderProgram( this ) )
  , mFilterKey( new Qt3DRender::QFilterKey( this ) )
{}

void QgsPBRMaterial::setBaseColor( const QColor &baseColor )
{
  mBaseColorParameter->setValue( Qgs3DUtils::srgbToLinear( baseColor ) );
  bool oldUsingBaseColorMap = mUsingBaseColorMap;

  mUsingBaseColorMap = false;
  if ( mPBREffect->parameters().contains( mBaseColorMapParameter ) )
    mPBREffect->removeParameter( mBaseColorMapParameter );
  mPBREffect->addParameter( mBaseColorParameter );

  if ( oldUsingBaseColorMap != mUsingBaseColorMap )
    updateFragmentShader();
}

void QgsPBRMaterial::setBaseColorTexture( Qt3DRender::QAbstractTexture *baseColor )
{
  mBaseColorMapParameter->setValue( QVariant::fromValue( baseColor ) );
  bool oldUsingBaseColorMap = mUsingBaseColorMap;

  mUsingBaseColorMap = true;
  mPBREffect->addParameter( mBaseColorMapParameter );
  if ( mPBREffect->parameters().contains( mBaseColorParameter ) )
    mPBREffect->removeParameter( mBaseColorParameter );

  if ( oldUsingBaseColorMap != mUsingBaseColorMap )
    updateFragmentShader();
}

void QgsPBRMaterial::setRoughness( float roughness )
{
  mRoughnessParameter->setValue( roughness );
  bool oldUsingRoughnessMap = mUsingRoughnessMap;

  mUsingRoughnessMap = false;
  if ( mPBREffect->parameters().contains( mRoughnessMapParameter ) )
    mPBREffect->removeParameter( mRoughnessMapParameter );
  mPBREffect->addParameter( mRoughnessParameter );

  if ( oldUsingRoughnessMap != mUsingRoughnessMap )
    updateFragmentShader();
}

void QgsPBRMaterial::setRoughnessTexture( Qt3DRender::QAbstractTexture *roughness )
{
  mRoughnessMapParameter->setValue( QVariant::fromValue( roughness ) );
  bool oldUsingRoughnessMap = mUsingRoughnessMap;

  mUsingRoughnessMap = true;
  mPBREffect->addParameter( mRoughnessMapParameter );
  if ( mPBREffect->parameters().contains( mRoughnessParameter ) )
    mPBREffect->removeParameter( mRoughnessParameter );

  if ( oldUsingRoughnessMap != mUsingRoughnessMap )
    updateFragmentShader();
}

void QgsPBRMaterial::setAmbientOcclusionTexture( Qt3DRender::QAbstractTexture *ambientOcclusion )
{
  bool oldUsingAmbientOcclusionMap = mUsingAmbientOcclusionMap;

  if ( ambientOcclusion )
  {
    mAmbientOcclusionMapParameter->setValue( QVariant::fromValue( ambientOcclusion ) );
    mUsingAmbientOcclusionMap = true;
    mPBREffect->addParameter( mAmbientOcclusionMapParameter );
  }
  else
  {
    mAmbientOcclusionMapParameter->setValue( QVariant() );
    mUsingAmbientOcclusionMap = false;
    if ( mPBREffect->parameters().contains( mAmbientOcclusionMapParameter ) )
      mPBREffect->removeParameter( mAmbientOcclusionMapParameter );
  }

  if ( oldUsingAmbientOcclusionMap != mUsingAmbientOcclusionMap )
    updateFragmentShader();
}

void QgsPBRMaterial::setNormalTexture( Qt3DRender::QAbstractTexture *normal )
{
  bool oldUsingNormalMap = mUsingNormalMap;

  if ( normal )
  {
    mNormalMapParameter->setValue( QVariant::fromValue( normal ) );
    mUsingNormalMap = true;
    mPBREffect->addParameter( mNormalMapParameter );
  }
  else
  {
    mNormalMapParameter->setValue( QVariant() );
    mUsingNormalMap = false;
    if ( mPBREffect->parameters().contains( mNormalMapParameter ) )
      mPBREffect->removeParameter( mNormalMapParameter );
  }

  if ( oldUsingNormalMap != mUsingNormalMap )
    updateFragmentShader();
}

void QgsPBRMaterial::setHeightTexture( Qt3DRender::QAbstractTexture *height )
{
  bool oldUsingHeightMap = mUsingHeightMap;

  if ( height )
  {
    mHeightMapParameter->setValue( QVariant::fromValue( height ) );
    mUsingHeightMap = true;
    mPBREffect->addParameter( mHeightMapParameter );
  }
  else
  {
    mHeightMapParameter->setValue( QVariant() );
    mUsingHeightMap = false;
    if ( mPBREffect->parameters().contains( mHeightMapParameter ) )
      mPBREffect->removeParameter( mHeightMapParameter );
  }

  if ( oldUsingHeightMap != mUsingHeightMap )
    updateFragmentShader();
}

void QgsPBRMaterial::setParallaxScale( double scale )
{
  mParallaxScaleParameter->setValue( scale );
}

void QgsPBRMaterial::setEmissionTexture( Qt3DRender::QAbstractTexture *emission )
{
  bool oldUsingEmissionMap = mUsingEmissionMap;

  if ( emission )
  {
    mEmissionMapParameter->setValue( QVariant::fromValue( emission ) );
    mUsingEmissionMap = true;
    mPBREffect->addParameter( mEmissionMapParameter );
  }
  else
  {
    mEmissionMapParameter->setValue( QVariant() );
    mUsingEmissionMap = false;
    if ( mPBREffect->parameters().contains( mEmissionMapParameter ) )
      mPBREffect->removeParameter( mEmissionMapParameter );
  }

  if ( oldUsingEmissionMap != mUsingEmissionMap )
    updateFragmentShader();
}

void QgsPBRMaterial::setEmissionFactor( double factor )
{
  mEmissionFactorParameter->setValue( factor );
}

void QgsPBRMaterial::setTextureScale( float textureScale )
{
  mTextureScaleParameter->setValue( textureScale );
}

void QgsPBRMaterial::setTextureRotation( float textureRotation )
{
  mTextureRotationParameter->setValue( textureRotation );
}

void QgsPBRMaterial::initMaterial()
{
  const QByteArray vertexShaderCode = Qt3DRender::QShaderProgram::loadSource( QUrl( u"qrc:/shaders/default.vert"_s ) );
  const QByteArray finalVertexShaderCode = Qgs3DUtils::addDefinesToShaderCode( vertexShaderCode, QStringList( { "TEXTURE_ROTATION" } ) );
  mPBRGL3Shader->setVertexShaderCode( finalVertexShaderCode );

  updateFragmentShader();

  mPBRGL3Technique->graphicsApiFilter()->setApi( Qt3DRender::QGraphicsApiFilter::OpenGL );
  mPBRGL3Technique->graphicsApiFilter()->setMajorVersion( 3 );
  mPBRGL3Technique->graphicsApiFilter()->setMinorVersion( 1 );
  mPBRGL3Technique->graphicsApiFilter()->setProfile( Qt3DRender::QGraphicsApiFilter::CoreProfile );

  mFilterKey->setParent( this );
  mFilterKey->setName( u"renderingStyle"_s );
  mFilterKey->setValue( u"forward"_s );

  mPBRGL3Technique->addFilterKey( mFilterKey );
  mPBRGL3RenderPass->setShaderProgram( mPBRGL3Shader );
  mPBRGL3Technique->addRenderPass( mPBRGL3RenderPass );
  mPBREffect->addTechnique( mPBRGL3Technique );

  // Given parameters a parent
  mBaseColorMapParameter->setParent( mPBREffect );
  mRoughnessMapParameter->setParent( mPBREffect );
  mNormalMapParameter->setParent( mPBREffect );
  mAmbientOcclusionMapParameter->setParent( mPBREffect );
  mEmissionMapParameter->setParent( mPBREffect );
  mHeightMapParameter->setParent( mPBREffect );

  mPBREffect->addParameter( mBaseColorParameter );
  mPBREffect->addParameter( mRoughnessParameter );
  mPBREffect->addParameter( mParallaxScaleParameter );
  mPBREffect->addParameter( mEmissionFactorParameter );
  mPBREffect->addParameter( mTextureScaleParameter );
  mPBREffect->addParameter( mTextureRotationParameter );

  setEffect( mPBREffect );
}

QStringList QgsPBRMaterial::fragmentShaderDefines()
{
  QStringList defines;
  if ( mUsingBaseColorMap )
    defines += "BASE_COLOR_MAP";
  if ( mUsingRoughnessMap )
    defines += "ROUGHNESS_MAP";
  if ( mUsingAmbientOcclusionMap )
    defines += "AMBIENT_OCCLUSION_MAP";
  if ( mUsingNormalMap )
    defines += "NORMAL_MAP";
  if ( mUsingHeightMap )
    defines += "HEIGHT_MAP";
  if ( mUsingEmissionMap )
    defines += "EMISSION_MAP";
  if ( mFlatShading )
    defines += "FLAT_SHADING";
  return defines;
}

void QgsPBRMaterial::updateFragmentShader()
{
  // pre-process fragment shader and add #defines based on whether using maps for some properties
  const QByteArray fragmentShaderCode = Qt3DRender::QShaderProgram::loadSource( QUrl( u"qrc:/shaders/pbr.frag"_s ) );
  const QByteArray finalShaderCode = Qgs3DUtils::addDefinesToShaderCode( fragmentShaderCode, fragmentShaderDefines() );
  mPBRGL3Shader->setFragmentShaderCode( finalShaderCode );
}

void QgsPBRMaterial::setFlatShadingEnabled( bool enabled )
{
  if ( enabled != mFlatShading )
  {
    mFlatShading = enabled;
    updateFragmentShader();
  }
}


///@endcond PRIVATE
