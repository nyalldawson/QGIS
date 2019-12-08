/***************************************************************************
    qgslabelremoveduplicatessettingswidget.h
    ----------------------
    begin                : December 2019
    copyright            : (C) 2019 by Nyall Dawson
    email                : nyall dot dawson at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include "qgslabelremoveduplicatesettingswidget.h"
#include "qgsexpressioncontextutils.h"

QgsLabelRemoveDuplicatesSettingsWidget::QgsLabelRemoveDuplicatesSettingsWidget( QWidget *parent, QgsVectorLayer *vl )
  : QgsLabelSettingsWidgetBase( parent, vl )
{
  setupUi( this );

  setPanelTitle( tr( "Duplicate Removal" ) );

  mNoRepeatDistUnitWidget->setUnits( QgsUnitTypes::RenderUnitList() << Qgis::RenderUnit::Millimeters << Qgis::RenderUnit::MetersInMapUnits << Qgis::RenderUnit::MapUnits << Qgis::RenderUnit::Pixels
                                     << Qgis::RenderUnit::Points << Qgis::RenderUnit::Inches );

  connect( mNoRepeatDistSpinBox, qOverload<double>( &QDoubleSpinBox::valueChanged ), this, [ = ]( double )
  {
    if ( !mBlockSignals )
      emit changed();
  } );
  connect( mNoRepeatDistUnitWidget, &QgsUnitSelectionWidget::changed, this, [ = ]
  {
    if ( !mBlockSignals )
      emit changed();
  } );

  registerDataDefinedButton( mNoRepeatDistDDBtn, QgsPalLayerSettings::RemoveDuplicateLabelDistance );
}

void QgsLabelRemoveDuplicatesSettingsWidget::setSettings( const QgsLabelThinningSettings &settings )
{
  mSettings = settings;
  mBlockSignals = true;

  mNoRepeatDistSpinBox->setValue( mSettings.minimumDistanceToDuplicate() );
  mNoRepeatDistUnitWidget->setUnit( mSettings.minimumDistanceToDuplicateUnits() );
  mNoRepeatDistUnitWidget->setMapUnitScale( mSettings.minimumDistanceToDuplicateMapUnitScale() );

  mBlockSignals = false;
}

QgsLabelThinningSettings QgsLabelRemoveDuplicatesSettingsWidget::settings() const
{
  mSettings.setMinimumDistanceToDuplicate( mNoRepeatDistSpinBox->value() );
  mSettings.setMinimumDistanceToDuplicateUnits( mNoRepeatDistUnitWidget->unit() );
  mSettings.setMinimumDistanceToDuplicateMapUnitScale( mNoRepeatDistUnitWidget->getMapUnitScale() );
  return mSettings;
}

void QgsLabelRemoveDuplicatesSettingsWidget::setGeometryType( Qgis::GeometryType )
{
}

void QgsLabelRemoveDuplicatesSettingsWidget::updateDataDefinedProperties( QgsPropertyCollection &properties )
{
  properties.setProperty( QgsPalLayerSettings::RemoveDuplicateLabelDistance, mDataDefinedProperties.property( QgsPalLayerSettings::RemoveDuplicateLabelDistance ) );
}
