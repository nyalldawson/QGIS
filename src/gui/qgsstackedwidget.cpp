/***************************************************************************
    qgsstackedwidget.cpp
    --------------------
    begin                : January 2024
    copyright            : (C) 2024 by Stefanos Natsis
    email                : uclaros at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsstackedwidget.h"
#include "qgis.h"
#include <QStackedWidget>
#include <QSize>
#include <QLayout>

#include "qgslogger.h"

QgsStackedWidget::QgsStackedWidget( QWidget *parent )
  : QStackedWidget( parent )
  , mSizeMode( SizeMode::ConsiderAllPages )  //#spellok
{
  connect( this, &QStackedWidget::currentChanged, this, &QgsStackedWidget::onCurrentChanged );
}

QSize QgsStackedWidget::sizeHint() const
{
  switch ( mSizeMode )
  {
    case SizeMode::ConsiderAllPages:  //#spellok
      return QStackedWidget::sizeHint();
    case SizeMode::CurrentPageOnly:
      return currentWidget() ? currentWidget()->layout()->sizeHint() : QSize();
  }
    BUILTIN_UNREACHABLE;
}

QSize QgsStackedWidget::minimumSizeHint() const
{
  switch ( mSizeMode )
  {
    case SizeMode::ConsiderAllPages:  //#spellok
      return QStackedWidget::sizeHint();
    case SizeMode::CurrentPageOnly:
      return currentWidget() ? currentWidget()->layout()->minimumSize() : QSize();
  }
    BUILTIN_UNREACHABLE;
}

int QgsStackedWidget::heightForWidth(int width) const
{
    switch ( mSizeMode )
    {
    case SizeMode::ConsiderAllPages:  //#spellok
      return QStackedWidget::heightForWidth( width );
    case SizeMode::CurrentPageOnly:
      return currentWidget() ? currentWidget()->layout()->heightForWidth( width ) : QStackedWidget::heightForWidth( width );
    }
    BUILTIN_UNREACHABLE
}

bool QgsStackedWidget::hasHeightForWidth() const
{
    switch ( mSizeMode )
    {
    case SizeMode::ConsiderAllPages:  //#spellok
      return QStackedWidget::hasHeightForWidth( );
    case SizeMode::CurrentPageOnly:
      return currentWidget() ? currentWidget()->layout()->hasHeightForWidth() : QStackedWidget::hasHeightForWidth();
    }
    BUILTIN_UNREACHABLE
}

void QgsStackedWidget::onCurrentChanged()
{
  switch ( mSizeMode )
  {
    case SizeMode::ConsiderAllPages:  //#spellok
      break;
    case SizeMode::CurrentPageOnly:
    {
      if ( QWidget *current = currentWidget() )
      {
          // Make sure stacked widget inherits the current page's size policy.
          // So e.g. if the current page is set to Maximum size policy, we ensure
          // that the stacked widget will take up the smallest size it can
          setSizePolicy( current->sizePolicy() );
          QgsDebugError( QStringLiteral("%1: %2 %3").arg( qgsEnumValueToKey( current->sizePolicy().verticalPolicy() ) )
                            .arg( current->sizeHint().height() )
                        .arg( current->minimumSizeHint().height() ));

          switch ( sizePolicy().verticalPolicy() )
          {
          case QSizePolicy::Maximum:
              //setMaximumHeight( current->minimumSizeHint().height() );
              break;

          case QSizePolicy::Preferred:
              //setMaximumHeight( QWIDGETSIZE_MAX );
              break;

          case QSizePolicy::Fixed:
          case QSizePolicy::Minimum:
          case QSizePolicy::MinimumExpanding:
          case QSizePolicy::Expanding:
          case QSizePolicy::Ignored:
              break;
          }

          updateGeometry();

      }

      break;
    }
  }
}
