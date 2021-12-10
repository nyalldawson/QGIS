/***************************************************************************
                         qgsrasterrendererwidget.cpp
                         ---------------------------
    begin                : June 2012
    copyright            : (C) 2012 by Marco Hugentobler
    email                : marco at sourcepole dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsrasterrendererwidget.h"
#include "qgsrasterdataprovider.h"
#include "qgsrasterlayer.h"
#include "qgsmapcanvas.h"
#include "qgspropertyoverridebutton.h"
#include "qgsexpressioncontextutils.h"

QgsRasterRendererWidget::QgsRasterRendererWidget( QgsRasterLayer *layer, const QgsRectangle &extent )
  : mRasterLayer( layer )
  , mExtent( extent )
{
  mContext << QgsExpressionContextUtils::globalScope()
           << QgsExpressionContextUtils::projectScope( QgsProject::instance() )
           << QgsExpressionContextUtils::atlasScope( nullptr );

  mContext << QgsExpressionContextUtils::layerScope( mRasterLayer );

  if ( mRasterLayer && mRasterLayer->pipe() )
  {
    mPropertyCollection = mRasterLayer->pipe()->dataDefinedProperties();
  }
}

void QgsRasterRendererWidget::setMapCanvas( QgsMapCanvas *canvas )
{
  mCanvas = canvas;

  if ( mCanvas )
    mContext << QgsExpressionContextUtils::mapSettingsScope( mCanvas->mapSettings() );
}

QgsMapCanvas *QgsRasterRendererWidget::mapCanvas()
{
  return mCanvas;
}

QgsExpressionContext QgsRasterRendererWidget::createExpressionContext() const
{
  return mContext;
}

void QgsRasterRendererWidget::initializeDataDefinedButton( QgsPropertyOverrideButton *button, QgsRasterPipe::Property key )
{
  button->blockSignals( true );
  button->init( key, mPropertyCollection, QgsRasterPipe::propertyDefinitions(), nullptr );
  connect( button, &QgsPropertyOverrideButton::changed, this, &QgsRasterRendererWidget::updateProperty );
  button->registerExpressionContextGenerator( this );
  button->blockSignals( false );
}

void QgsRasterRendererWidget::updateDataDefinedButtons()
{
  const auto propertyOverrideButtons { findChildren< QgsPropertyOverrideButton * >() };
  for ( QgsPropertyOverrideButton *button : propertyOverrideButtons )
  {
    updateDataDefinedButton( button );
  }
}

void QgsRasterRendererWidget::updateDataDefinedButton( QgsPropertyOverrideButton *button )
{
  if ( !button )
    return;

  if ( button->propertyKey() < 0 )
    return;

  QgsRasterPipe::Property key = static_cast< QgsRasterPipe::Property >( button->propertyKey() );
  whileBlocking( button )->setToProperty( mPropertyCollection.property( key ) );
}

void QgsRasterRendererWidget::updateProperty()
{
  QgsPropertyOverrideButton *button = qobject_cast<QgsPropertyOverrideButton *>( sender() );
  QgsRasterPipe::Property key = static_cast<  QgsRasterPipe::Property >( button->propertyKey() );
  mPropertyCollection.setProperty( key, button->toProperty() );
}
