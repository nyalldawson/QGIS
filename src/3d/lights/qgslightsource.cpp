/***************************************************************************
                          qgslightsource.cpp
                          -----------------
    begin                : April 2022
    copyright            : (C) 2022 by Nyall Dawson
    email                : nyall dot dawson at gmail dot com
***************************************************************************/


/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgslightsource.h"
#include <mutex>

QgsPropertiesDefinition QgsLightSource::sPropertyDefinitions;

QgsLightSource::~QgsLightSource() = default;

void QgsLightSource::resolveReferences( const QgsProject & )
{

}

QgsPropertiesDefinition QgsLightSource::propertyDefinitions()
{
  static std::once_flag initialized;
  std::call_once( initialized, [ = ]( )
  {
    initPropertyDefinitions();
  } );
  return sPropertyDefinitions;
}

void QgsLightSource::writeCommonProperties( QDomElement &element, QDomDocument &doc, const QgsReadWriteContext & ) const
{
  QDomElement elemDataDefinedProperties = doc.createElement( QStringLiteral( "data-defined-properties" ) );
  mDataDefinedProperties.writeXml( elemDataDefinedProperties, propertyDefinitions() );
  element.appendChild( elemDataDefinedProperties );
}

void QgsLightSource::readCommonProperties( const QDomElement &element, const QgsReadWriteContext & )
{
  const QDomElement elemDataDefinedProperties = element.firstChildElement( QStringLiteral( "data-defined-properties" ) );
  if ( !elemDataDefinedProperties.isNull() )
    mDataDefinedProperties.readXml( elemDataDefinedProperties, propertyDefinitions() );
}

void QgsLightSource::initPropertyDefinitions()
{
  const QString origin = QStringLiteral( "lighting" );

  sPropertyDefinitions = QgsPropertiesDefinition
  {
    { QgsLightSource::Color, QgsPropertyDefinition( "Color", QObject::tr( "Color" ), QgsPropertyDefinition::ColorNoAlpha, origin ) },
    { QgsLightSource::Intensity, QgsPropertyDefinition( "Intensity", QObject::tr( "Intensity" ), QgsPropertyDefinition::DoublePositive, origin ) },
  };
}

