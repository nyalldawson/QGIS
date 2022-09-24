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
#include "qgscodeeditor.h"

#include <QVBoxLayout>
#include <QLineEdit>
#include <QTextBrowser>
#include <QPushButton>
#include <QToolBar>

QgsRStatsConsole::QgsRStatsConsole( QWidget *parent, QgsRStatsRunner *runner )
  : QWidget( parent )
  , mRunner( runner )
{
  QToolBar *toolBar = new QToolBar( this );
  toolBar->setIconSize( QgisApp::instance()->iconSize( true ) );

  mDockableWidgetHelper = new QgsDockableWidgetHelper( true, tr( "R Stats Console" ), this, QgisApp::instance(), Qt::BottomDockWidgetArea,  QStringList(), true );
  mDockableWidgetHelper->setDockObjectName( QStringLiteral( "RStatsConsole" ) );
  QToolButton *toggleButton = mDockableWidgetHelper->createDockUndockToolButton();
  toggleButton->setToolTip( tr( "Dock R Stats Console" ) );
  toolBar->addWidget( toggleButton );

  QVBoxLayout *vl = new QVBoxLayout();
  vl->setContentsMargins( 0, 0, 0, 0 );
  vl->addWidget( toolBar );

  mOutput = new QTextBrowser();
  mOutput->setFont( QgsCodeEditor::getMonospaceFont() );
  vl->addWidget( mOutput, 1 );
  mInputEdit = new QLineEdit();
  mInputEdit->setFont( QgsCodeEditor::getMonospaceFont() );
  vl->addWidget( mInputEdit );
  connect( mInputEdit, &QLineEdit::returnPressed, this, [ = ]
  {
    if ( mRunner->busy() )
      return;

    const QString command = mInputEdit->text();
    mOutput->append( QStringLiteral( "> " ) + command );
    mRunner->execCommand( command );
  } );

  connect( mRunner, &QgsRStatsRunner::errorOccurred, this, [ = ]( const QString & error )
  {
    mOutput->setHtml( mOutput->toHtml() + QStringLiteral( "<p style=\"color: red\">%1</p>" ).arg( error ) );
  } );
  connect( mRunner, &QgsRStatsRunner::commandFinished, this, [ = ]( const QVariant & result )
  {
    if ( !result.isValid() )
      mOutput->append( "NA" );
    else
      mOutput->append( result.toString() );
  } );

  connect( mRunner, &QgsRStatsRunner::consoleMessage, this, [ = ]( const QString & message, int type )
  {
    if ( type == 0 )
      mOutput->append( message );
    else
      mOutput->setHtml( mOutput->toHtml() + QStringLiteral( "<p style=\"color: red\">%1</p>" ).arg( message ) );
  } );

  connect( mRunner, &QgsRStatsRunner::showMessage, this, [ = ]( const QString & message )
  {
    mOutput->append( message );
  } );

  connect( mRunner, &QgsRStatsRunner::busyChanged, this, [ = ]( bool busy )
  {
    mInputEdit->setEnabled( !busy );
  } );

  setLayout( vl );
}

QgsRStatsConsole::~QgsRStatsConsole()
{
  delete mDockableWidgetHelper;
}
