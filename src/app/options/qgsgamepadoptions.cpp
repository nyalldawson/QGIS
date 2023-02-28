/***************************************************************************
    qgsgamepadoptions.cpp
    -------------------------
    begin                : February 2023
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

#include "qgsgamepadoptions.h"
#include "qgssettings.h"
#include "qgsapplication.h"
#include "qgssettingsregistrycore.h"
#include <QtGamepad/QGamepadManager>
#include <QtGamepad/QGamepad>
#include <QMessageBox>

//
// QgsGamepadOptionsWidget
//

QgsGamepadOptionsWidget::QgsGamepadOptionsWidget( QWidget *parent )
  : QgsOptionsPageWidget( parent )
{
  setupUi( this );

  QGamepadManager *manager = QGamepadManager::instance();
  for ( int deviceId : manager->connectedGamepads() )
  {
    QString displayString;
    if ( manager->isGamepadConnected( deviceId ) )
    {
      displayString = tr( "%1 (Connected)" ).arg( manager->gamepadName( deviceId ) );
    }
    else
    {
      displayString = tr( "%1 (Disconnected)" ).arg( manager->gamepadName( deviceId ) );
    }
    mDeviceCombo->addItem( displayString, deviceId );
  }
  mDeviceCombo->setCurrentIndex( mDeviceCombo->findData( mDeviceId ) );
  if ( mDeviceCombo->currentIndex() < 0 )
    mDeviceCombo->setCurrentIndex( 0 );

  connect( mDeviceCombo, qOverload<int>( &QComboBox::currentIndexChanged ), this, &QgsGamepadOptionsWidget::deviceChanged );

  deviceChanged();
}

void QgsGamepadOptionsWidget::apply()
{

}

void QgsGamepadOptionsWidget::deviceChanged()
{
  if ( mDeviceCombo->currentIndex() < 0 )
    return;

  mDeviceId = mDeviceCombo->currentData().toInt();

  if ( mGamepad )
    delete mGamepad;

  mGamepad = new QGamepad( mDeviceId, this );

  connect( mGamepad, &QGamepad::axisLeftXChanged, this, &QgsGamepadOptionsWidget::updateValues );
  connect( mGamepad, &QGamepad::axisLeftYChanged, this, &QgsGamepadOptionsWidget::updateValues );
  connect( mGamepad, &QGamepad::axisRightXChanged, this, &QgsGamepadOptionsWidget::updateValues );
  connect( mGamepad, &QGamepad::axisRightYChanged, this, &QgsGamepadOptionsWidget::updateValues );

  connect( mGamepad, &QGamepad::buttonAChanged, this, &QgsGamepadOptionsWidget::updateValues );
  connect( mGamepad, &QGamepad::buttonBChanged, this, &QgsGamepadOptionsWidget::updateValues );
  connect( mGamepad, &QGamepad::buttonXChanged, this, &QgsGamepadOptionsWidget::updateValues );
  connect( mGamepad, &QGamepad::buttonYChanged, this, &QgsGamepadOptionsWidget::updateValues );

  connect( mGamepad, &QGamepad::buttonCenterChanged, this, &QgsGamepadOptionsWidget::updateValues );
  connect( mGamepad, &QGamepad::buttonGuideChanged, this, &QgsGamepadOptionsWidget::updateValues );
  connect( mGamepad, &QGamepad::buttonStartChanged, this, &QgsGamepadOptionsWidget::updateValues );
  connect( mGamepad, &QGamepad::buttonSelectChanged, this, &QgsGamepadOptionsWidget::updateValues );

  connect( mGamepad, &QGamepad::buttonL1Changed, this, &QgsGamepadOptionsWidget::updateValues );
  connect( mGamepad, &QGamepad::buttonL2Changed, this, &QgsGamepadOptionsWidget::updateValues );
  connect( mGamepad, &QGamepad::buttonL3Changed, this, &QgsGamepadOptionsWidget::updateValues );

  connect( mGamepad, &QGamepad::buttonR1Changed, this, &QgsGamepadOptionsWidget::updateValues );
  connect( mGamepad, &QGamepad::buttonR2Changed, this, &QgsGamepadOptionsWidget::updateValues );
  connect( mGamepad, &QGamepad::buttonR3Changed, this, &QgsGamepadOptionsWidget::updateValues );

  connect( mGamepad, &QGamepad::buttonLeftChanged, this, &QgsGamepadOptionsWidget::updateValues );
  connect( mGamepad, &QGamepad::buttonRightChanged, this, &QgsGamepadOptionsWidget::updateValues );
  connect( mGamepad, &QGamepad::buttonUpChanged, this, &QgsGamepadOptionsWidget::updateValues );
  connect( mGamepad, &QGamepad::buttonDownChanged, this, &QgsGamepadOptionsWidget::updateValues );

}

void QgsGamepadOptionsWidget::updateValues()
{
  if ( !mGamepad )
    return;

  mLeftAxisLabel->setText( QStringLiteral( "%1, %2" ).arg( mGamepad->axisLeftX() ).arg( mGamepad->axisLeftY() ) );
  mRightAxisLabel->setText( QStringLiteral( "%1, %2" ).arg( mGamepad->axisRightX() ).arg( mGamepad->axisRightY() ) );

  mButtonALabel->setText( mGamepad->buttonA() ? tr( "Pressed" ) : QString() );
  mButtonBLabel->setText( mGamepad->buttonB() ? tr( "Pressed" ) : QString() );
  mButtonXLabel->setText( mGamepad->buttonX() ? tr( "Pressed" ) : QString() );
  mButtonYLabel->setText( mGamepad->buttonY() ? tr( "Pressed" ) : QString() );

  mLabelButtonL1->setText( mGamepad->buttonL1() ? tr( "Pressed" ) : QString() );
  mLabelButtonL2->setText( QString::number( mGamepad->buttonL2() ) );
  mLabelButtonL3->setText( mGamepad->buttonL3() ? tr( "Pressed" ) : QString() );

  mLabelButtonR1->setText( mGamepad->buttonR1() ? tr( "Pressed" ) : QString() );
  mLabelButtonR2->setText( QString::number( mGamepad->buttonR2() ) );
  mLabelButtonR3->setText( mGamepad->buttonR3() ? tr( "Pressed" ) : QString() );

  mCenterButtonLabel->setText( mGamepad->buttonCenter() ? tr( "Pressed" ) : QString() );
  mGuideButtonLabel->setText( mGamepad->buttonGuide() ? tr( "Pressed" ) : QString() );
  mStartButtonLabel->setText( mGamepad->buttonStart() ? tr( "Pressed" ) : QString() );
  mSelectButtonLabel->setText( mGamepad->buttonSelect() ? tr( "Pressed" ) : QString() );

  mUpButtonLabel->setText( mGamepad->buttonUp() ? tr( "Pressed" ) : QString() );
  mDownButtonLabel->setText( mGamepad->buttonDown() ? tr( "Pressed" ) : QString() );
  mLeftButtonLabel->setText( mGamepad->buttonLeft() ? tr( "Pressed" ) : QString() );
  mRightButtonLabel->setText( mGamepad->buttonRight() ? tr( "Pressed" ) : QString() );
}

//
// QgsGamepadDeviceOptionsFactory
//
QgsGamepadDeviceOptionsFactory::QgsGamepadDeviceOptionsFactory()
  : QgsOptionsWidgetFactory( tr( "Gamepad" ), QIcon() )
{

}

QIcon QgsGamepadDeviceOptionsFactory::icon() const
{
  return QgsApplication::getThemeIcon( QStringLiteral( "/mIconGps.svg" ) );
}

QgsOptionsPageWidget *QgsGamepadDeviceOptionsFactory::createWidget( QWidget *parent ) const
{
  return new QgsGamepadOptionsWidget( parent );
}

QStringList QgsGamepadDeviceOptionsFactory::path() const
{
  return {QStringLiteral( "controllers" ) };
}

