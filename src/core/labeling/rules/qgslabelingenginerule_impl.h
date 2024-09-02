/***************************************************************************
    qgslabelingenginerule_impl.h
    ---------------------
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
#ifndef QGSLABELINGENGINERULEIMPL_H
#define QGSLABELINGENGINERULEIMPL_H

#include "qgis_core.h"
#include "qgis_sip.h"
#include "qgis.h"
#include "qgslabelingenginerule.h"
#include "qgsvectorlayerref.h"
#include "qgsmapunitscale.h"

/**
 * A labeling engine rule which prevents labels being placed too close to features from a different layer.
 *
 * \ingroup core
 * \since QGIS 3.40
 */
class CORE_EXPORT QgsLabelingEngineRuleMinimumDistanceLabelToFeature : public QgsAbstractLabelingEngineRule
{
  public:

    QgsLabelingEngineRuleMinimumDistanceLabelToFeature *clone() const override SIP_FACTORY;
    QString id() const override;
    bool prepare( QgsRenderContext &context ) override;
    bool modifyProblem() override;
    void writeXml( QDomDocument &doc, QDomElement &element, const QgsReadWriteContext &context ) const override;
    void readXml( const QDomElement &element, const QgsReadWriteContext &context ) override;

    /**
     * Returns the layer providing the labels.
     *
     * \see setLabeledLayer()
     */
    QgsVectorLayer *labeledLayer();

    /**
     * Sets the \a layer providing the labels.
     *
     * \see labeledLayer()
     */
    void setLabeledLayer( QgsVectorLayer *layer );

    /**
     * Returns the layer providing the features which labels must be distant from.
     *
     * \see setTargetLayer()
     */
    QgsVectorLayer *targetLayer();

    /**
     * Sets the \a layer providing the features which labels must be distant from.
     *
     * \see targetLayer()
     */
    void setTargetLayer( QgsVectorLayer *layer );

    /**
     * Returns the minimum permitted distance between labels and the features
     * from the targetLayer().
     *
     * \see setDistance()
     * \see distanceUnits()
     */
    double distance() const { return mDistance; }

    /**
     * Sets the minimum permitted \a distance between labels and the features
     * from the targetLayer().
     *
     * \see distance()
     * \see setDistanceUnits()
     */
    void setDistance( double distance ) { mDistance = distance; }

    /**
     * Returns the units for the distance between labels and the features
     * from the targetLayer().
     *
     * \see setDistanceUnit()
     * \see distance()
     */
    Qgis::RenderUnit distanceUnit() const { return mDistanceUnit; }

    /**
     * Sets the \a unit for the distance between labels and the features
     * from the targetLayer().
     *
     * \see distanceUnit()
     * \see setDistance()
     */
    void setDistanceUnit( Qgis::RenderUnit unit ) { mDistanceUnit = unit; }

    /**
     * Returns the scaling for the distance between labels and the features
     * from the targetLayer().
     *
     * \see setDistanceUnitScale()
     * \see distance()
     */
    const QgsMapUnitScale &distanceUnitScale() const { return mDistanceUnitScale; }

    /**
     * Sets the \a scale for the distance between labels and the features
     * from the targetLayer().
     *
     * \see distanceUnitScale()
     * \see setDistance()
     */
    void setDistanceUnitScale( const QgsMapUnitScale &scale ) { mDistanceUnitScale = scale; }

    /**
     * Returns the penalty cost incurred when the rule is violated.
     *
     * This is a value between 0 and 10, where 10 indicates that the rule must never be violated,
     * and 1-9 = nice to have if possible, where higher numbers will try harder to avoid violating the rule.
     *
     * \see setCost()
     */
    double cost() const { return mCost; }

    /**
     * Sets the penalty \a cost incurred when the rule is violated.
     *
     * This is a value between 0 and 10, where 10 indicates that the rule must never be violated,
     * and 1-9 = nice to have if possible, where higher numbers will try harder to avoid violating the rule.
     *
     * \see cost()
     */
    void setCost( double cost ) { mCost = cost; }

  private:
    QgsVectorLayerRef mLabeledLayer;
    QgsVectorLayerRef mTargetLayer;
    double mDistance = 0;
    Qgis::RenderUnit mDistanceUnit = Qgis::RenderUnit::Millimeters;
    QgsMapUnitScale mDistanceUnitScale;
    double mCost = 0;

};

/**
 * A labeling engine rule which prevents labels being placed too close to labels from a different layer.
 *
 * \ingroup core
 * \since QGIS 3.40
 */
class CORE_EXPORT QgsLabelingEngineRuleMinimumDistanceLabelToLabel : public QgsAbstractLabelingEngineRule
{
  public:

    QgsLabelingEngineRuleMinimumDistanceLabelToLabel *clone() const override SIP_FACTORY;
    QString id() const override;
    bool prepare( QgsRenderContext &context ) override;
    bool modifyProblem() override;
    void writeXml( QDomDocument &doc, QDomElement &element, const QgsReadWriteContext &context ) const override;
    void readXml( const QDomElement &element, const QgsReadWriteContext &context ) override;

    /**
     * Returns the layer providing the labels.
     *
     * \see setLabeledLayer()
     */
    QgsVectorLayer *labeledLayer();

    /**
     * Sets the \a layer providing the labels.
     *
     * \see labeledLayer()
     */
    void setLabeledLayer( QgsVectorLayer *layer );

    /**
     * Returns the reference layer providing the features which labels must be distant from.
     *
     * \see setTargetLayer()
     */
    QgsVectorLayer *targetLayer();

    /**
     * Sets the reference \a layer providing the features which labels must be distant from.
     *
     * \see targetLayer()
     */
    void setTargetLayer( QgsVectorLayer *layer );

    /**
     * Returns the minimum permitted distance between labels from the labeledLayer() and the labels
     * from the targetLayer().
     *
     * \see setDistance()
     * \see distanceUnits()
     */
    double distance() const { return mDistance; }

    /**
     * Sets the minimum permitted \a distance between labels from the labeledLayer() and the labels
     * from the targetLayer().
     *
     * \see distance()
     * \see setDistanceUnits()
     */
    void setDistance( double distance ) { mDistance = distance; }

    /**
     * Returns the units for the distance between labels from the labeledLayer() and the labels
     * from the targetLayer().
     *
     * \see setDistanceUnit()
     * \see distance()
     */
    Qgis::RenderUnit distanceUnit() const { return mDistanceUnit; }

    /**
     * Sets the \a unit for the distance between labels from the labeledLayer() and the labels
     * from the targetLayer().
     *
     * \see distanceUnit()
     * \see setDistance()
     */
    void setDistanceUnit( Qgis::RenderUnit unit ) { mDistanceUnit = unit; }

    /**
     * Returns the scaling for the distance between labels from the labeledLayer() and the labels
     * from the targetLayer().
     *
     * \see setDistanceUnitScale()
     * \see distance()
     */
    const QgsMapUnitScale &distanceUnitScale() const { return mDistanceUnitScale; }

    /**
     * Sets the \a scale for the distance between labels from the labeledLayer() and the labels
     * from the targetLayer().
     *
     * \see distanceUnitScale()
     * \see setDistance()
     */
    void setDistanceUnitScale( const QgsMapUnitScale &scale ) { mDistanceUnitScale = scale; }

  private:
    QgsVectorLayerRef mLabeledLayer;
    QgsVectorLayerRef mTargetLayer;
    double mDistance = 0;
    Qgis::RenderUnit mDistanceUnit = Qgis::RenderUnit::Millimeters;
    QgsMapUnitScale mDistanceUnitScale;
};


/**
 * A labeling engine rule which prevents labels being placed too far from features from a different layer.
 *
 * \ingroup core
 * \since QGIS 3.40
 */
class CORE_EXPORT QgsLabelingEngineRuleMaximumDistanceLabelToFeature : public QgsAbstractLabelingEngineRule
{
  public:

    QgsLabelingEngineRuleMaximumDistanceLabelToFeature *clone() const override SIP_FACTORY;
    QString id() const override;
    bool prepare( QgsRenderContext &context ) override;
    bool modifyProblem() override;
    void writeXml( QDomDocument &doc, QDomElement &element, const QgsReadWriteContext &context ) const override;
    void readXml( const QDomElement &element, const QgsReadWriteContext &context ) override;

    /**
     * Returns the layer providing the labels.
     *
     * \see setLabeledLayer()
     */
    QgsVectorLayer *labeledLayer();

    /**
     * Sets the \a layer providing the labels.
     *
     * \see labeledLayer()
     */
    void setLabeledLayer( QgsVectorLayer *layer );

    /**
     * Returns the layer providing the features which labels must be close to.
     *
     * \see setTargetLayer()
     */
    QgsVectorLayer *targetLayer();

    /**
     * Sets the \a layer providing the features which labels must be close to.
     *
     * \see targetLayer()
     */
    void setTargetLayer( QgsVectorLayer *layer );

    /**
     * Returns the maximum permitted distance between labels and the features
     * from the targetLayer().
     *
     * \see setDistance()
     * \see distanceUnits()
     */
    double distance() const { return mDistance; }

    /**
     * Sets the maximum permitted \a distance between labels and the features
     * from the targetLayer().
     *
     * \see distance()
     * \see setDistanceUnits()
     */
    void setDistance( double distance ) { mDistance = distance; }

    /**
     * Returns the units for the distance between labels and the features
     * from the targetLayer().
     *
     * \see setDistanceUnit()
     * \see distance()
     */
    Qgis::RenderUnit distanceUnit() const { return mDistanceUnit; }

    /**
     * Sets the \a unit for the distance between labels and the features
     * from the targetLayer().
     *
     * \see distanceUnit()
     * \see setDistance()
     */
    void setDistanceUnit( Qgis::RenderUnit unit ) { mDistanceUnit = unit; }

    /**
     * Returns the scaling for the distance between labels and the features
     * from the targetLayer().
     *
     * \see setDistanceUnitScale()
     * \see distance()
     */
    const QgsMapUnitScale &distanceUnitScale() const { return mDistanceUnitScale; }

    /**
     * Sets the \a scale for the distance between labels and the features
     * from the targetLayer().
     *
     * \see distanceUnitScale()
     * \see setDistance()
     */
    void setDistanceUnitScale( const QgsMapUnitScale &scale ) { mDistanceUnitScale = scale; }

  private:
    QgsVectorLayerRef mLabeledLayer;
    QgsVectorLayerRef mTargetLayer;
    double mDistance = 0;
    Qgis::RenderUnit mDistanceUnit = Qgis::RenderUnit::Millimeters;
    QgsMapUnitScale mDistanceUnitScale;

};


/**
 * A labeling engine rule which prevents labels being placed overlapping features from a different layer.
 *
 * \ingroup core
 * \since QGIS 3.40
 */
class CORE_EXPORT QgsLabelingEngineRuleAvoidLabelOverlapWithFeature : public QgsAbstractLabelingEngineRule
{
  public:

    QgsLabelingEngineRuleAvoidLabelOverlapWithFeature *clone() const override SIP_FACTORY;
    QString id() const override;
    bool prepare( QgsRenderContext &context ) override;
    bool modifyProblem() override;
    void writeXml( QDomDocument &doc, QDomElement &element, const QgsReadWriteContext &context ) const override;
    void readXml( const QDomElement &element, const QgsReadWriteContext &context ) override;

    /**
     * Returns the layer providing the labels.
     *
     * \see setLabeledLayer()
     */
    QgsVectorLayer *labeledLayer();

    /**
     * Sets the \a layer providing the labels.
     *
     * \see labeledLayer()
     */
    void setLabeledLayer( QgsVectorLayer *layer );

    /**
     * Returns the layer providing the features which labels must not overlap.
     *
     * \see setTargetLayer()
     */
    QgsVectorLayer *targetLayer();

    /**
     * Sets the \a layer providing the features which labels must not overlap.
     *
     * \see targetLayer()
     */
    void setTargetLayer( QgsVectorLayer *layer );

  private:
    QgsVectorLayerRef mLabeledLayer;
    QgsVectorLayerRef mTargetLayer;

};


#endif // QGSLABELINGENGINERULEIMPL_H
