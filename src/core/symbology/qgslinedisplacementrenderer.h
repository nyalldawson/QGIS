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
#include "qgsexpression.h"
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
class CORE_EXPORT QgsLineDisplacementRenderer : public QgsFeatureRenderer
{
  public:

    /**
     * Constructor for QgsLineDisplacementRenderer.
     * \param embeddedRenderer optional embeddedRenderer. Ownership will be transferred.
     */
    QgsLineDisplacementRenderer( QgsFeatureRenderer *embeddedRenderer SIP_TRANSFER );

    //! QgsLineDisplacementRenderer cannot be copied. Use clone() instead.
    QgsLineDisplacementRenderer( const QgsLineDisplacementRenderer & ) = delete;
    //! QgsLineDisplacementRenderer cannot be copied. Use clone() instead.
    QgsLineDisplacementRenderer &operator=( const QgsLineDisplacementRenderer & ) = delete;

    //! Creates a renderer out of an XML, for loading
    static QgsFeatureRenderer *create( QDomElement &element, const QgsReadWriteContext &context ) SIP_FACTORY;

    QgsLineDisplacementRenderer *clone() const override SIP_FACTORY;
    void startRender( QgsRenderContext &context, const QgsFields &fields ) override;

    bool renderFeature( const QgsFeature &feature, QgsRenderContext &context, int layer = -1, bool selected = false, bool drawVertexMarker = false ) override SIP_THROW( QgsCsException );
    void stopRender( QgsRenderContext &context ) override;

    QString dump() const override;
    QSet<QString> usedAttributes( const QgsRenderContext &context ) const override;
    bool filterNeedsGeometry() const override;
    QgsFeatureRenderer::Capabilities capabilities() override;
    QgsSymbolList symbols( QgsRenderContext &context ) const override;
    QgsSymbol *symbolForFeature( const QgsFeature &feature, QgsRenderContext &context ) const override;
    QgsSymbol *originalSymbolForFeature( const QgsFeature &feature, QgsRenderContext &context ) const override;
    QgsSymbolList symbolsForFeature( const QgsFeature &feature, QgsRenderContext &context ) const override;
    QgsSymbolList originalSymbolsForFeature( const QgsFeature &feature, QgsRenderContext &context ) const override;
    QSet< QString > legendKeysForFeature( const QgsFeature &feature, QgsRenderContext &context ) const override;
    QString legendKeyToExpression( const QString &key, QgsVectorLayer *layer, bool &ok ) const override;
    QgsLegendSymbolList legendSymbolItems() const override;
    bool willRenderFeature( const QgsFeature &feature, QgsRenderContext &context ) const override;
    QDomElement save( QDomDocument &doc, const QgsReadWriteContext &context ) override;
    void setEmbeddedRenderer( QgsFeatureRenderer *subRenderer SIP_TRANSFER ) override;
    const QgsFeatureRenderer *embeddedRenderer() const override;
    void setLegendSymbolItem( const QString &key, QgsSymbol *symbol ) override;
    bool legendSymbolItemsCheckable() const override;
    bool legendSymbolItemChecked( const QString &key ) override;
    void checkLegendSymbolItem( const QString &key, bool state = true ) override;
    bool accept( QgsStyleEntityVisitorInterface *visitor ) const override;

    /**
     * Creates a QgsLineDisplacementRenderer by a conversion from an existing renderer.
     * \returns a new renderer if the conversion was possible, otherwise NULLPTR.
     */
    static QgsLineDisplacementRenderer *convertFromRenderer( const QgsFeatureRenderer *renderer ) SIP_FACTORY;

  protected:

    /**
     * Constructor for QgsLineDisplacementRenderer.
     * \param type renderer ID string
     * \param embeddedRenderer optional embeddedRenderer. Ownership will be transferred.
     */
    QgsLineDisplacementRenderer( const QString &type, QgsFeatureRenderer *embeddedRenderer SIP_TRANSFER );

    //! Embedded renderer
    std::unique_ptr<QgsFeatureRenderer> mSubRenderer;

  private:

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
