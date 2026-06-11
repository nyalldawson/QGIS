/***************************************************************************
  qgspolygon3dsymbol_p.h
  --------------------------------------
  Date                 : July 2017
  Copyright            : (C) 2017 by Martin Dobias
  Email                : wonder dot sk at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSPOLYGON3DSYMBOL_P_H
#define QGSPOLYGON3DSYMBOL_P_H

/// @cond PRIVATE

//
//  W A R N I N G
//  -------------
//
// This file is not part of the QGIS API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//


class QgsAbstract3DSymbol;
class QgsFeature3DHandler;
class QgsPolygon3DSymbol;
class QgsVectorLayer;

namespace Qgs3DSymbolImpl
{
  //! factory method for QgsPolygon3DSymbol
  QgsFeature3DHandler *handlerForPolygon3DSymbol( const QgsVectorLayer *layer, const QgsAbstract3DSymbol *symbol );
} // namespace Qgs3DSymbolImpl

#include "qgs3dmapsceneentity.h"
#include "qgsbox3d.h"

#include <Qt3DCore/QEntity>

#define SIP_NO_FILE

class QgsVector3D;

class QgsVectorLayerLodEntity : public Qt3DCore::QEntity
{
    Q_OBJECT

  public:
    explicit QgsVectorLayerLodEntity( Qt3DCore::QNode *parent = nullptr )
      : Qt3DCore::QEntity( parent )
    {}

    void setBox3D( const QgsBox3D &box ) { mBox3D = box; }

    void handleSceneUpdate( const Qgs3DMapSceneEntity::SceneContext &sceneContext, const QgsVector3D &mapOrigin );

  private:
    QgsBox3D mBox3D;
};


/// @endcond

#endif // QGSPOLYGON3DSYMBOL_P_H
