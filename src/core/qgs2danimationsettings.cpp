/***************************************************************************
  qgs2danimationsettings.cpp
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

#include "qgs2danimationsettings.h"

#include <QDomDocument>
#include <QEasingCurve>
#include <QString>

using namespace Qt::StringLiterals;

Qgs2DAnimationSettings::Qgs2DAnimationSettings() = default;

Qgs2DAnimationKeyFrame Qgs2DAnimationSettings::interpolateKeyFrame( const Qgs2DAnimationKeyFrame &k0, const Qgs2DAnimationKeyFrame &k1, double eIp, double time ) const
{
  const double eIip = 1.0f - eIp;
  Qgs2DAnimationKeyFrame kf;
  kf.time = time;
  kf.center.set( k0.center.x() * eIip + k1.center.x() * eIp, k0.center.y() * eIip + k1.center.y() * eIp );
  kf.scale = k0.scale * eIip + k1.scale * eIp;
  kf.rotation = k0.rotation * eIip + k1.rotation * eIp;
  return kf;
}

void Qgs2DAnimationSettings::readXml( const QDomElement &elem, QgsReadWriteContext & )
{
  mEasingCurve = QEasingCurve( ( QEasingCurve::Type ) elem.attribute( u"interpolation"_s, u"0"_s ).toInt() );

  mKeyFrames.clear();

  const QDomElement elemKeyframes = elem.firstChildElement( u"keyframes"_s );
  QDomElement elemKeyframe = elemKeyframes.firstChildElement( u"keyframe"_s );
  while ( !elemKeyframe.isNull() )
  {
    Qgs2DAnimationKeyFrame kf;
    kf.time = elemKeyframe.attribute( u"time"_s ).toFloat();
    kf.center.set( elemKeyframe.attribute( u"x"_s ).toDouble(), elemKeyframe.attribute( u"y"_s ).toDouble() );
    kf.scale = elemKeyframe.attribute( u"scale"_s ).toFloat();
    kf.rotation = elemKeyframe.attribute( u"rotation"_s ).toFloat();
    mKeyFrames.append( kf );
    elemKeyframe = elemKeyframe.nextSiblingElement( u"keyframe"_s );
  }
}

QDomElement Qgs2DAnimationSettings::writeXml( QDomDocument &doc, QgsReadWriteContext & ) const
{
  QDomElement elem = doc.createElement( u"animation3d"_s );
  elem.setAttribute( u"interpolation"_s, mEasingCurve.type() );

  QDomElement elemKeyframes = doc.createElement( u"keyframes"_s );

  for ( const Qgs2DAnimationKeyFrame &keyframe : mKeyFrames )
  {
    QDomElement elemKeyframe = doc.createElement( u"keyframe"_s );
    elemKeyframe.setAttribute( u"time"_s, keyframe.time );
    elemKeyframe.setAttribute( u"x"_s, keyframe.center.x() );
    elemKeyframe.setAttribute( u"y"_s, keyframe.center.y() );
    elemKeyframe.setAttribute( u"scale"_s, keyframe.scale );
    elemKeyframe.setAttribute( u"rotation"_s, keyframe.rotation );
    elemKeyframes.appendChild( elemKeyframe );
  }

  elem.appendChild( elemKeyframes );

  return elem;
}
