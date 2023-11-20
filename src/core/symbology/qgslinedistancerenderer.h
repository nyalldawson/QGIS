/***************************************************************************
    qgslinedistancerenderer.h
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
#ifndef QGSLINEDISTANCERENDERER_H
#define QGSLINEDISTANCERENDERER_H

#include "qgis_core.h"
#include "qgis_sip.h"
#include "qgis.h"
#include "qgsrenderer.h"
#include "qgsexpression.h"
#include "qgsfeature.h"
#include "qgsgeometry.h"
#include "qgsrendercontext.h"

class QgsSpatialIndex;
class QgsLineSymbol;

/**
 * \ingroup core
 * \brief An abstract base class for distance based line renderers (e.g., clusterer and displacement renderers).
 *
 * QgsLineDistanceRenderer handles calculation of overlapping line segment clusters using a distance based threshold.
 * Subclasses must implement drawGroup() to handle the rendering of individual line segment clusters
 * in the desired style.
 *
 * \since QGIS 3.36
 */
class CORE_EXPORT QgsLineDistanceRenderer : public QgsFeatureRenderer SIP_ABSTRACT
{
  public:

    //! Contains properties for a feature within a clustered group.
    struct CORE_EXPORT GroupedFeature
    {

        /**
        * Constructor for GroupedFeature.
        * \param feature feature
        * \param symbol base symbol for rendering feature
        * \param isSelected set to TRUE if feature is selected and should be rendered in a selected state
        */
        GroupedFeature( const QgsFeature &feature, QgsLineSymbol *symbol, bool isSelected );
        ~GroupedFeature();

        //! Feature
        QgsFeature feature;

        //! Base symbol for rendering feature
        QgsLineSymbol *symbol() const { return mSymbol; }

        //! True if feature is selected and should be rendered in a selected state
        bool isSelected;

      private:
        QgsLineSymbol *mSymbol = nullptr;
    };

    /**
     * Constructor for QgsLineDistanceRenderer.
     */
    QgsLineDistanceRenderer( const QString &rendererName );
    ~QgsLineDistanceRenderer() override;

    void toSld( QDomDocument &doc, QDomElement &element, const QVariantMap &props = QVariantMap() ) const override;
    bool renderFeature( const QgsFeature &feature, QgsRenderContext &context, int layer = -1, bool selected = false, bool drawVertexMarker = false ) override SIP_THROW( QgsCsException );
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
    bool willRenderFeature( const QgsFeature &feature, QgsRenderContext &context ) const override;
    void startRender( QgsRenderContext &context, const QgsFields &fields ) override;
    void stopRender( QgsRenderContext &context ) override;
    QgsLegendSymbolList legendSymbolItems() const override;
    void setEmbeddedRenderer( QgsFeatureRenderer *r SIP_TRANSFER ) override;
    const QgsFeatureRenderer *embeddedRenderer() const override;
    void setLegendSymbolItem( const QString &key, QgsSymbol *symbol SIP_TRANSFER ) override;
    bool legendSymbolItemsCheckable() const override;
    bool legendSymbolItemChecked( const QString &key ) override;
    void checkLegendSymbolItem( const QString &key, bool state ) override;
    QString filter( const QgsFields &fields = QgsFields() ) override;
    bool accept( QgsStyleEntityVisitorInterface *visitor ) const override;

    /**
     * Sets the tolerance \a distance for grouping line segments. Units are specified using
     * setToleranceUnit().
     *
     * \see tolerance()
     * \see setToleranceUnit()
     */
    void setTolerance( double distance ) { mTolerance = distance; }

    /**
     * Returns the tolerance distance for grouping line segments. Units are retrieved using
     * toleranceUnit().
     *
     * \see setTolerance()
     * \see toleranceUnit()
     */
    double tolerance() const { return mTolerance; }

    /**
     * Sets the \a unit for the tolerance distance.
     *
     * \see setTolerance()
     * \see toleranceUnit()
     */
    void setToleranceUnit( Qgis::RenderUnit unit ) { mToleranceUnit = unit; }

    /**
     * Returns the units for the tolerance distance.
     *
     * \see tolerance()
     * \see setToleranceUnit()
     */
    Qgis::RenderUnit toleranceUnit() const { return mToleranceUnit; }

    /**
     * Sets the map unit \a scale object for the distance tolerance. This is only used if the
     * toleranceUnit() is set to Qgis::RenderUnit::MapUnits.
     *
     * \see toleranceMapUnitScale()
     * \see setToleranceUnit()
     */
    void setToleranceMapUnitScale( const QgsMapUnitScale &scale ) { mToleranceMapUnitScale = scale; }

    /**
     * Returns the map unit scale object for the distance tolerance. This is only used if the
     * toleranceUnit() is set to Qgis::RenderUnit::MapUnits.
     *
     * \see setToleranceMapUnitScale()
     * \see toleranceUnit()
     */
    const QgsMapUnitScale &toleranceMapUnitScale() const { return mToleranceMapUnitScale; }

    /**
    * Sets the angle \a threshold (in degrees) for nearby line segments to be considered overlapping.
    *
    * If the angle between the two segments is greater than this threshold than the segments
    * will not be considered overlapping. This setting can be used to avoid considering "T" type
    * intersections as overlapping segments.
    *
    * \see angleThreshold()
    */
    void setAngleThreshold( double threshold ) { mAngleThreshold = threshold; }

    /**
    * Returns the angle threshold (in degrees) for nearby line segments to be considered overlapping.
    *
    * If the angle between the two segments is greater than this threshold than the segments
    * will not be considered overlapping. This setting can be used to avoid considering "T" type
    * intersections as overlapping segments.
    *
    * \see setAngleThreshold()
    */
    double angleThreshold() const { return mAngleThreshold; }

    /**
     * Returns first symbol from the embedded renderer for a feature or NULLPTR if none
     * \param feature source feature
     * \param context target render context
    */
    QgsLineSymbol *firstSymbolForFeature( const QgsFeature &feature, QgsRenderContext &context ) const;

  protected:

#ifndef SIP_RUN
    //! Embedded renderer
    std::unique_ptr<QgsFeatureRenderer> mRenderer;

    //! Distance tolerance. Points that are closer together than this distance are considered clustered.
    double mTolerance = 6;
    //! Unit for distance tolerance.
    Qgis::RenderUnit mToleranceUnit = Qgis::RenderUnit::Millimeters;
    //! Map unit scale for distance tolerance.
    QgsMapUnitScale mToleranceMapUnitScale;

    double mAngleThreshold = 10;

    std::unique_ptr< QgsSpatialIndex > mSegmentIndex;
    QVector< GroupedFeature > mQueuedFeatures;

    struct SegmentData
    {
      double x1;
      double y1;
      double x2;
      double y2;
      QgsFeature feature;
      int segmentIndex;
      bool selected;
      bool drawVertexMarker;
    };

    struct SplitSegment
    {
      double x1;
      double y1;
      double x2;
      double y2;
      int segmentGroup;
      int indexInGroup;
      QgsFeature feature;
      int indexInGeometry;
      int indexInSplit;
    };

    int mSegmentId = 0;
    QVector< SegmentData > mSegmentData;
#endif

    /**
     * Creates an expression context scope for a clustered group, with variables reflecting the group's properties.
     * \param group clustered group
     * \returns new expression context scope
     */
    QgsExpressionContextScope *createGroupScope() const;

  private:

#ifndef SIP_RUN
    virtual void drawGroups( QgsRenderContext &context,
                             const QVector< GroupedFeature > &features,
                             const QHash< QgsFeatureId, QList< int > > &featureIdToSegments,
                             const QHash< int, QList< int> > &segmentGroups,
                             const QHash< int, SplitSegment> &splitSegments
                           ) const = 0;
#endif

};


#endif // QGSLINEDISTANCERENDERER_H
