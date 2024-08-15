/***************************************************************************
  qgsdemterrainsettings.cpp
  --------------------------------------
  Date                 : August 2024
  Copyright            : (C) 2024 by Nyall Dawson
  Email                : nyall dot dawson at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsdemterrainsettings.h"
#include "qgsrasterlayer.h"

QgsDemTerrainSettings *QgsDemTerrainSettings::clone() const
{
  return new QgsDemTerrainSettings( *this );
}

QString QgsDemTerrainSettings::type() const
{
  return QStringLiteral( "dem" );
}

void QgsDemTerrainSettings::readXml( const QDomElement &element, const QgsReadWriteContext & )
{
  mLayer = QgsMapLayerRef( element.attribute( QStringLiteral( "layer" ) ) );
  mResolution = element.attribute( QStringLiteral( "resolution" ) ).toInt();
  mSkirtHeight = element.attribute( QStringLiteral( "skirt-height" ) ).toDouble();
}

void QgsDemTerrainSettings::writeXml( QDomElement &element, const QgsReadWriteContext & ) const
{
  element.setAttribute( QStringLiteral( "layer" ), mLayer.layerId );
  element.setAttribute( QStringLiteral( "resolution" ), mResolution );
  element.setAttribute( QStringLiteral( "skirt-height" ), mSkirtHeight );
}

void QgsDemTerrainSettings::resolveReferences( const QgsProject *project )
{
  mLayer.resolve( project );
}

void QgsDemTerrainSettings::setLayer( QgsRasterLayer *layer )
{
  mLayer = QgsMapLayerRef( layer );
}

QgsRasterLayer *QgsDemTerrainSettings::layer() const
{
  return qobject_cast<QgsRasterLayer *>( mLayer.layer.data() );
}
