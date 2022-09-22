#include "qgsrstatsrunner.h"
#include "qgsapplication.h"

#include <RInside.h>
#include <Rcpp.h>


#include "qgslogger.h"
#include <QVariant>
#include <QString>
#include <QFile>
#include <QDir>

QgsRStatsRunner::QgsRStatsRunner()
{
  mRSession = std::make_unique< RInside >( 0, nullptr, false, false, true );

  const QString userPath = QgsApplication::qgisSettingsDirPath() + QStringLiteral( "r_libs" );
  if ( !QFile::exists( userPath ) )
  {
    QDir().mkpath( userPath );
  }
  QString error;
  execCommand( QStringLiteral( ".libPaths(\"%1\")" ).arg( userPath ), error );



  //( *mRSession )["val"] = 5;
  //mRSession->parseEvalQ( "val2<-7" );

//  double aDouble = Rcpp::as<double>( mRSession->parseEval( "1+2" ) );
// std::string aString = Rcpp::as<std::string>( mRSession->parseEval( "'asdasdas'" ) );

// R.parseEvalQ( "cat(txt)" );
//  QgsDebugMsg( QString::fromStdString( Rcpp::as<std::string>( R.parseEval( "cat(txt)" ) ) ) );
// QgsDebugMsg( QString::fromStdString( Rcpp::as<std::string>( mRSession->parseEval( "as.character(val+2)" ) ) ) );
// QgsDebugMsg( QStringLiteral( "val as double: %1" ).arg( Rcpp::as<double>( mRSession->parseEval( "val+val2" ) ) ) );

}

QVariant QgsRStatsRunner::execCommand( const QString &command, QString &error )
{
  try
  {
    SEXP res = mRSession->parseEval( command.toStdString() );

    switch ( TYPEOF( res ) )
    {
      case NILSXP:
        return QVariant();

      case LGLSXP:
        return Rcpp::as<bool>( res );

      case INTSXP:
        return Rcpp::as<int>( res );

      case REALSXP:
        return Rcpp::as<double>( res );

      case STRSXP:
        return QString::fromStdString( Rcpp::as<std::string>( res ) );

      //case RAWSXP:
      //  return R::rawPointer( res );

      default:
        QgsDebugMsg( "Unhandledtype!!!" );
        return QVariant();
    }
  }
  catch ( std::exception &ex )
  {
    error = QString::fromStdString( ex.what() );
  }
  catch ( ... )
  {
    std::cerr << "Unknown exception caught" << std::endl;
  }
  return QVariant();
}

QgsRStatsRunner::~QgsRStatsRunner() = default;
