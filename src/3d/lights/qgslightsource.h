/***************************************************************************
                          qgslightsource.h
                          ---------------
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

#ifndef QGSLIGHTSOURCE_H
#define QGSLIGHTSOURCE_H

#include "qgis_3d.h"
#include "qgis_sip.h"
#include "qgspropertycollection.h"
#include "qgsreadwritecontext.h"

#include <QList>

class Qgs3DMapSettings;
class QgsProject;
class QDomElement;
class QDomDocument;

#ifndef SIP_RUN
namespace Qt3DCore
{
  class QEntity;
}
#endif

/**
 * \ingroup 3d
 * \brief Base class for light sources in 3d scenes.
 *
 * \since QGIS 3.26
 */
class _3D_EXPORT QgsLightSource SIP_ABSTRACT
{

  public:

    /**
     * Data definable properties.
     */
    enum Property
    {
      Color, //! Light color
      Intensity, //!< Light intensity
    };


    virtual ~QgsLightSource();

    /**
     * Creates entities representing the light source.
     */
    virtual QList< Qt3DCore::QEntity * > createEntities( const Qgs3DMapSettings &map, Qt3DCore::QEntity *parent ) const = 0 SIP_SKIP;

    /**
     * Writes the light source's configuration to a new DOM element and returns it.
     *
     * \see readXml()
     */
    virtual QDomElement writeXml( QDomDocument &doc, const QgsReadWriteContext &context = QgsReadWriteContext() ) const = 0;

    /**
     * Reads configuration from a DOM element previously written using writeXml().
     *
     * \see writeXml()
     */
    virtual void readXml( const QDomElement &elem, const QgsReadWriteContext &context = QgsReadWriteContext() ) = 0;

    /**
     * After reading from XML, resolve references to any layers that have been read as layer IDs.
     */
    virtual void resolveReferences( const QgsProject &project );

    /**
     * Returns a reference to the object's property collection, used for data defined overrides.
     * \see setDataDefinedProperties()
     */
    QgsPropertyCollection &dataDefinedProperties() { return mDataDefinedProperties; }

    /**
     * Returns a reference to the object's property collection, used for data defined overrides.
     * \see setDataDefinedProperties()
     * \see Property
     * \note not available in Python bindings
     */
    const QgsPropertyCollection &dataDefinedProperties() const SIP_SKIP { return mDataDefinedProperties; }

    /**
     * Sets the object's property \a collection, used for data defined overrides.
     *
     * Any existing properties will be discarded.
     *
     * \see dataDefinedProperties()
     * \see Property
     */
    void setDataDefinedProperties( const QgsPropertyCollection &collection ) { mDataDefinedProperties = collection; }

    /**
     * Returns the definitions for data defined properties available for use in elevation properties.
     */
    static QgsPropertiesDefinition propertyDefinitions();

  protected:

    //! Property collection for data defined elevation settings
    QgsPropertyCollection mDataDefinedProperties;

    //! Property definitions
    static QgsPropertiesDefinition sPropertyDefinitions;

    /**
     * Writes common class properties to a DOM \a element, to be used later with readXml().
     *
     * \see readCommonProperties()
     */
    void writeCommonProperties( QDomElement &element, QDomDocument &doc, const QgsReadWriteContext &context ) const;

    /**
     * Reads common class properties from a DOM \a element previously written by writeXml().
     *
     * \see writeCommonProperties()
     */
    void readCommonProperties( const QDomElement &element, const QgsReadWriteContext &context );

  private:

    /**
     * Initializes property definitions.
     */
    static void initPropertyDefinitions();

};


#endif // QGSLIGHTSOURCE_H
