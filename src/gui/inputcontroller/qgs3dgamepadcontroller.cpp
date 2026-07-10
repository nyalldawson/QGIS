/***************************************************************************
    qgs3dgamepadcontroller.cpp
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

#include "qgs3dgamepadcontroller.h"

#include "qgslogger.h"

#include <QString>

using namespace Qt::StringLiterals;

#ifdef HAVE_QTGAMEPAD
#include "moc_qgs3dgamepadcontroller.cpp"

#include <QtGamepadLegacy/QGamepad>
#include <QTimer>

QgsGamepad3DMapController::QgsGamepad3DMapController( int gamepadDeviceId, QObject *parent )
  : QgsAbstract3DMapController( parent )
  , mGamepadDeviceId( gamepadDeviceId )
{
  mGamepad = new QGamepad( gamepadDeviceId, this );

  // proxy raw signals for interested PyQGIS
  connect( mGamepad, &QGamepad::connectedChanged, this, &QgsGamepad3DMapController::connectedChanged );
  connect( mGamepad, &QGamepad::axisLeftXChanged, this, &QgsGamepad3DMapController::axisLeftXChanged );
  connect( mGamepad, &QGamepad::axisLeftYChanged, this, &QgsGamepad3DMapController::axisLeftYChanged );
  connect( mGamepad, &QGamepad::axisRightXChanged, this, &QgsGamepad3DMapController::axisRightXChanged );
  connect( mGamepad, &QGamepad::axisRightYChanged, this, &QgsGamepad3DMapController::axisRightYChanged );
  connect( mGamepad, &QGamepad::buttonAChanged, this, &QgsGamepad3DMapController::buttonAChanged );
  connect( mGamepad, &QGamepad::buttonBChanged, this, &QgsGamepad3DMapController::buttonBChanged );
  connect( mGamepad, &QGamepad::buttonXChanged, this, &QgsGamepad3DMapController::buttonXChanged );
  connect( mGamepad, &QGamepad::buttonYChanged, this, &QgsGamepad3DMapController::buttonYChanged );
  connect( mGamepad, &QGamepad::buttonL1Changed, this, &QgsGamepad3DMapController::buttonL1Changed );
  connect( mGamepad, &QGamepad::buttonR1Changed, this, &QgsGamepad3DMapController::buttonR1Changed );
  connect( mGamepad, &QGamepad::buttonL2Changed, this, &QgsGamepad3DMapController::buttonL2Changed );
  connect( mGamepad, &QGamepad::buttonR2Changed, this, &QgsGamepad3DMapController::buttonR2Changed );
  connect( mGamepad, &QGamepad::buttonSelectChanged, this, &QgsGamepad3DMapController::buttonSelectChanged );
  connect( mGamepad, &QGamepad::buttonStartChanged, this, &QgsGamepad3DMapController::buttonStartChanged );
  connect( mGamepad, &QGamepad::buttonL3Changed, this, &QgsGamepad3DMapController::buttonL3Changed );
  connect( mGamepad, &QGamepad::buttonR3Changed, this, &QgsGamepad3DMapController::buttonR3Changed );
  connect( mGamepad, &QGamepad::buttonUpChanged, this, &QgsGamepad3DMapController::buttonUpChanged );
  connect( mGamepad, &QGamepad::buttonDownChanged, this, &QgsGamepad3DMapController::buttonDownChanged );
  connect( mGamepad, &QGamepad::buttonLeftChanged, this, &QgsGamepad3DMapController::buttonLeftChanged );
  connect( mGamepad, &QGamepad::buttonRightChanged, this, &QgsGamepad3DMapController::buttonRightChanged );
  connect( mGamepad, &QGamepad::buttonCenterChanged, this, &QgsGamepad3DMapController::buttonCenterChanged );
  connect( mGamepad, &QGamepad::buttonGuideChanged, this, &QgsGamepad3DMapController::buttonGuideChanged );

  // setup high level connections for 3d map navigation
  mTimer = new QTimer( this );
  connect( mTimer, &QTimer::timeout, this, &QgsGamepad3DMapController::navigationTimeout );

  connect( mGamepad.data(), &QGamepad::connectedChanged, this, [this]( bool connected ) {
    if ( !connected )
      mTimer->stop();
  } );

  connect( mGamepad.data(), &QGamepad::axisLeftXChanged, this, &QgsGamepad3DMapController::updateNavigation );
  connect( mGamepad.data(), &QGamepad::axisLeftYChanged, this, &QgsGamepad3DMapController::updateNavigation );
  connect( mGamepad.data(), &QGamepad::axisRightXChanged, this, &QgsGamepad3DMapController::updateNavigation );
  connect( mGamepad.data(), &QGamepad::axisRightYChanged, this, &QgsGamepad3DMapController::updateNavigation );
  connect( mGamepad.data(), &QGamepad::buttonL2Changed, this, &QgsGamepad3DMapController::updateNavigation );
  connect( mGamepad.data(), &QGamepad::buttonR2Changed, this, &QgsGamepad3DMapController::updateNavigation );
}

QgsGamepad3DMapController *QgsGamepad3DMapController::clone() const
{
  return new QgsGamepad3DMapController( mGamepadDeviceId );
}

QString QgsGamepad3DMapController::deviceId() const
{
  return u"gamepad3d:%1"_s.arg( mGamepadDeviceId );
}

bool QgsGamepad3DMapController::isConnected() const
{
  return mGamepad->isConnected();
}

void QgsGamepad3DMapController::updateNavigation()
{
  if ( mTimer->isActive() )
    return;

  if ( axisMax() > 0.10 )
  {
    mTimer->start( 16 );
    mElapsedTimer.restart();
    navigationTimeout();
  }
}

void QgsGamepad3DMapController::navigationTimeout()
{
  const qint64 elapsed = mElapsedTimer.elapsed();
  mElapsedTimer.restart();

  const double dt = static_cast< double >( elapsed ) / 1000.0; // Seconds

  constexpr double EPSILON = 0.001;
  const bool inputActive = hasInput();
  const bool velocityActive = ( std::fabs( mCurrentMoveX ) > EPSILON )
                              || ( std::fabs( mCurrentMoveY ) > EPSILON )
                              || ( std::fabs( mCurrentMoveZ ) > EPSILON )
                              || ( std::fabs( mCurrentPitch ) > EPSILON )
                              || ( std::fabs( mCurrentYaw ) > EPSILON );
  if ( !inputActive && !velocityActive )
  {
    mCurrentMoveX = 0.0;
    mCurrentMoveY = 0.0;
    mCurrentMoveZ = 0.0;
    mCurrentPitch = 0.0;
    mCurrentYaw = 0.0;
    mTimePushedToEdge = 0.0;
    mTimer->stop();
    return;
  }


  const double scale = std::min( 4.0, static_cast< double >( elapsed ) / 16 );

  constexpr double maxMovementSpeed = 3.0;
  constexpr double maxStrafeSpeed = 1.5;
  constexpr double expMovement = 3.0;

  constexpr double maxPitchYaw = 1.5;
  constexpr double expPitchYaw = 2;

  constexpr double SMOOTHING_FACTOR = 10.0; // Lower values glide further; higher values stop faster
  const double blend = 1.0 - std::exp( -SMOOTHING_FACTOR * dt );

  auto scaleExp = []( double value, double domainMin, double domainMax, double rangeMin, double rangeMax, double exponent ) -> double {
    return ( ( rangeMax - rangeMin ) / pow( domainMax - domainMin, exponent ) ) * pow( value - domainMin, exponent ) + rangeMin;
  };

  double targetMoveX = 0;
  double targetMoveY = 0;
  double targetMoveZ = 0;

  double processedLeftX = 0.0;
  double processedLeftY = 0.0;
  applyRadialDeadzone( mGamepad->axisLeftX(), mGamepad->axisLeftY(), processedLeftX, processedLeftY );

  if ( std::fabs( processedLeftY ) > 0.0 )
  {
    targetMoveX = scaleExp( std::fabs( processedLeftY ), 0, 1, 0, maxStrafeSpeed, expMovement ) * ( processedLeftY > 0 ? -1 : 1 );
  }
  if ( std::fabs( processedLeftX ) > 0.0 )
  {
    targetMoveY = scaleExp( std::fabs( processedLeftX ), 0, 1, 0, maxMovementSpeed, expMovement ) * ( processedLeftX > 0 ? -1 : 1 );
  }

  constexpr double TRIGGER_DEADZONE = 0.15;
  double rawL2 = mGamepad->buttonL2();
  double rawR2 = mGamepad->buttonR2();
  double processedL2 = rawL2 > TRIGGER_DEADZONE ? ( rawL2 - TRIGGER_DEADZONE ) / ( 1.0 - TRIGGER_DEADZONE ) : 0.0;
  double processedR2 = rawR2 > TRIGGER_DEADZONE ? ( rawR2 - TRIGGER_DEADZONE ) / ( 1.0 - TRIGGER_DEADZONE ) : 0.0;

  if ( processedL2 > 0.0 || processedR2 > 0.0 )
  {
    targetMoveZ = scaleExp( processedL2, 0, 1, 0, maxMovementSpeed, expMovement ) * -1 + scaleExp( processedR2, 0, 1, 0, maxMovementSpeed, expMovement );
  }

  mCurrentMoveX += ( targetMoveX - mCurrentMoveX ) * blend;
  mCurrentMoveY += ( targetMoveY - mCurrentMoveY ) * blend;
  mCurrentMoveZ += ( targetMoveZ - mCurrentMoveZ ) * blend;

  if ( !qgsDoubleNear( mCurrentMoveX * scale, 0.0 ) || !qgsDoubleNear( mCurrentMoveY * scale, 0.0 ) || !qgsDoubleNear( mCurrentMoveZ * scale, 0.0 ) )
  {
    emit walkView( scale * mCurrentMoveX, scale * mCurrentMoveY, scale * mCurrentMoveZ );
  }

  double processedRightX = 0.0;
  double processedRightY = 0.0;
  applyRadialDeadzone( mGamepad->axisRightX(), mGamepad->axisRightY(), processedRightX, processedRightY );

  // Look Acceleration Accumulator Logic
  double rightStickMag = std::sqrt( mGamepad->axisRightX() * mGamepad->axisRightX() + mGamepad->axisRightY() * mGamepad->axisRightY() );
  if ( rightStickMag > 0.95 )
  {
    mTimePushedToEdge += dt;
  }
  else
  {
    mTimePushedToEdge = 0.0;
  }
  // Up to an extra 1.5x speed multiplier over 0.4 seconds of continuous perimeter pinning
  double accelerationMultiplier = 1.0 + std::min( 0.5, mTimePushedToEdge / 0.8 );

  double targetPitch = 0.0;
  double targetYaw = 0.0;

  if ( std::fabs( processedRightY ) > 0.0 )
  {
    targetPitch = scaleExp( std::fabs( processedRightY ), 0, 1, 0, maxPitchYaw, expPitchYaw ) * ( processedRightY > 0 ? -1 : 1 ) * accelerationMultiplier;
  }
  if ( std::fabs( processedRightX ) > 0.0 )
  {
    targetYaw = scaleExp( std::fabs( processedRightX ), 0, 1, 0, maxPitchYaw, expPitchYaw ) * ( processedRightX > 0 ? -1 : 1 ) * accelerationMultiplier;
  }

  // Low-Pass Inertial Filter: Slide towards the rotation destination
  mCurrentPitch += ( targetPitch - mCurrentPitch ) * blend;
  mCurrentYaw += ( targetYaw - mCurrentYaw ) * blend;

  if ( !qgsDoubleNear( scale * mCurrentPitch, 0 ) || !qgsDoubleNear( scale * mCurrentYaw, 0 ) )
  {
    emit rotateCamera( scale * mCurrentPitch, scale * mCurrentYaw );
  }
}

bool QgsGamepad3DMapController::hasInput() const
{
  constexpr double DEADZONE = 0.15;
  double leftMag = std::sqrt( mGamepad->axisLeftX() * mGamepad->axisLeftX() + mGamepad->axisLeftY() * mGamepad->axisLeftY() );
  double rightMag = std::sqrt( mGamepad->axisRightX() * mGamepad->axisRightX() + mGamepad->axisRightY() * mGamepad->axisRightY() );

  return ( leftMag > DEADZONE ) || ( rightMag > DEADZONE ) || ( mGamepad->buttonL2() > DEADZONE ) || ( mGamepad->buttonR2() > DEADZONE );
}

void QgsGamepad3DMapController::applyRadialDeadzone( double rawX, double rawY, double &outX, double &outY ) const
{
  constexpr double DEADZONE = 0.15;
  double magnitude = std::sqrt( rawX * rawX + rawY * rawY );

  if ( magnitude < DEADZONE )
  {
    outX = 0.0;
    outY = 0.0;
  }
  else
  {
    double dirX = rawX / magnitude;
    double dirY = rawY / magnitude;

    // Rescale vector from zero right at the boundary up to full deflection
    double scaledMagnitude = ( magnitude - DEADZONE ) / ( 1.0 - DEADZONE );
    scaledMagnitude = std::min( 1.0, scaledMagnitude );

    outX = dirX * scaledMagnitude;
    outY = dirY * scaledMagnitude;
  }
}

double QgsGamepad3DMapController::axisMax()
{
  return std::max(
    std::fabs( mGamepad->axisLeftX() ),
    std::max(
      std::fabs( mGamepad->axisLeftY() ),
      std::max( std::fabs( mGamepad->axisRightX() ), std::max( std::fabs( mGamepad->axisRightY() ), std::max( std::fabs( mGamepad->buttonL2() ), std::fabs( mGamepad->buttonR2() ) ) ) )
    )
  );
}

QString QgsGamepad3DMapController::name() const
{
  return mGamepad->name();
}

double QgsGamepad3DMapController::axisLeftX() const
{
  return mGamepad->axisLeftX();
}

double QgsGamepad3DMapController::axisLeftY() const
{
  return mGamepad->axisLeftY();
}

double QgsGamepad3DMapController::axisRightX() const
{
  return mGamepad->axisRightX();
}

double QgsGamepad3DMapController::axisRightY() const
{
  return mGamepad->axisRightY();
}

bool QgsGamepad3DMapController::buttonA() const
{
  return mGamepad->buttonA();
}

bool QgsGamepad3DMapController::buttonB() const
{
  return mGamepad->buttonB();
}

bool QgsGamepad3DMapController::buttonX() const
{
  return mGamepad->buttonX();
}

bool QgsGamepad3DMapController::buttonY() const
{
  return mGamepad->buttonY();
}

bool QgsGamepad3DMapController::buttonL1() const
{
  return mGamepad->buttonL1();
}

bool QgsGamepad3DMapController::buttonR1() const
{
  return mGamepad->buttonR1();
}

double QgsGamepad3DMapController::buttonL2() const
{
  return mGamepad->buttonL2();
}

double QgsGamepad3DMapController::buttonR2() const
{
  return mGamepad->buttonR2();
}

bool QgsGamepad3DMapController::buttonSelect() const
{
  return mGamepad->buttonSelect();
}

bool QgsGamepad3DMapController::buttonStart() const
{
  return mGamepad->buttonStart();
}

bool QgsGamepad3DMapController::buttonL3() const
{
  return mGamepad->buttonL3();
}

bool QgsGamepad3DMapController::buttonR3() const
{
  return mGamepad->buttonR3();
}

bool QgsGamepad3DMapController::buttonUp() const
{
  return mGamepad->buttonUp();
}

bool QgsGamepad3DMapController::buttonDown() const
{
  return mGamepad->buttonDown();
}

bool QgsGamepad3DMapController::buttonLeft() const
{
  return mGamepad->buttonLeft();
}

bool QgsGamepad3DMapController::buttonRight() const
{
  return mGamepad->buttonRight();
}

bool QgsGamepad3DMapController::buttonCenter() const
{
  return mGamepad->buttonCenter();
}

bool QgsGamepad3DMapController::buttonGuide() const
{
  return mGamepad->buttonGuide();
}

#endif
