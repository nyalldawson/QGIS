/***************************************************************************
                             qgsprocessingparameterswidget.cpp
                             ------------------------------------
    Date                 : March 2020
    Copyright            : (C) 2020 Nyall Dawson
    Email                : nyall dot dawson at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsprocessingparameterswidget.h"

#include "qgsgui.h"
#include "qgsprocessingalgorithm.h"
#include "qgsprocessingguiregistry.h"
#include "qgsprocessingmodelalgorithm.h"
#include "qgsprocessingparameters.h"

#include <QLabel>
#include <QString>

#include "moc_qgsprocessingparameterswidget.cpp"

using namespace Qt::StringLiterals;

///@cond NOT_STABLE

QgsProcessingParametersWidget::QgsProcessingParametersWidget( const QgsProcessingAlgorithm *algorithm, bool inPlace, QgsMapLayer *activeLayer, QgsMessageBar *messageBar, QWidget *parent )
  : QgsPanelWidget( parent )
  , mAlgorithm( algorithm )
  , mInPlace( inPlace )
  , mActiveLayer( activeLayer )
  , mMessageBar( messageBar )
{
  Q_ASSERT( mAlgorithm );

  setupUi( this );

  //  mContext = QgsGui::processingGuiRegistry()-
  grpAdvanced->hide();
  scrollAreaWidgetContents->setContentsMargins( 4, 4, 4, 4 );
}

QgsProcessingParametersWidget::~QgsProcessingParametersWidget()
{
  qDeleteAll( mWrappers );
}

const QgsProcessingAlgorithm *QgsProcessingParametersWidget::algorithm() const
{
  return mAlgorithm;
}

QgsProcessingContext *QgsProcessingParametersWidget::processingContext() const
{
  return &mContext;
}

void QgsProcessingParametersWidget::parameterChanged()
{}

void QgsProcessingParametersWidget::initWidgets()
{
  QgsProcessingContext *context = processingContext();

  // if there are advanced parameters - show corresponding groupbox
  const QgsProcessingParameterDefinitions defs = mAlgorithm->parameterDefinitions();
  for ( const QgsProcessingParameterDefinition *param : defs )
  {
    if ( param->flags() & Qgis::ProcessingParameterFlag::Advanced )
    {
      grpAdvanced->show();
      break;
    }
  }

  if ( !algorithm() )
  {
    return;
  }

  QgsProcessingParameterWidgetContext widgetContext = QgsGui::processingGuiRegistry()->createWidgetContext();
  widgetContext.setMessageBar( mMessageBar );

  if ( auto modelAlgorithm = dynamic_cast< const QgsProcessingModelAlgorithm * >( mAlgorithm ) )
  {
    widgetContext.setModel( const_cast< QgsProcessingModelAlgorithm * >( modelAlgorithm ) );
  }

  QString inPlaceInputParameterName = u"INPUT"_s;
  if ( auto featureAlgorithm = dynamic_cast< const QgsProcessingFeatureBasedAlgorithm * >( mAlgorithm ) )
  {
    inPlaceInputParameterName = featureAlgorithm->inputParameterName();
  }

  bool hasNonInputParams = false;
  for ( const QgsProcessingParameterDefinition *definition : defs )
  {
    if ( definition->name() != inPlaceInputParameterName && definition->name() != "OUTPUT"_L1 )
    {
      hasNonInputParams = true;
      break;
    }
  }

  if ( mInPlace && !hasNonInputParams )
  {
    QWidget *widget = new QWidget( this );
    QVBoxLayout *layout = new QVBoxLayout( widget );
    QLabel *label = new QLabel( widget );
    label->setWordWrap( true );

    const QString layerName = mActiveLayer ? mActiveLayer->name() : QString();
    const QString infoText = tr( "<i>No additional parameters are required. This algorithm will activate edit mode and modify the features on layer <b>%1</b> in place.</i>" ).arg( layerName );
    label->setText( infoText );

    layout->addWidget( label );
    layout->addStretch();
    addExtraWidget( widget );
  }

  // Create widgets and put them in layouts
  for ( const QgsProcessingParameterDefinition *definition : defs )
  {
    if ( definition->flags().testFlag( Qgis::ProcessingParameterFlag::Hidden ) )
    {
      continue;
    }

    if ( definition->isDestination() )
    {
      continue;
    }

    if ( mInPlace && ( definition->name() == inPlaceInputParameterName || definition->name() == "OUTPUT"_L1 ) )
    {
      // don't show the input/output parameter widgets in in-place mode
      // we still need to CREATE them, because other wrappers may need to interact
      // with them (e.g. those parameters which need the input layer for field
      // selections/crs properties/etc)
      QgsProcessingHiddenWidgetWrapper *wrapper = new QgsProcessingHiddenWidgetWrapper( definition, Qgis::ProcessingMode::Standard, this );
      wrapper->setLinkedVectorLayer( qobject_cast< QgsVectorLayer * >( mActiveLayer.get() ) );
      mWrappers.insert( definition->name(), wrapper );
      continue;
    }

    QgsAbstractProcessingParameterWidgetWrapper *wrapper = QgsGui::processingGuiRegistry()->createParameterWidgetWrapper( definition, Qgis::ProcessingMode::Standard );
    if ( !wrapper )
    {
      continue;
    }

    wrapper->setDialog( parentWidget() );
    wrapper->setWidgetContext( widgetContext );
    wrapper->registerProcessingContextGenerator( this );
    wrapper->registerProcessingParametersGenerator( this );
    mWrappers.insert( definition->name(), wrapper );

    QWidget *widget = wrapper->createWrappedWidget( *context );
    connect( wrapper, &QgsAbstractProcessingParameterWidgetWrapper::widgetValueHasChanged, this, &QgsProcessingParametersWidget::parameterChanged );

    if ( widget )
    {
      if ( QLabel *label = wrapper->createWrappedLabel() )
      {
        addParameterLabel( definition, label );
      }
      addParameterWidget( definition, widget, wrapper->stretch() );
    }
  }

  const QgsProcessingParameterDefinitions destinationDefinitions = algorithm()->destinationParameterDefinitions();
  for ( const QgsProcessingParameterDefinition *output : destinationDefinitions )
  {
    if ( output->flags().testFlag( Qgis::ProcessingParameterFlag::Hidden ) )
    {
      continue;
    }

    if ( mInPlace && ( output->name() == inPlaceInputParameterName || output->name() == "OUTPUT"_L1 ) )
    {
      continue;
    }

    QgsAbstractProcessingParameterWidgetWrapper *wrapper = QgsGui::processingGuiRegistry()->createParameterWidgetWrapper( output, Qgis::ProcessingMode::Standard );
    if ( !wrapper )
    {
      continue;
    }

    wrapper->setWidgetContext( widgetContext );
    wrapper->registerProcessingContextGenerator( this );
    wrapper->registerProcessingParametersGenerator( this );
    mWrappers.insert( output->name(), wrapper );

    if ( QLabel *label = wrapper->createWrappedLabel() )
    {
      addOutputLabel( label );
    }

    QWidget *widget = wrapper->createWrappedWidget( *context );
    addOutputWidget( widget, wrapper->stretch() );
  }

  const QList< QgsAbstractProcessingParameterWidgetWrapper * > wrapperList = mWrappers.values();
  for ( auto it = mWrappers.constBegin(); it != mWrappers.constEnd(); ++it )
  {
    it.value()->postInitialize( wrapperList );
  }
}

void QgsProcessingParametersWidget::addParameterWidget( const QgsProcessingParameterDefinition *parameter, QWidget *widget, int stretch )
{
  if ( parameter->flags() & Qgis::ProcessingParameterFlag::Advanced )
    mAdvancedGroupLayout->addWidget( widget, stretch );
  else
    mScrollAreaLayout->insertWidget( mScrollAreaLayout->count() - 2, widget, stretch );
}

void QgsProcessingParametersWidget::addParameterLabel( const QgsProcessingParameterDefinition *parameter, QWidget *label )
{
  if ( parameter->flags() & Qgis::ProcessingParameterFlag::Advanced )
    mAdvancedGroupLayout->addWidget( label );
  else
    mScrollAreaLayout->insertWidget( mScrollAreaLayout->count() - 2, label );
}

void QgsProcessingParametersWidget::addOutputLabel( QWidget *label )
{
  mScrollAreaLayout->insertWidget( mScrollAreaLayout->count() - 1, label );
}

void QgsProcessingParametersWidget::addOutputWidget( QWidget *widget, int stretch )
{
  mScrollAreaLayout->insertWidget( mScrollAreaLayout->count() - 1, widget, stretch );
}

void QgsProcessingParametersWidget::addExtraWidget( QWidget *widget )
{
  mScrollAreaLayout->addWidget( widget );
}

///@endcond
