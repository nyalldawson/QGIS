#include "qgsrstatsrunner.h"
#include "qgsapplication.h"

#include <RcppCommon.h>
#include <Rcpp.h>
#include <RInside.h>

#include "qgisapp.h"
#include "qgslogger.h"
#include "qgsvariantutils.h"
#include "qgstaskmanager.h"
#include "qgsproxyprogresstask.h"
#include <QVariant>
#include <QString>
#include <QFile>
#include <QDir>

#include "qgsproviderregistry.h"
#include "qgsvectorlayerfeatureiterator.h"


class MapLayerWrapper
{
  public:

    MapLayerWrapper( const QgsMapLayer *layer = nullptr )
      : mLayerId( layer ? layer->id() : QString() )
    {

    }

    std::string id() const
    {
      return mLayerId.toStdString(); \
    }

    long long featureCount() const
    {
      long long res = -1;
      auto countOnMainThread = [&res, this]
      {
        Q_ASSERT_X( QThread::currentThread() == qApp->thread(), "featureCount", "featureCount must be run on the main thread" );

        if ( QgsMapLayer *layer = QgsProject::instance()->mapLayer( mLayerId ) )
        {
          if ( QgsVectorLayer *vl = qobject_cast< QgsVectorLayer * >( layer ) )
          {
            res = vl->featureCount();
          }
        }
      };

      QMetaObject::invokeMethod( qApp, countOnMainThread, Qt::BlockingQueuedConnection );
      return res;
    }

    Rcpp::DataFrame toDataFrame( bool selectedOnly ) const
    {
      Rcpp::DataFrame result = Rcpp::DataFrame();

      bool prepared = false;
      QgsFields fields;
      long long featureCount = -1;
      std::unique_ptr< QgsVectorLayerFeatureSource > source;
      std::unique_ptr< QgsScopedProxyProgressTask > task;
      QgsFeatureIds selectedFeatureIds;
      auto prepareOnMainThread = [&prepared, &fields, &featureCount, &source, &task, selectedOnly, &selectedFeatureIds, this]
      {
        Q_ASSERT_X( QThread::currentThread() == qApp->thread(), "toDataFrame", "prepareOnMainThread must be run on the main thread" );

        prepared = false;
        if ( QgsMapLayer *layer = QgsProject::instance()->mapLayer( mLayerId ) )
        {
          if ( QgsVectorLayer *vlayer = qobject_cast< QgsVectorLayer * >( layer ) )
          {
            fields = vlayer->fields();
            source = std::make_unique< QgsVectorLayerFeatureSource >( vlayer );
            if ( selectedOnly )
            {
              selectedFeatureIds = vlayer->selectedFeatureIds();
              featureCount  = selectedFeatureIds.size();
            }
            else
            {
              featureCount = vlayer->featureCount();
            }
          }
        }
        prepared = true;

        task = std::make_unique< QgsScopedProxyProgressTask >( QObject::tr( "Creating R dataframe" ), true );
      };

      QMetaObject::invokeMethod( qApp, prepareOnMainThread, Qt::BlockingQueuedConnection );
      if ( !prepared )
        return result;

      QList< int > attributesToFetch;
      for ( int index = 0; index < fields.count(); ++index )
      {
        Rcpp::RObject column;
        const QgsField field = fields.at( index );

        switch ( field.type() )
        {
          case QVariant::Bool:
          {
            column = Rcpp::LogicalVector( featureCount );
            break;
          }
          case QVariant::Int:
          {
            column = Rcpp::IntegerVector( featureCount );
            break;
          }
          case QVariant::Double:
          {
            column = Rcpp::DoubleVector( featureCount );
            break;
          }
          case QVariant::LongLong:
          {
            column = Rcpp::DoubleVector( featureCount );
            break;
          }
          case QVariant::String:
          {
            column = Rcpp::StringVector( featureCount );
            break;
          }

          default:
            continue;
        }

        result.push_back( column, field.name().toStdString() );
        attributesToFetch.append( index );
      }

      if ( selectedOnly && selectedFeatureIds.empty() )
        return result;

      QgsFeature feature;
      QgsFeatureRequest req;
      req.setFlags( QgsFeatureRequest::NoGeometry );
      req.setSubsetOfAttributes( attributesToFetch );
      if ( selectedOnly )
        req.setFilterFids( selectedFeatureIds );

      QgsFeatureIterator it = source->getFeatures( req );
      std::size_t featureNumber = 0;

      int prevProgress = 0;
      while ( it.nextFeature( feature ) )
      {
        const int progress = 100 * static_cast< double>( featureNumber ) / featureCount;
        if ( progress > prevProgress )
        {
          task->setProgress( progress );
          prevProgress = progress;
        }

        if ( task->isCanceled() )
          break;

        int settingColumn = 0;

        const QgsAttributes attributes = feature.attributes();
        const QVariant *attributeData = attributes.constData();

        for ( int i = 0; i < fields.count(); i ++, attributeData++ )
        {
          QgsField field = fields.at( i );

          switch ( field.type() )
          {
            case QVariant::Bool:
            {
              Rcpp::LogicalVector column = result[settingColumn];
              column[featureNumber] = attributeData->toBool();
              break;
            }
            case QVariant::Int:
            {
              Rcpp::IntegerVector column = result[settingColumn];
              column[featureNumber] = attributeData->toInt();
              break;
            }
            case QVariant::LongLong:
            {
              Rcpp::DoubleVector column = result[settingColumn];
              bool ok;
              double val = attributeData->toDouble( &ok );
              if ( ok )
                column[featureNumber] = val;
              else
                column[featureNumber] = R_NaReal;
              break;
            }
            case QVariant::Double:
            {
              Rcpp::DoubleVector column = result[settingColumn];
              column[featureNumber] = attributeData->toDouble();
              break;
            }
            case QVariant::String:
            {
              Rcpp::StringVector column = result[settingColumn];
              column[featureNumber] = attributeData->toString().toStdString();
              break;
            }

            default:
              continue;
          }
          settingColumn++;
        }
        featureNumber++;
      }
      return result;
    }


  private:

    QString mLayerId;

};


SEXP MapLayerWrapperId( Rcpp::XPtr<MapLayerWrapper> obj )
{
  return Rcpp::wrap( obj->id() );
}

SEXP MapLayerWrapperFeatureCount( Rcpp::XPtr<MapLayerWrapper> obj )
{
  return Rcpp::wrap( obj->featureCount() );
}

SEXP MapLayerWrapperToDataFrame( Rcpp::XPtr<MapLayerWrapper> obj, bool selectedOnly )
{
  return obj->toDataFrame( selectedOnly );
}


SEXP MapLayerWrapperByName( std::string name )
{
  QList< QgsMapLayer * > layers = QgsProject::instance()->mapLayersByName( QString::fromStdString( name ) );
  if ( !layers.empty() )
    return Rcpp::XPtr<MapLayerWrapper>( new MapLayerWrapper( layers.at( 0 ) ) );

  return nullptr;
}



SEXP MapLayerWrapperDollar( Rcpp::XPtr<MapLayerWrapper> obj, std::string name )
{
  if ( name == "id" )
  {
    return Rcpp::wrap( obj->id() );
  }
  else
  {
    return NULL;
  }
}


class QgsApplicationRWrapper
{
  public:
    QgsApplicationRWrapper() {}

    int version() const { return Qgis::versionInt(); }

    SEXP activeLayer() const
    {
      Rcpp::XPtr<MapLayerWrapper> res( new MapLayerWrapper( QgisApp::instance()->activeLayer() ) );
      res.attr( "class" ) = "MapLayerWrapper";
      // res.attr( "id" ) = Rcpp::InternalFunction( & MapLayerWrapperId );
      //res.assign( Rcpp::InternalFunction( & MapLayerWrapperDollar ), "$.QGIS" );

      // res.attr( "axxx" ) = "QGIS";
      return res;
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
    return obj->activeLayer();
  }
  else if ( name == "mapLayerByName" )
  {
    return Rcpp::InternalFunction( & MapLayerWrapperByName );
  }
  else if ( name == "layerId" )
  {
    return Rcpp::InternalFunction( & MapLayerWrapperId );
  }
  else if ( name == "featureCount" )
  {
    return Rcpp::InternalFunction( & MapLayerWrapperFeatureCount );
  }
  else if ( name == "toDataFrame" )
  {
    return Rcpp::InternalFunction( & MapLayerWrapperToDataFrame );
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
  ret.push_back( "layerId" );
  ret.push_back( "featureCount" );
  ret.push_back( "mapLayerByName" );
  return ret;
}

Rcpp::NumericVector activeLayerNumericField( const std::string fieldName )
{
  Rcpp::NumericVector result;

  QgsMapLayer *layer = QgisApp::instance()->activeLayer();
  QgsVectorLayer *vlayer = qobject_cast<QgsVectorLayer *>( layer );

  if ( !vlayer || ( vlayer->dataProvider()->featureCount() < 1 ) )
    return result;

  int fieldIndex = vlayer->fields().lookupField( QString::fromStdString( fieldName ) );

  if ( fieldIndex < 0 )
    return result;

  QgsField field = vlayer->fields().field( fieldIndex );

  if ( !( field.type() == QVariant::Double || field.type() == QVariant::Int ) )
    return result;

  result = Rcpp::NumericVector( vlayer->dataProvider()->featureCount(), 0 );

  QgsFeature feature;

  int i = 0;
  QgsFeatureIterator it = vlayer->dataProvider()->getFeatures( QgsFeatureRequest() );

  while ( it.nextFeature( feature ) )
  {
    result[i] = feature.attribute( fieldIndex ).toDouble();
    i++;
  }

  return result;
}

SEXP activeLayerToSf()
{

  QgsMapLayer *layer = QgisApp::instance()->activeLayer();
  QgsVectorLayer *vlayer = qobject_cast<QgsVectorLayer *>( layer );

  if ( !vlayer )
    return R_NilValue;

  if ( vlayer->dataProvider()->name() != QStringLiteral( "ogr" ) )
    return R_NilValue;

  const QVariantMap parts = QgsProviderRegistry::instance()->decodeUri( layer->dataProvider()->name(), layer->source() );
  std::string path = parts[ QStringLiteral( "path" ) ].toString().toStdString();
  std::string layerName = parts[ QStringLiteral( "layerName" ) ].toString().toStdString();

  if ( path.empty() )
    return R_NilValue;

  Rcpp::Function st_read( "st_read" );

  return st_read( path, layerName );
}

//
// QgsRStatsSession
//
QgsRStatsSession::QgsRStatsSession()
{
  mRSession = std::make_unique< RInside >( 0, nullptr, true, false, true );
  mRSession->set_callbacks( this );

  const QString userPath = QgsApplication::qgisSettingsDirPath() + QStringLiteral( "r_libs" );
  if ( !QFile::exists( userPath ) )
  {
    QDir().mkpath( userPath );
  }
  execCommandNR( QStringLiteral( ".libPaths(\"%1\")" ).arg( userPath ) );

  Rcpp::XPtr<QgsApplicationRWrapper> wr( new QgsApplicationRWrapper() );
  wr.attr( "class" ) = ".QGISPrivate";
  mRSession->assign( wr, ".QGISPrivate" );
  mRSession->assign( Rcpp::InternalFunction( & Dollar ), "$..QGISPrivate" );

  QString error;
  execCommandPrivate( QStringLiteral( R"""(
  QGIS <- list(
    toDataFrame=function(layer, selectedOnly=FALSE) { .QGISPrivate$toDataFrame(layer, selectedOnly) },
    versionInt=function() { .QGISPrivate$versionInt },
    mapLayerByName=function(name) { .QGISPrivate$mapLayerByName(name) },
    activeLayer=function() { .QGISPrivate$activeLayer }
  )
  class(QGIS) <- "QGIS"
  )""" ), error );

  if ( !error.isEmpty() )
  {
    QgsDebugMsg( error );
  }

  /*
  mRSession->assign( Rcpp::InternalFunction( & activeLayerNumericField ), "activeLayerNumericField" );
  mRSession->assign( Rcpp::InternalFunction( & activeLayerToSf ), "readActiveLayerToSf" );
  */
}

void QgsRStatsSession::showStartupMessage()
{
  QVariant versionString;
  QString error;
  execCommandPrivate( QStringLiteral( "R.version$version.string" ), error, &versionString );
  QVariant nicknameString;
  execCommandPrivate( QStringLiteral( "R.version$nickname" ), error, &nicknameString );
  QVariant platformString;
  execCommandPrivate( QStringLiteral( "R.version$platform" ), error, &platformString );
  QVariant yearString;
  execCommandPrivate( QStringLiteral( "R.version$year" ), error, &yearString );
  QVariant sizeInt;
  execCommandPrivate( QStringLiteral( ".Machine$sizeof.pointer" ), error, &sizeInt );

  emit showMessage( QStringLiteral( "%1 -- %2" ).arg( versionString.toString(), nicknameString.toString() ) );
  emit showMessage( QStringLiteral( "Copyright (C) %1 The R Foundation for Statistical Computing" ).arg( yearString.toString() ) );
  const int bits = sizeInt.toInt() == 8 ? 64 : 32;
  emit showMessage( QStringLiteral( "Platform: %1 (%2-bit)" ).arg( platformString.toString() ).arg( bits ) );
  emit showMessage( QString() );

  emit showMessage( QStringLiteral( "R is free software and comes with ABSOLUTELY NO WARRANTY." ) );
  emit showMessage( QStringLiteral( "You are welcome to redistribute it under certain conditions." ) );
  emit showMessage( QStringLiteral( "Type 'license()' or 'licence()' for distribution details." ) );
  emit showMessage( QString() );

  emit showMessage( QStringLiteral( "R is a collaborative project with many contributors." ) );
  emit showMessage( QStringLiteral( "Type 'contributors()' for more information and" ) );
  emit showMessage( QStringLiteral( "'citation()' on how to cite R or R packages in publications." ) );
  emit showMessage( QString() );

  // TODO -- these don't actually work!
  // emit showMessage( QStringLiteral( "Type 'demo()' for some demos, 'help()' for on-line help, or" ) );
  // emit showMessage( QStringLiteral( "'help.start()' for an HTML browser interface to help." ) );
  emit showMessage( QString() );
}

QgsRStatsSession::~QgsRStatsSession() = default;

QString QgsRStatsSession::sexpToString( const SEXP exp )
{
  switch ( TYPEOF( exp ) )
  {
    case EXPRSXP:
    case CLOSXP:
    case ENVSXP:
    case LANGSXP:
    case S4SXP:
      // these types can't be converted to StringVector, will raise exceptions
      return QString();

    case CHARSXP:
    {
      // special case
      return QStringLiteral( "[1] \"%1\"" ).arg( QString::fromStdString( Rcpp::as<std::string>( exp ) ) );
    }

    case LGLSXP:
    case INTSXP:
    case REALSXP:
    case STRSXP:
    case EXTPTRSXP:
    case VECSXP:
      break; // we know these types are fine to convert to StringVector

    case NILSXP:
      return QStringLiteral( "NULL" );

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
  return QString::fromStdString( outcome );
}

QVariant QgsRStatsSession::sexpToVariant( const SEXP exp )
{
  switch ( TYPEOF( exp ) )
  {
    case S4SXP:
    case LANGSXP:
    case SYMSXP:
    case EXTPTRSXP:
    case CLOSXP:
    case ENVSXP:
      // not safe to call LENGTH on!
      return QVariant();

    default:
      break;
  }

  QgsDebugMsg( QStringLiteral( "Handing type: %1" ).arg( TYPEOF( exp ) ) );
  const int length = LENGTH( exp );
  if ( length == 0 )
  {
    if ( TYPEOF( exp ) == NILSXP )
      return QVariant();
    else if ( TYPEOF( exp ) == CHARSXP )
      return QString( "" );
    else
      return QVariantList();
  }

  switch ( TYPEOF( exp ) )
  {
    case NILSXP:
      return QVariant();

    case LGLSXP:
    {
      if ( length > 1 )
      {
        const Rcpp::LogicalVector logicalVector( exp );

        QVariantList res;
        res.reserve( length );
        for ( int i = 0; i < length; i++ )
        {
          const int expInt = logicalVector[i];
          if ( expInt < 0 )
            res << QVariant();
          else
            res << static_cast< bool >( expInt );
        }
        return res;
      }
      else
      {
        const int expInt = Rcpp::as<int>( exp );
        if ( expInt < 0 )
          return QVariant();
        else
          return static_cast< bool >( expInt );
      }
    }

    case INTSXP:
    {
      if ( length > 1 )
      {
        const Rcpp::IntegerVector intVector( exp );

        QVariantList res;
        res.reserve( length );
        for ( int i = 0; i < length; i++ )
        {
          const int elementInt = intVector[i];
          res << ( elementInt == NA_INTEGER ? QVariant() : QVariant( elementInt ) );
        }
        return res;
      }
      else
      {
        const int res = Rcpp::as<int>( exp );
        return res == NA_INTEGER ? QVariant() : QVariant( res );
      }
    }

    case REALSXP:
    {
      if ( length > 1 )
      {
        const Rcpp::DoubleVector realVector( exp );

        QVariantList res;
        res.reserve( length );
        for ( int i = 0; i < length; i++ )
        {
          const double elementReal = realVector[i];
          res << ( std::isnan( elementReal ) ? QVariant() : QVariant( elementReal ) );
        }
        return res;
      }
      else
      {
        const double res = Rcpp::as<double>( exp );
        return std::isnan( res ) ? QVariant() : res;
      }
    }

    case STRSXP:
      if ( length > 1 )
      {
        const Rcpp::StringVector stringVector( exp );

        QVariantList res;
        res.reserve( length );
        for ( int i = 0; i < length; i++ )
        {
          const char *elementString = stringVector[i];
          res << QVariant( QString( elementString ) );
        }
        return res;
      }
      else
      {
        return QString::fromStdString( Rcpp::as<std::string>( exp ) );
      }

    case CHARSXP:
      return QString::fromStdString( Rcpp::as<std::string>( exp ) );

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

SEXP QgsRStatsSession::variantToSexp( const QVariant &variant )
{
  switch ( variant.type() )
  {
    case QVariant::Invalid:
      return R_NilValue;

    case QVariant::Bool:
      if ( QgsVariantUtils::isNull( variant ) )
        return Rcpp::wrap( NA_LOGICAL );

      return Rcpp::wrap( variant.toBool() ? 1 : 0 );

    case QVariant::Int:
      if ( QgsVariantUtils::isNull( variant ) )
        return Rcpp::wrap( NA_INTEGER );

      return Rcpp::wrap( variant.toInt() );

    case QVariant::Double:
      if ( QgsVariantUtils::isNull( variant ) )
        return Rcpp::wrap( std::numeric_limits< double >::quiet_NaN() );

      return Rcpp::wrap( variant.toDouble() );

    case QVariant::String:
      return Rcpp::wrap( variant.toString().toStdString() );

    case QVariant::UserType:
      QgsDebugMsg( QStringLiteral( "unsupported user variant type %1" ).arg( QMetaType::typeName( variant.userType() ) ) );
      return nullptr;

    default:
      QgsDebugMsg( QStringLiteral( "unsupported variant type %1" ).arg( QVariant::typeToName( variant.type() ) ) );
      return nullptr;
  }
}

void QgsRStatsSession::execCommandPrivate( const QString &command, QString &error, QVariant *res, QString *output )
{
  try
  {
    const SEXP sexpRes = mRSession->parseEval( command.toStdString() );
    if ( res )
      *res = sexpToVariant( sexpRes );
    if ( output )
      *output = sexpToString( sexpRes );
  }
  catch ( std::exception &ex )
  {
    error = QString::fromStdString( ex.what() );
  }
  catch ( ... )
  {
    std::cerr << "Unknown exception caught" << std::endl;
  }
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

void QgsRStatsRunner::showStartupMessage()
{
  QMetaObject::invokeMethod( mSession.get(), "showStartupMessage", Qt::QueuedConnection );
}
