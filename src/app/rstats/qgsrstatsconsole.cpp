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
#include "qgscodeeditorr.h"
#include "qgscodeeditor.h"
#include "qgsapplication.h"

#include <QVBoxLayout>
#include <QLineEdit>
#include <QTextBrowser>
#include <QPushButton>
#include <QToolBar>
#include <QSplitter>

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

  QSplitter *splitter = new QSplitter();
  splitter->setOrientation( Qt::Vertical );
  splitter->setHandleWidth( 3 );
  splitter->setChildrenCollapsible( false );

  mOutput = new QgsCodeEditorR( nullptr, QgsCodeEditor::Mode::OutputDisplay );
  splitter->addWidget( mOutput );
  mInputEdit = new QgsCodeEditorR( nullptr, QgsCodeEditor::Mode::CommandInput );
  mInputEdit->setInterpreter( mRunner );
  mInputEdit->setHistoryFilePath( QgsApplication::qgisSettingsDirPath() + QStringLiteral( "/r_console_history.txt" ) );

  splitter->addWidget( mInputEdit );

  vl->addWidget( splitter );

  connect( mRunner, &QgsRStatsRunner::commandStarted, this, [ = ]( const QString & command )
  {
    mOutput->append( ( mOutput->text().isEmpty() ? QString() : QString( '\n' ) ) + QStringLiteral( "> " ) + command );
    mOutput->moveCursorToEnd();
  } );

  connect( mRunner, &QgsRStatsRunner::errorOccurred, this, [ = ]( const QString & error )
  {
    mOutput->append( ( mOutput->text().isEmpty() ? QString() : QString( '\n' ) ) + error );
    mOutput->moveCursorToEnd();
  } );

  connect( mRunner, &QgsRStatsRunner::consoleMessage, this, [ = ]( const QString & message, int type )
  {
    if ( type == 0 )
      mOutput->append( ( mOutput->text().isEmpty() ? QString() : QString( '\n' ) ) + message );
    else // TODO should we format errors differently?
      mOutput->append( ( mOutput->text().isEmpty() ? QString() : QString( '\n' ) ) + message );
    mOutput->moveCursorToEnd();
  } );

  connect( mRunner, &QgsRStatsRunner::showMessage, this, [ = ]( const QString & message )
  {
    mOutput->append( ( mOutput->text().isEmpty() ? QString() : QString( '\n' ) ) + message );
    mOutput->moveCursorToEnd();
  } );

  connect( mRunner, &QgsRStatsRunner::busyChanged, this, [ = ]( bool busy )
  {
    //mInputEdit->setEnabled( !busy );
  } );

  setLayout( vl );

  mRunner->showStartupMessage();
}

QgsRStatsConsole::~QgsRStatsConsole()
{
  mInputEdit->writeHistoryFile();

  delete mDockableWidgetHelper;
}

