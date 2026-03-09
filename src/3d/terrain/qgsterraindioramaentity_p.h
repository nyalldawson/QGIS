/***************************************************************************
  qgsterraindioramaentity_p.h
  --------------------------------------
  Date                 : March 2026
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

#ifndef QGSTERRAINDIORAMAENTITY_P_H
#define QGSTERRAINDIORAMAENTITY_P_H

///@cond PRIVATE

//
//  W A R N I N G
//  -------------
//
// This file is not part of the QGIS API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//


#include <Qt3DCore/QEntity>

#define SIP_NO_FILE

class Qgs3DMapSettings;

/**
 * \ingroup qgis_3d
 * \brief Entity that renders the bottom face of the diorama.
 *
 * The diorama walls are created per-tile by QgsDemTerrainTileLoader so that
 * they exactly match the terrain edge vertices. This entity only handles
 * the bottom quad that seals the base of the diorama.
 *
 * \since QGIS 4.0
 */
class QgsTerrainDioramaEntity : public Qt3DCore::QEntity
{
    Q_OBJECT

  public:
    /**
   * Constructs a diorama bottom entity for the given map \a settings.
   */
    QgsTerrainDioramaEntity( const Qgs3DMapSettings &mapSettings, Qt3DCore::QNode *parent = nullptr );

  private:
    void buildGeometry( const Qgs3DMapSettings &mapSettings );
};

/// @endcond

#endif // QGSTERRAINDIORAMAENTITY_P_H
