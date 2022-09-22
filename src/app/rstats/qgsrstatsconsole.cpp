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

#include <QVBoxLayout>
#include <QLineEdit>
#include <QTextBrowser>
#include <QPushButton>

QgsRStatsConsole::QgsRStatsConsole( QWidget *parent, QgsRStatsRunner *runner )
  : QWidget( parent )
  , mRunner( runner )
{
  QVBoxLayout *vl = new QVBoxLayout();
  mOutput = new QTextBrowser();
  vl->addWidget( mOutput, 1 );
  mInputEdit = new QLineEdit();
  vl->addWidget( mInputEdit );
  QPushButton *run = new QPushButton( "go" );
  connect( run, &QPushButton::clicked, this, [ = ]
  {
    const QString command = mInputEdit->text();
    QString error;
    const QVariant out = mRunner->execCommand( command, error );
    if ( !error.isEmpty() )
    {
      mOutput->setHtml( mOutput->toHtml() + QStringLiteral( "<p style=\"color: red\">%1</p>" ).arg( error ) );
    }
    else
    {
      mOutput->append( out.toString() );
    }
  } );

  vl->addWidget( run );
  setLayout( vl );

}
