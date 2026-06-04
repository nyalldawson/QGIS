/***************************************************************************
    qgsnormaldebugmaterial.h
    ---------------------
  Date                 : June 2026
  Copyright            : (C) 2026 by Nyall Dawson
  Email                : nyall dot dawson at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSNORMALDEBUGMATERIAL_H
#define QGSNORMALDEBUGMATERIAL_H

#include "qgis.h"
#include "qgis_3d.h"
#include "qgsmaterial.h"

#define SIP_NO_FILE

///@cond PRIVATE

/**
 * \ingroup qgis_3d
 * \brief A single color material for showing geometry normals.
 *
 * \since QGIS 4.2
 */
class _3D_EXPORT QgsNormalDebugMaterial : public QgsMaterial
{
    Q_OBJECT
  public:
    /**
   * Constructor for QgsNormalDebugMaterial, with the specified \a parent node.
   */
    QgsNormalDebugMaterial( Qt3DCore::QNode *parent = nullptr );

    void setNormalLength( float length );

    void setNormalColor( const QColor &color );

  private:
    Qt3DRender::QParameter *mNormalLength = nullptr;
    Qt3DRender::QParameter *mColor = nullptr;
};

///@endcond PRIVATE

#endif // QGSNORMALDEBUGMATERIAL_H
