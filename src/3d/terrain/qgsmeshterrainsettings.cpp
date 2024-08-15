/***************************************************************************
  qgsmeshterrainsettings.cpp
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

#include "qgsmeshterrainsettings.h"
#include "qgsmeshlayer.h"

QgsMeshTerrainSettings *QgsMeshTerrainSettings::clone() const
{
  return new QgsMeshTerrainSettings( *this );
}

QString QgsMeshTerrainSettings::type() const
{
  return QStringLiteral( "mesh" );
}

void QgsMeshTerrainSettings::readXml( const QDomElement &element, const QgsReadWriteContext & )
{
  mLayer = QgsMapLayerRef( element.attribute( QStringLiteral( "layer" ) ) );
}

void QgsMeshTerrainSettings::writeXml( QDomElement &element, const QgsReadWriteContext & ) const
{
  element.setAttribute( QStringLiteral( "layer" ), mLayer.layerId );
}

void QgsMeshTerrainSettings::resolveReferences( const QgsProject *project )
{
  mLayer.resolve( project );
}

void QgsMeshTerrainSettings::setLayer( QgsMeshLayer *layer )
{
  mLayer = QgsMapLayerRef( layer );
}

QgsMeshLayer *QgsMeshTerrainSettings::layer() const
{
  return qobject_cast<QgsMeshLayer *>( mLayer.layer.data() );
}
