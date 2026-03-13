/***************************************************************************
  qgs3dmapexportsettings.h
  --------------------------------------
  Date                 : July 2020
  Copyright            : (C) 2020 by Belgacem Nedjima
  Email                : gb underscore nedjima at esi dot dz
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGS3DMAPEXPORTSETTINGS_H
#define QGS3DMAPEXPORTSETTINGS_H

#include "qgis_3d.h"

#include <QDir>
#include <QObject>
#include <QString>

/**
 * \brief Manages the various settings the user can choose from when exporting a 3D scene.
 * \ingroup qgis_3d
 * \since QGIS 3.16
 */
class _3D_EXPORT Qgs3DMapExportSettings
{
  public:
    //! Constructor
    Qgs3DMapExportSettings();

    //! destructor (save the export settings before deallocation)
    ~Qgs3DMapExportSettings();

    //! Returns the scene name
    QString sceneName() const { return mSceneName; }
    //! Returns the scene folder path
    QString sceneFolderPath() const { return mSceneFolderPath; }
    //! Returns the terrain resolution
    int terrrainResolution() const { return mTerrainResolution; }
    //! Returns whether triangles edges will look smooth
    bool smoothEdges() const { return mSmoothEdges; }
    //! Returns whether normals will be exported
    bool exportNormals() const { return mExportNormals; }
    //! Returns whether textures will be exported
    bool exportTextures() const { return mExportTextures; }
    //! Returns the terrain texture resolution
    int terrainTextureResolution() const { return mTerrainTextureResolution; }
    //! Returns the scale of the exported model
    float scale() const { return mScale; }

    /**
     * Returns whether terrain export is enabled.
     * It terrain export is disabled, the terrain resolution and terrain texture resolution
     * parameters have no effect.
     *
     * \see setTerrainExportEnabled()
     * \since QGIS 4.0
     */
    bool terrainExportEnabled() const { return mTerrainExportEnabled; }

    //! Sets the scene name
    void setSceneName( const QString &sceneName ) { mSceneName = sceneName; }
    //! Sets the scene's .obj file folder path
    void setSceneFolderPath( const QString &sceneFolderPath ) { mSceneFolderPath = sceneFolderPath; }
    //! Sets the terrain resolution
    void setTerrainResolution( int resolution ) { mTerrainResolution = resolution; }
    //! Sets whether triangles edges will look smooth
    void setSmoothEdges( bool smoothEdges ) { mSmoothEdges = smoothEdges; }
    //! Sets whether normals should be exported
    void setExportNormals( bool exportNormals ) { mExportNormals = exportNormals; }
    //! Sets whether textures will be exported
    void setExportTextures( bool exportTextures ) { mExportTextures = exportTextures; }
    //! Sets the terrain texture resolution
    void setTerrainTextureResolution( int resolution ) { mTerrainTextureResolution = resolution; }
    //! Sets the scale of exported model
    void setScale( float scale ) { mScale = scale; }

    /**
     * Sets whether terrain export is enabled.
     *
     * \see terrainExportEnabled()
     * \since QGIS 4.0
     */
    void setTerrainExportEnabled( bool enabled ) { mTerrainExportEnabled = enabled; }

    /**
     * Returns whether the terrain should be exported in diorama mode.
     *
     * When enabled, the terrain surface will be exported without skirts and
     * will include walls extending down to dioramaHeight() and a bottom face,
     * forming a closed mesh suitable for 3D printing.
     *
     * \see setDioramaExportEnabled()
     * \see dioramaHeight()
     *
     * \since QGIS 4.2
     */
    bool dioramaExportEnabled() const { return mDioramaExportEnabled; }

    /**
     * Sets whether the terrain should be exported in diorama mode.
     *
     * \see dioramaExportEnabled()
     * \since QGIS 4.2
     */
    void setDioramaExportEnabled( bool enabled ) { mDioramaExportEnabled = enabled; }

    /**
     * Returns the base height (in map units) for the diorama export.
     *
     * The walls and bottom face will extend down to this Z value.
     *
     * \see setDioramaHeight()
     * \see dioramaExportEnabled()
     * \since QGIS 4.2
     */
    double dioramaHeight() const { return mDioramaHeight; }

    /**
     * Sets the base \a height (in map units) for the diorama export.
     *
     * \see dioramaHeight()
     * \since QGIS 4.2
     */
    void setDioramaHeight( double height ) { mDioramaHeight = height; }

    /**
     * Returns the terrain tile zoom level for export.
     *
     * A value of 0 means export as a single tile (the default).
     *
     * Higher values produce more tiles at higher resolution. The actual
     * number of tiles is 4^zoomLevel (quadtree subdivision).
     *
     * \see setTerrainTileZoomLevel()
     * \since QGIS 4.2
     */
    int terrainTileZoomLevel() const { return mTerrainTileZoomLevel; }

    /**
     * Sets the terrain tile zoom \a level for export.
     *
     * \see terrainTileZoomLevel()
     * \since QGIS 4.2
     */
    void setTerrainTileZoomLevel( int level ) { mTerrainTileZoomLevel = level; }

  private:
    QString mSceneName = QString( "Scene" );
    QString mSceneFolderPath = QDir::homePath();
    int mTerrainResolution = 128;
    bool mSmoothEdges = false;
    bool mExportNormals = true;
    bool mExportTextures = false;
    int mTerrainTextureResolution = 512;
    float mScale = 1.0f;
    bool mTerrainExportEnabled = true;
    bool mDioramaExportEnabled = true;
    double mDioramaHeight = 0.0;
    int mTerrainTileZoomLevel = 0;
};

#endif // QGS3DMAPEXPORTSETTINGS_H
