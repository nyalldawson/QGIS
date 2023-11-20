/***************************************************************************
    qgslineclusterrendererwidget.cpp
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
#include "qgslineclusterrendererwidget.h"
#include "qgslineclusterrenderer.h"
#include "qgsrendererregistry.h"
#include "qgsvectorlayer.h"
#include "qgsapplication.h"
#include "qgslinesymbol.h"

QgsRendererWidget *QgsLineClusterRendererWidget::create( QgsVectorLayer *layer, QgsStyle *style, QgsFeatureRenderer *renderer )
{
  return new QgsLineClusterRendererWidget( layer, style, renderer );
}

QgsLineClusterRendererWidget::QgsLineClusterRendererWidget( QgsVectorLayer *layer, QgsStyle *style, QgsFeatureRenderer *renderer )
  : QgsRendererWidget( layer, style )
{
  if ( !layer )
  {
    return;
  }

  const Qgis::GeometryType type = QgsWkbTypes::geometryType( layer->wkbType() );

  // the renderer only applies to line vector layers
  if ( type != Qgis::GeometryType::Line )
  {
    //setup blank dialog
    mRenderer.reset( nullptr );
    QGridLayout *layout = new QGridLayout( this );
    QLabel *label = new QLabel( tr( "The clustered lines renderer only applies to line layers. \n"
                                    "'%1' is not a line layer and cannot be displayed" )
                                .arg( layer->name() ), this );
    this->setLayout( layout );
    layout->addWidget( label );
    return;
  }
  setupUi( this );
  connect( mRendererComboBox, static_cast<void ( QComboBox::* )( int )>( &QComboBox::currentIndexChanged ), this, &QgsLineClusterRendererWidget::mRendererComboBox_currentIndexChanged );
  connect( mDistanceSpinBox, static_cast < void ( QDoubleSpinBox::* )( double ) > ( &QDoubleSpinBox::valueChanged ), this, [ = ]( double value )
  {
    if ( mRenderer )
    {
      mRenderer->setTolerance( value );
    }
    emit widgetChanged();
  } );
  connect( mDistanceUnitWidget, &QgsUnitSelectionWidget::changed, this, [ = ]
  {
    if ( mRenderer )
    {
      mRenderer->setToleranceUnit( mDistanceUnitWidget->unit() );
      mRenderer->setToleranceMapUnitScale( mDistanceUnitWidget->getMapUnitScale() );
    }
    emit widgetChanged();
  } );
  this->layout()->setContentsMargins( 0, 0, 0, 0 );

  mDistanceUnitWidget->setUnits( { Qgis::RenderUnit::Millimeters, Qgis::RenderUnit::MetersInMapUnits, Qgis::RenderUnit::MapUnits, Qgis::RenderUnit::Pixels,
                                   Qgis::RenderUnit::Points, Qgis::RenderUnit::Inches } );
  mClusterLineSymbolToolButton->setSymbolType( Qgis::SymbolType::Line );

  // try to recognize the previous renderer
  // (null renderer means "no previous renderer")
  if ( renderer )
  {
    mRenderer.reset( QgsLineClusterRenderer::convertFromRenderer( renderer ) );
  }
  if ( ! mRenderer )
  {
    mRenderer.reset( new QgsLineClusterRenderer() );
    if ( renderer )
      renderer->copyRendererData( mRenderer.get() );
  }

  int currentEmbeddedIdx = 0;
  //insert possible renderer types
  const QStringList rendererList = QgsApplication::rendererRegistry()->renderersList( type == Qgis::GeometryType::Polygon ? QgsRendererAbstractMetadata::PolygonLayer :  QgsRendererAbstractMetadata::LineLayer );
  QStringList::const_iterator it = rendererList.constBegin();
  int idx = 0;
  mRendererComboBox->blockSignals( true );
  for ( ; it != rendererList.constEnd(); ++it, ++idx )
  {
    if ( *it != QLatin1String( "mergedFeatureRenderer" )
         && *it != QLatin1String( "lineCluster" )
         && *it != QLatin1String( "lineDisplacement" ) ) //< these make no sense as embedded renderers
    {
      QgsRendererAbstractMetadata *m = QgsApplication::rendererRegistry()->rendererMetadata( *it );
      mRendererComboBox->addItem( m->icon(), m->visibleName(), /* data */ *it );
      const QgsFeatureRenderer *embeddedRenderer = mRenderer->embeddedRenderer();
      if ( embeddedRenderer && embeddedRenderer->type() == m->name() )
      {
        // store the combo box index of the current renderer
        currentEmbeddedIdx = idx;
      }
    }
  }
  mRendererComboBox->blockSignals( false );

  whileBlocking( mDistanceSpinBox )->setValue( mRenderer->tolerance() );
  whileBlocking( mDistanceUnitWidget )->setUnit( mRenderer->toleranceUnit() );
  whileBlocking( mDistanceUnitWidget )->setMapUnitScale( mRenderer->toleranceMapUnitScale() );
  mClusterLineSymbolToolButton->setSymbol( mRenderer->clusterSymbol()->clone() );

  const int oldIdx = mRendererComboBox->currentIndex();
  mRendererComboBox->setCurrentIndex( currentEmbeddedIdx );
  if ( oldIdx == currentEmbeddedIdx )
  {
    // force update
    mRendererComboBox_currentIndexChanged( currentEmbeddedIdx );
  }

  connect( mClusterLineSymbolToolButton, &QgsSymbolButton::changed, this, [ = ]
  {
    mRenderer->setClusterSymbol( mClusterLineSymbolToolButton->clonedSymbol< QgsLineSymbol >() );
    emit widgetChanged();
  } );
  mClusterLineSymbolToolButton->setLayer( mLayer );
  mClusterLineSymbolToolButton->registerExpressionContextGenerator( this );
}

QgsLineClusterRendererWidget::~QgsLineClusterRendererWidget() = default;

QgsFeatureRenderer *QgsLineClusterRendererWidget::renderer()
{
  if ( mRenderer && mEmbeddedRendererWidget )
  {
    QgsFeatureRenderer *embeddedRenderer = mEmbeddedRendererWidget->renderer();
    if ( embeddedRenderer )
    {
      mRenderer->setEmbeddedRenderer( embeddedRenderer->clone() );
    }
  }
  return mRenderer.get();
}

void QgsLineClusterRendererWidget::setContext( const QgsSymbolWidgetContext &context )
{
  QgsRendererWidget::setContext( context );
  if ( mEmbeddedRendererWidget )
    mEmbeddedRendererWidget->setContext( context );

  if ( mDistanceUnitWidget )
    mDistanceUnitWidget->setMapCanvas( context.mapCanvas() );
  if ( mClusterLineSymbolToolButton )
  {
    mClusterLineSymbolToolButton->setMapCanvas( context.mapCanvas() );
    mClusterLineSymbolToolButton->setMessageBar( context.messageBar() );
  }
}

void QgsLineClusterRendererWidget::setDockMode( bool dockMode )
{
  QgsRendererWidget::setDockMode( dockMode );
  if ( mEmbeddedRendererWidget )
    mEmbeddedRendererWidget->setDockMode( dockMode );
}

QgsExpressionContext QgsLineClusterRendererWidget::createExpressionContext() const
{
  QgsExpressionContext context;
  if ( auto *lExpressionContext = mContext.expressionContext() )
    context = *lExpressionContext;
  else
    context.appendScopes( mContext.globalProjectAtlasMapLayerScopes( mLayer ) );
  QList< QgsExpressionContextScope > scopes = mContext.additionalExpressionContextScopes();
  for ( const QgsExpressionContextScope &s : std::as_const( scopes ) )
  {
    context << new QgsExpressionContextScope( s );
  }
  return context;
}

void QgsLineClusterRendererWidget::mRendererComboBox_currentIndexChanged( int index )
{
  const QString rendererId = mRendererComboBox->itemData( index ).toString();
  QgsRendererAbstractMetadata *m = QgsApplication::rendererRegistry()->rendererMetadata( rendererId );
  if ( m )
  {
    const std::unique_ptr< QgsFeatureRenderer > oldRenderer( mRenderer->embeddedRenderer()->clone() );
    mEmbeddedRendererWidget.reset( m->createRendererWidget( mLayer, mStyle, oldRenderer.get() ) );
    connect( mEmbeddedRendererWidget.get(), &QgsRendererWidget::widgetChanged, this, &QgsLineClusterRendererWidget::widgetChanged );
    mEmbeddedRendererWidget->setContext( mContext );
    mEmbeddedRendererWidget->disableSymbolLevels();
    mEmbeddedRendererWidget->setDockMode( this->dockMode() );
    connect( mEmbeddedRendererWidget.get(), &QgsPanelWidget::showPanel, this, &QgsPanelWidget::openPanel );

    if ( layout()->count() > 2 )
    {
      // remove the current renderer widget
      layout()->takeAt( 2 );
    }
    layout()->addWidget( mEmbeddedRendererWidget.get() );
  }
}
