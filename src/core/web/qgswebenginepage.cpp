/***************************************************************************
                          qgswebenginepage.h
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

#include "qgsconfig.h"
#include "qgswebenginepage.h"

#include "qgsfeedback.h"

#include <QCoreApplication>
#include <QEventLoop>
#include <QSizeF>
#include <QThread>
#include <QWebEnginePage>

#include "moc_qgswebenginepage.cpp"

#ifdef HAVE_PDF4QT
#include "qgspdfrenderer.h"
#include <QTemporaryFile>
#else
#include "qgsexception.h"
#endif

QgsWebEnginePage::QgsWebEnginePage( QObject *parent )
  : QObject( parent )
  , mPage { std::make_unique< QWebEnginePage >() }
{
  // proxy some signals from the page
  connect( mPage.get(), &QWebEnginePage::loadStarted, this, &QgsWebEnginePage::loadStarted );
  connect( mPage.get(), &QWebEnginePage::loadProgress, this, &QgsWebEnginePage::loadProgress );
  connect( mPage.get(), &QWebEnginePage::loadFinished, this, &QgsWebEnginePage::loadFinished );
}

QgsWebEnginePage::~QgsWebEnginePage() = default;

QWebEnginePage *QgsWebEnginePage::page()
{
  return mPage.get();
}

bool QgsWebEnginePage::setContent( const QByteArray &data, const QString &mimeType, const QUrl &baseUrl, bool blocking )
{
  mCachedSize = QSize();
  if ( blocking )
  {
    QEventLoop loop;
    bool finished = false;
    bool result = true;
    connect( mPage.get(), &QWebEnginePage::loadFinished, &loop, [&loop, &finished, &result]( bool ok ) {
      finished = true;
      result = ok;
      loop.exit();
    } );
    mPage->setContent( data, mimeType, baseUrl );
    if ( !finished )
    {
      loop.exec( QEventLoop::ExcludeUserInputEvents );
    }
    if ( result )
      handlePostBlockingLoadOperations();
    return result;
  }
  else
  {
    mPage->setContent( data, mimeType, baseUrl );
    return true;
  }
}

bool QgsWebEnginePage::setHtml( const QString &html, const QUrl &baseUrl, bool blocking )
{
  mCachedSize = QSize();
  if ( blocking )
  {
    QEventLoop loop;
    bool finished = false;
    bool result = true;
    connect( mPage.get(), &QWebEnginePage::loadFinished, &loop, [&loop, &finished, &result]( bool ok ) {
      finished = true;
      result = ok;
      loop.exit();
    } );
    mPage->setHtml( html, baseUrl );
    if ( !finished )
    {
      loop.exec( QEventLoop::ExcludeUserInputEvents );
    }
    if ( result )
      handlePostBlockingLoadOperations();

    return result;
  }
  else
  {
    mPage->setHtml( html, baseUrl );
    return true;
  }
}

bool QgsWebEnginePage::setUrl( const QUrl &url, bool blocking )
{
  mCachedSize = QSize();
  if ( blocking )
  {
    QEventLoop loop;
    bool finished = false;
    bool result = true;
    connect( mPage.get(), &QWebEnginePage::loadFinished, &loop, [&loop, &finished, &result]( bool ok ) {
      finished = true;
      result = ok;
      loop.exit();
    } );
    mPage->setUrl( url );
    if ( !finished )
    {
      loop.exec( QEventLoop::ExcludeUserInputEvents );
    }
    if ( result )
      handlePostBlockingLoadOperations();

    return result;
  }
  else
  {
    mPage->setUrl( url );
    return true;
  }
}

QSize QgsWebEnginePage::documentSize() const
{
  if ( mCachedSize.isValid() )
    return mCachedSize;

  QEventLoop loop;
  int width = -1;
  int height = -1;
  bool finished = false;
  mPage->runJavaScript( "[document.documentElement.scrollWidth, document.documentElement.scrollHeight];", [&width, &height, &loop, &finished]( QVariant result ) {
    width = result.toList().value( 0 ).toInt();
    height = result.toList().value( 1 ).toInt();
    finished = true;
    loop.exit();
  } );
  if ( !finished )
  {
    loop.exec( QEventLoop::ExcludeUserInputEvents );
  }


  mCachedSize = QSize( width, height );
  return mCachedSize;
}

void QgsWebEnginePage::handlePostBlockingLoadOperations()
{
  // Following a blocking content load, do some other quick calculations which involve local event loops.
  // This allows callers to avoid having to make another later call to a method which would other involve a local event loop.
  QEventLoop loop;
  int width = 0;
  int height = 0;
  bool finished = false;
  mPage->runJavaScript( "[document.documentElement.scrollWidth, document.documentElement.scrollHeight];", [&width, &height, &loop, &finished]( QVariant result ) {
    width = result.toList().value( 0 ).toInt();
    height = result.toList().value( 1 ).toInt();
    finished = true;
    loop.exit();
  } );
  if ( !finished )
  {
    loop.exec( QEventLoop::ExcludeUserInputEvents );
  }

  mCachedSize = QSize( width, height );
}

#ifdef HAVE_PDF4QT
bool QgsWebEnginePage::render( QPainter *painter, const QRectF &painterRect )
{
  const QSize actualSize = documentSize();

  const QSizeF pageSize = QSizeF( actualSize.width() / RENDER_TO_PDF_DPI, actualSize.height() / RENDER_TO_PDF_DPI );

  QEventLoop loop;
  bool finished = false;
  bool printOk = false;
  QString renderedPdfPath;
  connect( mPage.get(), &QWebEnginePage::pdfPrintingFinished, &loop, [&loop, &finished, &printOk, &renderedPdfPath]( const QString &pdfPath, bool success ) {
    finished = true;
    renderedPdfPath = pdfPath;
    printOk = success;
    loop.exit();
  } );

  // generate file name for temporary intermediate PDF file
  QTemporaryFile f;
  if ( !f.open() )
    return false;

  f.close();

  const QPageLayout layout = QPageLayout( QPageSize( pageSize, QPageSize::Inch ), QPageLayout::Portrait, QMarginsF( 0, 0, 0, 0 ), QPageLayout::Inch, QMarginsF( 0, 0, 0, 0 ) );
  mPage->printToPdf( f.fileName(), layout );

  if ( !finished )
  {
    loop.exec( QEventLoop::ExcludeUserInputEvents );
  }

  if ( printOk )
  {
    QgsPdfRenderer renderer( renderedPdfPath );
    renderer.render( painter, painterRect, 0 );
  }
  return printOk;
}
#else
bool QgsWebEnginePage::render( QPainter *, const QRectF & )
{
  throw QgsNotSupportedException( QObject::tr( "Rendering web pages requires a QGIS build with PDF4Qt library support" ) );
}
#endif

bool QgsWebEnginePage::printToPdfBlockingInternal( const QString &pdfFileName, QgsFeedback *feedback, std::function<void( QWebEnginePage * )> loadContent )
{
  const bool requestMadeFromMainThread = QThread::currentThread() == QCoreApplication::instance()->thread();

  bool result = false;

  const std::function<void()> runFunction = [feedback, pdfFileName, &loadContent, &result]() {
    // this function will always be run in worker threads -- either the blocking call is being made in a worker thread,
    // or the blocking call has been made from the main thread and we've fired up a new thread for this function
    Q_ASSERT( QThread::currentThread() != QCoreApplication::instance()->thread() );

    QWebEnginePage page;

    QEventLoop loadLoop;
    // connecting to aboutToQuit avoids an on-going process to remain stalled
    // when QThreadPool::globalInstance()->waitForDone()
    // is called at process termination
    connect( qApp, &QCoreApplication::aboutToQuit, &loadLoop, &QEventLoop::quit, Qt::DirectConnection );

    bool finished = false;
    connect( &page, &QWebEnginePage::loadFinished, &loadLoop, [&loadLoop, &finished, &result]( bool ok ) {
      finished = true;
      result = ok;
      loadLoop.exit();
    } );

    if ( feedback )
    {
      QObject::connect( feedback, &QgsFeedback::canceled, &loadLoop, &QEventLoop::quit );
    }

    loadContent( &page );
    if ( !finished )
    {
      loadLoop.exec();
    }

    if ( !result || ( feedback && feedback->isCanceled() ) )
    {
      result = false;
      return;
    }

    QEventLoop calculateSizeLoop;
    connect( qApp, &QCoreApplication::aboutToQuit, &calculateSizeLoop, &QEventLoop::quit, Qt::DirectConnection );

    if ( feedback )
    {
      QObject::connect( feedback, &QgsFeedback::canceled, &calculateSizeLoop, &QEventLoop::quit );
    }

    finished = false;
    result = false;
    int width = -1;
    int height = -1;
    page.runJavaScript( "[document.documentElement.scrollWidth, document.documentElement.scrollHeight];", [&width, &height, &calculateSizeLoop, &finished, &result]( QVariant javaScriptResult ) {
      width = javaScriptResult.toList().value( 0 ).toInt();
      height = javaScriptResult.toList().value( 1 ).toInt();
      finished = true;
      result = true;
      calculateSizeLoop.exit();
    } );
    if ( !finished )
    {
      calculateSizeLoop.exec();
    }

    if ( !result || ( feedback && feedback->isCanceled() ) )
    {
      result = false;
      return;
    }


    QEventLoop printLoop;
    connect( qApp, &QCoreApplication::aboutToQuit, &printLoop, &QEventLoop::quit, Qt::DirectConnection );

    finished = false;
    connect( &page, &QWebEnginePage::pdfPrintingFinished, &printLoop, [&printLoop, &finished, &result]( const QString &, bool ok ) {
      finished = true;
      result = ok;
      printLoop.exit();
    } );

    if ( feedback )
    {
      QObject::connect( feedback, &QgsFeedback::canceled, &printLoop, &QEventLoop::quit );
    }

    const QSizeF pageSize = QSizeF( width / RENDER_TO_PDF_DPI, height / RENDER_TO_PDF_DPI );

    const QPageLayout layout = QPageLayout( QPageSize( pageSize, QPageSize::Inch ), QPageLayout::Portrait, QMarginsF( 0, 0, 0, 0 ), QPageLayout::Inch, QMarginsF( 0, 0, 0, 0 ) );
    page.printToPdf( pdfFileName, layout );

    if ( !finished )
    {
      printLoop.exec();
    }
  };

  if ( requestMadeFromMainThread )
  {
    auto processThread = std::make_unique<WebEngineRenderThread>( runFunction );
    processThread->start();
    // wait for thread to gracefully exit
    processThread->wait();
  }
  else
  {
    runFunction();
  }

  return result;
}

bool QgsWebEnginePage::printToPdfBlocking( const QByteArray &data, const QString &pdfFileName, const QString &mimeType, const QUrl &baseUrl, QgsFeedback *feedback )
{
  return printToPdfBlockingInternal( pdfFileName, feedback, [data, mimeType, baseUrl]( QWebEnginePage *page ) { page->setContent( data, mimeType, baseUrl ); } );
}

bool QgsWebEnginePage::printToPdfBlocking( const QString &html, const QString &pdfFileName, const QUrl &baseUrl, QgsFeedback *feedback )
{
  return printToPdfBlockingInternal( pdfFileName, feedback, [html, baseUrl]( QWebEnginePage *page ) { page->setHtml( html, baseUrl ); } );
}

bool QgsWebEnginePage::printToPdfBlocking( const QUrl &url, const QString &pdfFileName, QgsFeedback *feedback )
{
  return printToPdfBlockingInternal( pdfFileName, feedback, [url]( QWebEnginePage *page ) { page->setUrl( url ); } );
}
