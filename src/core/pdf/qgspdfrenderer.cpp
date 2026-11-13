/***************************************************************************
                          qgspdfrenderer.cpp
                             -------------------
    begin                : December 2023
    copyright            : (C) 2023 by Nyall Dawson
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

#include "qgspdfrenderer.h"

#include "qgsfeedback.h"

#ifdef HAVE_PDF4QT
#include "pdfdocumentreader.h"
#include "pdfrenderer.h"
#include "pdffont.h"
#include "pdfcms.h"
#else
#include "qgsexception.h"
#include <QObject>
#endif

#include <QRectF>
#include <QPainter>
#include <QtConcurrentRun>
#include <QTimer>

#include "moc_qgspdfrenderer.cpp"

#ifdef HAVE_PDF4QT
class PdfDocumentContainer
{
  public:
    PdfDocumentContainer( const QString &path, Qgis::PdfRenderFlags flags )
      : reader(
          nullptr, []( bool * ) -> QString { return QString(); }, true, false
        )
      , document( reader.readFromFile( path ) )
      , modifiedDocument( &document, nullptr )
      , fontCache( 1000, 1000 )
    {
      fontCache.setDocument( modifiedDocument );

      pdf::PDFRenderer::Features features;
      if ( flags.testFlag( Qgis::PdfRenderFlag::RenderTextAsText ) )
      {
        features.setFlag( pdf::PDFRenderer::Feature::RealText, true );
      }
      renderer = std::make_unique< pdf::PDFRenderer >( &document, &fontCache, &pdfCms, nullptr, features, meshQualitySettings );
    }
    pdf::PDFDocumentReader reader;
    pdf::PDFDocument document;
    pdf::PDFModifiedDocument modifiedDocument;
    pdf::PDFFontCache fontCache;
    pdf::PDFCMSGeneric pdfCms;
    pdf::PDFMeshQualitySettings meshQualitySettings;
    std::unique_ptr< pdf::PDFRenderer > renderer;
};
#endif

QgsPdfRenderer::QgsPdfRenderer( const QString &path, Qgis::PdfRenderFlags flags )
  : mPath( path )
  , mFlags( flags )
{
#ifdef HAVE_PDF4QT
  mDocumentContainer = std::make_unique< PdfDocumentContainer >( path, flags );
#endif
}

QgsPdfRenderer::~QgsPdfRenderer() = default;

#ifdef HAVE_PDF4QT
int QgsPdfRenderer::pageCount() const
{
  const pdf::PDFCatalog *catalog = mDocumentContainer->document.getCatalog();
  return static_cast< int >( catalog->getPageCount() );
}
#else
int QgsPdfRenderer::pageCount() const
{
  throw QgsNotSupportedException( QObject::tr( "Rendering PDF requires a QGIS build with PDF4Qt library support" ) );
}
#endif

#ifdef HAVE_PDF4QT
QRectF QgsPdfRenderer::pageMediaBox( int pageNumber ) const
{
  if ( pageNumber < 0 || pageNumber >= pageCount() )
    return QRectF();

  const pdf::PDFCatalog *catalog = mDocumentContainer->document.getCatalog();
  return catalog->getPage( pageNumber )->getMediaBox();
}
#else
QRectF QgsPdfRenderer::pageMediaBox( int ) const
{
  throw QgsNotSupportedException( QObject::tr( "Rendering PDF requires a QGIS build with PDF4Qt library support" ) );
}
#endif

#ifdef HAVE_PDF4QT
bool QgsPdfRenderer::render( QPainter *painter, const QRectF &rectangle, int pageIndex )
{
  mDocumentContainer->renderer->render( painter, rectangle, pageIndex );
  return true;
}
#else
bool QgsPdfRenderer::render( QPainter *, const QRectF &, int )
{
  throw QgsNotSupportedException( QObject::tr( "Rendering PDF requires a QGIS build with PDF4Qt library support" ) );
}

#endif


//
// QgsPdfRasterizerJob
//
///@cond PRIVATE
QgsPdfRasterizerJob::QgsPdfRasterizerJob( const QString &filename, const QSize &targetSize )
  : mTargetSize( targetSize )
  , mFeedback( std::make_unique< QgsFeedback >() )
{
  connect( &mWatcher, &QFutureWatcher< QImage >::finished, this, &QgsPdfRasterizerJob::finished );

  QgsFeedback *feedback = mFeedback.get();
  mWatcher.setFuture( QtConcurrent::run( [filename, targetSize, feedback]() -> QImage {
#ifdef HAVE_PDF4QT
    if ( filename.isEmpty() || ( feedback && feedback->isCanceled() ) )
      return QImage();

    if ( targetSize.isEmpty() )
      return QImage();

    QImage image( targetSize, QImage::Format_ARGB32_Premultiplied );
    image.fill( Qt::transparent );
    if ( feedback && feedback->isCanceled() )
      return QImage();

    QPainter painter( &image );
    painter.setRenderHint( QPainter::Antialiasing, true );
    painter.setRenderHint( QPainter::TextAntialiasing, true );
    painter.setRenderHint( QPainter::SmoothPixmapTransform, true );

    QgsPdfRenderer renderer( filename, Qgis::PdfRenderFlag::RenderTextAsText );
    renderer.render( &painter, QRectF( 0, 0, targetSize.width(), targetSize.height() ), 0 );
    painter.end();

    if ( feedback && feedback->isCanceled() )
      return QImage();

    return image;
#else
    Q_UNUSED( filename )
    Q_UNUSED( targetSize )
    Q_UNUSED( feedback )
    return QImage();
#endif
  } ) );
}

void QgsPdfRasterizerJob::cancelWithoutBlocking()
{
  mFeedback->cancel();
}

void QgsPdfRasterizerJob::cancelAndWait()
{
  mFeedback->cancel();
  mWatcher.waitForFinished();
}

bool QgsPdfRasterizerJob::isCanceled() const
{
  return mFeedback->isCanceled();
}
///@endcond

//
// QgsPdfRasterizer
//

QgsPdfRasterizer::QgsPdfRasterizer( QObject *parent )
  : QObject( parent )
{
  mUpdateTimer = new QTimer( this );
  mUpdateTimer->setSingleShot( true );
  connect( mUpdateTimer, &QTimer::timeout, this, &QgsPdfRasterizer::startRenderJob );
}

QgsPdfRasterizer::~QgsPdfRasterizer()
{
  mUpdateTimer->stop();
  cancelRunningJob( true );
}

void QgsPdfRasterizer::setSource( const QString &filename )
{
  mSource = filename;

  mInvalidated = true;

  cancelRunningJob( false );

  if ( mSource.isEmpty() )
  {
    mUpdateTimer->stop();
    mCachedImage = QImage();
    mInvalidated = false;
  }
}

void QgsPdfRasterizer::clear()
{
  mUpdateTimer->stop();
  cancelRunningJob( false );

  mSource = QString();
  mCachedImage = QImage();
  mTargetSize = QSize();
  mInvalidated = false;
}

void QgsPdfRasterizer::invalidate()
{
  mInvalidated = true;
  if ( !mSource.isEmpty() && !mTargetSize.isEmpty() )
  {
    mUpdateTimer->start( 1 );
  }
}

QImage QgsPdfRasterizer::image( const QSize &targetSize )
{
  if ( targetSize.isEmpty() || mSource.isEmpty() )
    return QImage();

  // force size to match picture aspect ratio
  QSize size = targetSize;
  if ( mMaxCacheSize > 0 && ( targetSize.width() > mMaxCacheSize || targetSize.height() > mMaxCacheSize ) )
  {
    size = size.scaled( mMaxCacheSize, mMaxCacheSize, Qt::KeepAspectRatio );
  }

  if ( !mCachedImage.isNull() && !mInvalidated && mCachedImage.size() == size )
  {
    return mCachedImage;
  }

  const bool jobMatchesTarget = mJob && !mInvalidated && mJob->targetSize() == size;
  const bool timerMatchesTarget = mUpdateTimer->isActive() && !mInvalidated && mTargetSize == size;

  if ( !jobMatchesTarget && !timerMatchesTarget )
  {
    mTargetSize = size;
    mUpdateTimer->start( 1 );
  }

  return mCachedImage;
}

bool QgsPdfRasterizer::isRendering() const
{
  return static_cast< bool >( mJob ) || mUpdateTimer->isActive();
}

void QgsPdfRasterizer::setMaxCacheSize( int maxSize )
{
  if ( mMaxCacheSize == maxSize )
    return;

  mMaxCacheSize = maxSize;
  invalidate();
}

void QgsPdfRasterizer::startRenderJob()
{
  cancelRunningJob( false );

  if ( mSource.isEmpty() || mTargetSize.isEmpty() )
    return;

  mInvalidated = false;
  mJob = std::make_unique< QgsPdfRasterizerJob >( mSource, mTargetSize );
  connect( mJob.get(), &QgsPdfRasterizerJob::finished, this, &QgsPdfRasterizer::onRenderJobFinished );
}

void QgsPdfRasterizer::onRenderJobFinished()
{
  if ( !mJob )
    return;

  if ( !mJob->isCanceled() )
  {
    const QImage result = mJob->result();
    if ( !result.isNull() )
    {
      mCachedImage = result;
    }
  }

  mJob.reset();
  emit rendered();
}

void QgsPdfRasterizer::cancelRunningJob( bool block )
{
  if ( !mJob )
    return;

  disconnect( mJob.get(), &QgsPdfRasterizerJob::finished, this, &QgsPdfRasterizer::onRenderJobFinished );
  if ( block )
  {
    mJob->cancelAndWait();
    mJob.reset();
  }
  else
  {
    QgsPdfRasterizerJob *oldJob = mJob.release();
    connect( oldJob, &QgsPdfRasterizerJob::finished, oldJob, &QObject::deleteLater );
    oldJob->cancelWithoutBlocking();
  }
}
