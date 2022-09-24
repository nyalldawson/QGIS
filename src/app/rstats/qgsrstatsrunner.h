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
#include <QThread>
#include "Callbacks.h"

class RInside;
class QVariant;
class QString;

class QgsRStatsSession: public QObject, public Callbacks
{
    Q_OBJECT
  public:

    QgsRStatsSession();
    ~QgsRStatsSession() override;

    QVariant execCommand( const QString &command, QString &error );

    void WriteConsole( const std::string &line, int type ) override
    {
      emit consoleMessage( QString::fromStdString( line ), type );
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

    bool busy() const { return mBusy; }

  public slots:

    void execCommand( const QString &command );

  signals:

    void busyChanged( bool busy );

    void consoleMessage( const QString &message, int type );
    void showMessage( const QString &message );
    void errorOccurred( const QString &error );
    void commandFinished( const QVariant &result );

  private:

    std::unique_ptr< RInside > mRSession;
    bool mBusy = false;

};


class QgsRStatsRunner: public QObject
{
    Q_OBJECT
  public:

    QgsRStatsRunner();
    ~QgsRStatsRunner();

    void execCommand( const QString &command );
    bool busy() const;

  signals:

    void consoleMessage( const QString &message, int type );
    void showMessage( const QString &message );
    void errorOccurred( const QString &error );
    void busyChanged( bool busy );
    void commandFinished( const QVariant &result );

  private:

    QThread mSessionThread;
    std::unique_ptr<QgsRStatsSession> mSession;
};

#endif // QGSRSTATSRUNNER_H
