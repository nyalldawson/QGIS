/***************************************************************************
                          qgspdfrenderer.h
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

#ifndef QGSPDFRENDERER_H
#define QGSPDFRENDERER_H

#include "qgsconfig.h"

#include <memory>

#include "qgis.h"
#include "qgis_core.h"
#include "qgis_sip.h"

#include <QFutureWatcher>
#include <QImage>
#include <QString>

class QPainter;
class QRectF;
class PdfDocumentContainer;


/**
 * \class QgsPdfRenderer
 * \ingroup core
 * \brief Utility class for rendering PDF documents.
 *
 * This functionality is not available on all platforms -- it requires a build
 * with the PDF4Qt library support enabled. On other platforms calling these
 * methods will raise a QgsNotSupportedException.
 *
 * \since QGIS 3.36
 */
class CORE_EXPORT QgsPdfRenderer
{
  public:
    /**
     * Constructs a PDF renderer for the file at the specified \a path.
     */
    QgsPdfRenderer( const QString &path, Qgis::PdfRenderFlags flags = Qgis::PdfRenderFlags() );
    ~QgsPdfRenderer();

    QgsPdfRenderer( const QgsPdfRenderer &other ) = delete;
    QgsPdfRenderer &operator=( const QgsPdfRenderer &other ) = delete;

    /**
     * Returns the file path of the associated PDF file.
     */
    QString path() const { return mPath; }

    /**
     * Returns the number of pages in the PDF.
     *
     * \throws QgsNotSupportedException on QGIS builds without PDF4Qt library support.
     */
    int pageCount() const SIP_THROW( QgsNotSupportedException );

    /**
     * Returns the media box for the specified page. Units are in PDF points.
     *
     * \throws QgsNotSupportedException on QGIS builds without PDF4Qt library support.
     */
    QRectF pageMediaBox( int pageNumber ) const SIP_THROW( QgsNotSupportedException );

    /**
     * Renders the PDF from the specified \a path to a \a painter.
     *
     * The \a painterRect argument specifies the target rectangle for the PDF page in
     * \a painter coordinates.
     *
     * \throws QgsNotSupportedException on QGIS builds without PDF4Qt library support.
     */
    bool render( QPainter *painter, const QRectF &painterRect, int pageIndex ) SIP_THROW( QgsNotSupportedException );

  private:
#ifdef SIP_RUN
    QgsPdfRenderer( const QgsPdfRenderer &other );
#endif

    QString mPath;
    Qgis::PdfRenderFlags mFlags;

#ifdef HAVE_PDF4QT
    std::unique_ptr< PdfDocumentContainer> mDocumentContainer;
#endif
};


class QgsFeedback;

#ifndef SIP_RUN
///@cond PRIVATE
class QgsPdfRasterizerJob : public QObject
{
    Q_OBJECT

  public:
    QgsPdfRasterizerJob( const QString &filename, const QSize &targetSize );

    void cancelWithoutBlocking();
    void cancelAndWait();
    bool isCanceled() const;
    QSize targetSize() const { return mTargetSize; }
    QImage result() const { return mWatcher.result(); }

  signals:
    void finished();

  private:
    QSize mTargetSize;
    std::unique_ptr< QgsFeedback > mFeedback;
    QFutureWatcher< QImage > mWatcher;
};
///@endcond PRIVATE


/**
 * \class QgsPdfRasterizer
 * \ingroup core
 * \brief Rasterizes and caches a PDF as a QImage at a requested size.
 *
 * Allows for background asynchronous regeneration of the cached image at a different size.
 *
 * This class is useful for generating pre-rendered versions of PDFs for rendering
 * on the main GUI thread, where those PDFs are too expensive to draw directly.
 *
 * \note Not available in Python bindings
 *
 * \since QGIS 4.4
*/
class CORE_EXPORT QgsPdfRasterizer : public QObject
{
    Q_OBJECT

  public:
    /**
     * Constructor for QgsPdfRasterizer, with the specified \a parent object.
     */
    explicit QgsPdfRasterizer( QObject *parent SIP_TRANSFERTHIS = nullptr );
    ~QgsPdfRasterizer() override;

    /**
     * Sets the source PDF file to cache.
     *
     * \see source()
     */
    void setSource( const QString &filename );

    /**
     * Returns the PDF source file.
     *
     * \see setSource()
     */
    QString source() const { return mSource; }

    /**
     * Clears the source PDF and cached image, canceling any in-progress background render.
     */
    void clear();

    /**
     * Immediately invalidates the current cached image.
     */
    void invalidate();

    /**
     * Immediately returns the best available QImage for the given \a targetSize.
     *
     * This method never blocks. If no cached image exists for the specified \a targetSize
     * or the cache has been invalidated then the previously cached image is returned,
     * and a background render at the desired \a targetSize will be started.
     */
    QImage image( const QSize &targetSize );

    /**
     * Returns TRUE if a background job is currently running or scheduled.
     */
    bool isRendering() const;

    /**
     * Sets the maximum width or height (in pixels) of the cached QImage.
     *
     * \see maxCacheSize()
     */
    void setMaxCacheSize( int maxSize );

    /**
     * Returns the maximum width or height (in pixels) of the cached QImage.
     *
     * \see setMaxCacheSize()
     */
    int maxCacheSize() const { return mMaxCacheSize; }

  signals:

    /**
     * Emitted when a background rasterization finishes and a new cached image is available.
     */
    void rendered();

  private slots:
    void startRenderJob();
    void onRenderJobFinished();

  private:
#ifdef SIP_RUN
    QgsPdfRasterizer( const QgsPdfRasterizer & );
#endif

    void cancelRunningJob( bool block );

    QString mSource;
    QImage mCachedImage;
    QSize mTargetSize;
    bool mInvalidated = true;

    int mMaxCacheSize = 5000;

    QTimer *mUpdateTimer = nullptr;
    std::unique_ptr< QgsPdfRasterizerJob > mJob;

    QgsPdfRasterizer( const QgsPdfRasterizer & ) = delete;
    QgsPdfRasterizer &operator=( const QgsPdfRasterizer & ) = delete;
};
#endif

#endif // QGSPDFRENDERER_H
