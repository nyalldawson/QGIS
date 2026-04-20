/***************************************************************************
    qgsvideoexportsettingswidget.h
    --------------------------------------
    begin                : April 2026
    copyright            : (C) 2026 by Nyall Dawson
    email                : nyall dot dawson at gmail dot com
***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#ifndef QGSVIDEOEXPORTSETTINGSWIDGET_H
#define QGSVIDEOEXPORTSETTINGSWIDGET_H

#include "ui_qgsvideoexportsettingswidgetbase.h"

#include "qgis.h"
#include "qgis_gui.h"
#include "qgis_sip.h"
#include "qgspanelwidget.h"

#include <QMediaFormat>
#include <QMediaRecorder>

/**
 * \ingroup gui
 * \brief A widget for configuring video export settings.
 * \since QGIS 4.2
 */
class GUI_EXPORT QgsVideoExportSettingsWidget : public QgsPanelWidget, private Ui::QgsVideoExportSettingsWidgetBase
{
    Q_OBJECT

  public:
    /**
     * Constructor for QgsVideoExportSettingsWidget, with the specified \a parent widget.
     */
    explicit QgsVideoExportSettingsWidget( QWidget *parent SIP_TRANSFERTHIS = nullptr );

    /**
     * Returns the selected output file format.
     *
     * \see setFileFormat()
     */
    QMediaFormat::FileFormat fileFormat() const;

    /**
     * Sets the selected output file \a format.
     *
     * \see fileFormat()
     */
    void setFileFormat( QMediaFormat::FileFormat format );

    /**
     * Returns the selected output video codec.
     *
     * \see setVideoCodec()
     */
    QMediaFormat::VideoCodec videoCodec() const;

    /**
     * Sets the selected output video \a codec.
     *
     * \see videoCodec()
     */
    void setVideoCodec( QMediaFormat::VideoCodec codec );

    /**
     * Returns the selected video encoding mode.
     *
     * \see setEncodingMode()
     */
    QMediaRecorder::EncodingMode encodingMode() const;

    /**
     * Sets the selected video encoding \a mode.
     *
     * \see encodingMode()
     */
    void setEncodingMode( QMediaRecorder::EncodingMode mode );

    /**
     * Returns the selected video encoding quality.
     *
     * \see setQuality()
     */
    QMediaRecorder::Quality quality() const;

    /**
     * Sets the selected video encoding \a quality.
     *
     * \see quality()
     */
    void setQuality( QMediaRecorder::Quality quality );

    /**
     * Returns the selected video encoding bit rate.
     *
     * \see setVideoBitRate()
     */
    int videoBitRate() const;

    /**
     * Sets the video encoding bit \a rate.
     *
     * \see videoBitRate()
     */
    void setVideoBitRate( int rate );

    /**
     * Returns the selected frames per second.
     *
     * \see setFramesPerSecond()
     */
    double framesPerSecond() const;

    /**
     * Sets the frames per second.
     *
     * \see framesPerSecond()
     */
    void setFramesPerSecond( double framesPerSecond );

  private slots:

    void formatChanged();
    void encodingModeChanged();
};

#endif // QGSVIDEOEXPORTSETTINGSWIDGET_H
