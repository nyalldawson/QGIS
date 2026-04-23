/***************************************************************************
  qgs2danimationsettings.h
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

#ifndef QGS2DANIMATIONSETTINGS_H
#define QGS2DANIMATIONSETTINGS_H

#include "qgis_core.h"

#ifndef SIP_RUN
#include "qgsanimationsettingsbase.h"
#endif
#include "qgspointxy.h"

/**
 * \ingroup core
 * \brief Represents a key frame in a 2D animation.
 *
 * \since QGIS 4.2
 */
struct CORE_EXPORT Qgs2DAnimationKeyFrame
{
    //! Relative time of the keyframe in seconds
    double time = 0;
    //! Center point of the map
    QgsPointXY center;
    //! Map scale denominator, e.g. 1000.0 for a 1:1000 map.
    double scale = 0;
    //! Map rotation
    double rotation = 0;
};

/**
 * \ingroup core
 * \brief Holds information about animation in 2D view.
 *
 * The animation is defined as a series of keyframes.
 * \since QGIS 4.2
 */
#ifndef SIP_RUN
class CORE_EXPORT Qgs2DAnimationSettings : public QgsAnimationSettingsBase<Qgs2DAnimationKeyFrame>
{
#else
class CORE_EXPORT Qgs2DAnimationSettings
{
#endif

  public:
    Qgs2DAnimationSettings();

    void readXml( const QDomElement &elem, QgsReadWriteContext &context ) override;
    QDomElement writeXml( QDomDocument &doc, QgsReadWriteContext &context ) const override;
#ifdef SIP_RUN


    /**
     * Configures \a keyFrames of the animation.
     *
     * \note Key frames must be ordered by time.
     * \see keyFrames()
     */
    void setKeyFrames( const QVector<Qgs2DAnimationKeyFrame> &keyFrames );

    /**
     * Returns the key frames of the animation.
     *
     * \see setKeyFrames()
     */
    QVector<Qgs2DAnimationKeyFrame> keyFrames() const;

    /**
     * Sets the interpolation method for transitions.
     *
     * \see easingCurve()
     */
    void setEasingCurve( const QEasingCurve &curve ) { mEasingCurve = curve; }

    /**
     * Returns the interpolation method for transitions.
     *
     * \see setEasingCurve()
     */
    QEasingCurve easingCurve() const { return mEasingCurve; }

    /**
     * Returns the duration of the whole animation (in seconds).
     */
    double duration() const;

    /**
     * Finds the correct keyframes and calculates the interpolation progress.
     */
    Qgs2DAnimationKeyFrame interpolate( double time ) const;
#endif
  protected:
    Qgs2DAnimationKeyFrame interpolateKeyFrame( const Qgs2DAnimationKeyFrame &k0, const Qgs2DAnimationKeyFrame &k1, double eIp, double time ) const override;
};

Q_DECLARE_METATYPE( Qgs2DAnimationKeyFrame )

#endif // QGS2DANIMATIONSETTINGS_H
