/***************************************************************************
     testqgsrstats.cpp
     --------------------
    Date                 : October 2022
    Copyright            : (C) 2022 by Nyall Dawson
    Email                : nyall dot dawson at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include <QApplication>
#include <QObject>
#include <QSplashScreen>
#include <QString>
#include <QStringList>
#include "qgstest.h"

#include "qgisapp.h"
#include "qgsapplication.h"

/**
 * \ingroup UnitTests
 * This is a unit test for the R stats support
 */
class TestQgsRStats : public QObject
{
    Q_OBJECT

  public:
    TestQgsRStats();

  private slots:
    void initTestCase();// will be called before the first testfunction is executed.
    void cleanupTestCase();// will be called after the last testfunction was executed.

  private:
    QString mTestDataDir;
    QgisApp *mQgisApp;
};

TestQgsRStats::TestQgsRStats() = default;

//runs before all tests
void TestQgsRStats::initTestCase()
{
  qputenv( "QGIS_PLUGINPATH", QByteArray( TEST_DATA_DIR ) + "/test_plugin_path" );

  // Set up the QgsSettings environment
  QCoreApplication::setOrganizationName( QStringLiteral( "QGIS" ) );
  QCoreApplication::setOrganizationDomain( QStringLiteral( "qgis.org" ) );
  QCoreApplication::setApplicationName( QStringLiteral( "QGIS-TEST" ) );

  qDebug() << "TestQgisAppClipboard::initTestCase()";
  // init QGIS's paths - true means that all path will be inited from prefix
  QgsApplication::init();
  QgsApplication::initQgis();
  mTestDataDir = QStringLiteral( TEST_DATA_DIR ) + '/'; //defined in CmakeLists.txt
  mQgisApp = new QgisApp();
}

void TestQgsRStats::cleanupTestCase()
{
  QgsApplication::exitQgis();
}

QGSTEST_MAIN( TestQgsRStats )
#include "testqgsrstats.moc"
