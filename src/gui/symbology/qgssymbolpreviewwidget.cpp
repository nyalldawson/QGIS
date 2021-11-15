/***************************************************************************
    qgssymbolpreviewwidget.cpp
    ---------------------
    begin                : November 2021
    copyright            : (C) 2021 by Nyall Dawson
    email                : nyall dot dawson at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "qgssymbolpreviewwidget.h"
#include "qgssymbol.h"
#include "qgsmapsettings.h"
#include "qgsrendercontext.h"
#include "qgsmarkersymbol.h"
#include "qgslinesymbol.h"
#include "qgsfillsymbol.h"
#include "qgsstyle.h"
#include "qgssettings.h"
#include <QMouseEvent>
#include <QPainter>

QgsSymbolPreviewWidget::QgsSymbolPreviewWidget( QWidget *parent )
  : QWidget( parent )
{

}

QgsSymbolPreviewWidget::~QgsSymbolPreviewWidget() = default;

void QgsSymbolPreviewWidget::setSymbol( QgsSymbol *symbol )
{
  mSymbol.reset( symbol );
  update();
}

QgsSymbol *QgsSymbolPreviewWidget::symbol() const
{
  return mSymbol.get();
}

void QgsSymbolPreviewWidget::setPatchShape( const QgsLegendPatchShape &shape )
{
  mPatchShape = shape;
  update();
}

void QgsSymbolPreviewWidget::mousePressEvent( QMouseEvent *event )
{
  if ( mIsMoving )
    return;

  if ( event->button() != Qt::LeftButton && event->button() != Qt::MiddleButton )
    return;

  mIsMoving = true;
  mMouseStartMove = event->pos();
  mMoveButton = event->button();
}

void QgsSymbolPreviewWidget::mouseReleaseEvent( QMouseEvent *event )
{
  if ( !mIsMoving || mMoveButton != event->button() )
    return;

  mIsMoving = false;
  mSymbolOffset = QPointF( mSymbolOffset.x() + mTemporaryOffset.x(), mSymbolOffset.y() + mTemporaryOffset.y() );
  mTemporaryOffset = QPointF( 0, 0 );
}

void QgsSymbolPreviewWidget::mouseMoveEvent( QMouseEvent *event )
{
  if ( !mIsMoving )
    return;

  mTemporaryOffset = QPointF( event->pos().x() - mMouseStartMove.x(), event->pos().y() - mMouseStartMove.y() );
  update();
}

void QgsSymbolPreviewWidget::wheelEvent( QWheelEvent *event )
{
  QgsSettings settings;
  const double zoomFactor = settings.value( QStringLiteral( "qgis/zoom_factor" ), 2.0 ).toDouble();

  // "Normal" mouse have an angle delta of 120, precision mouses provide data faster, in smaller steps
  double mouseZoom = 1.0 + ( zoomFactor - 1.0 ) / 120.0 * std::fabs( event->angleDelta().y() );
  // but here we emulate the "ctrl" zoom "fine" zoom mode we use elsewhere...
  mouseZoom = 1 + ( zoomFactor - 1 ) / 20.0;

  const bool zoomIn = event->angleDelta().y() > 0;
  const double previousZoom = mZoomScale;
  const double scaleFactor = zoomIn ? 1 / mouseZoom : mouseZoom;
  mZoomScale /= scaleFactor;

  // zoom happens keeping the mouse point static
  mSymbolOffset = QPointF( event->pos().x() - size().width() / 2 - ( mZoomScale / previousZoom ) * ( event->pos().x() - size().width() / 2 - mSymbolOffset.x() ),
                           event->pos().y() - size().height() / 2 - ( mZoomScale / previousZoom ) * ( event->pos().y() - size().height() / 2 - mSymbolOffset.y() ) );

  event->accept();

  update();
}

void QgsSymbolPreviewWidget::paintEvent( QPaintEvent * )
{
  if ( !mSymbol )
    return;

  QPainter painter( this );
  QgsMapSettings ms;
  ms.setMagnificationFactor( mZoomScale );

  QgsRenderContext rc( QgsRenderContext::fromMapSettings( ms ) );
  rc.setPainter( &painter );
  rc.setFlag( Qgis::RenderContextFlag::RenderSymbolPreview );
  rc.setFlag( Qgis::RenderContextFlag::Antialiasing );
  rc.setFlag( Qgis::RenderContextFlag::HighQualityImageTransforms );
  rc.setPainterFlagsUsingContext( &painter );
  rc.setIsGuiPreview( true );

  QTransform t;
  t.translate( size().width() / 2 + mSymbolOffset.x() + mTemporaryOffset.x(),
               size().height() / 2 + mSymbolOffset.y() + mTemporaryOffset.y() );
  t.scale( mZoomScale, mZoomScale );

  mSymbol->startRender( rc );

  switch ( mSymbol->type() )
  {
    case Qgis::SymbolType::Marker:
    {
      QgsMarkerSymbol *markerSymbol = qgis::down_cast< QgsMarkerSymbol * >( mSymbol.get() );
      markerSymbol->renderPoint( t.map( QPointF( 0, 0 ) ), nullptr, rc );

      break;
    }

    case Qgis::SymbolType::Line:
    {
      const QList< QList< QPolygonF > > points = mPatchShape.isNull() ?
          QgsStyle::defaultStyle()->defaultPatchAsQPolygonF( Qgis::SymbolType::Line, QSizeF( 100, 100 ) )
          : mPatchShape.toQPolygonF( Qgis::SymbolType::Line, QSizeF( 100, 100 ) );
      QgsLineSymbol *lineSymbol = qgis::down_cast< QgsLineSymbol * >( mSymbol.get() );
      for ( const QList< QPolygonF > &path : points )
        lineSymbol->renderPolyline( t.map( path.at( 0 ) ), nullptr, rc );

      break;
    }

    case Qgis::SymbolType::Fill:
    {
      const QList< QList< QPolygonF > > points = mPatchShape.isNull() ?
          QgsStyle::defaultStyle()->defaultPatchAsQPolygonF( Qgis::SymbolType::Fill, QSizeF( 100, 100 ) )
          : mPatchShape.toQPolygonF( Qgis::SymbolType::Fill, QSizeF( 100, 100 ) );
      QgsFillSymbol *fillSymbol = qgis::down_cast< QgsFillSymbol * >( mSymbol.get() );
      for ( const QList< QPolygonF > &poly : points )
      {
        QVector< QPolygonF > rings;
        rings.reserve( poly.size() - 1 );
        for ( int i = 1; i < poly.size(); ++i )
          rings << t.map( poly.at( i ) );
        fillSymbol->renderPolygon( t.map( poly.value( 0 ) ), &rings, nullptr, rc );
      }

      break;
    }
    case Qgis::SymbolType::Hybrid:
      break;
  }

  mSymbol->stopRender( rc );
  painter.end();
}
