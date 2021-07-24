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

class QgsBrowserModel;
class QgsLayerItem;
class QgsMapLayer;
class QgsMapToolPan;

class DATAMANAGER_EXPORT QgsDataManagerDialog : public QMainWindow, private Ui::QgsDataManagerDialogBase
{
    Q_OBJECT
  public:
    QgsDataManagerDialog( QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags() );
    ~QgsDataManagerDialog();

    // Expand to given path
    void expandPath( const QString &path );
    void setLayer( QgsVectorLayer *vLayer );


  public slots:
    void itemClicked( const QModelIndex &index );
    void itemDoubleClicked( const QModelIndex &index );
    void on_mActionSetProjection_triggered();
    void on_mActionWmsConnections_triggered();
    void on_mActionRefresh_triggered();
    void newVectorLayer();

    void saveWindowState();
    void restoreWindowState();

    void tabChanged();
    void updateCurrentTab();
    void stopRendering();

    // Refresh all leaf or expanded items
    void refresh( const QModelIndex &index = QModelIndex() );

  protected:
    void keyPressEvent( QKeyEvent *e ) override;
    void keyReleaseEvent( QKeyEvent *e ) override;

    bool layerClicked( QgsLayerItem *ptr );

    enum Tab
    {
      Metadata,
      Preview,
      Attributes
    };
    Tab activeTab();

    bool mDirtyMetadata, mDirtyPreview, mDirtyAttributes;

    QgsBrowserModel *mModel = nullptr;
    QgsMapLayer *mLayer = nullptr;
    QModelIndex mIndex;
    QWidget *mParamWidget = nullptr;
    // last (selected) tab for each
    QMap<QString, int> mLastTab;
    QgsAttributeTableFilterModel *mAttributeTableFilterModel = nullptr;

  private:

    std::unique_ptr< QgsMapToolPan > mMapToolPan;
};

#endif // QGSDATAMANAGERDIALOG_H
