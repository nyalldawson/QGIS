/***************************************************************************
    qgsinputcontroller.h
    ---------------------
    begin                : March 2023
    copyright            : (C) 2023 by Nyall Dawson
    email                : nyall dot dawson at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGINPUTCONTROLLER_H
#define QGINPUTCONTROLLER_H

#include "qgis_gui.h"
#include "qgis_sip.h"
#include "qgis.h"

#include <QObject>
#include <QMap>

class QgsAbstract2DMapController;
class QgsAbstract3DMapController;

/**
 * \ingroup gui
 * \class QgsInputControllerManager
 * \brief Manages input control devices.
 *
 * QgsInputControllerManager is not usually directly created, but rather accessed through
 * QgsGui::inputControllerManager().
 *
 * \since QGIS 3.32
 */
class GUI_EXPORT QgsInputControllerManager : public QObject
{
    Q_OBJECT

  public:

    enum class InputControllerType : int
    {
      Map2D,
      Map3D
    };
    Q_ENUM( InputControllerType );

    /**
     * Constructor for QgsInputControllerManager, with the specified \a parent object.
     *
     * \note QgsInputControllerManager is not usually directly created, but rather accessed through
     * QgsGui::inputControllerManager().
     */
    QgsInputControllerManager( QObject *parent SIP_TRANSFERTHIS = nullptr );
    ~QgsInputControllerManager() override;

    /**
     * Returns a list of the device IDs of available 2D map controllers.
     *
     * \see create2DMapController()
     * \see register2DMapController()
     */
    QStringList available2DMapControllers() const;

    /**
     * Returns a new instance of the 2D map controller with the specified \a deviceId.
     *
     * The caller takes ownership of the returned object.
     *
     * Will return NULLPTR if no matching controller is found.
     *
     * \see available2DMapControllers()
     */
    QgsAbstract2DMapController *create2DMapController( const QString &deviceId ) const SIP_FACTORY;

    /**
     * Registers a new 2D map \a controller.
     *
     * Ownership of \a controller is transferred to the manager.
     *
     * Returns TRUE if the controller was successfully registered, or FALSE if it could
     * not be registered (e.g. if a controller with matching deviceId has already been registered).
     *
     * \see available2DMapControllers()
     */
    bool register2DMapController( QgsAbstract2DMapController *controller SIP_TRANSFER );

    /**
     * Returns a list of the device IDs of available 3D map controllers.
     *
     * \see create3DMapController()
     * \see register3DMapController()
     */
    QStringList available3DMapControllers() const;

    /**
     * Returns a new instance of the 3D map controller with the specified \a deviceId.
     *
     * The caller takes ownership of the returned object.
     *
     * Will return NULLPTR if no matching controller is found.
     *
     * \see available3DMapControllers()
     */
    QgsAbstract3DMapController *create3DMapController( const QString &deviceId ) const SIP_FACTORY;

    /**
     * Registers a new 3D map \a controller.
     *
     * Ownership of \a controller is transferred to the manager.
     *
     * Returns TRUE if the controller was successfully registered, or FALSE if it could
     * not be registered (e.g. if a controller with matching deviceId has already been registered).
     *
     * \see available3DMapControllers()
     */
    bool register3DMapController( QgsAbstract3DMapController *controller SIP_TRANSFER );

  private:

    QMap< QString, QgsAbstract2DMapController * > m2DMapControllers;
    QMap< QString, QgsAbstract3DMapController * > m3DMapControllers;

};

/**
 * \ingroup gui
 * \class QgsAbstractInputController
 * \brief Abstract base class for all input controllers.
 *
 * \since QGIS 3.32
 */
class GUI_EXPORT QgsAbstractInputController : public QObject
{
    Q_OBJECT

  public:

    /**
     * Constructor for QgsAbstractInputController, with the specified \a parent object.
     */
    QgsAbstractInputController( QObject *parent SIP_TRANSFERTHIS = nullptr );

    /**
     * Returns a new copy of the controller.
     */
    virtual QgsAbstractInputController *clone() const = 0 SIP_FACTORY;

    /**
     * Returns a string uniquely identifying the device.
     */
    virtual QString deviceId() const = 0;

    /**
     * Returns the input controller type.
     */
    virtual QgsInputControllerManager::InputControllerType type() const = 0;

};


/**
 * \ingroup gui
 * \class QgsAbstract2DMapController
 * \brief Abstract base class for all 2D map controllers.
 *
 * \since QGIS 3.32
 */
class GUI_EXPORT QgsAbstract2DMapController : public QgsAbstractInputController
{
    Q_OBJECT

#ifdef SIP_RUN
    SIP_CONVERT_TO_SUBCLASS_CODE
    if ( qobject_cast< QgsAbstract2DMapController * >( sipCpp ) )

      sipType = sipType_QgsAbstract2DMapController;
    else
      sipType = nullptr;
    SIP_END
#endif

  public:

    /**
     * Constructor for QgsAbstract2DMapController, with the specified \a parent object.
     */
    QgsAbstract2DMapController( QObject *parent SIP_TRANSFERTHIS = nullptr );

    QgsInputControllerManager::InputControllerType type() const override;

  signals:

    // a bunch of signals relating to navigating a 2D map, eg
    void zoomMap( double factor );
    // etc

};


/**
 * \ingroup gui
 * \class QgsAbstract3DMapController
 * \brief Abstract base class for all 3D map controllers.
 *
 * \since QGIS 3.32
 */
class GUI_EXPORT QgsAbstract3DMapController : public QgsAbstractInputController
{
    Q_OBJECT

#ifdef SIP_RUN
    SIP_CONVERT_TO_SUBCLASS_CODE
    if ( qobject_cast< QgsAbstract3DMapController * >( sipCpp ) )

      sipType = sipType_QgsAbstract3DMapController;
    else
      sipType = nullptr;
    SIP_END
#endif

  public:

    /**
     * Constructor for QgsAbstract3DMapController, with the specified \a parent object.
     */
    QgsAbstract3DMapController( QObject *parent SIP_TRANSFERTHIS = nullptr );

    QgsInputControllerManager::InputControllerType type() const override;

  signals:

    // a bunch of signals relating to navigating a 3D map, eg
    void changeCameraAngleByDelta( double delta );
    // etc
};


#endif // QGINPUTCONTROLLER_H
