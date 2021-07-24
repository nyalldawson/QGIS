/***************************************************************************
    qgsdatamanagerdialog.h
    ----------------------
    begin                : July 2021
    copyright            : (C) 2021 by Nyall Dawson
    email                : nyall dot dawson at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QSettings>
#include <QMessageBox>
#include <QKeyEvent>
#include <QMetaObject>

#include "qgsdatamanagerdialog.h"
#include "qgsapplication.h"
#include "qgsdataitem.h"
#include "qgsbrowsermodel.h"
#include "qgsencodingfiledialog.h"
//#include "qgsgenericprojectionselector.h"
#include "qgslogger.h"
#include "qgsconditionalstyle.h"
#include "qgsproject.h"
#include "qgsmaptoolpan.h"
#include "qgsproviderregistry.h"
#include "qgsvectorlayer.h"
#include "qgsrasterlayer.h"
#include "qgsnewvectorlayerdialog.h"
#include "qgsattributetablemodel.h"
#include "qgsattributetablefiltermodel.h"
#include "qgscredentialdialog.h"
#include "qgsrasterdataprovider.h"
#include "qgslayeritem.h"
#include "qgsvectorlayercache.h"
#include "qgsdirectoryitem.h"

#ifdef ANDROID
#define QGIS_ICON_SIZE 32
#else
#define QGIS_ICON_SIZE 24
#endif

QgsDataManagerDialog::QgsDataManagerDialog( QWidget *parent, Qt::WindowFlags flags )
  : QMainWindow( parent, flags )
  , mDirtyMetadata( true )
  , mDirtyPreview( true )
  , mDirtyAttributes( true )
  , mLayer( nullptr )
  , mParamWidget( nullptr )
  , mAttributeTableFilterModel( nullptr )
{
  setupUi( this );

  mMapToolPan.reset( new QgsMapToolPan( mapCanvas ) );
  mapCanvas->setMapTool( mMapToolPan.get() );

  // Disable tabs by default
  tabWidget->setTabEnabled( tabWidget->indexOf( paramTab ), false );
  tabWidget->setTabEnabled( tabWidget->indexOf( metaTab ), false );
  tabWidget->setTabEnabled( tabWidget->indexOf( previewTab ), false );
  tabWidget->setTabEnabled( tabWidget->indexOf( attributesTab ), false );

  mModel = new QgsBrowserModel( treeView );
  treeView->setModel( mModel );

  // Last expanded is stored, don't cover whole height with file system
  //treeView->expand( mModel->index(0,0) );

  connect( treeView, SIGNAL( clicked( const QModelIndex & ) ), this, SLOT( itemClicked( const QModelIndex & ) ) );

  treeView->setExpandsOnDoubleClick( false );
  connect( treeView, SIGNAL( doubleClicked( const QModelIndex & ) ), this, SLOT( itemDoubleClicked( const QModelIndex & ) ) );
  connect( treeView, SIGNAL( expanded( const QModelIndex & ) ), this, SLOT( itemExpanded( const QModelIndex & ) ) );

  connect( tabWidget, SIGNAL( currentChanged( int ) ), this, SLOT( tabChanged() ) );

  connect( mActionNewVectorLayer, SIGNAL( triggered() ), this, SLOT( newVectorLayer() ) );

  connect( stopRenderingButton, SIGNAL( clicked() ), this, SLOT( stopRendering() ) );

  mapCanvas->setCanvasColor( Qt::white );

  //Set the icon size of for all the toolbars created in the future.
  QSettings settings;
  int size = settings.value( QStringLiteral( "/IconSize" ), QGIS_ICON_SIZE ).toInt();
  setIconSize( QSize( size, size ) );

  //Change all current icon sizes.
  const QList<QToolBar *> toolbars = findChildren<QToolBar *>();
  for ( QToolBar *toolbar : toolbars )
  {
    toolbar->setIconSize( QSize( size, size ) );
  }

  // set graphical credential requester
  new QgsCredentialDialog( this );
}

QgsDataManagerDialog::~QgsDataManagerDialog()
{

}

void QgsDataManagerDialog::expandPath( const QString &path )
{
  QModelIndex idx = mModel->findPath( path );
  if ( idx.isValid() )
  {
    treeView->expand( idx );
    treeView->scrollTo( idx, QAbstractItemView::PositionAtTop );
  }
}

void QgsDataManagerDialog::itemClicked( const QModelIndex &index )
{
  mIndex = index;

  QgsDataItem *item = mModel->dataItem( index );
  if ( !item )
    return;

  // Disable preview, attributes tab

  bool paramEnable = false;
  bool metaEnable = false;
  bool previewEnable = false;
  bool attributesEnable = false;

  // mark all tabs as dirty
  mDirtyMetadata = true;
  mDirtyPreview = true;
  mDirtyAttributes = true;

  // clear the previous stuff
  setLayer( nullptr );

  mapCanvas->setLayers( QList<QgsMapLayer *>() );
  metaTextBrowser->clear();
  if ( mParamWidget )
  {
    paramLayout->removeWidget( mParamWidget );
    mParamWidget->hide();
    delete mParamWidget;
    mParamWidget = nullptr;
  }

  // QgsProject deletes the previous layer(s) for us
  // TODO: in future we could cache the layers in the registry
  QgsProject::instance()->removeAllMapLayers();
  mLayer = nullptr;

  // this should probably go to the model and only emit signal when a layer is clicked
  mParamWidget = item->paramWidget();
  if ( mParamWidget )
  {
    paramLayout->addWidget( mParamWidget );
    mParamWidget->show();
    paramEnable = true;
  }

  QgsLayerItem *layerItem = qobject_cast<QgsLayerItem *>( mModel->dataItem( index ) );
  if ( layerItem )
  {
    bool res = layerClicked( layerItem );

    if ( res )
    {
      metaEnable = true;
      previewEnable = true;
      if ( mLayer->type() == QgsMapLayerType::VectorLayer )
      {
        attributesEnable = true;
      }
    }
  }
  else
  {
    mActionSetProjection->setEnabled( false );
  }

  // force update of the current tab
  updateCurrentTab();

  int selected = -1;
  if ( mLastTab.contains( item->metaObject()->className() ) )
  {
    selected = mLastTab[ item->metaObject()->className()];
  }

  // Enabling tabs call tabChanged !
  tabWidget->setTabEnabled( tabWidget->indexOf( paramTab ), paramEnable );
  tabWidget->setTabEnabled( tabWidget->indexOf( metaTab ), metaEnable );
  tabWidget->setTabEnabled( tabWidget->indexOf( previewTab ), previewEnable );
  tabWidget->setTabEnabled( tabWidget->indexOf( attributesTab ), attributesEnable );

  // select tab according last selection for this data item
  if ( selected >= 0 )
  {
    QgsDebugMsg( QString( "set tab %1 %2" ).arg( item->metaObject()->className() ).arg( selected ) );
    tabWidget->setCurrentIndex( selected );
  }

  QgsDebugMsg( QString( "clicked: %1 %2 %3" ).arg( index.row() ).arg( index.column() ).arg( item->name() ) );
}

bool QgsDataManagerDialog::layerClicked( QgsLayerItem *item )
{
  if ( !item )
    return false;

  //mActionSetProjection->setEnabled( item->capabilities2().testFlag( QgsLayerItem::SetCrs ) );

  QString uri = item->uri();
  if ( !uri.isEmpty() )
  {
    QgsMapLayerType type = item->mapLayerType();
    QString providerKey = item->providerKey();

    QgsDebugMsg( providerKey + " : " + uri );
    if ( type == QgsMapLayerType::VectorLayer )
    {
      mLayer = new QgsVectorLayer( uri, QString(), providerKey );
    }
    if ( type == QgsMapLayerType::RasterLayer )
    {
      mLayer = new QgsRasterLayer( uri, QLatin1String( "" ), providerKey );
    }
  }

  if ( !mLayer || !mLayer->isValid() )
  {
    qDebug( "No layer" );
    return false;
  }

  QgsDebugMsg( "Layer created" );

  QgsProject::instance()->addMapLayers(
    QList<QgsMapLayer *>() << mLayer );

  return true;
}


void QgsDataManagerDialog::itemDoubleClicked( const QModelIndex &index )
{
  QgsDataItem *item = mModel->dataItem( index );
  if ( !item )
    return;

  // Currently doing nothing
  QgsDebugMsg( QString( "%1 %2 %3" ).arg( index.row() ).arg( index.column() ).arg( item->name() ) );
}

void QgsDataManagerDialog::newVectorLayer()
{
  // Set file dialog to last selected dir
  QModelIndex selectedIndex = treeView->selectionModel()->currentIndex();
  if ( selectedIndex.isValid() )
  {
    QgsDirectoryItem *dirItem = qobject_cast<QgsDirectoryItem *>( mModel->dataItem( selectedIndex ) );
    if ( dirItem )
    {
      QSettings settings;
      settings.setValue( QStringLiteral( "/UI/lastVectorFileFilterDir" ), dirItem->dirPath() );
    }
  }

  QString fileName = QgsNewVectorLayerDialog::runAndCreateLayer( this );

  if ( !fileName.isEmpty() )
  {
    QgsDebugMsg( "New vector layer: " + fileName );
    expandPath( fileName );
    QFileInfo fileInfo( fileName );
    QString dirPath = fileInfo.absoluteDir().path();
    mModel->refresh( dirPath );
  }
}

void QgsDataManagerDialog::on_mActionWmsConnections_triggered()
{
#if 0
  QDialog *wmss = dynamic_cast<QDialog *>( QgsProviderRegistry::instance()->selectWidget( QStringLiteral( "wms" ), this ) );
  if ( !wmss )
  {
    QMessageBox::warning( this, tr( "WMS" ), tr( "Cannot get WMS select dialog from provider." ) );
    return;
  }
  wmss->exec();
  delete wmss;
  // TODO: refresh only WMS
  refresh();
#endif
}

void QgsDataManagerDialog::on_mActionSetProjection_triggered()
{
#if 0
  if ( !mLayer )
    return;

  QgsGenericProjectionSelector *mySelector = new QgsGenericProjectionSelector( this );
  mySelector->setMessage();
  mySelector->setSelectedCrsId( mLayer->crs().srsid() );
  if ( mySelector->exec() )
  {
    QgsCoordinateReferenceSystem srs = QgsCoordinateReferenceSystem::fromSrsId( mySelector->selectedCrsId() );

    // TODO: open data source in write mode set crs and save
#if 0
    mLayer->setCrs( srs );
    // Is this safe?
    // selectedIndexes() is protected
#endif

    QgsDataItem *item = mModel->dataItem( mIndex );
    QgsLayerItem *layerItem = qobject_cast<QgsLayerItem *>( item );
    if ( layerItem )
    {
      if ( !layerItem->setCrs( srs ) )
      {
        QMessageBox::critical( this, tr( "CRS" ), tr( "Cannot set layer CRS" ) );
      }
    }
    QgsDebugMsg( srs.authid() + " - " + srs.description() );
  }
  else
  {
    QApplication::restoreOverrideCursor();
  }

  delete mySelector;
#endif
}

void QgsDataManagerDialog::saveWindowState()
{
  QSettings settings;
  settings.setValue( QStringLiteral( "/Windows/Browser/state" ), saveState() );
  settings.setValue( QStringLiteral( "/Windows/Browser/geometry" ), saveGeometry() );
  settings.setValue( QStringLiteral( "/Windows/Browser/sizes/0" ), splitter->sizes().at( 0 ) );
  settings.setValue( QStringLiteral( "/Windows/Browser/sizes/1" ), splitter->sizes().at( 1 ) );
}

void QgsDataManagerDialog::restoreWindowState()
{
  QSettings settings;
  if ( !restoreState( settings.value( QStringLiteral( "/Windows/Browser/state" ) ).toByteArray() ) )
  {
    QgsDebugMsg( "restore of UI state failed" );
  }
  if ( !restoreGeometry( settings.value( QStringLiteral( "/Windows/Browser/geometry" ) ).toByteArray() ) )
  {
    QgsDebugMsg( "restore of UI geometry failed" );
  }
  int size0 = settings.value( QStringLiteral( "/Windows/Browser/sizes/0" ) ).toInt();
  if ( size0 > 0 )
  {

    QList<int> sizes;
    sizes << size0;
    sizes << settings.value( QStringLiteral( "/Windows/Browser/sizes/1" ) ).toInt();
    QgsDebugMsg( QString( "set splitter sizes to %1 %2" ).arg( sizes[0] ).arg( sizes[1] ) );
    splitter->setSizes( sizes );
  }
}

void QgsDataManagerDialog::keyPressEvent( QKeyEvent *e )
{
  if ( e->key() == Qt::Key_Escape )
  {
    stopRendering();
  }
  else
  {
    e->ignore();
  }
}

void QgsDataManagerDialog::keyReleaseEvent( QKeyEvent *e )
{
  if ( treeView->hasFocus() && ( e->key() == Qt::Key_Up || e->key() == Qt::Key_Down ) )
  {
    itemClicked( treeView->selectionModel()->currentIndex() );
  }
  else
  {
    e->ignore();
  }
}

void QgsDataManagerDialog::stopRendering()
{
  if ( mapCanvas )
    mapCanvas->stopRendering();
}

QgsDataManagerDialog::Tab QgsDataManagerDialog::activeTab()
{
  QWidget *curr = tabWidget->currentWidget();
  if ( curr == metaTab )
    return Metadata;
  if ( curr == previewTab )
    return Preview;
  return Attributes;
}

void QgsDataManagerDialog::updateCurrentTab()
{
  // update contents of the current tab

  Tab current = activeTab();

  if ( current == Metadata && mDirtyMetadata )
  {
    if ( mLayer && mLayer->isValid() )
    {
      // Set meta
      QString myStyle = QgsApplication::reportStyleSheet();

      metaTextBrowser->document()->setDefaultStyleSheet( myStyle );
#if 0
      metaTextBrowser->setHtml( mLayer->metadata() );
#endif
    }
    else
    {
      metaTextBrowser->setHtml( QString() );
    }
    mDirtyMetadata = false;
  }

  if ( current == Preview && mDirtyPreview )
  {
    if ( mLayer && mLayer->isValid() )
    {
      // Create preview: add to map canvas
      mapCanvas->setLayers( QList<QgsMapLayer *>() << mLayer );
      QgsRectangle fullExtent = mLayer->extent();
      fullExtent.scale( 1.05 ); // add some border
      mapCanvas->setExtent( fullExtent );
      mapCanvas->refresh();

      QgsRasterLayer *rlayer = qobject_cast< QgsRasterLayer * >( mLayer );
      if ( rlayer )
      {
        connect( rlayer->dataProvider(), &QgsRasterDataProvider::dataChanged, rlayer, [rlayer] { rlayer->triggerRepaint(); } );
        connect( rlayer->dataProvider(), &QgsRasterDataProvider::dataChanged, mapCanvas, &QgsMapCanvas::refresh );
      }
    }
    mDirtyPreview = false;
  }

  if ( current == Attributes && mDirtyAttributes )
  {
    if ( mLayer  && mLayer->isValid() && mLayer->type() == QgsMapLayerType::VectorLayer )
    {
      QgsVectorLayer *vlayer = qobject_cast<QgsVectorLayer *>( mLayer );
      QApplication::setOverrideCursor( Qt::WaitCursor );
      setLayer( vlayer );
      QApplication::restoreOverrideCursor();
    }
    else
    {
      setLayer( nullptr );
    }
    mDirtyAttributes = false;
  }
}

void QgsDataManagerDialog::tabChanged()
{
  updateCurrentTab();
  // Store last selected tab for selected data item
  if ( mIndex.isValid() )
  {
    QgsDataItem *item = mModel->dataItem( mIndex );
    if ( !item )
      return;

    QgsDebugMsg( QString( "save last tab %1 : %2" ).arg( item->metaObject()->className() ).arg( tabWidget->currentIndex() ) );
    mLastTab[ item->metaObject()->className()] = tabWidget->currentIndex();
  }
}

void QgsDataManagerDialog::on_mActionRefresh_triggered()
{
  refresh();
}

void QgsDataManagerDialog::refresh( const QModelIndex &index )
{
  if ( index.isValid() )
  {
    QgsDataItem *item = mModel->dataItem( index );
    if ( item )
    {
      QgsDebugMsg( "path = " + item->path() );
    }
    else
    {
      QgsDebugMsg( "invalid item" );
    }
  }

  mModel->refresh( index );

  for ( int i = 0 ; i < mModel->rowCount( index ); i++ )
  {
    QModelIndex idx = mModel->index( i, 0, index );
    if ( treeView->isExpanded( idx ) || !mModel->hasChildren( idx ) )
    {
      refresh( idx );
    }
  }
}

void QgsDataManagerDialog::setLayer( QgsVectorLayer *vLayer )
{
  attributeTable->setModel( nullptr );

  if ( mAttributeTableFilterModel )
  {
    // Cleanup
    delete mAttributeTableFilterModel;
    mAttributeTableFilterModel = nullptr;
  }

  if ( vLayer )
  {
    // Initialize the cache
    QSettings settings;
    int cacheSize = qMax( 1, settings.value( QStringLiteral( "/qgis/attributeTableRowCache" ), "10000" ).toInt() );
    QgsVectorLayerCache *layerCache = new QgsVectorLayerCache( vLayer, cacheSize, this );
    layerCache->setCacheGeometry( false );

    QgsAttributeTableModel *tableModel = new QgsAttributeTableModel( layerCache );

    mAttributeTableFilterModel = new QgsAttributeTableFilterModel( nullptr, tableModel, this );

    // Let Qt do the garbage collection
    layerCache->setParent( tableModel );
    tableModel->setParent( mAttributeTableFilterModel );

    attributeTable->setModel( mAttributeTableFilterModel );
    tableModel->loadLayer();
  }
}
