/***************************************************************************
                         qgsmodelchildalgorithmwidgets.cpp
                         ------------------------------------------
    begin                : August 2026
    copyright            : (C) 2026 by Nyall Dawson
    email                : nyall dot dawson at gmail dot com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include "qgsmodelchildalgorithmwidgets.h"

#include "qgscollapsiblegroupbox.h"
#include "qgscolorbutton.h"
#include "qgsgui.h"
#include "qgsmessagebar.h"
#include "qgsmodeldesignerdialog.h"
#include "qgsprocessingalgorithm.h"
#include "qgsprocessingalgorithmconfigurationwidget.h"
#include "qgsprocessingguiregistry.h"
#include "qgsprocessingmodelalgorithm.h"
#include "qgsprocessingmodelerparameterwidget.h"
#include "qgsprocessingwidgetwrapper.h"
#include "qgsscrollarea.h"

#include <QLabel>
#include <QLineEdit>
#include <QString>
#include <QVBoxLayout>

using namespace Qt::StringLiterals;

//
// QgsProcessingModelerParametersPanelWidget
//

QgsProcessingModelerParametersPanelWidget::QgsProcessingModelerParametersPanelWidget(
  const QgsProcessingAlgorithm *alg, QgsProcessingModelAlgorithm *model, const QString &algName, const QVariantMap &configuration, QWidget *parent, QgsProcessingContext *context, QWidget *dialog
)
  : QgsPanelWidget( parent )
  , mAlgorithm( alg ? alg->create() : nullptr )
  , mModel( model )
  , mChildId( algName )
  , mConfiguration( configuration )
  , mContext( context )
  , mDialog( dialog )
{
  //mContextGenerator = std::make_unique< InternalContextGenerator >( mContext );
  setupUi();

  setStateFromChildAlgorithm();
}

QgsProcessingModelerParametersPanelWidget::~QgsProcessingModelerParametersPanelWidget() = default;

const QgsProcessingAlgorithm *QgsProcessingModelerParametersPanelWidget::algorithm() const
{
  return mAlgorithm.get();
}

void QgsProcessingModelerParametersPanelWidget::setupUi()
{
  auto mainLayout = new QVBoxLayout();
  mainLayout->setContentsMargins( 0, 0, 0, 0 );

  auto verticalLayout = new QVBoxLayout();

  mMessageBar = new QgsMessageBar();
  mMessageBar->setSizePolicy( QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Fixed );
  verticalLayout->addWidget( mMessageBar );

  auto hLayout = new QHBoxLayout();
  hLayout->setContentsMargins( 0, 0, 0, 0 );
  hLayout->addWidget( new QLabel( tr( "Description" ) ) );
  mDescriptionBox = new QLineEdit();
  if ( mAlgorithm )
  {
    mDescriptionBox->setText( mAlgorithm->displayName() );
  }
  hLayout->addWidget( mDescriptionBox );
  connect( mDescriptionBox, &QLineEdit::textChanged, this, &QgsProcessingModelerParametersPanelWidget::emitChangedSignal );

  verticalLayout->addLayout( hLayout );

  auto line = new QFrame();
  line->setFrameShape( QFrame::Shape::HLine );
  line->setFrameShadow( QFrame::Shadow::Sunken );
  verticalLayout->addWidget( line );

  // TODO -- fix this!
  QgsProcessingParameterWidgetContext widgetContext;
  widgetContext.setProject( QgsProject::instance() );
  widgetContext.setModel( mModel );
  widgetContext.setModelChildAlgorithmId( mChildId );

  if ( mAlgorithm )
  {
    mAlgorithmItem = QgsGui::processingGuiRegistry()->algorithmConfigurationWidget( mAlgorithm.get() );
    if ( mAlgorithmItem )
    {
      mAlgorithmItem->setWidgetContext( widgetContext );
      mAlgorithmItem->registerProcessingContextGenerator( mContextGenerator.get() );
      if ( !mConfiguration.isEmpty() )
        mAlgorithmItem->setConfiguration( mConfiguration );

      verticalLayout->addWidget( mAlgorithmItem );
    }
  }

  auto grpAdvanced = new QgsCollapsibleGroupBox( tr( "Advanced Parameters" ) );
  auto grpAdvancedVLayout = new QVBoxLayout();
  grpAdvanced->setLayout( grpAdvancedVLayout );
  grpAdvanced->hide();
  verticalLayout->addWidget( grpAdvanced );

  if ( mAlgorithm )
  {
    const QList<const QgsProcessingParameterDefinition *> parameters = mAlgorithm->parameterDefinitions();
    // only show advanced group when there are SOME advanced parameters:
    for ( const QgsProcessingParameterDefinition *param : parameters )
    {
      if ( param->flags() & Qgis::ProcessingParameterFlag::Advanced )
      {
        grpAdvanced->show();
        break;
      }
    }

    for ( const QgsProcessingParameterDefinition *param : parameters )
    {
      if ( param->isDestination() || ( param->flags() & Qgis::ProcessingParameterFlag::Hidden ) )
        continue;

      // note: ownership of widget is transferred to parent layout below
      QgsProcessingModelerParameterWidget *widget = QgsGui::processingGuiRegistry()->createModelerParameterWidget( mModel, mChildId, param, *mContext );
      if ( !widget )
        continue;

      widget->setDialog( mDialog );
      widget->setWidgetContext( widgetContext );
      widget->registerProcessingContextGenerator( mContextGenerator.get() );
      connect( widget, &QgsProcessingModelerParameterWidget::changed, this, &QgsProcessingModelerParametersPanelWidget::emitChangedSignal );
      mWrappers.insert( param->name(), widget );

      QLabel *label = widget->createLabel();
      // advanced parameters get inserted to a different layout:
      if ( param->flags() & Qgis::ProcessingParameterFlag::Advanced )
      {
        if ( label )
          grpAdvancedVLayout->addWidget( label );
        grpAdvancedVLayout->addWidget( widget );
      }
      else
      {
        if ( label )
          verticalLayout->insertWidget( verticalLayout->count() - 1, label );
        verticalLayout->insertWidget( verticalLayout->count() - 1, widget );
      }
    }

    const QList< const QgsProcessingParameterDefinition * > destinationsParameters = mAlgorithm->destinationParameterDefinitions();
    for ( const QgsProcessingParameterDefinition *output : destinationsParameters )
    {
      if ( output->flags() & Qgis::ProcessingParameterFlag::Hidden )
        continue;

      // note: ownership of widget is transferred to parent layout below
      QgsProcessingModelerParameterWidget *widget = QgsGui::processingGuiRegistry()->createModelerParameterWidget( mModel, mChildId, output, *mContext );
      if ( !widget )
        continue;

      widget->setDialog( mDialog );
      widget->setWidgetContext( widgetContext );
      widget->registerProcessingContextGenerator( mContextGenerator.get() );
      connect( widget, &QgsProcessingModelerParameterWidget::changed, this, &QgsProcessingModelerParametersPanelWidget::emitChangedSignal );

      mWrappers.insert( output->name(), widget );

      if ( QLabel *label = widget->createLabel() )
        verticalLayout->addWidget( label );

      verticalLayout->addWidget( widget );
    }
  }

  // TODO ... really??
  auto spacerLabel = new QLabel( u" "_s );
  verticalLayout->addWidget( spacerLabel );

  auto dependenciesLabel = new QLabel( tr( "Dependencies" ) );
  mDependenciesPanel = new QgsModelChildDependenciesWidget( this, mModel, mChildId );
  verticalLayout->addWidget( dependenciesLabel );
  verticalLayout->addWidget( mDependenciesPanel );
  // TODO... really?
  verticalLayout->addStretch( 1000 );

  auto scrollAreaContainer = new QVBoxLayout();
  scrollAreaContainer->setSpacing( 2 );
  scrollAreaContainer->setContentsMargins( 0, 0, 0, 0 );

  auto paramPanel = new QWidget();
  paramPanel->setLayout( verticalLayout );

  auto scrollArea = new QgsScrollArea();
  scrollArea->setWidget( paramPanel );
  scrollArea->setWidgetResizable( true );
  scrollArea->setFrameStyle( QFrame::Shape::NoFrame );

  scrollAreaContainer->addWidget( scrollArea );

  auto scrollAreaContainerWidget = new QWidget();
  scrollAreaContainerWidget->setLayout( scrollAreaContainer );
  mainLayout->addWidget( scrollAreaContainerWidget );
  setLayout( mainLayout );
}

void QgsProcessingModelerParametersPanelWidget::emitChangedSignal()
{
  if ( !mBlockChangesSignal )
    emit widgetChanged();
}

void QgsProcessingModelerParametersPanelWidget::setStateFromChildAlgorithm()
{
  if ( mChildId.isEmpty() || !mModel->childAlgorithms().contains( mChildId ) )
    return;

  const QgsProcessingModelChildAlgorithm childAlgorithm = mModel->childAlgorithm( mChildId );
  const QgsProcessingAlgorithm *sourceAlgorithm = mAlgorithm.get();
  std::unique_ptr< QgsProcessingAlgorithm > tempAlgorithm;
  if ( mAlgorithmItem )
  {
    // for algorithms with a custom config widget, we need to iterate over parameters defined
    // when that algorithm is created respecting the custom config widget.

    // WARNING: we CANNOT overwrite mAlgorithm here, as all the exiting wrappers have already
    // been created with references to that algorithm instance!
    tempAlgorithm.reset( childAlgorithm.algorithm()->create( mAlgorithmItem->configuration() ) );
    sourceAlgorithm = tempAlgorithm.get();
  }

  if ( !sourceAlgorithm )
    return;

  const QList< const QgsProcessingParameterDefinition * > parameters = sourceAlgorithm->parameterDefinitions();
  const QList< const QgsProcessingParameterDefinition * > destinationParameters = sourceAlgorithm->destinationParameterDefinitions();

  mBlockChangesSignal++;

  mDescriptionBox->setText( childAlgorithm.description() );

  for ( const QgsProcessingParameterDefinition *param : parameters )
  {
    if ( param->isDestination() || ( param->flags() & Qgis::ProcessingParameterFlag::Hidden ) )
      continue;

    QList< QgsProcessingModelChildParameterSource > valueList;
    std::optional< QgsProcessingModelChildParameterSource > singleValue;
    if ( childAlgorithm.parameterSources().contains( param->name() ) )
    {
      valueList = childAlgorithm.parameterSources().value( param->name() );
      if ( valueList.size() == 1 )
      {
        singleValue = valueList.at( 0 );
        valueList.clear();
      }
    }

    if ( valueList.isEmpty() && !singleValue.has_value() )
    {
      singleValue = QgsProcessingModelChildParameterSource::fromStaticValue( param->defaultValue() );
    }

    if ( mWrappers.contains( param->name() ) )
    {
      if ( !valueList.isEmpty() )
      {
        mWrappers.value( param->name() )->setWidgetValue( valueList );
      }
      else if ( singleValue.has_value() )
      {
        mWrappers.value( param->name() )->setWidgetValue( singleValue.value() );
      }
    }
  }

  for ( const QgsProcessingParameterDefinition *output : destinationParameters )
  {
    if ( output->flags() & Qgis::ProcessingParameterFlag::Hidden )
      continue;

    QString modelOutputName;
    const QMap< QString, QgsProcessingModelOutput > outputs = childAlgorithm.modelOutputs();
    for ( auto it = outputs.constBegin(); it != outputs.constEnd(); ++it )
    {
      if ( it.value().childId() == mChildId && it.value().childOutputName() == output->name() )
      {
        // this destination parameter is linked to a model output
        modelOutputName = it.value().name();
        mPreviousOutputDefinitions.insert( output->name(), it.value() );
        break;
      }
    }

    QList< QgsProcessingModelChildParameterSource > valueList;
    std::optional< QgsProcessingModelChildParameterSource > singleValue;
    if ( modelOutputName.isEmpty() && childAlgorithm.parameterSources().contains( output->name() ) )
    {
      valueList = childAlgorithm.parameterSources().value( output->name() );
      if ( valueList.size() == 1 )
      {
        singleValue = valueList.at( 0 );
        valueList.clear();
      }
    }

    if ( mWrappers.contains( output->name() ) )
    {
      QgsProcessingModelerParameterWidget *wrapper = mWrappers.value( output->name() );
      if ( !modelOutputName.isEmpty() )
      {
        wrapper->setToModelOutput( modelOutputName );
      }
      else if ( !valueList.empty() )
      {
        wrapper->setWidgetValue( valueList );
      }
      else if ( singleValue.has_value() )
      {
        wrapper->setWidgetValue( singleValue.value() );
      }
      else if ( output->defaultValue().isValid() )
      {
        wrapper->setWidgetValue( QgsProcessingModelChildParameterSource::fromStaticValue( output->defaultValue() ) );
      }
    }
  }

  mDependenciesPanel->setValue( childAlgorithm.dependencies() );

  mBlockChangesSignal--;
}

std::unique_ptr< QgsProcessingModelChildAlgorithm > QgsProcessingModelerParametersPanelWidget::createAlgorithm()
{
  if ( !mAlgorithm )
    return nullptr;

  auto alg = std::make_unique< QgsProcessingModelChildAlgorithm >( mAlgorithm->id() );
  if ( mChildId.isEmpty() )
    alg->generateChildId( *mModel );
  else
    alg->setChildId( mChildId );

  alg->setDescription( mDescriptionBox->text() );
  if ( mAlgorithmItem )
  {
    alg->setConfiguration( mAlgorithmItem->configuration() );
    mAlgorithm.reset( alg->algorithm()->create( mAlgorithmItem->configuration() ) );
  }

  const QList<const QgsProcessingParameterDefinition * > parameterDefinitions = mAlgorithm->parameterDefinitions();
  for ( const QgsProcessingParameterDefinition *param : parameterDefinitions )
  {
    if ( param->isDestination() || ( param->flags() & Qgis::ProcessingParameterFlag::Hidden ) )
      continue;

    if ( !mWrappers.contains( param->name() ) )
      continue;

    const QgsProcessingModelerParameterWidget *wrapper = mWrappers.value( param->name() );
    QVariant val = wrapper->value();

    QList< QgsProcessingModelChildParameterSource > sources;
    if ( val.userType() == qMetaTypeId< QgsProcessingModelChildParameterSource >() )
    {
      sources.append( val.value< QgsProcessingModelChildParameterSource >() );
    }
    else if ( val.userType() == QMetaType::Type::QVariantList )
    {
      const QVariantList list = val.toList();
      for ( const QVariant &subValue : list )
      {
        if ( subValue.userType() == qMetaTypeId< QgsProcessingModelChildParameterSource >() )
          sources.append( subValue.value< QgsProcessingModelChildParameterSource >() );
        else
          sources.append( QgsProcessingModelChildParameterSource::fromStaticValue( subValue ) );
      }
    }
    else
    {
      sources.append( QgsProcessingModelChildParameterSource::fromStaticValue( val ) );
    }

    bool valid = true;
    for ( const QgsProcessingModelChildParameterSource &subValue : std::as_const( sources ) )
    {
      if ( subValue.source() == Qgis::ProcessingModelChildParameterSource::StaticValue && !param->checkValueIsAcceptable( subValue.staticValue() ) )
      {
        valid = false;
        break;
      }
    }

    if ( valid )
      alg->addParameterSources( param->name(), sources );
  }

  QMap< QString, QgsProcessingModelOutput > outputs;
  const QList< const QgsProcessingParameterDefinition * > destinationParameters = mAlgorithm->destinationParameterDefinitions();
  for ( const QgsProcessingParameterDefinition *output : destinationParameters )
  {
    if ( !( output->flags() & Qgis::ProcessingParameterFlag::Hidden ) )
    {
      if ( mWrappers.contains( output->name() ) )
      {
        QgsProcessingModelerParameterWidget *wrapper = mWrappers.value( output->name() );
        if ( wrapper->isModelOutput() )
        {
          const QString name = wrapper->modelOutputName();
          if ( !name.isEmpty() )
          {
            // if there was a previous output definition already for this output, we start with it,
            // otherwise we'll lose any existing output comments, coloring, position, etc
            QgsProcessingModelOutput modelOutput = mPreviousOutputDefinitions.value( output->name(), QgsProcessingModelOutput( name, name ) );
            modelOutput.setDescription( name );
            modelOutput.setChildId( alg->childId() );
            modelOutput.setChildOutputName( output->name() );
            outputs.insert( name, modelOutput );
          }
        }
        else
        {
          const QVariant val = wrapper->value();
          QList< QgsProcessingModelChildParameterSource > sources;
          if ( val.userType() == qMetaTypeId< QgsProcessingModelChildParameterSource >() )
          {
            sources.append( val.value< QgsProcessingModelChildParameterSource >() );
          }
          else if ( val.userType() == QMetaType::Type::QVariantList )
          {
            const QVariantList list = val.toList();
            for ( const QVariant &subValue : list )
            {
              if ( subValue.userType() == qMetaTypeId< QgsProcessingModelChildParameterSource >() )
                sources.append( subValue.value< QgsProcessingModelChildParameterSource >() );
              else
                sources.append( QgsProcessingModelChildParameterSource::fromStaticValue( subValue ) );
            }
          }

          alg->addParameterSources( output->name(), sources );
        }
      }
    }

    if ( output->flags() & Qgis::ProcessingParameterFlag::IsModelOutput )
    {
      if ( !outputs.contains( output->name() ) )
      {
        QgsProcessingModelOutput modelOutput( output->name(), output->name() );
        modelOutput.setChildId( alg->childId() );
        modelOutput.setChildOutputName( output->name() );
        outputs.insert( output->name(), modelOutput );
      }
    }
  }

  alg->setModelOutputs( outputs );
  alg->setDependencies( mDependenciesPanel->value() );

  return alg;
}


//
// QgsProcessingModelerParametersWidget
//

QgsProcessingModelerParametersWidget::QgsProcessingModelerParametersWidget(
  const QgsProcessingAlgorithm *alg, QgsProcessingModelAlgorithm *model, const QString &algName, const QVariantMap &configuration, QWidget *parent, QgsProcessingContext *context, QWidget *dialog
)
  : QgsProcessingModelConfigWidget( parent )
  , mAlg( alg ? alg->create() : nullptr )
{
  mParametersPanel = new QgsProcessingModelerParametersPanelWidget( alg, model, algName, configuration, this, context, dialog );
  connect( mParametersPanel, &QgsProcessingModelerParametersPanelWidget::widgetChanged, this, &QgsProcessingModelConfigWidget::widgetChanged );

  setupUi();
}

QgsProcessingModelerParametersWidget::~QgsProcessingModelerParametersWidget() = default;

const QgsProcessingAlgorithm *QgsProcessingModelerParametersWidget::algorithm() const
{
  return mAlg.get();
}

void QgsProcessingModelerParametersWidget::switchToCommentTab()
{
  mTab->setCurrentIndex( 1 );
  mCommentEdit->setFocus();
  mCommentEdit->selectAll();
}

void QgsProcessingModelerParametersWidget::setupUi()
{
  auto mainLayout = new QVBoxLayout();
  mainLayout->setContentsMargins( 0, 0, 0, 0 );

  mTab = new QTabWidget();
  mainLayout->addWidget( mTab );

  mPanelWidgetStack = new QgsPanelWidgetStack();
  mParametersPanel->setDockMode( true );
  mPanelWidgetStack->setMainPanel( mParametersPanel );

  mTab->addTab( mPanelWidgetStack, tr( "Properties" ) );

  auto commentLayout = new QVBoxLayout();
  mCommentEdit = new QTextEdit();
  mCommentEdit->setAcceptRichText( false );
  commentLayout->addWidget( mCommentEdit, 1 );

  auto hl = new QHBoxLayout();
  hl->setContentsMargins( 0, 0, 0, 0 );
  hl->addWidget( new QLabel( tr( "Color" ) ) );

  mCommentColorButton = new QgsColorButton();
  mCommentColorButton->setAllowOpacity( true );
  mCommentColorButton->setWindowTitle( tr( "Comment Color" ) );
  mCommentColorButton->setShowNull( true, tr( "Default" ) );
  hl->addWidget( mCommentColorButton );
  commentLayout->addLayout( hl );

  auto commentWidget = new QWidget();
  commentWidget->setLayout( commentLayout );
  mTab->addTab( commentWidget, tr( "Comments" ) );

  setLayout( mainLayout );
}

void QgsProcessingModelerParametersWidget::setComments( const QString &text )
{
  mCommentEdit->setPlainText( text );
}

QString QgsProcessingModelerParametersWidget::comments() const
{
  return mCommentEdit->toPlainText();
}

void QgsProcessingModelerParametersWidget::setCommentColor( const QColor &color )
{
  if ( color.isValid() )
    mCommentColorButton->setColor( color );
  else
    mCommentColorButton->setToNull();
}

QColor QgsProcessingModelerParametersWidget::commentColor() const
{
  return !mCommentColorButton->isNull() ? mCommentColorButton->color() : QColor();
}

void QgsProcessingModelerParametersWidget::setStateFromChildAlgorithm()
{
  mParametersPanel->setStateFromChildAlgorithm();
}

std::unique_ptr< QgsProcessingModelChildAlgorithm > QgsProcessingModelerParametersWidget::createAlgorithm()
{
  std::unique_ptr< QgsProcessingModelChildAlgorithm > alg = mParametersPanel->createAlgorithm();
  if ( alg )
  {
    alg->comment()->setDescription( comments() );
    alg->comment()->setColor( commentColor() );
  }
  return alg;
}
