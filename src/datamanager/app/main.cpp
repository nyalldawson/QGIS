/***************************************************************************
                             main.cpp
                             --------
    begin                : 2021-07-24
    copyright            : (C) 2021 Nyall Dawson
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
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QTimer>

#include <cstdio>
#include <cstdlib>

#include "qgsapplication.h"
#ifdef Q_OS_WIN
// Open files in binary mode
#include <fcntl.h> /*  _O_BINARY */
#ifdef MSVC
#undef _fmode
int _fmode = _O_BINARY;
#else
// Only do this if we are not building on windows with msvc.
// Recommended method for doing this with msvc is with a call to _set_fmode
// which is the first thing we do in main().
// Similarly, with MinGW set _fmode in main().
#endif  //_MSC_VER
#else
#include <getopt.h>
#endif

#ifdef Q_OS_MACX
#include <ApplicationServices/ApplicationServices.h>
#if MAC_OS_X_VERSION_MAX_ALLOWED < 1050
typedef SInt32 SRefCon;
#endif
#endif

#include "qgsdatamanagerdialog.h"
#include "qgsapplication.h"
#include "qgsproviderregistry.h"

#undef QgsDebugCall
#undef QgsDebugMsg
#undef QgsDebugMsgLevel
#define QgsDebugCall
#define QgsDebugMsg(str)
#define QgsDebugMsgLevel(str, level)



/////////////////////////////////////////////////////////////////
// Command line options 'behavior' flag setup
////////////////////////////////////////////////////////////////

// These two are global so that they can be set by the OpenDocuments
// AppleEvent handler as well as by the main routine argv processing

// This behavior will cause QGIS to autoload a project
static QString myProjectFileName;

// This is the 'leftover' arguments collection
static QStringList sFileList;

int main( int argc, char *argv[] )
{
#ifdef Q_OS_WIN  // Windows
#ifdef _MSC_VER
  _set_fmode( _O_BINARY );
#else //MinGW
  _fmode = _O_BINARY;
#endif  // _MSC_VER
#endif  // Q_OS_WIN

  QgsApplication app( argc, argv, true );
  QString myPrefixPath;
  if ( myPrefixPath.isEmpty() )
  {
    QDir dir( QCoreApplication::applicationDirPath() );
    dir.cdUp();
    myPrefixPath = dir.absolutePath();
  }
  QgsApplication::setPrefixPath( myPrefixPath, true );

  // Set up the QSettings environment must be done after qapp is created
  QgsApplication::setOrganizationName( QStringLiteral( "QGIS" ) );
  QgsApplication::setOrganizationDomain( QStringLiteral( "qgis.org" ) );
  QgsApplication::setApplicationName( QStringLiteral( "QGIS3" ) );

  QgsApplication::init();
  QgsApplication::initQgis();

  QgsProviderRegistry::instance( QgsApplication::pluginPath() );

  ( void ) QgsApplication::resolvePkgPath(); // trigger storing of application path in QgsApplication

  app.setWindowIcon( QIcon( QgsApplication::iconsPath() + "qbrowser-icon-60x60.png" ) );

  QgsDataManagerDialog w;

  QObject::connect( &app, &QApplication::aboutToQuit, &w, &QgsDataManagerDialog::saveWindowState );
  w.restoreWindowState();

  w.show();

  QObject::connect( &app, &QApplication::lastWindowClosed, &app, &QApplication::quit );

//  QgsEditorWidgetRegistry::initEditors();

  return app.exec();
}
