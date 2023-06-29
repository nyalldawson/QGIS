/***************************************************************************
                         qgsalgorithmpdaltranslatexyz.cpp
                         ---------------------
    begin                : February 2023
    copyright            : (C) 2023 by Alexander Bruy
    email                : alexander dot bruy at gmail dot com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsalgorithmpdaltranslatexyz.h"

#include "qgsrunprocess.h"
#include "qgspointcloudlayer.h"

///@cond PRIVATE

QString QgsPdalTranslateXyzAlgorithm::name() const
{
  return QStringLiteral( "translatexyz" );
}

QString QgsPdalTranslateXyzAlgorithm::displayName() const
{
  return QObject::tr( "Translate" );
}

QString QgsPdalTranslateXyzAlgorithm::group() const
{
  return QObject::tr( "Point cloud data management" );
}

QString QgsPdalTranslateXyzAlgorithm::groupId() const
{
  return QStringLiteral( "pointclouddatamanagement" );
}

QStringList QgsPdalTranslateXyzAlgorithm::tags() const
{
  return QObject::tr( "pdal,lidar,translate,shift,move" ).split( ',' );
}

QString QgsPdalTranslateXyzAlgorithm::shortHelpString() const
{
  return QObject::tr( "This algorithm translates a point cloud by specified distances in the X, Y and Z axis." );
}

QgsPdalTranslateXyzAlgorithm *QgsPdalTranslateXyzAlgorithm::createInstance() const
{
  return new QgsPdalTranslateXyzAlgorithm();
}

void QgsPdalTranslateXyzAlgorithm::initAlgorithm( const QVariantMap & )
{
  addParameter( new QgsProcessingParameterPointCloudLayer( QStringLiteral( "INPUT" ), QObject::tr( "Input layer" ) ) );
  addParameter( new QgsProcessingParameterNumber( QStringLiteral( "X_TRANSLATE" ), QObject::tr( "X translation" ), QgsProcessingParameterNumber::Double, 0 ) );
  addParameter( new QgsProcessingParameterNumber( QStringLiteral( "Y_TRANSLATE" ), QObject::tr( "Y translation" ), QgsProcessingParameterNumber::Double, 0 ) );
  addParameter( new QgsProcessingParameterNumber( QStringLiteral( "Z_TRANSLATE" ), QObject::tr( "Z translation" ), QgsProcessingParameterNumber::Double, 0 ) );

  addParameter( new QgsProcessingParameterPointCloudDestination( QStringLiteral( "OUTPUT" ), QObject::tr( "Translated" ) ) );
}

QStringList QgsPdalTranslateXyzAlgorithm::createArgumentLists( const QVariantMap &parameters, QgsProcessingContext &context, QgsProcessingFeedback *feedback )
{
  Q_UNUSED( feedback );

  QgsPointCloudLayer *layer = parameterAsPointCloudLayer( parameters, QStringLiteral( "INPUT" ), context, QgsProcessing::LayerOptionsFlag::SkipIndexGeneration );
  if ( !layer )
    throw QgsProcessingException( invalidPointCloudError( parameters, QStringLiteral( "INPUT" ) ) );

  const QString outputFile = parameterAsOutputLayer( parameters, QStringLiteral( "OUTPUT" ), context );
  setOutputValue( QStringLiteral( "OUTPUT" ), outputFile );

  const double translateX = parameterAsDouble( parameters, QStringLiteral( "X_TRANSLATE" ), context );
  const double translateY = parameterAsDouble( parameters, QStringLiteral( "Y_TRANSLATE" ), context );
  const double translateZ = parameterAsDouble( parameters, QStringLiteral( "Z_TRANSLATE" ), context );
  const QString matrix = QStringLiteral("1  0  0  %1  0  1  0  %2  0  0  1  %3  0  0  0  1").arg( qgsDoubleToString( translateX ),
                                                                                                 qgsDoubleToString( translateY ),
                                                                                                 qgsDoubleToString(translateZ ) );

  QStringList args = { QStringLiteral( "transformation" ),
                       QStringLiteral( "--input=%1" ).arg( layer->source() ),
                       QStringLiteral( "--output=%1" ).arg( outputFile ),
                       QStringLiteral( "--matrix=%1" ).arg( matrix )
                     };

  applyThreadsParameter( args, context );
  return args;
}

///@endcond
