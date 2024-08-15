/***************************************************************************
  qgsterrainsettings.h
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

#ifndef QGSTERRAINSETTINGS_H
#define QGSTERRAINSETTINGS_H

#include "qgis_3d.h"
#include "qgis_sip.h"
#include <QString>

class QDomElement;
class QgsReadWriteContext;
class QgsProject;

/**
 * \ingroup 3d
 * \brief Base class for all terrain settings classes.
 *
 * QgsAbstractTerrainSettings subclasses are responsible for storing the configuration
 * of terrain generators.
 *
 * \warning This is not considered stable API, and may change in future QGIS releases. It is
 * exposed to the Python bindings as a tech preview only.
 *
 * \since QGIS 3.40
 */
class _3D_EXPORT QgsAbstractTerrainSettings
{

#ifdef SIP_RUN
    SIP_CONVERT_TO_SUBCLASS_CODE
    if ( sipCpp->type() == "flat" )
      sipType = sipType_QgsFlatTerrainSettings;
    else
      sipType = 0;
    SIP_END
#endif

  public:

    virtual ~QgsAbstractTerrainSettings();

    /**
     * Returns a copy of the terrain settings.
     */
    virtual QgsAbstractTerrainSettings *clone() const = 0 SIP_FACTORY;

    /**
     * Returns the unique type name for the terrain generator.
     */
    virtual QString type() const = 0;

    /**
     * Reads settings from a DOM \a element.
     *
     * \see resolveReferences()
     * \see writeXml()
     */
    virtual void readXml( const QDomElement &element, const QgsReadWriteContext &context ) = 0;

    /**
     * Writes settings to a DOM \a element
     *
     * \see readXml()
     */
    virtual void writeXml( QDomElement &element, const QgsReadWriteContext &context ) const = 0;

    /**
     * After reading settings from XML, resolves references to any layers in a \a project that have been read as layer IDs.
     *
     * \see readXml()
     */
    virtual void resolveReferences( const QgsProject *project );

};


#endif // QGSTERRAINSETTINGS_H
