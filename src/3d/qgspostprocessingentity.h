/***************************************************************************
  qgspostprocessingentity.h
  --------------------------------------
  Date                 : August 2020
  Copyright            : (C) 2020 by Belgacem Nedjima
  Email                : gb underscore nedjima at esi dot dz
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSPOSTPROCESSINGENTITY_H
#define QGSPOSTPROCESSINGENTITY_H

#include "qgsrenderpassquad.h"

class QgsFrameGraph;

#define SIP_NO_FILE

/**
 * \ingroup 3d
 * \brief An entity that is responsible for applying post processing effect.
 *
 * \note Not available in Python bindings
 *
 * \since QGIS 3.16
 */
class QgsPostprocessingEntity : public QgsRenderPassQuad
{
    Q_OBJECT

  public:
    //! Constructor
    QgsPostprocessingEntity( QgsFrameGraph *frameGraph, Qt3DRender::QLayer *layer, QNode *parent = nullptr );
    //! Sets the parts of the scene where objects cast shadows
    void setupShadowRenderingExtent( float minX, float maxX, float minY, float maxY );
    //! Sets up a directional light that is used to render shadows
    void setupDirectionalLight( QVector3D position, QVector3D direction );
    //! Sets whether shadow rendering is enabled
    void setShadowRenderingEnabled( bool enabled );
    //! Sets the shadow bias value
    void setShadowBias( float shadowBias );
    //! Sets whether eye dome lighting is enabled
    void setEyeDomeLightingEnabled( bool enabled );
    //! Sets the eye dome lighting strength
    void setEyeDomeLightingStrength( double strength );
    //! Sets the eye dome lighting distance (contributes to the contrast of the image)
    void setEyeDomeLightingDistance( int distance );

    /**
     * Sets whether screen space ambient occlusion is enabled
     * \since QGIS 3.28
     */
    void setAmbientOcclusionEnabled( bool enabled );

    //! Sets whether vignette effect is enabled
    void setVignetteEnabled( bool enabled );
    //! Sets the intensity of the vignette effect (0-1)
    void setVignetteIntensity( float intensity );
    //! Sets the radius of the vignette effect (0-1)
    void setVignetteRadius( float radius );

    //! Sets whether tilt-shift effect is enabled
    void setTiltShiftEnabled( bool enabled );
    //! Sets the center line position of the tilt-shift effect (0-1)
    void setTiltShiftCenter( float center );
    //! Sets the width of the in-focus band (0-1)
    void setTiltShiftWidth( float width );
    //! Sets how quickly the blur increases outside the focus area (0-1)
    void setTiltShiftFalloff( float falloff );
    //! Sets the rotation angle of the tilt-shift plane in degrees
    void setTiltShiftRotation( float degrees );

    //! Sets whether film grain effect is enabled
    void setFilmGrainEnabled( bool enabled );
    //! Sets the intensity of film grain (0-1)
    void setFilmGrainAmount( float amount );
    //! Updates the time parameter for animated grain
    void updateTime( float time );

  private:
    Qt3DRender::QCamera *mMainCamera = nullptr;

    Qt3DRender::QParameter *mColorTextureParameter = nullptr;
    Qt3DRender::QParameter *mDepthTextureParameter = nullptr;
    Qt3DRender::QParameter *mShadowMapParameter = nullptr;
    Qt3DRender::QParameter *mAmbientOcclusionTextureParameter = nullptr;
    Qt3DRender::QParameter *mFarPlaneParameter = nullptr;
    Qt3DRender::QParameter *mNearPlaneParameter = nullptr;
    Qt3DRender::QParameter *mMainCameraInvViewMatrixParameter = nullptr;
    Qt3DRender::QParameter *mMainCameraInvProjMatrixParameter = nullptr;

    Qt3DRender::QCamera *mLightCamera = nullptr;
    Qt3DRender::QParameter *mLightFarPlaneParameter = nullptr;
    Qt3DRender::QParameter *mLightNearPlaneParameter = nullptr;

    Qt3DRender::QParameter *mLightPosition = nullptr;
    Qt3DRender::QParameter *mLightDirection = nullptr;

    Qt3DRender::QParameter *mShadowMinX = nullptr;
    Qt3DRender::QParameter *mShadowMaxX = nullptr;
    Qt3DRender::QParameter *mShadowMinY = nullptr;
    Qt3DRender::QParameter *mShadowMaxY = nullptr;

    Qt3DRender::QParameter *mRenderShadowsParameter = nullptr;
    Qt3DRender::QParameter *mShadowBiasParameter = nullptr;
    Qt3DRender::QParameter *mEyeDomeLightingEnabledParameter = nullptr;
    Qt3DRender::QParameter *mEyeDomeLightingStrengthParameter = nullptr;
    Qt3DRender::QParameter *mEyeDomeLightingDistanceParameter = nullptr;

    Qt3DRender::QParameter *mAmbientOcclusionEnabledParameter = nullptr;

    Qt3DRender::QParameter *mFogEnabledParameter = nullptr;
    Qt3DRender::QParameter *mFogColorParameter = nullptr;
    Qt3DRender::QParameter *mFogStartHeightParameter = nullptr;
    Qt3DRender::QParameter *mFogFalloffParameter = nullptr;
    Qt3DRender::QParameter *mFogDensityParameter = nullptr;
    Qt3DRender::QParameter *mCameraPositionParameter = nullptr;

};

#endif // QGSPOSTPROCESSINGENTITY_H
