/***************************************************************************
    qgsgamepadoptions.h
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
#ifndef QGSGAMEPADOPTIONS_H
#define QGSGAMEPADOPTIONS_H

#include "ui_qgsgamepadoptionsbase.h"
#include "qgsoptionswidgetfactory.h"

#include <QPointer>

class QGamepad;

/**
 * \ingroup app
 * \class QgsGamepadOptionsWidget
 * \brief An options widget showing GPS device configuration.
 *
 * \since QGIS 3.32
 */
class QgsGamepadOptionsWidget : public QgsOptionsPageWidget, private Ui::QgsGamepadOptionsWidgetBase
{
    Q_OBJECT

  public:

    /**
     * Constructor for QgsGamepadOptionsWidget with the specified \a parent widget.
     */
    QgsGamepadOptionsWidget( QWidget *parent );

    void apply() override;

  private slots:

    void deviceChanged();
    void updateValues();

  private:

    int mDeviceId = -1;
    QPointer< QGamepad > mGamepad;

};


class QgsGamepadDeviceOptionsFactory : public QgsOptionsWidgetFactory
{
    Q_OBJECT

  public:

    QgsGamepadDeviceOptionsFactory();

    QIcon icon() const override;
    QgsOptionsPageWidget *createWidget( QWidget *parent = nullptr ) const override;
    QStringList path() const override;

};


#endif // QGSGAMEPADOPTIONS_H
