/***************************************************************************
  qgs3danimationsettings.h
  --------------------------------------
  Date                 : July 2018
  Copyright            : (C) 2018 by Martin Dobias
  Email                : wonder dot sk at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGS3DANIMATIONSETTINGS_H
#define QGS3DANIMATIONSETTINGS_H

#include "qgis_3d.h"
#include "qgsanimationsettingsbase.h"
#include "qgsvector3d.h"

#define SIP_NO_FILE

/**
 * \ingroup qgis_3d
 * \brief Represents a key frame in a 3D animation.
 *
 * \note Not available in Python bindings
 * \since QGIS 4.2
 */
struct Qgs3DAnimationKeyFrame
{
    double time = 0;   //!< Relative time of the keyframe in seconds
    QgsVector3D point; //!< Point towards which the camera is looking in 3D map coords
    float dist = 0;    //!< Distance of the camera from the focal point
    float pitch = 0;   //!< Tilt of the camera in degrees (0 = looking from the top, 90 = looking from the side, 180 = looking from the bottom)
    float yaw = 0;     //!< Horizontal rotation around the focal point in degrees
};


/**
 * \ingroup qgis_3d
 * \brief Holds information about animation in 3D view.
 *
 * The animation is defined as a series of keyframes.
 * \note Not available in Python bindings
 * \since QGIS 3.8
 */
class _3D_EXPORT Qgs3DAnimationSettings : public QgsAnimationSettingsBase<Qgs3DAnimationKeyFrame>
{
  public:
    Qgs3DAnimationSettings();

    void readXml( const QDomElement &elem, QgsReadWriteContext &context ) override;
    QDomElement writeXml( QDomDocument &doc, QgsReadWriteContext &context ) const override;

  protected:
    Qgs3DAnimationKeyFrame interpolateKeyFrame( const Qgs3DAnimationKeyFrame &k0, const Qgs3DAnimationKeyFrame &k1, double eIp, double time ) const override;
};

Q_DECLARE_METATYPE( Qgs3DAnimationKeyFrame )

#endif // QGS3DANIMATIONSETTINGS_H
