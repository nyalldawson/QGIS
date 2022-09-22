/***************************************************************************
                             qgsrstatsconsole.cpp
                             --------------
    begin                : September 2022
    copyright            : (C) 2022 Nyall Dawson
    email                : nyall dot dawson at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include "qgsrstatsconsole.h"
#include "qgsrstatsrunner.h"
#include "qgisapp.h"
#include "qgsdockablewidgethelper.h"

#include <QVBoxLayout>
#include <QLineEdit>
#include <QTextBrowser>
#include <QPushButton>
#include <QToolBar>

QgsRStatsConsole::QgsRStatsConsole( QWidget *parent, QgsRStatsRunner *runner )
  : QWidget( parent )
  , mRunner( runner )
{
  setObjectName( QStringLiteral( "RStatsConsole" ) );

  QToolBar *toolBar = new QToolBar( this );
  toolBar->setIconSize( QgisApp::instance()->iconSize( true ) );

  mDockableWidgetHelper = new QgsDockableWidgetHelper( true, tr( "R Stats Console" ), this, QgisApp::instance(), Qt::BottomDockWidgetArea,  QStringList(), true );
  QToolButton *toggleButton = mDockableWidgetHelper->createDockUndockToolButton();
  toggleButton->setToolTip( tr( "Dock R Stats Console" ) );
  toolBar->addWidget( toggleButton );
  connect( mDockableWidgetHelper, &QgsDockableWidgetHelper::closed, this, [ = ]()
  {
//   close();
  } );

  QVBoxLayout *vl = new QVBoxLayout();
  vl->setContentsMargins( 0, 0, 0, 0 );
  vl->addWidget( toolBar );

  mOutput = new QTextBrowser();
  vl->addWidget( mOutput, 1 );
  mInputEdit = new QLineEdit();
  vl->addWidget( mInputEdit );
  QPushButton *run = new QPushButton( "go" );
  connect( run, &QPushButton::clicked, this, [ = ]
  {
    const QString command = mInputEdit->text();
    QString error;
    mOutput->append( QStringLiteral( "> " ) + command );
    const QVariant out = mRunner->execCommand( command, error );
    if ( !error.isEmpty() )
    {
      mOutput->setHtml( mOutput->toHtml() + QStringLiteral( "<p style=\"color: red\">%1</p>" ).arg( error ) );
    }
    else
    {
      if ( !out.isValid() )
        mOutput->append( "NA" );
      else
        mOutput->append( out.toString() );
    }
  } );

  connect( mRunner, &QgsRStatsRunner::consoleMessage, this, [ = ]( const QString & message )
  {
    mOutput->append( message );
  } );

  connect( mRunner, &QgsRStatsRunner::showMessage, this, [ = ]( const QString & message )
  {
    mOutput->append( message );
  } );

  vl->addWidget( run );
  setLayout( vl );
}

QgsRStatsConsole::~QgsRStatsConsole()
{
  delete mDockableWidgetHelper;
}
