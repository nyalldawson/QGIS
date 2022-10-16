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

#include "qgis_app.h"


class RInside;
class QVariant;
class QString;

#include "qgscodeeditor.h"


class APP_EXPORT QgsRStatsSession: public QObject, public Callbacks
{
    Q_OBJECT
  public:

    QgsRStatsSession();
    ~QgsRStatsSession() override;

    void execCommandNR( const QString &command );

    void WriteConsole( const std::string &line, int type ) override;;

    bool has_WriteConsole() override;;

    void ShowMessage( const char *message ) override;

    bool has_ShowMessage() override;

    bool busy() const { return mBusy; }

    /**
     * Converts a SEXP object to a string.
     */
    static QString sexpToString( const SEXP exp );

    /**
     * Converts a SEXP object to a QVariant.
     */
    static QVariant sexpToVariant( const SEXP exp );

    /**
     * Converts a variant to a SEXP.
     */
    static SEXP variantToSexp( const QVariant &variant );

  public slots:

    void execCommand( const QString &command );

    void showStartupMessage();

  signals:

    void busyChanged( bool busy );

    void consoleMessage( const QString &message, int type );
    void showMessage( const QString &message );
    void errorOccurred( const QString &error );
    void commandFinished( const QVariant &result );

  private:
    void execCommandPrivate( const QString &command, QString &error, QVariant *res = nullptr, QString *output = nullptr );

    std::unique_ptr< RInside > mRSession;
    bool mBusy = false;
    bool mEncounteredErrorMessageType = false;


};


class QgsRStatsRunner: public QObject, public QgsCodeInterpreter
{
    Q_OBJECT
  public:

    QgsRStatsRunner();
    ~QgsRStatsRunner();

    bool busy() const;
    void showStartupMessage();

    QString promptForState( int state ) const override;

  signals:

    void consoleMessage( const QString &message, int type );
    void showMessage( const QString &message );
    void errorOccurred( const QString &error );
    void busyChanged( bool busy );
    void commandStarted( const QString &command );
    void commandFinished( const QVariant &result );

  protected:

    int execCommandImpl( const QString &command ) override;

  private:

    QThread mSessionThread;
    std::unique_ptr<QgsRStatsSession> mSession;
};

#endif // QGSRSTATSRUNNER_H
