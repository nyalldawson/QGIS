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
  , mEffect( new Qt3DRender::QEffect( this ) )
  , mBaseColorParameter( new Qt3DRender::QParameter( u"baseColor"_s, Qgs3DUtils::srgbToLinear( QColor( "grey" ) ), this ) )
  , mRoughnessParameter( new Qt3DRender::QParameter( u"roughness"_s, 0.0f, this ) )
  , mBaseColorMapParameter( new Qt3DRender::QParameter( u"baseColorMap"_s, QVariant(), this ) )
  , mRoughnessMapParameter( new Qt3DRender::QParameter( u"roughnessMap"_s, QVariant(), this ) )
  , mAmbientOcclusionMapParameter( new Qt3DRender::QParameter( u"ambientOcclusionMap"_s, QVariant(), this ) )
  , mNormalMapParameter( new Qt3DRender::QParameter( u"normalMap"_s, QVariant(), this ) )
  , mHeightMapParameter( new Qt3DRender::QParameter( u"heightMap"_s, QVariant(), this ) )
  , mParallaxScaleParameter( new Qt3DRender::QParameter( u"parallaxScale"_s, 0.1f, this ) )
  , mEmissionMapParameter( new Qt3DRender::QParameter( u"emissionMap"_s, QVariant(), this ) )
  , mEmissiveColorParameter( new Qt3DRender::QParameter( u"emissiveColor"_s, Qgs3DUtils::srgbToLinear( QColor( 0, 0, 0 ) ), this ) )
  , mEmissionFactorParameter( new Qt3DRender::QParameter( u"emissiveFactor"_s, 1.0f, this ) )
  , mTextureScaleParameter( new Qt3DRender::QParameter( u"texCoordScale"_s, 1.0f, this ) )
  , mTextureRotationParameter( new Qt3DRender::QParameter( u"texCoordRotation"_s, 0.0f, this ) )
  , mOpacityParameter( new Qt3DRender::QParameter( u"opacity"_s, 1.0f ) )
  , mMetalRoughGL3Technique( new Qt3DRender::QTechnique( this ) )
  , mMetalRoughGL3RenderPass( new Qt3DRender::QRenderPass( this ) )
  , mMetalRoughGL3Shader( new Qt3DRender::QShaderProgram( this ) )
  , mFilterKey( new Qt3DRender::QFilterKey( this ) )
{}

QgsPBRMaterial::~QgsPBRMaterial() = default;

void QgsPBRMaterial::setBaseColor( const QColor &baseColor )
{
  mBaseColorParameter->setValue( Qgs3DUtils::srgbToLinear( baseColor ) );
  bool oldUsingBaseColorMap = mUsingBaseColorMap;

  mUsingBaseColorMap = false;
  if ( mEffect->parameters().contains( mBaseColorMapParameter ) )
    mEffect->removeParameter( mBaseColorMapParameter );
  mEffect->addParameter( mBaseColorParameter );

  if ( oldUsingBaseColorMap != mUsingBaseColorMap )
  {
    updateShaders();
  }
}

void QgsPBRMaterial::setBaseColorTexture( Qt3DRender::QAbstractTexture *baseColor )
{
  mBaseColorMapParameter->setValue( QVariant::fromValue( baseColor ) );
  bool oldUsingBaseColorMap = mUsingBaseColorMap;

  mUsingBaseColorMap = true;
  mEffect->addParameter( mBaseColorMapParameter );
  if ( mEffect->parameters().contains( mBaseColorParameter ) )
    mEffect->removeParameter( mBaseColorParameter );

  if ( oldUsingBaseColorMap != mUsingBaseColorMap )
  {
    updateShaders();
  }
}

void QgsPBRMaterial::setRoughness( float roughness )
{
  mRoughnessParameter->setValue( roughness );
  bool oldUsingRoughnessMap = mUsingRoughnessMap;

  mUsingRoughnessMap = false;
  if ( mEffect->parameters().contains( mRoughnessMapParameter ) )
    mEffect->removeParameter( mRoughnessMapParameter );
  mEffect->addParameter( mRoughnessParameter );

  if ( oldUsingRoughnessMap != mUsingRoughnessMap )
  {
    updateShaders();
  }
}

void QgsPBRMaterial::setRoughnessTexture( Qt3DRender::QAbstractTexture *roughness )
{
  mRoughnessMapParameter->setValue( QVariant::fromValue( roughness ) );
  bool oldUsingRoughnessMap = mUsingRoughnessMap;

  mUsingRoughnessMap = true;
  mEffect->addParameter( mRoughnessMapParameter );
  if ( mEffect->parameters().contains( mRoughnessParameter ) )
    mEffect->removeParameter( mRoughnessParameter );

  if ( oldUsingRoughnessMap != mUsingRoughnessMap )
  {
    updateShaders();
  }
}

void QgsPBRMaterial::setAmbientOcclusionTexture( Qt3DRender::QAbstractTexture *ambientOcclusion )
{
  bool oldUsingAmbientOcclusionMap = mUsingAmbientOcclusionMap;

  if ( ambientOcclusion )
  {
    mAmbientOcclusionMapParameter->setValue( QVariant::fromValue( ambientOcclusion ) );
    mUsingAmbientOcclusionMap = true;
    mEffect->addParameter( mAmbientOcclusionMapParameter );
  }
  else
  {
    mAmbientOcclusionMapParameter->setValue( QVariant() );
    mUsingAmbientOcclusionMap = false;
    if ( mEffect->parameters().contains( mAmbientOcclusionMapParameter ) )
      mEffect->removeParameter( mAmbientOcclusionMapParameter );
  }

  if ( oldUsingAmbientOcclusionMap != mUsingAmbientOcclusionMap )
  {
    updateShaders();
  }
}

void QgsPBRMaterial::setNormalTexture( Qt3DRender::QAbstractTexture *normal )
{
  bool oldUsingNormalMap = mUsingNormalMap;

  if ( normal )
  {
    mNormalMapParameter->setValue( QVariant::fromValue( normal ) );
    mUsingNormalMap = true;
    mEffect->addParameter( mNormalMapParameter );
  }
  else
  {
    mNormalMapParameter->setValue( QVariant() );
    mUsingNormalMap = false;
    if ( mEffect->parameters().contains( mNormalMapParameter ) )
      mEffect->removeParameter( mNormalMapParameter );
  }

  if ( oldUsingNormalMap != mUsingNormalMap )
  {
    updateShaders();
  }
}

void QgsPBRMaterial::setHeightTexture( Qt3DRender::QAbstractTexture *height )
{
  bool oldUsingHeightMap = mUsingHeightMap;

  if ( height )
  {
    mHeightMapParameter->setValue( QVariant::fromValue( height ) );
    mUsingHeightMap = true;
    mEffect->addParameter( mHeightMapParameter );
  }
  else
  {
    mHeightMapParameter->setValue( QVariant() );
    mUsingHeightMap = false;
    if ( mEffect->parameters().contains( mHeightMapParameter ) )
      mEffect->removeParameter( mHeightMapParameter );
  }

  if ( oldUsingHeightMap != mUsingHeightMap )
  {
    updateShaders();
  }
}

void QgsPBRMaterial::setParallaxScale( double scale )
{
  mParallaxScaleParameter->setValue( scale );
}

void QgsPBRMaterial::setEmissionColor( const QColor &color )
{
  mEmissiveColorParameter->setValue( Qgs3DUtils::srgbToLinear( color ) );
  const bool oldUsingEmissionMap = mUsingEmissionMap;

  mUsingEmissionMap = false;
  if ( mEffect->parameters().contains( mEmissionMapParameter ) )
    mEffect->removeParameter( mEmissionMapParameter );
  mEffect->addParameter( mEmissiveColorParameter );

  if ( oldUsingEmissionMap != mUsingEmissionMap )
  {
    updateShaders();
  }
}

void QgsPBRMaterial::setEmissionTexture( Qt3DRender::QAbstractTexture *emission )
{
  const bool oldUsingEmissionMap = mUsingEmissionMap;

  if ( emission )
  {
    mEmissionMapParameter->setValue( QVariant::fromValue( emission ) );
    mUsingEmissionMap = true;
    mEffect->addParameter( mEmissionMapParameter );
    if ( mEffect->parameters().contains( mEmissiveColorParameter ) )
      mEffect->removeParameter( mEmissiveColorParameter );
  }
  else
  {
    mEmissionMapParameter->setValue( QVariant() );
    mUsingEmissionMap = false;
    if ( mEffect->parameters().contains( mEmissionMapParameter ) )
      mEffect->removeParameter( mEmissionMapParameter );
    mEffect->addParameter( mEmissiveColorParameter );
  }

  if ( oldUsingEmissionMap != mUsingEmissionMap )
  {
    updateShaders();
  }
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
  mMetalRoughGL3Technique->graphicsApiFilter()->setApi( Qt3DRender::QGraphicsApiFilter::OpenGL );
  mMetalRoughGL3Technique->graphicsApiFilter()->setMajorVersion( 3 );
  mMetalRoughGL3Technique->graphicsApiFilter()->setMinorVersion( 3 );
  mMetalRoughGL3Technique->graphicsApiFilter()->setProfile( Qt3DRender::QGraphicsApiFilter::CoreProfile );

  mFilterKey->setParent( this );
  mFilterKey->setName( u"renderingStyle"_s );
  mFilterKey->setValue( u"forward"_s );

  mMetalRoughGL3Technique->addFilterKey( mFilterKey );
  mMetalRoughGL3RenderPass->setShaderProgram( mMetalRoughGL3Shader );
  mMetalRoughGL3Technique->addRenderPass( mMetalRoughGL3RenderPass );
  mEffect->addTechnique( mMetalRoughGL3Technique );

  // Given parameters a parent
  mBaseColorMapParameter->setParent( mEffect );
  mRoughnessMapParameter->setParent( mEffect );
  mNormalMapParameter->setParent( mEffect );
  mHeightMapParameter->setParent( mEffect );
  mAmbientOcclusionMapParameter->setParent( mEffect );
  mEmissionMapParameter->setParent( mEffect );

  mEffect->addParameter( mBaseColorParameter );
  mEffect->addParameter( mRoughnessParameter );
  mEffect->addParameter( mParallaxScaleParameter );
  mEffect->addParameter( mEmissiveColorParameter );
  mEffect->addParameter( mEmissionFactorParameter );
  mEffect->addParameter( mTextureScaleParameter );
  mEffect->addParameter( mTextureRotationParameter );
  mEffect->addParameter( mOpacityParameter );

  setEffect( mEffect );

  updateShaders();
}

QStringList QgsPBRMaterial::fragmentShaderDefines() const
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
  if ( mDataDefinedEnabled )
    defines += "DATA_DEFINED";
  return defines;
}

void QgsPBRMaterial::updateShaders()
{
  QByteArray fragmentShaderCode = Qt3DRender::QShaderProgram::loadSource( QUrl( u"qrc:/shaders/pbr.frag"_s ) );

  // pre-process fragment shader and add #defines based on whether using maps for some properties

  if ( mDataDefinedEnabled )
  {
    mMetalRoughGL3Shader->setShaderCode( Qt3DRender::QShaderProgram::Vertex, Qt3DRender::QShaderProgram::loadSource( QUrl( u"qrc:/shaders/pbrDataDefined.vert"_s ) ) );
  }
  else
  {
    const QByteArray vertexShaderCode = Qt3DRender::QShaderProgram::loadSource( QUrl( u"qrc:/shaders/default.vert"_s ) );
    const QByteArray finalVertexShaderCode = Qgs3DUtils::addDefinesToShaderCode( vertexShaderCode, { "TEXTURE_ROTATION" } );
    mMetalRoughGL3Shader->setVertexShaderCode( finalVertexShaderCode );
  }

  const QByteArray finalShaderCode = Qgs3DUtils::addDefinesToShaderCode( fragmentShaderCode, fragmentShaderDefines() );
  mMetalRoughGL3Shader->setFragmentShaderCode( finalShaderCode );
}

void QgsPBRMaterial::setFlatShadingEnabled( bool enabled )
{
  if ( enabled != mFlatShading )
  {
    mFlatShading = enabled;
    updateShaders();
  }
}

void QgsPBRMaterial::setOpacity( float opacity )
{
  mOpacityParameter->setValue( opacity );
}

void QgsPBRMaterial::setDataDefinedEnabled(bool enabled)
{
  if ( enabled != mDataDefinedEnabled )
  {
    mDataDefinedEnabled = enabled;
    updateShaders();
  }
}

void QgsPBRMaterial::setInstancingEnabled( bool enabled )
{
  if ( enabled == mInstancingEnabled )
    return;
  mInstancingEnabled = enabled;

  QByteArray vertexCode = Qt3DRender::QShaderProgram::loadSource( QUrl( u"qrc:/shaders/default.vert"_s ) );
  if ( enabled )
    vertexCode = Qgs3DUtils::addDefinesToShaderCode( vertexCode, QStringList( { u"INSTANCING"_s } ) );
  mMetalRoughGL3Shader->setVertexShaderCode( vertexCode );
}

///@endcond PRIVATE
