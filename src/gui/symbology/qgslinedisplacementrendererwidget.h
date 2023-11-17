/***************************************************************************
    qgslinedisplacementrendererwidget.h
    ---------------------
    begin                : November 2023
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
#ifndef QGSLINEDISPLACEMENTRENDERERWIDGET_H
#define QGSLINEDISPLACEMENTRENDERERWIDGET_H

#include "ui_qgsdisplacedlinesrendererwidgetbase.h"
#include "qgis_sip.h"
#include "qgsrendererwidget.h"
#include "qgis_gui.h"

class QMenu;
class QgsLineDisplacementRenderer;

/**
 * \ingroup gui
 * \brief A widget used represent options of a QgsLineDisplacementRenderer
 *
 * \since QGIS 3.36
 */
class GUI_EXPORT QgsLineDisplacementRendererWidget : public QgsRendererWidget, private Ui::QgsDisplacedLinesRendererWidgetBase
{
    Q_OBJECT

  public:

    /**
     * Static creation method
     * \param layer the layer where this renderer is applied
     * \param style
     * \param renderer the merged feature renderer (will not take ownership)
     */
    static QgsRendererWidget *create( QgsVectorLayer *layer, QgsStyle *style, QgsFeatureRenderer *renderer ) SIP_FACTORY;

    /**
     * Constructor
     * \param layer the layer where this renderer is applied
     * \param style
     * \param renderer the displaced lines feature renderer (will not take ownership)
     */
    QgsLineDisplacementRendererWidget( QgsVectorLayer *layer, QgsStyle *style, QgsFeatureRenderer *renderer );
    ~QgsLineDisplacementRendererWidget() override;

    QgsFeatureRenderer *renderer() override;
    void setContext( const QgsSymbolWidgetContext &context ) override;
    void setDockMode( bool dockMode ) override;

  private:
    //! The renderer
    std::unique_ptr<QgsLineDisplacementRenderer> mRenderer;
    //! The widget used to represent the embedded renderer
    std::unique_ptr<QgsRendererWidget> mEmbeddedRendererWidget;

  private slots:
    void mRendererComboBox_currentIndexChanged( int index );
};


#endif // QGSLINEDISPLACEMENTRENDERERWIDGET_H
