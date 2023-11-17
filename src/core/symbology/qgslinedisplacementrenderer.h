/***************************************************************************
    qgslinedisplacementrenderer.h
    ---------------------
    begin                : November 2023
    copyright            : (C) 2023 by Nyall Dawson
    email                : nyall dot dawson at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef QGSLINEDISPLACEMENTRENDERER_H
#define QGSLINEDISPLACEMENTRENDERER_H

#include "qgis_core.h"
#include "qgis_sip.h"
#include "qgis.h"
#include "qgsrenderer.h"
#include "qgslinedistancerenderer.h"
#include "qgsfeature.h"
#include "qgsgeometry.h"
#include "qgsrendercontext.h"

/**
 * \ingroup core
 * \brief QgsLineDisplacementRenderer is a line-only feature renderer used to
 * render overlapping line features in a displaced manner.
 *
 * It is designed on top of another feature renderer, which is called "embedded"
 * Most of the methods are then only proxies to the embedded renderer.
 *
 * \since QGIS 3.36
 */
class CORE_EXPORT QgsLineDisplacementRenderer : public QgsLineDistanceRenderer
{
  public:

    /**
     * Constructor for QgsLineDisplacementRenderer.
     * \param embeddedRenderer optional embeddedRenderer. Ownership will be transferred.
     */
    QgsLineDisplacementRenderer( QgsFeatureRenderer *embeddedRenderer SIP_TRANSFER );

    //! Creates a renderer out of an XML, for loading
    static QgsFeatureRenderer *create( QDomElement &element, const QgsReadWriteContext &context ) SIP_FACTORY;

    QgsLineDisplacementRenderer *clone() const override SIP_FACTORY;
    void startRender( QgsRenderContext &context, const QgsFields &fields ) override;
    void stopRender( QgsRenderContext &context ) override;
    QDomElement save( QDomDocument &doc, const QgsReadWriteContext &context ) override;
    QSet<QString> usedAttributes( const QgsRenderContext &context ) const override;
    bool accept( QgsStyleEntityVisitorInterface *visitor ) const override;

    /**
     * Creates a QgsLineDisplacementRenderer by a conversion from an existing renderer.
     * \returns a new renderer if the conversion was possible, otherwise NULLPTR.
     */
    static QgsLineDisplacementRenderer *convertFromRenderer( const QgsFeatureRenderer *renderer ) SIP_FACTORY;

  private:
    void drawGroup( QPointF centerPoint, QgsRenderContext &context, const QgsLineDistanceRenderer::ClusteredGroup &group ) const override SIP_FORCE;

    //! Structure where the reversed geometry is built during renderFeature
    struct CombinedFeature
    {
      QVector<QgsGeometry> geometries; //< list of geometries
      QgsFeature feature;             //< one feature (for attriute-based rendering)
    };
    typedef QVector<CombinedFeature> FeatureCategoryVector;
    //! Where features are stored, based on the index of their symbol category \see mSymbolCategories
    FeatureCategoryVector mFeaturesCategories;

    //! Maps a category to an index
    QMap<QByteArray, int> mSymbolCategories;

    //! The polygon used as exterior ring that covers the current extent
    QgsPolygonXY mExtentPolygon;

    //! The context used for rendering
    QgsRenderContext mContext;

    //! Fields of each feature
    QgsFields mFields;

    /**
     * Class used to represent features that must be rendered
     *  with decorations (selection, vertex markers)
     */
    struct FeatureDecoration
    {
      QgsFeature feature;
      bool selected;
      bool drawMarkers;
      int layer;
      FeatureDecoration( const QgsFeature &a_feature, bool a_selected, bool a_drawMarkers, int a_layer )
        : feature( a_feature )
        , selected( a_selected )
        , drawMarkers( a_drawMarkers )
        , layer( a_layer )
      {}
    };
    QList<FeatureDecoration> mFeatureDecorations;

};


#endif // QGSLINEDISPLACEMENTRENDERER_H
