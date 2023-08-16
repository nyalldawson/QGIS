/***************************************************************************
                         qgstiledscenelayerrenderer.h
                         --------------------
    begin                : June 2023
    copyright            : (C) 2023 by Nyall Dawson
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

#ifndef QGSTILEDSCENELAYERRENDERER_H
#define QGSTILEDSCENELAYERRENDERER_H

#include "qgis_core.h"
#include "qgsmaplayerrenderer.h"
#include "qgscoordinatereferencesystem.h"
#include "qgstiledsceneboundingvolume.h"
#include "qgstiledsceneindex.h"
#include "qgstiledscenerequest.h"
#include "qgstiledscenetile.h"

#include <memory>
#include <QElapsedTimer>
#include <QSet>
#include <QImage>
#include <QHash>
#include <QThread>
#include <QSemaphore>

#define SIP_NO_FILE

class QgsTiledSceneLayer;
class QgsFeedback;
class QgsMapClippingRegion;
class QgsTiledSceneRenderer;
class QgsTiledSceneRenderContext;

namespace tinygltf
{
  struct Image;
  class Model;
  class Node;
  class TinyGLTF;
  struct Primitive;
}

/**
 * \ingroup core
 *
 * \brief Implementation of threaded 2D rendering for tiled scene layers.
 *
 * \note The API is considered EXPERIMENTAL and can be changed without a notice
 * \note Not available in Python bindings
 *
 * \since QGIS 3.34
 */
class CORE_EXPORT QgsTiledSceneLayerRenderer: public QgsMapLayerRenderer
{
  public:

    //! Ctor
    explicit QgsTiledSceneLayerRenderer( QgsTiledSceneLayer *layer, QgsRenderContext &context );
    ~QgsTiledSceneLayerRenderer();

    bool render() override;
    Qgis::MapLayerRendererFlags flags() const override;
    bool forceRasterRender() const override;
    QgsFeedback *feedback() const override { return mFeedback.get(); }

  private:

    QgsTiledSceneRequest createBaseRequest();

    bool renderTiles( QgsTiledSceneRenderContext &context );

    void renderTile( const QgsTiledSceneTile &tile, const QByteArray &content, QgsTiledSceneRenderContext &context );

    /**
     * Renders the content for a \a tile.
     *
     * Returns TRUE if the tile had content to render, or FALSE if it is an empty tile.
     */
    bool renderTileContent( const QgsTiledSceneTile &tile, const QByteArray &content, QgsTiledSceneRenderContext &context );

    void renderPrimitive( const tinygltf::Model &model,
                          const tinygltf::Primitive &primitive,
                          const QgsTiledSceneTile &tile,
                          const QgsVector3D &tileTranslationEcef,
                          const QMatrix4x4 *gltfLocalTransform,
                          QgsTiledSceneRenderContext &context );

    void renderTrianglePrimitive( const tinygltf::Model &model,
                                  const tinygltf::Primitive &primitive,
                                  const QgsTiledSceneTile &tile,
                                  const QgsVector3D &tileTranslationEcef,
                                  const QMatrix4x4 *gltfLocalTransform,
                                  QgsTiledSceneRenderContext &context );


    std::unique_ptr< QgsTiledSceneRenderer > mRenderer;
    bool mRenderTileBorders = false;

    QList< QgsMapClippingRegion > mClippingRegions;

    QgsCoordinateReferenceSystem mSceneCrs;
    QgsTiledSceneBoundingVolume mLayerBoundingVolume;
    QgsTiledSceneIndex mIndex;

    QgsCoordinateTransform mSceneToMapTransform;

    struct TriangleData
    {
      QPolygonF triangle;
      float z;
      QPair< int, int > textureId { -1, -1 };
      float textureCoords[6];
    };

    QVector< TriangleData > mTriangleData;
    int mCurrentModelId = 0;
    QHash< QPair< int, int >, QImage > mTextures;

    struct TileDetails
    {
      QPolygonF boundary;
      bool hasContent = false;
    };
    QVector< TileDetails > mTileDetails;

    std::unique_ptr<QgsFeedback> mFeedback;
    QSet< int > mWarnedPrimitiveTypes;

    QElapsedTimer mElapsedTimer;

    // shared objects with producer

    friend class TileLayerRendererProducer;

    static const int sBufferSize = 30;
    struct QueuedTileContent
    {
      QgsTiledSceneTile tile;
      QByteArray content;
    };

    QueuedTileContent mBuffer[sBufferSize];

    QSemaphore mFreeSemaphore = QSemaphore( sBufferSize );
    QSemaphore mUsedSemaphore;
};

class TileLayerRendererProducer : public QThread
{
  public:
    TileLayerRendererProducer( QgsTiledSceneLayerRenderer &consumer,
                               const QgsTiledSceneRequest &request,
                               const QgsTiledSceneIndex &index,
                               QgsFeedback *feedback,
                               const std::function< bool ( const QgsTiledSceneTile &tile ) > &isVisibleFunction );
    void run() override;

  private:
    QgsTiledSceneLayerRenderer &mConsumer;
    QgsTiledSceneRequest mRequest;
    QgsTiledSceneIndex mIndex;
    QgsFeedback *mFeedback = nullptr;
    const std::function< bool ( const QgsTiledSceneTile &tile ) > mIsVisibleFunction;
};

#endif // QGSTILEDSCENELAYERRENDERER_H
