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
#include <Rcpp.h>

#include "qgstest.h"

#include "qgisapp.h"
#include "qgsapplication.h"
#include "rstats/qgsrstatsrunner.h"

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
    void testSexpToVariant();
    void testSexpToString();
    void testVariantToSexp();

  private:
    QString mTestDataDir;
    QgisApp *mQgisApp;
    std::unique_ptr< QgsRStatsSession > mSession;
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

  mSession = std::make_unique< QgsRStatsSession >();
}

void TestQgsRStats::cleanupTestCase()
{
  mSession.reset();
  QgsApplication::exitQgis();
}

void TestQgsRStats::testSexpToVariant()
{
  QCOMPARE( QgsRStatsSession::sexpToVariant( R_NilValue ), QVariant() );

  // logical
  Rcpp::LogicalVector vLogical = Rcpp::LogicalVector::create( 1, 0, NA_LOGICAL );
  QCOMPARE( QgsRStatsSession::sexpToVariant( Rcpp::wrap( vLogical[0] ) ), QVariant( true ) );
  QCOMPARE( QgsRStatsSession::sexpToVariant( Rcpp::wrap( vLogical[1] ) ), QVariant( false ) );
  QCOMPARE( QgsRStatsSession::sexpToVariant( Rcpp::wrap( vLogical[2] ) ), QVariant() );
  QCOMPARE( QgsRStatsSession::sexpToVariant( Rcpp::wrap( vLogical ) ), QVariant( QVariantList() << true << false << QVariant() ) );

  // int
  Rcpp::IntegerVector vInteger = Rcpp::IntegerVector::create( 100, 0, -100, NA_INTEGER );
  QCOMPARE( QgsRStatsSession::sexpToVariant( Rcpp::wrap( vInteger[0] ) ), QVariant( 100 ) );
  QCOMPARE( QgsRStatsSession::sexpToVariant( Rcpp::wrap( vInteger[1] ) ), QVariant( 0 ) );
  QCOMPARE( QgsRStatsSession::sexpToVariant( Rcpp::wrap( vInteger[2] ) ), QVariant( - 100 ) );
  QCOMPARE( QgsRStatsSession::sexpToVariant( Rcpp::wrap( vInteger[3] ) ), QVariant() );
  QCOMPARE( QgsRStatsSession::sexpToVariant( Rcpp::wrap( vInteger ) ), QVariant( QVariantList() << 100 << 0 << -100 << QVariant() ) );

  // double
  Rcpp::DoubleVector vDouble = Rcpp::DoubleVector::create( 100.2, 0, -100.2, std::numeric_limits< double >::quiet_NaN() );
  QCOMPARE( QgsRStatsSession::sexpToVariant( Rcpp::wrap( vDouble[0] ) ), QVariant( 100.2 ) );
  QCOMPARE( QgsRStatsSession::sexpToVariant( Rcpp::wrap( vDouble[1] ) ), QVariant( 0.0 ) );
  QCOMPARE( QgsRStatsSession::sexpToVariant( Rcpp::wrap( vDouble[2] ) ), QVariant( - 100.2 ) );
  QCOMPARE( QgsRStatsSession::sexpToVariant( Rcpp::wrap( vDouble[3] ) ), QVariant() );
  QCOMPARE( QgsRStatsSession::sexpToVariant( Rcpp::wrap( vDouble ) ), QVariant( QVariantList() << 100.2 << 0.0 << -100.2 << QVariant() ) );

  // string
  Rcpp::StringVector vString = Rcpp::StringVector::create( "string 1", "string2", "", std::string() );
  QCOMPARE( QgsRStatsSession::sexpToVariant( Rcpp::wrap( vString[0] ) ), QVariant( QStringLiteral( "string 1" ) ) );
  QCOMPARE( QgsRStatsSession::sexpToVariant( Rcpp::wrap( vString[1] ) ), QVariant( QStringLiteral( "string2" ) ) );
  QCOMPARE( QgsRStatsSession::sexpToVariant( Rcpp::wrap( vString[2] ) ), QVariant( QString( "" ) ) );
  QCOMPARE( QgsRStatsSession::sexpToVariant( Rcpp::wrap( vString[3] ) ), QVariant( QString( "" ) ) );
  QCOMPARE( QgsRStatsSession::sexpToVariant( Rcpp::wrap( std::string( "test" ) ) ), QVariant( QStringLiteral( "test" ) ) );
  QCOMPARE( QgsRStatsSession::sexpToVariant( Rcpp::wrap( vString ) ), QVariant( QVariantList() << QStringLiteral( "string 1" ) << QStringLiteral( "string2" ) << QString( "" ) << QString( "" ) ) );
}

void TestQgsRStats::testSexpToString()
{
  QCOMPARE( QgsRStatsSession::sexpToString( R_NilValue ), QStringLiteral( "NULL" ) );

  // logical
  Rcpp::LogicalVector vLogical = Rcpp::LogicalVector::create( 1, 0, NA_LOGICAL );
  QCOMPARE( QgsRStatsSession::sexpToString( Rcpp::wrap( vLogical[0] ) ), QStringLiteral( "[1] 1" ) );
  QCOMPARE( QgsRStatsSession::sexpToString( Rcpp::wrap( vLogical[1] ) ), QStringLiteral( "[1] 0" ) );
  QCOMPARE( QgsRStatsSession::sexpToString( Rcpp::wrap( vLogical[2] ) ), QStringLiteral( "[1] NA" ) );
  QCOMPARE( QgsRStatsSession::sexpToString( Rcpp::wrap( vLogical ) ), QStringLiteral( "[1]  TRUE FALSE    NA" ) );

  // int
  Rcpp::IntegerVector vInteger = Rcpp::IntegerVector::create( 100, 0, -100, NA_INTEGER );
  QCOMPARE( QgsRStatsSession::sexpToString( Rcpp::wrap( vInteger[0] ) ), QStringLiteral( "[1] 100" ) );
  QCOMPARE( QgsRStatsSession::sexpToString( Rcpp::wrap( vInteger[1] ) ), QStringLiteral( "[1] 0" ) );
  QCOMPARE( QgsRStatsSession::sexpToString( Rcpp::wrap( vInteger[2] ) ), QStringLiteral( "[1] -100" ) );
  QCOMPARE( QgsRStatsSession::sexpToString( Rcpp::wrap( vInteger[3] ) ), QStringLiteral( "[1] NA" ) );
  QCOMPARE( QgsRStatsSession::sexpToString( Rcpp::wrap( vInteger ) ), QStringLiteral( "[1]  100    0 -100   NA" ) );

  // double
  Rcpp::DoubleVector vDouble = Rcpp::DoubleVector::create( 100.2, 0, -100.2, std::numeric_limits< double >::quiet_NaN() );
  QCOMPARE( QgsRStatsSession::sexpToString( Rcpp::wrap( vDouble[0] ) ), QStringLiteral( "[1] 100.2" ) );
  QCOMPARE( QgsRStatsSession::sexpToString( Rcpp::wrap( vDouble[1] ) ), QStringLiteral( "[1] 0" ) );
  QCOMPARE( QgsRStatsSession::sexpToString( Rcpp::wrap( vDouble[2] ) ), QStringLiteral( "[1] -100.2" ) );
  QCOMPARE( QgsRStatsSession::sexpToString( Rcpp::wrap( vDouble[3] ) ), QStringLiteral( "[1] NaN" ) );
  QCOMPARE( QgsRStatsSession::sexpToString( Rcpp::wrap( vDouble ) ), QStringLiteral( "[1]  100.2    0.0 -100.2    NaN" ) );

  // string
  Rcpp::StringVector vString = Rcpp::StringVector::create( "string 1", "string2", "", std::string() );
  QCOMPARE( QgsRStatsSession::sexpToString( Rcpp::wrap( vString[0] ) ), QStringLiteral( "[1] \"string 1\"" ) );
  QCOMPARE( QgsRStatsSession::sexpToString( Rcpp::wrap( vString[1] ) ), QStringLiteral( "[1] \"string2\"" ) );
  QCOMPARE( QgsRStatsSession::sexpToString( Rcpp::wrap( vString[2] ) ), QStringLiteral( "[1] \"\"" ) );
  QCOMPARE( QgsRStatsSession::sexpToString( Rcpp::wrap( vString[3] ) ), QStringLiteral( "[1] \"\"" ) );
  QCOMPARE( QgsRStatsSession::sexpToString( Rcpp::wrap( std::string( "test" ) ) ), QStringLiteral( "[1] \"test\"" ) );
  QCOMPARE( QgsRStatsSession::sexpToString( Rcpp::wrap( vString ) ), QStringLiteral( "[1] \"string 1\" \"string2\"  \"\"         \"\"        " ) );
}

void TestQgsRStats::testVariantToSexp()
{
  QCOMPARE( QgsRStatsSession::variantToSexp( QVariant() ), R_NilValue );

  QCOMPARE( Rcpp::as<int>( QgsRStatsSession::variantToSexp( QVariant( QVariant::Bool ) ) ), NA_LOGICAL );
  QCOMPARE( Rcpp::as<int>( QgsRStatsSession::variantToSexp( QVariant( true ) ) ), 1 );
  QCOMPARE( Rcpp::as<int>( QgsRStatsSession::variantToSexp( QVariant( false ) ) ), 0 );

  QCOMPARE( Rcpp::as<int>( QgsRStatsSession::variantToSexp( QVariant( QVariant::Int ) ) ), NA_INTEGER );
  QCOMPARE( Rcpp::as<int>( QgsRStatsSession::variantToSexp( QVariant( 100 ) ) ), 100 );
  QCOMPARE( Rcpp::as<int>( QgsRStatsSession::variantToSexp( QVariant( 0 ) ) ), 0 );
  QCOMPARE( Rcpp::as<int>( QgsRStatsSession::variantToSexp( QVariant( -100 ) ) ), -100 );

  QVERIFY( std::isnan( Rcpp::as<double>( QgsRStatsSession::variantToSexp( QVariant( QVariant::Double ) ) ) ) );
  QCOMPARE( Rcpp::as<double>( QgsRStatsSession::variantToSexp( QVariant( 100.2 ) ) ), 100.2 );
  QCOMPARE( Rcpp::as<double>( QgsRStatsSession::variantToSexp( QVariant( 0.0 ) ) ), 0.0 );
  QCOMPARE( Rcpp::as<double>( QgsRStatsSession::variantToSexp( QVariant( -100.2 ) ) ), -100.2 );

  QCOMPARE( Rcpp::as<std::string>( QgsRStatsSession::variantToSexp( QVariant( QVariant::String ) ) ), "" );
  QCOMPARE( Rcpp::as<std::string>( QgsRStatsSession::variantToSexp( QVariant( QStringLiteral( "test string" ) ) ) ), "test string" );
  QCOMPARE( Rcpp::as<std::string>( QgsRStatsSession::variantToSexp( QVariant( QString( "" ) ) ) ), "" );
}

QGSTEST_MAIN( TestQgsRStats )
#include "testqgsrstats.moc"
