/***************************************************************************
   qgsvideoexportsettingswidget.cpp
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

#include "qgsvideoexportsettingswidget.h"

#include "moc_qgsvideoexportsettingswidget.cpp"

QgsVideoExportSettingsWidget::QgsVideoExportSettingsWidget( QWidget *parent )
  : QgsPanelWidget { parent }
{
  setupUi( this );

  mFpsSpinBox->setShowClearButton( false );
  mBitRateSpin->setShowClearButton( false );

  mEncodingModeComboBox->addItem( tr( "Constant Quality" ), QVariant::fromValue( QMediaRecorder::EncodingMode::ConstantQualityEncoding ) );
  mEncodingModeComboBox->addItem( tr( "Constant Bitrate (CBR)" ), QVariant::fromValue( QMediaRecorder::EncodingMode::ConstantBitRateEncoding ) );
  mEncodingModeComboBox->addItem( tr( "Average Bitrate (ABR)" ), QVariant::fromValue( QMediaRecorder::EncodingMode::AverageBitRateEncoding ) );
  mEncodingModeComboBox->addItem( tr( "Two-Pass Encoding" ), QVariant::fromValue( QMediaRecorder::EncodingMode::TwoPassEncoding ) );

  mEncodingModeComboBox->setCurrentIndex( mEncodingModeComboBox->findData( QVariant::fromValue( QMediaRecorder::EncodingMode::ConstantQualityEncoding ) ) );
  connect( mEncodingModeComboBox, qOverload< int>( &QComboBox::currentIndexChanged ), this, &QgsVideoExportSettingsWidget::encodingModeChanged );
  encodingModeChanged();

  mQualityComboBox->addItem( tr( "Very High" ), QVariant::fromValue( QMediaRecorder::Quality::VeryHighQuality ) );
  mQualityComboBox->addItem( tr( "High" ), QVariant::fromValue( QMediaRecorder::Quality::HighQuality ) );
  mQualityComboBox->addItem( tr( "Normal" ), QVariant::fromValue( QMediaRecorder::Quality::NormalQuality ) );
  mQualityComboBox->addItem( tr( "Low" ), QVariant::fromValue( QMediaRecorder::Quality::LowQuality ) );
  mQualityComboBox->addItem( tr( "Very Low" ), QVariant::fromValue( QMediaRecorder::Quality::VeryLowQuality ) );
  mQualityComboBox->setCurrentIndex( mQualityComboBox->findData( QVariant::fromValue( QMediaRecorder::Quality::HighQuality ) ) );

  const QList<QMediaFormat::FileFormat> formats = QMediaFormat().supportedFileFormats( QMediaFormat::ConversionMode::Encode );
  for ( QMediaFormat::FileFormat format : formats )
  {
    QString description;
    switch ( format )
    {
      case QMediaFormat::UnspecifiedFormat:
        continue;

      case QMediaFormat::WMV:
        description = tr( "Windows Media Video (*.wmv)" );
        break;
      case QMediaFormat::AVI:
        description = tr( "Audio Video Interleave (*.avi)" );
        break;
      case QMediaFormat::Matroska:
        description = tr( "Matroska (*.mkv)" );
        break;
      case QMediaFormat::MPEG4:
        description = tr( "MPEG-4 (*.mp4)" );
        break;
      case QMediaFormat::Ogg:
        description = tr( "Ogg (*.ogg, *.ogv)" );
        break;
      case QMediaFormat::QuickTime:
        description = tr( "QuickTime (*.mov)" );
        break;
      case QMediaFormat::WebM:
        description = tr( "WebM (*.webm)" );
        break;
      case QMediaFormat::Mpeg4Audio:
      case QMediaFormat::AAC:
      case QMediaFormat::WMA:
      case QMediaFormat::MP3:
      case QMediaFormat::FLAC:
      case QMediaFormat::Wave:
        continue; // audio only
    }
    mFormatComboBox->addItem( description, QVariant::fromValue( format ) );
  }

  // default to mp4, a safe choice
  const int index = mFormatComboBox->findData( QVariant::fromValue( QMediaFormat::FileFormat::MPEG4 ) );
  mFormatComboBox->setCurrentIndex( index >= 0 ? index : 0 );

  connect( mFormatComboBox, qOverload< int>( &QComboBox::currentIndexChanged ), this, &QgsVideoExportSettingsWidget::formatChanged );
  formatChanged();
}

void QgsVideoExportSettingsWidget::formatChanged()
{
  const QMediaFormat::FileFormat format = mFormatComboBox->currentData().value< QMediaFormat::FileFormat >();

  mVideoCodecComboBox->clear();
  const QList<QMediaFormat::VideoCodec> codecs = QMediaFormat( format ).supportedVideoCodecs( QMediaFormat::ConversionMode::Encode );
  for ( QMediaFormat::VideoCodec codec : codecs )
  {
    QString description;
    switch ( codec )
    {
      case QMediaFormat::VideoCodec::Unspecified:
        continue;
      case QMediaFormat::VideoCodec::MPEG1:
        description = tr( "MPEG-1" );
        break;
      case QMediaFormat::VideoCodec::MPEG2:
        description = tr( "MPEG-2" );
        break;
      case QMediaFormat::VideoCodec::MPEG4:
        description = tr( "MPEG-4" );
        break;
      case QMediaFormat::VideoCodec::H264:
        description = tr( "H.264 / AVC" );
        break;
      case QMediaFormat::VideoCodec::H265:
        description = tr( "H.265 / HEVC" );
        break;
      case QMediaFormat::VideoCodec::VP8:
        description = tr( "VP8" );
        break;
      case QMediaFormat::VideoCodec::VP9:
        description = tr( "VP9" );
        break;
      case QMediaFormat::VideoCodec::AV1:
        description = tr( "AV1" );
        break;
      case QMediaFormat::VideoCodec::Theora:
        description = tr( "Theora" );
        break;
      case QMediaFormat::VideoCodec::WMV:
        description = tr( "Windows Media Video (WMV)" );
        break;
      case QMediaFormat::VideoCodec::MotionJPEG:
        description = tr( "Motion JPEG (MJPEG)" );
        break;
    }

    mVideoCodecComboBox->addItem( description, QVariant::fromValue( codec ) );
  }

  // default to H264, a safe choice
  const int index = mVideoCodecComboBox->findData( QVariant::fromValue( QMediaFormat::VideoCodec::H264 ) );
  mVideoCodecComboBox->setCurrentIndex( index >= 0 ? index : 0 );
}

void QgsVideoExportSettingsWidget::encodingModeChanged()
{
  const QMediaRecorder::EncodingMode encodingMode = mEncodingModeComboBox->currentData().value< QMediaRecorder::EncodingMode >();
  bool enableQuality = false;
  bool enableBitrate = false;
  switch ( encodingMode )
  {
    case QMediaRecorder::ConstantQualityEncoding:
      enableQuality = true;
      enableBitrate = false;
      break;
    case QMediaRecorder::ConstantBitRateEncoding:
    case QMediaRecorder::AverageBitRateEncoding:
    case QMediaRecorder::TwoPassEncoding:
      enableQuality = false;
      enableBitrate = true;
      break;
  }

  mLabelQuality->setEnabled( enableQuality );
  mQualityComboBox->setEnabled( enableQuality );
  mLabelBitRate->setEnabled( enableBitrate );
  mBitRateSpin->setEnabled( enableBitrate );
}

QMediaFormat::FileFormat QgsVideoExportSettingsWidget::fileFormat() const
{
  return mFormatComboBox->currentData().value< QMediaFormat::FileFormat >();
}

void QgsVideoExportSettingsWidget::setFileFormat( QMediaFormat::FileFormat format )
{
  const int index = mFormatComboBox->findData( QVariant::fromValue( format ) );
  if ( index >= 0 )
  {
    mFormatComboBox->setCurrentIndex( index );
  }
}

QMediaFormat::VideoCodec QgsVideoExportSettingsWidget::videoCodec() const
{
  return mVideoCodecComboBox->currentData().value< QMediaFormat::VideoCodec >();
}

void QgsVideoExportSettingsWidget::setVideoCodec( QMediaFormat::VideoCodec codec )
{
  const int index = mVideoCodecComboBox->findData( QVariant::fromValue( codec ) );
  if ( index >= 0 )
  {
    mVideoCodecComboBox->setCurrentIndex( index );
  }
}

QMediaRecorder::EncodingMode QgsVideoExportSettingsWidget::encodingMode() const
{
  return mEncodingModeComboBox->currentData().value< QMediaRecorder::EncodingMode >();
}

void QgsVideoExportSettingsWidget::setEncodingMode( QMediaRecorder::EncodingMode mode )
{
  const int index = mEncodingModeComboBox->findData( QVariant::fromValue( mode ) );
  if ( index >= 0 )
  {
    mEncodingModeComboBox->setCurrentIndex( index );
  }
}

QMediaRecorder::Quality QgsVideoExportSettingsWidget::quality() const
{
  return mQualityComboBox->currentData().value< QMediaRecorder::Quality >();
}

void QgsVideoExportSettingsWidget::setQuality( QMediaRecorder::Quality quality )
{
  const int index = mQualityComboBox->findData( QVariant::fromValue( quality ) );
  if ( index >= 0 )
  {
    mQualityComboBox->setCurrentIndex( index );
  }
}

int QgsVideoExportSettingsWidget::videoBitRate() const
{
  return mBitRateSpin->value() * 1000;
}

void QgsVideoExportSettingsWidget::setVideoBitRate( int rate )
{
  mBitRateSpin->setValue( rate / 1000 );
}

double QgsVideoExportSettingsWidget::framesPerSecond() const
{
  return mFpsSpinBox->value();
}

void QgsVideoExportSettingsWidget::setFramesPerSecond( double framesPerSecond )
{
  mFpsSpinBox->setValue( framesPerSecond );
}
