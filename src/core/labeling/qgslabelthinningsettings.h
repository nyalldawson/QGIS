/***************************************************************************
  qgslabelthinningsettings.h
  --------------------------
  Date                 : December 2019
  Copyright            : (C) 2019 by Nyall Dawson
  Email                : nyall dot dawson at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSLABELTHINNINGSETTINGS_H
#define QGSLABELTHINNINGSETTINGS_H

#include "qgis_core.h"
#include "qgis_sip.h"
#include "qgsmapunitscale.h"

class QgsPropertyCollection;
class QgsExpressionContext;

/**
 * \ingroup core
 * \class QgsLabelThinningSettings
 *
 * \brief Contains settings related to how the label engine removes candidate label positions and reduces the number
 * of displayed labels.
 *
 * \since QGIS 3.12
 */
class CORE_EXPORT QgsLabelThinningSettings
{
  public:

    /**
     * Returns TRUE if the number of labels drawn for the layer should be limited.
     * \see maximumNumberLabels()
     * \see setLimitNumberLabelsEnabled()
     */
    bool limitNumberOfLabelsEnabled() const { return mLimitNumLabels; }

    /**
     * Sets whether the the number of labels drawn for the layer should be limited.
     * \see setMaximumNumberLabels()
     * \see limitNumberOfLabelsEnabled()
     */
    void setLimitNumberLabelsEnabled( bool enabled ) { mLimitNumLabels = enabled; }

    /**
     * Returns the maximum number of labels which should be drawn for this layer.
     * This only has an effect if limitNumberOfLabelsEnabled() is TRUE.
     * \see limitNumberOfLabelsEnabled()
     * \see setMaximumNumberLabels()
     */
    int maximumNumberLabels() const { return mMaxNumLabels; }

    /**
     * Sets the maximum \a number of labels which should be drawn for this layer.
     * This only has an effect if limitNumberOfLabelsEnabled() is TRUE.
     * \see setLimitNumberLabelsEnabled()
     * \see maximumNumberLabels()
     */
    void setMaximumNumberLabels( int number ) { mMaxNumLabels = number; }

    /**
     * Returns the minimum feature size (in millimeters) for a feature to be labelled.
     * \see setMinimumFeatureSize()
     */
    double minimumFeatureSize() const { return mMinFeatureSize; }

    /**
     * Sets the minimum feature \a size (in millimeters) for a feature to be labelled.
     * \see minimumFeatureSize()
     */
    void setMinimumFeatureSize( double size ) { mMinFeatureSize = size; }

    /**
     * Returns the minimum distance to labels with duplicate text. Units are specified through minimumDistanceToDuplicateUnits().
     * \see setMinimumDistanceToDuplicate()
     * \see minimumDistanceToDuplicateUnits()
     *
     * \since QGIS 3.20
     */
    double minimumDistanceToDuplicate() const { return mMinDistanceToDuplicate; }

    /**
     * Sets the minimum \a distance to labels with duplicate text. Units are specified through setMinimumLengthUnit().
     * \see minimumDistanceToDuplicate()
     * \see setMinimumDistanceToDuplicateUnits()
     *
     * \since QGIS 3.20
     */
    void setMinimumDistanceToDuplicate( double length ) { mMinDistanceToDuplicate = length; }

    /**
     * Sets the \a unit for the minimum distance to labels with duplicate text.
     * \see minimumDistanceToDuplicateUnits()
     * \see setMinimumDistanceToDuplicate()
     *
     * \since QGIS 3.20
    */
    void setMinimumDistanceToDuplicateUnits( Qgis::RenderUnit unit ) { mMinDistanceToDuplicateUnits = unit; }

    /**
     * Returns the units for the minimum distance to labels with duplicate text.
     * \see setMinimumDistanceToDuplicateUnits()
     * \see minimumDistanceToDuplicate()
     *
     * \since QGIS 3.20
    */
    Qgis::RenderUnit minimumDistanceToDuplicateUnits() const { return mMinDistanceToDuplicateUnits; }

    /**
     * Sets the map unit \a scale for the minimum distance to labels with duplicate text.
     * \see minimumDistanceToDuplicateMapUnitScale()
     * \see minimumDistanceToDuplicateUnits()
     * \see setMinimumDistanceToDuplicate()
     *
     * \since QGIS 3.20
     */
    void setMinimumDistanceToDuplicateMapUnitScale( const QgsMapUnitScale &scale ) { mMinDistanceToDuplicateScale = scale; }

    /**
     * Returns the map unit scale for the minimum distance to labels with duplicate text.
     * \see setMinimumDistanceToDuplicateMapUnitScale()
     * \see minimumDistanceToDuplicateUnits()
     * \see minimumDistanceToDuplicate()
     *
     * \since QGIS 3.20
     */
    const QgsMapUnitScale &minimumDistanceToDuplicateMapUnitScale() const { return mMinDistanceToDuplicateScale; }

    /**
     * Returns whether duplicate label removal is permitted for this layer.
     * \see setAllowDuplicateRemoval()
     *
     * \since QGIS 3.20
     */
    bool allowDuplicateRemoval() const { return mAllowDuplicateRemoval; }

    /**
     * Sets whether duplicate label removal is permitted for this layer.
     * \see setAllowDuplicateRemoval()
     *
     * \since QGIS 3.20
     */
    void setAllowDuplicateRemoval( bool allow ) { mAllowDuplicateRemoval = allow; }

    /**
     * Updates the thinning settings to respect any data defined properties
     * set within the specified \a properties collection.
     */
    void updateDataDefinedProperties( const QgsPropertyCollection &properties, QgsExpressionContext &context );

  private:

    bool mLimitNumLabels = false;
    int mMaxNumLabels = 2000;
    double mMinFeatureSize = 0;
    bool mAllowDuplicateRemoval = false;
    double mMinDistanceToDuplicate = 0;
    Qgis::RenderUnit mMinDistanceToDuplicateUnits = Qgis::RenderUnit::Millimeters;
    QgsMapUnitScale mMinDistanceToDuplicateScale;
};

#ifndef SIP_RUN

/**
 * \ingroup core
 * \class QgsLabelFeatureThinningSettings
 *
 * Contains settings related to how the label engine removes candidate label positions and reduces the number
 * of displayed labels for one particular label feature.
 *
 * \since QGIS 3.20
 */
class CORE_EXPORT QgsLabelFeatureThinningSettings
{
  public:

    /**
     * Returns the minimum distance (in label units) between labels for this
     * feature and other labels with the same label text.
     * \see setNoRepeatDistance()
     */
    double noRepeatDistance() const { return mNoRepeatDistance; }

    /**
     * Sets the minimum \a distance (in label units) between labels for this
     * feature and other labels with the same label text.
     * \see noRepeatDistance()
     */
    void setNoRepeatDistance( double distance ) { mNoRepeatDistance = distance; }

  private:

    double mNoRepeatDistance = 0;

};

#endif

#endif // QGSLABELTHINNINGSETTINGS_H
