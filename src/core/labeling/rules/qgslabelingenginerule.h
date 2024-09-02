/***************************************************************************
    qgslabelingenginerule.h
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
#ifndef QGSLABELINGENGINERULE_H
#define QGSLABELINGENGINERULE_H

#include "qgis_core.h"
#include "qgis_sip.h"
#include "qgis.h"

class QgsRenderContext;
class QDomDocument;
class QDomElement;
class QgsReadWriteContext;
class QgsProject;

/**
 * Abstract base class for labeling engine rules.
 *
 * Labeling engine rules implement custom logic to modify the labeling solution for a map render,
 * e.g. by preventing labels being placed which violate custom constraints.
 *
 * \ingroup core
 * \since QGIS 3.40
 */
class CORE_EXPORT QgsAbstractLabelingEngineRule
{

#ifdef SIP_RUN
    SIP_CONVERT_TO_SUBCLASS_CODE
    if ( sipCpp->id() == "minimumDistanceLabelToFeature" )
    {
      sipType = sipType_QgsLabelingEngineRuleMinimumDistanceLabelToFeature;
    }
    else if ( sipCpp->id() == "minimumDistanceLabelToLabel" )
    {
      sipType = sipType_QgsLabelingEngineRuleMinimumDistanceLabelToLabel;
    }
    else if ( sipCpp->id() == "maximumDistanceLabelToFeature" )
    {
      sipType = sipType_QgsLabelingEngineRuleMaximumDistanceLabelToFeature;
    }
    else if ( sipCpp->id() == "avoidLabelOverlapWithFeature" )
    {
      sipType = sipType_QgsLabelingEngineRuleAvoidLabelOverlapWithFeature;
    }
    else
    {
      sipType = 0;
    }
    SIP_END
#endif

  public:

    virtual ~QgsAbstractLabelingEngineRule();

    /**
     * Creates a clone of this rule.
     *
     * The caller takes ownership of the returned object.
     */
    virtual QgsAbstractLabelingEngineRule *clone() const = 0 SIP_FACTORY;

    /**
     * Returns a string uniquely identifying the rule subclass.
     */
    virtual QString id() const = 0;

    /**
     * Prepares the rule.
     *
     * This must be called on the main render thread, prior to commencing the render operation. Thread sensitive
     * logic (such as creation of feature sources) can be performed in this method.
     */
    virtual bool prepare( QgsRenderContext &context ) = 0;

    /**
     *
     */
    virtual bool modifyProblem() SIP_SKIP;

    /**
     * Writes the rule properties to an XML \a element.
     *
     * \see readXml()
     */
    virtual void writeXml( QDomDocument &doc, QDomElement &element, const QgsReadWriteContext &context ) const = 0;

    /**
     * Reads the rule properties from an XML \a element.
     *
     * \see resolveReferences()
     * \see writeXml()
     */
    virtual void readXml( const QDomElement &element, const QgsReadWriteContext &context ) = 0;

    /**
     * Resolves reference to layers from stored layer ID.
     *
     * Should be called following a call readXml().
     */
    virtual void resolveReferences( const QgsProject *project );
};

#endif // QGSLABELINGENGINESETTINGS_H
