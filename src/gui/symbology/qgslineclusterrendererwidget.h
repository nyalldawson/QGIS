/***************************************************************************
    qgslineclusterrendererwidget.h
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
#ifndef QGSLINECLUSTERRENDERERWIDGET_H
#define QGSLINECLUSTERRENDERERWIDGET_H

#include "ui_qgslineclusterrendererwidgetbase.h"
#include "qgis_sip.h"
#include "qgsrendererwidget.h"
#include "qgis_gui.h"

class QMenu;
class QgsLineClusterRenderer;

/**
 * \ingroup gui
 * \brief A widget used represent options of a QgsLineClusterRenderer
 *
 * \since QGIS 3.36
 */
class GUI_EXPORT QgsLineClusterRendererWidget : public QgsRendererWidget, private Ui::QgsLineClusterRendererWidgetBase
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
    QgsLineClusterRendererWidget( QgsVectorLayer *layer, QgsStyle *style, QgsFeatureRenderer *renderer );
    ~QgsLineClusterRendererWidget() override;

    QgsFeatureRenderer *renderer() override;
    void setContext( const QgsSymbolWidgetContext &context ) override;
    void setDockMode( bool dockMode ) override;

  private:
    //! The renderer
    std::unique_ptr<QgsLineClusterRenderer> mRenderer;
    //! The widget used to represent the embedded renderer
    std::unique_ptr<QgsRendererWidget> mEmbeddedRendererWidget;

  private slots:
    void mRendererComboBox_currentIndexChanged( int index );
};


#endif // QGSLINECLUSTERRENDERERWIDGET_H
