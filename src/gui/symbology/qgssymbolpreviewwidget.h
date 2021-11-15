/***************************************************************************
    qgssymbolpreviewwidget.h
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
#ifndef QGSSYMBOLPREVIEWWIDGET_H
#define QGSSYMBOLPREVIEWWIDGET_H

#include <QWidget>
#include "qgis_sip.h"
#include "qgis_gui.h"
#include <memory>

class QgsSymbol;

/**
 * \ingroup gui
 * \class QgsSymbolPreviewWidget
 * \brief An widget for showing an interactive preview of a QgsSymbol
 * \since QGIS 3.24
 */
class GUI_EXPORT QgsSymbolPreviewWidget : public QWidget
{
    Q_OBJECT

  public:

    //! Constructor for QgsSymbolPreviewWidget
    QgsSymbolPreviewWidget( QWidget *parent SIP_TRANSFERTHIS = nullptr );
    ~QgsSymbolPreviewWidget() override;

    /**
     * Sets the symbol to show in the widget.
     *
     * Ownership is transferred to the widget.
     */
    void setSymbol( QgsSymbol *symbol SIP_TRANSFER );

    /**
     * Returns the symbol shown in the widget.
     */
    QgsSymbol *symbol() const;

    void mousePressEvent( QMouseEvent *event ) override;
    void mouseReleaseEvent( QMouseEvent *event ) override;
    void mouseMoveEvent( QMouseEvent *event ) override;
    void wheelEvent( QWheelEvent *event ) override;
    void paintEvent( QPaintEvent *event ) override;

  private:

    std::unique_ptr< QgsSymbol > mSymbol;
    double mZoomScale = 1;
    QPointF mSymbolOffset;
    Qt::MouseButton mMoveButton = Qt::LeftButton;
    bool mIsMoving = false;
    QPointF mMouseStartMove;
    QPointF mTemporaryOffset;

};


#endif // QGSSYMBOLPREVIEWWIDGET_H
