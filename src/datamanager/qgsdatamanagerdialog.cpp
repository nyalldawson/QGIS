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
#include "qgsbrowserdockwidget.h"
#include "qgsbrowserguimodel.h"
#include "qgsgui.h"

#ifdef ANDROID
#define QGIS_ICON_SIZE 32
#else
#define QGIS_ICON_SIZE 24
#endif

QgsDataManagerDialog::QgsDataManagerDialog( QWidget *parent, Qt::WindowFlags flags )
  : QMainWindow( parent, flags )
{
  setupUi( this );
  setWindowTitle( tr( "Data Manager" ) );

  QgsGui::enableAutoGeometryRestore( this );

  setDockOptions( dockOptions() | QMainWindow::GroupedDragging );

  mBrowserModel = new QgsBrowserGuiModel( this );
  mBrowserWidget = new QgsBrowserDockWidget( tr( "Browser" ), mBrowserModel, this );
  mBrowserWidget->setObjectName( QStringLiteral( "Browser" ) );
  mBrowserWidget->setFeatures( QDockWidget::DockWidgetMovable );
// mBrowserWidget->setMessageBar( mInfoBar );

  addDockWidget( Qt::LeftDockWidgetArea, mBrowserWidget );
}

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
