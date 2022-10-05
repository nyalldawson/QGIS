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
  execCommandNR( QStringLiteral( ".libPaths(\"%1\")" ).arg( userPath ) );

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

std::string QgsRStatsSession::sexpToString( const SEXP exp )
{
  switch ( TYPEOF( exp ) )
  {
    case EXPRSXP:
    case CLOSXP:
    case ENVSXP:
    case LANGSXP:
      // these types can't be converted to StringVector, will raise exceptions
      return {};

    case LGLSXP:
    case INTSXP:
    case REALSXP:
    case STRSXP:
      break; // we know these types are fine to convert to StringVector

    default:
      QgsDebugMsg( QStringLiteral( "Possibly unsafe type: %1" ).arg( TYPEOF( exp ) ) );
      break;
  }

  Rcpp::StringVector lines = Rcpp::StringVector( Rf_eval( Rf_lang2( Rf_install( "capture.output" ), exp ), R_GlobalEnv ) );
  std::string outcome = "";
  for ( auto it = lines.begin(); it != lines.end(); it++ )
  {
    Rcpp::String line( it->get() );
    outcome.append( line );
    if ( it < lines.end() - 1 )
      outcome.append( "\n" );
  }
  return outcome;
}

QVariant QgsRStatsSession::sexpToVariant( const SEXP exp )
{
  switch ( TYPEOF( exp ) )
  {
    case NILSXP:
      return QVariant();

    case LGLSXP:
    {
      if ( LENGTH( exp ) == 1 )
      {
        const int expInt = Rcpp::as<int>( exp );
        if ( expInt < 0 )
          return QVariant();
        else
          return static_cast< bool >( expInt );
      }
      else
        return QVariant();

    }

    case INTSXP:
      // handle NA_integer_ as NA!
      if ( LENGTH( exp ) == 1 )
        return Rcpp::as<int>( exp );
      else
        return QVariant();

    case REALSXP:
      // handle nan as NA
      if ( LENGTH( exp ) == 1 )
        return Rcpp::as<double>( exp );
      else
        return QVariant();

    case STRSXP:
      if ( LENGTH( exp ) == 1 )
        return QString::fromStdString( Rcpp::as<std::string>( exp ) );
      else
        return QVariant();

    //case RAWSXP:
    //  return R::rawPointer( exp );

    case EXPRSXP:
    case CLOSXP:
    case ENVSXP:
    case LANGSXP:
      // these types can't possibly be converted to variants
      return QVariant();

    default:
      QgsDebugMsg( QStringLiteral( "Unhandled type: %1" ).arg( TYPEOF( exp ) ) );
      return QVariant();
  }

  return QVariant();
}

void QgsRStatsSession::execCommandPrivate( const QString &command, QString &error, QVariant *res, QString *output )
{
  try
  {
    const SEXP sexpRes = mRSession->parseEval( command.toStdString() );
    if ( res )
      *res = sexpToVariant( sexpRes );
    if ( output )
      *output = QString::fromStdString( sexpToString( sexpRes ) );
  }
  catch ( std::exception &ex )
  {
    error = QString::fromStdString( ex.what() );
  }
  catch ( ... )
  {
    std::cerr << "Unknown exception caught" << std::endl;
  }
  if ( res )
    *res = QVariant();
}

void QgsRStatsSession::execCommandNR( const QString &command )
{
  if ( mBusy )
    return;

  mBusy = true;
  emit busyChanged( true );

  mEncounteredErrorMessageType = false;
  QString error;
  execCommandPrivate( command, error );

  if ( ! error.isEmpty() && !mEncounteredErrorMessageType )
    emit errorOccurred( error );

  mBusy = false;
  emit busyChanged( false );
}

void QgsRStatsSession::execCommand( const QString &command )
{
  if ( mBusy )
    return;

  mBusy = true;
  emit busyChanged( true );
  QString error;
  QVariant res;
  QString output;
  mEncounteredErrorMessageType = false;
  execCommandPrivate( command, error, &res, &output );

  if ( ! error.isEmpty() )
  {
    if ( !mEncounteredErrorMessageType )
      emit errorOccurred( error );
  }
  else
  {
    if ( !output.isEmpty() )
      emit consoleMessage( output, 0 );
    emit commandFinished( res );
  }

  mBusy = false;
  emit busyChanged( false );
}

void QgsRStatsSession::WriteConsole( const std::string &line, int type )
{
  if ( type > 0 )
    mEncounteredErrorMessageType = true;

  const QString message = QString::fromStdString( line );
  emit consoleMessage( message, type );
}

bool QgsRStatsSession::has_WriteConsole()
{
  return true;
}

void QgsRStatsSession::ShowMessage( const char *message )
{
  const QString messageString( message );
  emit showMessage( messageString );
}

bool QgsRStatsSession::has_ShowMessage()
{
  return true;
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
