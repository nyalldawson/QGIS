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
#include "qgsmodeldesignerconfigwidget.h"
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
class QTabWidget;
class QgsPanelWidgetStack;
class QTextEdit;
class QgsColorButton;

#ifndef SIP_RUN
/**
 * A panel widget displaying the configuration for a child algorithm in a Processing model.
 *
 * \note Not available in Python bindings
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

    /**
     * Sets widget state from the existing child algorithm definition in the model.
     */
    void setStateFromChildAlgorithm();

  private:
    void setupUi();
    void emitChangedSignal();

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
#endif

/**
 * A panel config widget combining parameter settings and comments for a child algorithm in a Processing model.
 *
 * \warning Not stable API
 * \ingroup gui
 * \since QGIS 4.4
 */
class GUI_EXPORT QgsProcessingModelerParametersWidget : public QgsProcessingModelConfigWidget
{
    Q_OBJECT

  public:
    /**
   * Constructor for QgsProcessingModelerParametersWidget.
   */
    QgsProcessingModelerParametersWidget(
      const QgsProcessingAlgorithm *alg,
      QgsProcessingModelAlgorithm *model,
      const QString &algName = QString(),
      const QVariantMap &configuration = QVariantMap(),
      QWidget *parent = nullptr,
      QgsProcessingContext *context = nullptr,
      QWidget *dialog = nullptr
    );

    ~QgsProcessingModelerParametersWidget() override;

    /**
   * Returns the algorithm associated with the widget.
   */
    const QgsProcessingAlgorithm *algorithm() const;

    /**
   * Sets the comment \a text.
   *
   * \see comments()
   */
    void setComments( const QString &text );

    /**
   * Returns the comment text.
   *
   * \see setComments()
   */
    QString comments() const;

    /**
   * Sets the comment's \a color.
   *
   * \see commentColor()
   */
    void setCommentColor( const QColor &color );

    /**
   * Returns the comment's color.
   *
   * \see setCommentColor()
   */
    QColor commentColor() const;

    /**
   * Focuses the widget on the comment editing tab.
   */
    void switchToCommentTab();

    /**
   * Sets widget state from the existing child algorithm definition in the model.
   */
    void setStateFromChildAlgorithm();

    /**
   * Creates the child algorithm instance, populated with the current widget parameter values and comments.
   */
    std::unique_ptr< QgsProcessingModelChildAlgorithm > createAlgorithm();

  private:
    void setupUi();

    std::unique_ptr< QgsProcessingAlgorithm > mAlg;

    QTabWidget *mTab = nullptr;
    QgsPanelWidgetStack *mPanelWidgetStack = nullptr;
    QgsProcessingModelerParametersPanelWidget *mParametersPanel = nullptr;
    QTextEdit *mCommentEdit = nullptr;
    QgsColorButton *mCommentColorButton = nullptr;
};

#endif // QGSPROCESSINGMODELCHILDALGORITHMNWIDGETS_H
