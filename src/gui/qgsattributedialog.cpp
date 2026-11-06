/***************************************************************************
                         qgsattributedialog.cpp  -  description
                             -------------------
    begin                : October 2004
    copyright            : (C) 2004 by Marco Hugentobler
    email                : marco.hugentobler@autoform.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsattributedialog.h"

#include "qgsactionmenu.h"
#include "qgsattributeform.h"
#include "qgshighlight.h"
#include "qgsmaplayeractioncontext.h"
#include "qgsmessagebar.h"
#include "qgspanelwidgetstack.h"
#include "qgssettings.h"

#include <QString>

#include "moc_qgsattributedialog.cpp"

using namespace Qt::StringLiterals;

//
// QgsAttributePanelWidget
//

QgsAttributePanelWidget::QgsAttributePanelWidget( QgsVectorLayer *vl, QgsFeature *thepFeature, bool featureOwner, QWidget *parent, bool showDialogButtons, const QgsAttributeEditorContext &context )
  : QgsPanelWidget( parent )
  , mOwnedFeature( featureOwner ? thepFeature : nullptr )
{
  init( vl, thepFeature, context, showDialogButtons );
}

QgsAttributePanelWidget::~QgsAttributePanelWidget()
{}

void QgsAttributePanelWidget::accept()
{
  QString error;
  const bool didSave = mAttributeForm->saveWithDetails( &error );
  if ( didSave )
  {
    emit accepted();
  }
  else
  {
    if ( error.isEmpty() )
      error = tr( "An unknown error was encountered saving attributes" );

    mMessageBar->pushMessage( QString(), error, Qgis::MessageLevel::Critical );
  }
}

void QgsAttributePanelWidget::reject()
{
  // Delete any actions on other layers that may have been triggered from this dialog
  if ( mAttributeForm->mode() == QgsAttributeEditorContext::AddFeatureMode )
    mTrackedVectorLayerTools.rollback();

  emit rejected();
}

void QgsAttributePanelWidget::setHighlight( QgsHighlight *h )
{
  delete mHighlight;

  mHighlight = h;
}

QgsAttributeForm *QgsAttributePanelWidget::attributeForm() const
{
  return mAttributeForm;
}

QgsMessageBar *QgsAttributePanelWidget::messageBar()
{
  return mMessageBar;
}

const QgsFeature *QgsAttributePanelWidget::feature() const
{
  return &mAttributeForm->feature();
}

bool QgsAttributePanelWidget::editable() const
{
  return mAttributeForm->editable();
}

void QgsAttributePanelWidget::setMode( QgsAttributeEditorContext::Mode mode )
{
  mAttributeForm->setMode( mode );
  mMenu->setMode( mode );

  if ( !mMenu->isEmpty() && !mMenuBar )
  {
    mMenuBar = new QMenuBar( this );
    mMenuBar->addMenu( mMenu );
    layout()->setMenuBar( mMenuBar );
  }
  else if ( mMenu->isEmpty() && mMenuBar )
  {
    layout()->setMenuBar( nullptr );
    delete mMenuBar;
    mMenuBar = nullptr;
  }
}

void QgsAttributePanelWidget::setEditCommandMessage( const QString &message )
{
  mAttributeForm->setEditCommandMessage( message );
}

void QgsAttributePanelWidget::setExtraContextScope( QgsExpressionContextScope *extraScope )
{
  mAttributeForm->setExtraContextScope( extraScope );
}

QgsMapLayerActionContext QgsAttributePanelWidget::createActionContext()
{
  QgsMapLayerActionContext context;
  context.setMessageBar( mMessageBar );
  return context;
}

bool QgsAttributePanelWidget::event( QEvent *e )
{
  if ( e->type() == QEvent::WindowActivate && mHighlight )
    mHighlight->show();
  else if ( e->type() == QEvent::WindowDeactivate && mHighlight )
    mHighlight->hide();

  return QgsPanelWidget::event( e );
}

void QgsAttributePanelWidget::init( QgsVectorLayer *layer, QgsFeature *feature, const QgsAttributeEditorContext &context, bool showDialogButtons )
{
  QgsAttributeEditorContext trackedContext = context;
  setPanelTitle( tr( "Feature Attributes" ) );
  setLayout( new QGridLayout() );
  layout()->setContentsMargins( 0, 0, 0, 0 );
  mMessageBar = new QgsMessageBar( this );
  mMessageBar->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Fixed );
  layout()->addWidget( mMessageBar );

  setLayout( layout() );

  mTrackedVectorLayerTools.setVectorLayerTools( trackedContext.vectorLayerTools() );
  trackedContext.setVectorLayerTools( &mTrackedVectorLayerTools );
  if ( showDialogButtons )
    trackedContext.setFormMode( QgsAttributeEditorContext::StandaloneDialog );

  mAttributeForm = new QgsAttributeForm( layer, *feature, trackedContext, this );
  mAttributeForm->disconnectButtonBox();
  layout()->addWidget( mAttributeForm );
  QDialogButtonBox *buttonBox = mAttributeForm->findChild<QDialogButtonBox *>();
  connect( buttonBox, &QDialogButtonBox::rejected, this, &QgsAttributePanelWidget::reject );
  connect( buttonBox, &QDialogButtonBox::accepted, this, &QgsAttributePanelWidget::accept );
  connect( layer, &QObject::destroyed, this, &QWidget::close );

  mMenu = new QgsActionMenu( layer, mAttributeForm->feature(), u"Feature"_s, this );
  mMenu->setActionContextGenerator( this );
  if ( !mMenu->isEmpty() )
  {
    mMenuBar = new QMenuBar( this );
    mMenuBar->addMenu( mMenu );
    layout()->setMenuBar( mMenuBar );
  }
  focusNextChild();
}


//
// QgsAttributeDialog
//

QgsAttributeDialog::QgsAttributeDialog( QgsVectorLayer *vl, QgsFeature *thepFeature, bool featureOwner, QWidget *parent, bool showDialogButtons, const QgsAttributeEditorContext &context )
  : QDialog( parent )
{
  mWidgetStack = new QgsPanelWidgetStack();
  auto vLayout = new QVBoxLayout();
  vLayout->addWidget( mWidgetStack );
  setLayout( vLayout );

  mPanelWidget = new QgsAttributePanelWidget( vl, thepFeature, featureOwner, nullptr, showDialogButtons, context );
  mPanelWidget->setDockMode( true );

  mWidgetStack->setMainPanel( mPanelWidget );

  connect( mPanelWidget, &QgsAttributePanelWidget::rejected, this, &QgsAttributeDialog::onPanelRejected );
  connect( mPanelWidget, &QgsAttributePanelWidget::accepted, this, &QgsAttributeDialog::accept );
  connect( vl, &QObject::destroyed, this, &QWidget::close );

  setWindowTitle( tr( "%1 - Feature Attributes" ).arg( vl->name() ) );
}

QgsAttributeDialog::~QgsAttributeDialog()
{
  saveGeometry();
}

void QgsAttributeDialog::saveGeometry()
{
  // WARNING!!!! Don't use QgsGui::enableAutoGeometryRestore for this dialog -- the object name
  // is dynamic and is set to match the layer/feature combination.
  QgsSettings().setValue( u"Windows/AttributeDialog/geometry"_s, QDialog::saveGeometry() );
}

void QgsAttributeDialog::restoreGeometry()
{
  // WARNING!!!! Don't use QgsGui::enableAutoGeometryRestore for this dialog -- the object name
  // is dynamic and is set to match the layer/feature combination.
  QDialog::restoreGeometry( QgsSettings().value( u"Windows/AttributeDialog/geometry"_s ).toByteArray() );
}

void QgsAttributeDialog::setHighlight( QgsHighlight *h )
{
  mPanelWidget->setHighlight( h );
}

QgsAttributeForm *QgsAttributeDialog::attributeForm() const
{
  return mPanelWidget->attributeForm();
}

const QgsFeature *QgsAttributeDialog::feature() const
{
  return mPanelWidget->feature();
}

bool QgsAttributeDialog::editable() const
{
  return mPanelWidget->editable();
}

void QgsAttributeDialog::show()
{
  QDialog::show();

  raise();
  activateWindow();
}

void QgsAttributeDialog::onPanelRejected()
{
  mPanelWasRejected = true;
  reject();
}

void QgsAttributeDialog::showEvent( QShowEvent *event )
{
  QDialog::showEvent( event );
  // We cannot call restoreGeometry() in the constructor or init because the dialog is not yet visible
  // and the geometry restoration will not take the window decorations (frame) into account.
  if ( mFirstShow )
  {
    mFirstShow = false;
    restoreGeometry();
  }
}

void QgsAttributeDialog::reject()
{
  if ( !mPanelWasRejected )
  {
    disconnect( mPanelWidget, &QgsAttributePanelWidget::rejected, this, &QgsAttributeDialog::onPanelRejected );
    mPanelWidget->reject();
  }
  QDialog::reject();
}

void QgsAttributeDialog::setMode( QgsAttributeEditorContext::Mode mode )
{
  mPanelWidget->setMode( mode );
}

void QgsAttributeDialog::setEditCommandMessage( const QString &message )
{
  mPanelWidget->setEditCommandMessage( message );
}

void QgsAttributeDialog::setExtraContextScope( QgsExpressionContextScope *extraScope )
{
  mPanelWidget->setExtraContextScope( extraScope );
}

QgsMapLayerActionContext QgsAttributeDialog::createActionContext()
{
  QgsMapLayerActionContext context = mPanelWidget->createActionContext();
  context.setAttributeDialog( this );
  return context;
}
