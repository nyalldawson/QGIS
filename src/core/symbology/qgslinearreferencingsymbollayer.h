/***************************************************************************
    qgslinearreferencingsymbollayer.h
    ---------------------
    begin                : August 2024
    copyright            : (C) 2024 by Nyall Dawson
    email                : nyall dot dawson at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef QGSLINEARREFERENCINGSYMBOLLAYER_H
#define QGSLINEARREFERENCINGSYMBOLLAYER_H

#include "qgis_core.h"
#include "qgis.h"
#include "qgssymbollayer.h"
#include "qgstextformat.h"


/**
 * \ingroup core
 * \brief Line symbol layer used for decorating accordingly to linear referencing.
 *
 * This symbol layer type allows placing text labels at regular intervals along
 * a line (or at positions corresponding to existing vertices). Positions
 * can be calculated using Cartesian distances, or interpolated from z or m values.
 *
 * \since QGIS 3.40
 */
class CORE_EXPORT QgsLinearReferencingSymbolLayer : public QgsLineSymbolLayer
{
  public:
    QgsLinearReferencingSymbolLayer();
    ~QgsLinearReferencingSymbolLayer() override;

    /**
     * Creates a new QgsLinearReferencingSymbolLayer, using the specified \a properties.
     *
     * The caller takes ownership of the returned object.
     */
    static QgsSymbolLayer *create( const QVariantMap &properties = QVariantMap() ) SIP_FACTORY;

    QgsLinearReferencingSymbolLayer *clone() const override SIP_FACTORY;
    QVariantMap properties() const override;
    QString layerType() const override;
    Qgis::SymbolLayerFlags flags() const override;
    QgsSymbol *subSymbol() override;
    bool setSubSymbol( QgsSymbol *symbol SIP_TRANSFER ) override;
    void startRender( QgsSymbolRenderContext &context ) override;
    void stopRender( QgsSymbolRenderContext &context ) override;
    void renderPolyline( const QPolygonF &points, QgsSymbolRenderContext &context ) override;

    /**
     * Returns the text format used to render the layer.
     *
     * \see setTextFormat()
     */
    QgsTextFormat textFormat() const;

    /**
     * Sets the text \a format used to render the layer.
     *
     * \see textFormat()
     */
    void setTextFormat( const QgsTextFormat &format );

  private:

    QgsTextFormat mTextFormat;
    std::unique_ptr<QgsMarkerSymbol> mMarkerSymbol;


};

#endif // QGSLINEARREFERENCINGSYMBOLLAYER_H
