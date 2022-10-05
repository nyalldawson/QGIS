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
  mInputEdit = new QgsInteractiveRWidget();
  mInputEdit->setFont( QgsCodeEditor::getMonospaceFont() );
  splitter->addWidget( mInputEdit );

  vl->addWidget( splitter );

  connect( mInputEdit, &QgsInteractiveRWidget::runCommand, this, [ = ]( const QString & command )
  {
    if ( mRunner->busy() )
      return;

    mOutput->append( ( mOutput->text().isEmpty() ? QString() : QString( '\n' ) ) + QStringLiteral( "> " ) + command );
    mRunner->execCommand( command );
  } );

  connect( mRunner, &QgsRStatsRunner::errorOccurred, this, [ = ]( const QString & error )
  {
    mOutput->append( ( mOutput->text().isEmpty() ? QString() : QString( '\n' ) ) + error );
  } );

  connect( mRunner, &QgsRStatsRunner::consoleMessage, this, [ = ]( const QString & message, int type )
  {
    if ( type == 0 )
      mOutput->append( ( mOutput->text().isEmpty() ? QString() : QString( '\n' ) ) + message );
    else // TODO should we format errors differently?
      mOutput->append( ( mOutput->text().isEmpty() ? QString() : QString( '\n' ) ) + message );
  } );

  connect( mRunner, &QgsRStatsRunner::showMessage, this, [ = ]( const QString & message )
  {
    mOutput->append( ( mOutput->text().isEmpty() ? QString() : QString( '\n' ) ) + message );
  } );

  connect( mRunner, &QgsRStatsRunner::busyChanged, this, [ = ]( bool busy )
  {
    mInputEdit->setEnabled( !busy );
  } );

  setLayout( vl );

  mRunner->showStartupMessage();
}

QgsRStatsConsole::~QgsRStatsConsole()
{
  delete mDockableWidgetHelper;
}

QgsInteractiveRWidget::QgsInteractiveRWidget( QWidget *parent )
  : QgsCodeEditorR( parent, QgsCodeEditor::Mode::CommandInput )
{
  displayPrompt( false );

  QgsInteractiveRWidget::initializeLexer();
}

void QgsInteractiveRWidget::keyPressEvent( QKeyEvent *event )
{
  switch ( event->key() )
  {
    case Qt::Key_Return:
    case Qt::Key_Enter:
      emit runCommand( text() );
      clear();
      displayPrompt( false );
      break;

    default:
      QgsCodeEditorR::keyPressEvent( event );
  }
}

void QgsInteractiveRWidget::initializeLexer()
{
  QgsCodeEditorR::initializeLexer();

  setCaretLineVisible( false );
  setLineNumbersVisible( false ); // NO linenumbers for the input line
  setFoldingVisible( false );
  // Margin 1 is used for the '>' prompt (console input)
  setMarginLineNumbers( 1, true );
  setMarginWidth( 1, "00" );
  setMarginType( 1, QsciScintilla::MarginType::TextMarginRightJustified );
  setMarginsBackgroundColor( color( QgsCodeEditorColorScheme::ColorRole::Background ) );
  setEdgeMode( QsciScintilla::EdgeNone );
}

void QgsInteractiveRWidget::displayPrompt( bool more )
{
  const QString prompt = !more ? ">" : "+";
  SendScintilla( QsciScintilla::SCI_MARGINSETTEXT, static_cast< uintptr_t >( 0 ), prompt.toUtf8().constData() );
}
