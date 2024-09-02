/***************************************************************************
    qgslabelingengineruleregistry.cpp
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
#include "qgslabelingengineruleregistry.h"
#include "qgslabelingenginerule.h"

QgsLabelingEngineRuleRegistry::QgsLabelingEngineRuleRegistry()
{

}

QgsLabelingEngineRuleRegistry::~QgsLabelingEngineRuleRegistry() = default;

QgsAbstractLabelingEngineRule *QgsLabelingEngineRuleRegistry::create( const QString &id ) const
{

}

void QgsLabelingEngineRuleRegistry::addRule( QgsAbstractLabelingEngineRule *rule )
{

}

