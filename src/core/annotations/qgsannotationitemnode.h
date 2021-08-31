/***************************************************************************
    qgsannotationitemnode.h
    ----------------
    copyright            : (C) 2021 by Nyall Dawson
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

#ifndef QGSANNOTATIONITEMEDITNODE_H
#define QGSANNOTATIONITEMEDITNODE_H

#include "qgis_core.h"
#include "qgis_sip.h"
#include "qgspointxy.h"
#include "qgis.h"

/**
 * \ingroup core
 * \brief Contains information about a node used for editing an annotation item.
 * \since QGIS 3.22
 */
class CORE_EXPORT QgsAnnotationItemNode
{
  public:

    /**
     * Default constructor
     */
    QgsAnnotationItemNode() = default;

    /**
     * Constructor for QgsAnnotationItemNode, with the specified \a point and \a type.
     */
    QgsAnnotationItemNode( const QgsPointXY &point, Qgis::AnnotationItemNodeType type )
      : mPoint( point )
      , mType( type )
    {}

    /**
     * Returns the node's position, in geographic coordinates.
     *
     * The coordinates will match the annotation item's CRS.
     *
     * \see setPoint()
     */
    QgsPointXY point() const { return mPoint; }

    /**
     * Sets the node's position, in geographic coordinates.
     *
     * The coordinates will match the annotation item's CRS.
     *
     * \see point()
     */
    void setPoint( QgsPointXY point ) { mPoint = point; }

    /**
     * Returns the node type.
     *
     * \see setType()
     */
    Qgis::AnnotationItemNodeType type() const { return mType; }

    /**
     * Sets the node type.
     *
     * \see type()
     */
    void setType( Qgis::AnnotationItemNodeType type ) { mType = type; }

  private:

    QgsPointXY mPoint;
    Qgis::AnnotationItemNodeType mType = Qgis::AnnotationItemNodeType::VertexHandle;

};

#endif // QGSANNOTATIONITEMEDITNODE_H
