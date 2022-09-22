/***************************************************************************
                             qgsrstatsrunner.h
                             --------------
    begin                : September 2022
    copyright            : (C) 2022 Nyall Dawson
    email                : nyall dot dawson at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef QGSRSTATSRUNNER_H
#define QGSRSTATSRUNNER_H

#include <memory>
#include <QObject>
#include "Callbacks.h"

class RInside;
class QVariant;
class QString;

class QgsRStatsRunner: public QObject, public Callbacks
{
    Q_OBJECT
  public:

    QgsRStatsRunner();
    ~QgsRStatsRunner();

    QVariant execCommand( const QString &command, QString &error );

    void WriteConsole( const std::string &line, int type ) override
    {
      emit consoleMessage( QString::fromStdString( line ) );
    };

    bool has_WriteConsole() override
    {
      return true;
    };

    void ShowMessage( const char *message ) override
    {
      emit showMessage( QString( message ) );
    }

    bool has_ShowMessage() override
    {
      return true;
    }

  signals:

    void consoleMessage( const QString &message );
    void showMessage( const QString &message );

  private:

    std::unique_ptr< RInside > mRSession;
};

#endif // QGSRSTATSRUNNER_H
