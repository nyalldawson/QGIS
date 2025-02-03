/***************************************************************************
    qgslinuxnative.h
                             -------------------
    begin                : July 2018
    copyright            : (C) 2018 by Nyall Dawson
    email                : nyall dot dawson at gmail dot com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSLINUXNATIVE_H
#define QGSLINUXNATIVE_H

#include "qgsnative.h"

#include <QEventLoop>

/**
 * Native implementation for linux platforms.
 *
 * Implements the native platform interface for Linux based platforms. This is
 * intended to expose desktop-agnostic implementations of the QgsNative methods,
 * so that they work without issue across the wide range of Linux desktop environments
 * (e.g. Gnome/KDE).
 *
 * Typically, this means implementing methods using DBUS calls to freedesktop standards.
 */
class NATIVE_EXPORT QgsLinuxNative final : public QgsNative
{
    Q_OBJECT

  public:
    QgsNative::Capabilities capabilities() const final;
    void initializeMainWindow( QWindow *window, const QString &applicationName, const QString &organizationName, const QString &version ) final;
    void openFileExplorerAndSelectFile( const QString &path ) final;
    void showFileProperties( const QString &path ) final;
    void showUndefinedApplicationProgress() final;
    void setApplicationProgress( double progress ) final;
    void hideApplicationProgress() final;
    void setApplicationBadgeCount( int count ) final;
    bool openTerminalAtPath( const QString &path ) final;
    NotificationResult showDesktopNotification( const QString &summary, const QString &body, const NotificationSettings &settings = NotificationSettings() ) final;
    QPixmap grabScreenshot( QScreen *screen, QRect region = QRect() ) final;

  private slots:

    void DbusScreenshotArrived( uint response, const QVariantMap &results );

  private:
    QString mScreenshotPath;
    bool mScreenshotFinished = false;
    QEventLoop mScreenshotReplyLoop;
    QString mDesktopFile;
};

#endif // QGSLINUXNATIVE_H
