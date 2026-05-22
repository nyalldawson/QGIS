/***************************************************************************
  qgsmetalroughmaterial.h
  --------------------------------------
  Date                 : December 2023
  Copyright            : (C) 2023 by Nyall Dawson
  Email                : nyall dot dawson at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSMETALROUGHMATERIAL_H
#define QGSMETALROUGHMATERIAL_H

#include "qgis_3d.h"
#include "qgspbrmaterial.h"

#include <QObject>

#define SIP_NO_FILE

// adapted from Qt's qmetalroughmaterial.h
namespace Qt3DRender
{

  class QFilterKey;
  class QEffect;
  class QAbstractTexture;
  class QTechnique;
  class QParameter;
  class QShaderProgram;
  class QShaderProgramBuilder;
  class QRenderPass;

} // namespace Qt3DRender

///@cond PRIVATE

/**
 * \ingroup qgis_3d
 * \brief A PBR metal rough material.
 * \since QGIS 3.36
 */
class _3D_EXPORT QgsMetalRoughMaterial : public QgsPBRMaterial
{
    Q_OBJECT
  public:
    /**
     * Constructor for QgsMetalRoughMaterial, with the specified \a parent node.
     */
    explicit QgsMetalRoughMaterial( Qt3DCore::QNode *parent = nullptr );
    ~QgsMetalRoughMaterial() override;

  public slots:

    //! Set constant metalness value (between 0 - 1.0)
    void setMetalness( float metalness );

    //! Sets the metalness texture. Takes ownership
    void setMetalnessTexture( Qt3DRender::QAbstractTexture *metalness );

  protected:
    QStringList fragmentShaderDefines() const final;

  private:
    void init();

    Qt3DRender::QParameter *mMetalnessParameter = nullptr;
    Qt3DRender::QParameter *mMetalnessMapParameter = nullptr;

    bool mUsingMetalnessMap = false;

    friend class TestQgsGltf3DUtils;
};

///@endcond PRIVATE

#endif // QGSMETALROUGHMATERIAL_H
