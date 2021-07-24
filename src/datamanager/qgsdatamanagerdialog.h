/***************************************************************************
    qgsdatamanagerdialog.h
    ----------------------
    begin                : July 2021
    copyright            : (C) 2021 by Nyall Dawson
    email                : nyall dot dawson at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSDATAMANAGERDIALOG_H
#define QGSDATAMANAGERDIALOG_H

#include <QMainWindow>
#include <QMap>
#include <QModelIndex>
#include "ui_qgsdatamanagerdialogbase.h"
#include "qgis_datamanager.h"

class QgsBrowserGuiModel;
class QgsBrowserDockWidget;

class DATAMANAGER_EXPORT QgsDataManagerDialog : public QMainWindow, private Ui::QgsDataManagerDialogBase
{
    Q_OBJECT
  public:
    QgsDataManagerDialog( QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags() );
    ~QgsDataManagerDialog();


  public slots:

    void saveWindowState();
    void restoreWindowState();

  private:

    QgsBrowserGuiModel *mBrowserModel = nullptr;
    QgsBrowserDockWidget *mBrowserWidget = nullptr;

};

#endif // QGSDATAMANAGERDIALOG_H
