#include "qgsrstatsrunner.h"
#include "qgsapplication.h"

#include <RInside.h>
#include <Rcpp.h>

#include "qgisapp.h"
#include "qgslogger.h"
#include <QVariant>
#include <QString>
#include <QFile>
#include <QDir>

class QgsApplicationRWrapper
{
  public:
    QgsApplicationRWrapper() {}

    int version() const { return Qgis::versionInt(); }

    std::string activeLayer() const
    {
      return QgisApp::instance()->activeLayer() ?
             QgisApp::instance()->activeLayer()->name().toStdString() : std::string{};
    }


};

// The function which is called when running QGIS$...
SEXP Dollar( Rcpp::XPtr<QgsApplicationRWrapper> obj, std::string name )
{
  if ( name == "versionInt" )
  {
    return Rcpp::wrap( obj->version() );
  }
  else if ( name == "activeLayer" )
  {
    return Rcpp::wrap( obj->activeLayer() );
  }
  else
  {
    return NULL;
  }
}


// The function listing the elements of QGIS
Rcpp::CharacterVector Names( Rcpp::XPtr<QgsApplicationRWrapper> )
{
  Rcpp::CharacterVector ret;
  ret.push_back( "versionInt" );
  ret.push_back( "activeLayer" );
  return ret;
}

//
// QgsRStatsSession
//
QgsRStatsSession::QgsRStatsSession()
{
  mRSession = std::make_unique< RInside >( 0, nullptr, false, false, true );
  mRSession->set_callbacks( this );

  const QString userPath = QgsApplication::qgisSettingsDirPath() + QStringLiteral( "r_libs" );
  if ( !QFile::exists( userPath ) )
  {
    QDir().mkpath( userPath );
  }
  QString error;
  execCommand( QStringLiteral( ".libPaths(\"%1\")" ).arg( userPath ), error );


  Rcpp::XPtr<QgsApplicationRWrapper> wr( new QgsApplicationRWrapper() );
  wr.attr( "class" ) = "QGIS";
  mRSession->assign( wr, "QGIS" );
  mRSession->assign( Rcpp::InternalFunction( & Dollar ), "$.QGIS" );
  mRSession->assign( Rcpp::InternalFunction( & Names ), "names.QGIS" );

//( *mRSession )["val"] = 5;
//mRSession->parseEvalQ( "val2<-7" );

//  double aDouble = Rcpp::as<double>( mRSession->parseEval( "1+2" ) );
// std::string aString = Rcpp::as<std::string>( mRSession->parseEval( "'asdasdas'" ) );

// R.parseEvalQ( "cat(txt)" );
//  QgsDebugMsg( QString::fromStdString( Rcpp::as<std::string>( R.parseEval( "cat(txt)" ) ) ) );
// QgsDebugMsg( QString::fromStdString( Rcpp::as<std::string>( mRSession->parseEval( "as.character(val+2)" ) ) ) );
// QgsDebugMsg( QStringLiteral( "val as double: %1" ).arg( Rcpp::as<double>( mRSession->parseEval( "val+val2" ) ) ) );
}

QgsRStatsSession::~QgsRStatsSession() = default;

std::string QgsRStatsSession::sexpToString(const SEXP exp)
{
    Rcpp::StringVector lines = Rcpp::StringVector(Rf_eval(Rf_lang2(Rf_install("capture.output"), exp), R_GlobalEnv));
    std::string outcome = "";
    for (auto it = lines.begin(); it != lines.end(); it++)
    {
        Rcpp::String line(it->get());
        outcome.append(line);
        if (it < lines.end() - 1)
            outcome.append("\n");
    }
    return outcome;
}

QVariant QgsRStatsSession::execCommand( const QString &command, QString &error )
{
  try
  {
    SEXP res = mRSession->parseEval( command.toStdString() );
    switch ( TYPEOF( res ) )
    {
      case NILSXP:
        return QVariant();

      case LGLSXP:
      {
        const int resInt = Rcpp::as<int>( res );
        if ( resInt < 0 )
          return QVariant();
        else
          return static_cast< bool >( resInt );
      }

      case INTSXP:
        // handle NA_integer_ as NA!
        return Rcpp::as<int>( res );

      case REALSXP:
        // handle nan as NA
        return Rcpp::as<double>( res );

      case STRSXP:
        return QString::fromStdString( Rcpp::as<std::string>( res ) );

      //case RAWSXP:
      //  return R::rawPointer( res );

      default:
        QgsDebugMsg( "Unhandledtype!!!" );
        return QVariant();
    }

    return QVariant();
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

void QgsRStatsSession::execCommand( const QString &command )
{
  if ( mBusy )
    return;

  mBusy = true;
  emit busyChanged( true );
  QString error;
  const QVariant res = execCommand( command, error );
  if ( ! error.isEmpty() )
    emit errorOccurred( error );
  else
    emit commandFinished( res );

  mBusy = false;
  emit busyChanged( false );
}


//
// QgsRStatsRunner
//

QgsRStatsRunner::QgsRStatsRunner()
{
  mSession = std::make_unique<QgsRStatsSession>();
  mSession->moveToThread( &mSessionThread );
  mSessionThread.start();

  connect( mSession.get(), &QgsRStatsSession::consoleMessage, this, &QgsRStatsRunner::consoleMessage );
  connect( mSession.get(), &QgsRStatsSession::showMessage, this, &QgsRStatsRunner::showMessage );
  connect( mSession.get(), &QgsRStatsSession::errorOccurred, this, &QgsRStatsRunner::errorOccurred );
  connect( mSession.get(), &QgsRStatsSession::busyChanged, this, &QgsRStatsRunner::busyChanged );
  connect( mSession.get(), &QgsRStatsSession::commandFinished, this, &QgsRStatsRunner::commandFinished );
}

QgsRStatsRunner::~QgsRStatsRunner()
{
  // todo -- gracefully shut down session!
  mSessionThread.quit();
  mSessionThread.wait();
}

void QgsRStatsRunner::execCommand( const QString &command )
{
  // todo result handling...
  QMetaObject::invokeMethod( mSession.get(), "execCommand", Qt::QueuedConnection,
                             Q_ARG( QString, command ) );
}

bool QgsRStatsRunner::busy() const
{
  return mSession->busy();
}
