/***************************************************************************
                              qgslineclusterrenderer.h
                              -------------------------
  begin                : November 2023
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

#ifndef QGSLINECLUSTERRENDERER_H
#define QGSLINECLUSTERRENDERER_H

#include "qgis_core.h"
#include "qgis.h"
#include "qgslinedistancerenderer.h"

class QgsLineSymbol;

/**
 * \class QgsLineClusterRenderer
 * \ingroup core
 * \brief A renderer that automatically clusters line segments with the same geographic position.
 * \since QGIS 3.36
*/
class CORE_EXPORT QgsLineClusterRenderer: public QgsLineDistanceRenderer
{
  public:

    QgsLineClusterRenderer();

    QgsLineClusterRenderer *clone() const override SIP_FACTORY;
    void startRender( QgsRenderContext &context, const QgsFields &fields ) override;
    void stopRender( QgsRenderContext &context ) override;
    QDomElement save( QDomDocument &doc, const QgsReadWriteContext &context ) override;
    QSet<QString> usedAttributes( const QgsRenderContext &context ) const override;
    bool accept( QgsStyleEntityVisitorInterface *visitor ) const override;

    //! Creates a renderer from XML element
    static QgsFeatureRenderer *create( QDomElement &symbologyElem, const QgsReadWriteContext &context ) SIP_FACTORY;

    /**
     * Returns the symbol used for rendering clustered line segments (but not ownership of the symbol).
     * \see setClusterSymbol()
    */
    QgsLineSymbol *clusterSymbol();

    /**
     * Sets the symbol for rendering clustered line segments.
     * \param symbol new cluster symbol. Ownership is transferred to the renderer.
     * \see clusterSymbol()
    */
    void setClusterSymbol( QgsLineSymbol *symbol SIP_TRANSFER );

    /**
     * Creates a QgsLineClusterRenderer from an existing renderer.
     * \returns a new renderer if the conversion was possible, otherwise NULLPTR.
     */
    static QgsLineClusterRenderer *convertFromRenderer( const QgsFeatureRenderer *renderer ) SIP_FACTORY;

  private:
#ifdef SIP_RUN
    QgsLineClusterRenderer( const QgsLineClusterRenderer & );
    QgsLineClusterRenderer &operator=( const QgsLineClusterRenderer & );
#endif

    //! Symbol for line segment clusters
    std::unique_ptr< QgsLineSymbol > mClusterSymbol;

    void drawGroup( QPointF centerPoint, QgsRenderContext &context, const QgsLineDistanceRenderer::ClusteredGroup &group ) const override SIP_FORCE;

};

#endif // QGSLINECLUSTERRENDERER_H
