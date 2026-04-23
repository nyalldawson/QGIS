/***************************************************************************
  qgsanimationsettingsbase.h
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

#ifndef QGSANIMATIONSETTINGSBASE_H
#define QGSANIMATIONSETTINGSBASE_H

#include "qgis_core.h"

#include <QEasingCurve>
#include <QVector>

class QDomDocument;
class QDomElement;
class QgsReadWriteContext;


/**
 * \ingroup core
 * \brief Base template class for animation properties.
 *
 * Handles key frame duration, lookup, and easing curve calculations.
 *
 * \since QGIS 4.2
 */
template<typename KeyFrameT> class CORE_EXPORT QgsAnimationSettingsBase
{
  public:
    virtual ~QgsAnimationSettingsBase() {};

    /**
     * Configures \a keyFrames of the animation.
     *
     * \note Key frames must be ordered by time.
     * \see keyFrames()
     */
    void setKeyFrames( const QVector<KeyFrameT> &keyFrames ) { mKeyFrames = keyFrames; }

    /**
     * Returns the key frames of the animation.
     *
     * \see setKeyFrames()
     */
    QVector<KeyFrameT> keyFrames() const { return mKeyFrames; }

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
    double duration() const { return mKeyFrames.isEmpty() ? 0 : mKeyFrames.constLast().time; };

    /**
     * Finds the correct key frames and calculates the interpolation between them for a given point in \a time (in seconds).
     */
    KeyFrameT interpolate( double time ) const
    {
      if ( mKeyFrames.isEmpty() )
        return KeyFrameT();

      if ( time < mKeyFrames.constFirst().time )
      {
        return mKeyFrames.first();
      }
      else if ( time >= mKeyFrames.constLast().time )
      {
        return mKeyFrames.last();
      }
      else
      {
        // TODO: make easing curves configurable.
        // QEasingCurve is probably not flexible enough, we may need more granular
        // control with Bezier curves to allow smooth transition at keyframes
        for ( int i = 0; i < mKeyFrames.size() - 1; i++ )
        {
          const KeyFrameT &k0 = mKeyFrames.at( i );
          const KeyFrameT &k1 = mKeyFrames.at( i + 1 );
          if ( time >= k0.time && time <= k1.time )
          {
            const double ip = ( time - k0.time ) / ( k1.time - k0.time );
            const double eIp = mEasingCurve.valueForProgress( ip );
            return interpolateKeyFrame( k0, k1, eIp, time );
          }
        }
      }
      Q_ASSERT( false );
      return KeyFrameT();
    }

    /**
     * Reads configuration from a DOM element previously written by writeXml().
     *
     * \since writeXml()
     */
    virtual void readXml( const QDomElement &elem, QgsReadWriteContext &context ) = 0;

    /**
     * Writes configuration to a DOM element, to be used later with readXml().
     *
     * \see readXml()
     */
    virtual QDomElement writeXml( QDomDocument &doc, QgsReadWriteContext &context ) const = 0;

  protected:
    //! Subclasses must implement this to blend their specific properties
    virtual KeyFrameT interpolateKeyFrame( const KeyFrameT &k0, const KeyFrameT &k1, double eIp, double time ) const = 0;

    QVector<KeyFrameT> mKeyFrames;
    QEasingCurve mEasingCurve;
};

#endif // QGSANIMATIONSETTINGSBASE_H
