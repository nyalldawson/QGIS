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
#include <QScreen>

#include "qgsdatamanagerdialog.h"
#include "qgsapplication.h"
#include "qgsbrowserdockwidget.h"
#include "qgsbrowserguimodel.h"
#include "qgsgui.h"
#include "qgssettings.h"
#include "qgsguiutils.h"
#include "qgsbrowserproxymodel.h"
#include "qgsdataitemguiprovider.h"
#include "qgsbrowserdockwidget_p.h"

QgsDataManagerDialog::QgsDataManagerDialog( QWidget *parent, Qt::WindowFlags flags )
  : QMainWindow( parent, flags )
{
  setupUi( this );
  setWindowTitle( tr( "Data Manager" ) );

  QgsGui::enableAutoGeometryRestore( this );

  setDockOptions( dockOptions() | QMainWindow::GroupedDragging );

  mBrowserModel = new QgsBrowserGuiModel( this );
  mBrowserWidget = new QgsDataManagerBrowserDock( tr( "Browser" ), mBrowserModel, this );
  mBrowserWidget->setObjectName( QStringLiteral( "Browser" ) );
  mBrowserWidget->setFeatures( QDockWidget::DockWidgetMovable );
// mBrowserWidget->setMessageBar( mInfoBar );

  addDockWidget( Qt::LeftDockWidgetArea, mBrowserWidget );

  QgsSettings settings;
  // Set icon size of toolbars
  if ( settings.contains( QStringLiteral( "/qgis/iconSize" ) ) )
  {
    int size = settings.value( QStringLiteral( "/qgis/iconSize" ) ).toInt();
    if ( size < 16 )
      size = 24;
    setIconSizes( size );
  }
  else
  {
    // first run, guess a good icon size
    int size = chooseReasonableDefaultIconSize();
    settings.setValue( QStringLiteral( "/qgis/iconSize" ), size );
    setIconSizes( size );
  }

  connect( mBrowserWidget, &QgsDataManagerBrowserDock::dataItemDoubleClicked, this, [ = ]( QgsDataItem * item )
  {

    QgsDataItemGuiContext context; // = createContext();
    QgsBrowserPropertiesWidget *propertiesWidget = QgsBrowserPropertiesWidget::createWidget( item, context, this );
    if ( propertiesWidget )
    {
      //propertiesWidget->setCondensedMode( true );

      propertiesWidget->setWindowTitle( item->name() );
      mMdiArea->addSubWindow( propertiesWidget );
      propertiesWidget->show();
    }


  } );
};

QgsDataManagerDialog::~QgsDataManagerDialog()
{

}

void QgsDataManagerDialog::saveWindowState()
{
#if 0
  QSettings settings;
  settings.setValue( QStringLiteral( "/Windows/Browser/state" ), saveState() );
  settings.setValue( QStringLiteral( "/Windows/Browser/geometry" ), saveGeometry() );
  settings.setValue( QStringLiteral( "/Windows/Browser/sizes/0" ), splitter->sizes().at( 0 ) );
  settings.setValue( QStringLiteral( "/Windows/Browser/sizes/1" ), splitter->sizes().at( 1 ) );
#endif
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

#if 0
    splitter->setSizes( sizes );
#endif
  }
}

void QgsDataManagerDialog::setIconSizes( int size )
{
  QSize iconSize = QSize( size, size );
  QSize panelIconSize = QgsGuiUtils::panelIconSize( iconSize );

  //Set the icon size of for all the toolbars created in the future.
  setIconSize( iconSize );

  //Change all current icon sizes.
  QList<QToolBar *> toolbars = findChildren<QToolBar *>();
  const auto constToolbars = toolbars;
  for ( QToolBar *toolbar : constToolbars )
  {
    QString className = toolbar->parent()->metaObject()->className();
    if ( className == QLatin1String( "QgisApp" ) )
    {
      toolbar->setIconSize( iconSize );
    }
    else
    {
      toolbar->setIconSize( panelIconSize );
    }
  }
}

int QgsDataManagerDialog::chooseReasonableDefaultIconSize() const
{
  QScreen *screen = QApplication::screens().at( 0 );
  if ( screen->physicalDotsPerInch() < 115 )
  {
    // no hidpi screen, use default size
    return 24;
  }
  else
  {
    double size = fontMetrics().horizontalAdvance( 'X' ) * 3;
    if ( size < 24 )
      return 16;
    else if ( size < 32 )
      return 24;
    else if ( size < 48 )
      return 32;
    else if ( size < 64 )
      return 48;
    else
      return 64;
  }

}

QgsDataManagerBrowserDock::QgsDataManagerBrowserDock( const QString &name, QgsBrowserGuiModel *browserModel, QWidget *parent )
  : QgsBrowserDockWidget( name, browserModel, parent )
{

}

void QgsDataManagerBrowserDock::itemDoubleClicked( const QModelIndex &index )
{
  QgsDataItem *item = mModel->dataItem( mProxyModel->mapToSource( index ) );
  if ( !item )
    return;

  emit dataItemDoubleClicked( item );
}
