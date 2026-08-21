/***************************************************************************
                         qgsmodelchildalgorithmwidgets.h
                         ----------------------------------------
    begin                : August 2026
    copyright            : (C) 2026 by Nyall Dawson
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


#ifndef QGSPROCESSINGMODELCHILDALGORITHMNWIDGETS_H
#define QGSPROCESSINGMODELCHILDALGORITHMNWIDGETS_H

#include <memory>

#include "qgis_gui.h"
#include "qgspanelwidget.h"

class QgsProcessingAlgorithm;
class QgsProcessingModelAlgorithm;
class QgsProcessingContext;
class QgsProcessingModelChildAlgorithm;
class QgsProcessingModelOutput;
class QLineEdit;
class QgsMessageBar;
class QgsProcessingAlgorithmConfigurationWidget;
class QgsProcessingModelerParameterWidget;
class QgsModelChildDependenciesWidget;
class QgsProcessingContextGenerator;

/**
 * A panel widget displaying the configuration for a child algorithm in a Processing model.
 *
 * \warning Not stable API
 *
 * \ingroup gui
 * \since QGIS 4.4
 */
class GUI_EXPORT QgsProcessingModelerParametersPanelWidget : public QgsPanelWidget
{
    Q_OBJECT

  public:
    /**
   * Constructor for QgsProcessingModelerParametersPanelWidget.
   */
    QgsProcessingModelerParametersPanelWidget(
      const QgsProcessingAlgorithm *alg,
      QgsProcessingModelAlgorithm *model,
      const QString &algName = QString(),
      const QVariantMap &configuration = QVariantMap(),
      QWidget *parent = nullptr,
      QgsProcessingContext *context = nullptr,
      QWidget *dialog = nullptr
    );

    ~QgsProcessingModelerParametersPanelWidget() override;

    /**
   * Returns the algorithm associated with the widget.
   */
    const QgsProcessingAlgorithm *algorithm() const;

    /**
   * Creates the child algorithm instance populated with current widget values.
   */
    std::unique_ptr< QgsProcessingModelChildAlgorithm > createAlgorithm();

  private:
    void setupUi();
    void emitChangedSignal();

    /**
   * Sets widget state from the existing child algorithm definition in the model.
   */
    void setStateFromChildAlgorithm();

    std::unique_ptr< QgsProcessingAlgorithm > mAlgorithm;
    QgsProcessingModelAlgorithm *mModel = nullptr;
    QString mChildId;
    QVariantMap mConfiguration;
    QgsProcessingContext *mContext = nullptr;
    QWidget *mDialog = nullptr;

    QMap< QString, QgsProcessingModelOutput > mPreviousOutputDefinitions;
    int mBlockChangesSignal = 0;

    QLineEdit *mDescriptionBox = nullptr;
    QgsMessageBar *mMessageBar = nullptr;
    QgsProcessingAlgorithmConfigurationWidget *mAlgorithmItem = nullptr;
    QMap< QString, QgsProcessingModelerParameterWidget * > mWrappers;
    QgsModelChildDependenciesWidget *mDependenciesPanel = nullptr;
    std::unique_ptr< QgsProcessingContextGenerator > mContextGenerator;
};


#endif // QGSPROCESSINGMODELCHILDALGORITHMNWIDGETS_H
