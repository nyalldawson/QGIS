/***************************************************************************
                         qgsattributedialog.h  -  description
                             -------------------
    begin                : October 2004
    copyright            : (C) 2004 by Marco Hugentobler
    email                : marco.hugentobler@autoform.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef QGSATTRIBUTEDIALOG_H
#define QGSATTRIBUTEDIALOG_H

#include "qgis_gui.h"
#include "qgis_sip.h"
#include "qgsattributeeditorcontext.h"
#include "qgsattributeform.h"
#include "qgsmaplayeractioncontextgenerator.h"
#include "qgspanelwidget.h"
#include "qgstrackedvectorlayertools.h"

#include <QDialog>
#include <QGridLayout>
#include <QMenuBar>

class QgsHighlight;
class QgsActionMenu;
class QgsPanelWidgetStack;

/**
 * \ingroup gui
 * \class QgsAttributePanelWidget
 * \brief A panel widget for showing the attribute entry for a vector layer.
 *
 * \since QGIS 4.4
 */
class GUI_EXPORT QgsAttributePanelWidget : public QgsPanelWidget, public QgsMapLayerActionContextGenerator
{
    Q_OBJECT

  public:
    QgsAttributePanelWidget(
      QgsVectorLayer *vl,
      QgsFeature *thepFeature,
      bool featureOwner,
      QWidget *parent SIP_TRANSFERTHIS = nullptr,
      bool showDialogButtons = true,
      const QgsAttributeEditorContext &context = QgsAttributeEditorContext()
    );

    ~QgsAttributePanelWidget() override;

    void accept();
    void reject();

    /**
     * \brief setHighlight
     * \param h The highlight. Ownership is taken.
     */
    void setHighlight( QgsHighlight *h );

    /**
     * Returns the attribute form.
     */
    QgsAttributeForm *attributeForm() const;

    /**
     * Returns the message bar.
     */
    QgsMessageBar *messageBar();

    /**
     * Returns the current feature of the attribute form.
     */
    const QgsFeature *feature() const;

    /**
     * Is this dialog editable?
     *
     * \returns returns TRUE, if this dialog was created in an editable manner.
     */
    bool editable() const;

    /**
     * Toggles the form mode.
     * \param mode form mode. For example, if set to QgsAttributeEditorContext::AddFeatureMode, the dialog will be editable even with an invalid feature and
     * will add a new feature when the form is accepted.
     */
    void setMode( QgsAttributeEditorContext::Mode mode );

    /**
     * Sets the edit command message (Undo) that will be used when the dialog is accepted
     *
     * \param message The message
     */
    void setEditCommandMessage( const QString &message );

    /**
     * Sets \a extraScope as an additional expression context scope to be used
     * for calculations in this form.
     *
     * \since QGIS 3.16
     */
    void setExtraContextScope( QgsExpressionContextScope *extraScope SIP_TRANSFER );

    QgsMapLayerActionContext createActionContext() override;

    bool event( QEvent *e ) override;

  signals:

    void accepted();
    void rejected();

  private:
    void init( QgsVectorLayer *layer, QgsFeature *feature, const QgsAttributeEditorContext &context, bool showDialogButtons );

    std::unique_ptr<QgsFeature> mOwnedFeature;
    QgsMessageBar *mMessageBar = nullptr;
    QgsTrackedVectorLayerTools mTrackedVectorLayerTools;
    QgsAttributeForm *mAttributeForm = nullptr;

    QgsActionMenu *mMenu;
    QMenuBar *mMenuBar = nullptr;

    QgsHighlight *mHighlight = nullptr;
};


/**
 * \ingroup gui
 * \class QgsAttributeDialog
 * \brief An attribute table dialog for a vector layer.
 */
class GUI_EXPORT QgsAttributeDialog : public QDialog, public QgsMapLayerActionContextGenerator
{
    Q_OBJECT

  public:
    /**
     * Create an attribute dialog for a given layer and feature
     *
     * \param vl                The layer for which the dialog will be generated
     * \param thepFeature       A feature for which the dialog will be generated
     * \param featureOwner      Set to TRUE, if the dialog should take ownership of the feature
     * \param parent            A parent widget for the dialog
     * \param showDialogButtons TRUE: Show the dialog buttons accept/cancel
     * \param context           The context in which this dialog is created
     *
     */
    QgsAttributeDialog(
      QgsVectorLayer *vl,
      QgsFeature *thepFeature,
      bool featureOwner,
      QWidget *parent SIP_TRANSFERTHIS = nullptr,
      bool showDialogButtons = true,
      const QgsAttributeEditorContext &context = QgsAttributeEditorContext()
    );

    ~QgsAttributeDialog() override;

    /**
     * \brief setHighlight
     * \param h The highlight. Ownership is taken.
     */
    void setHighlight( QgsHighlight *h );

    /**
     * Returns the attribute form.
     */
    QgsAttributeForm *attributeForm() const;

    /**
     * Returns the current feature of the attribute form.
     */
    const QgsFeature *feature() const;

    /**
     * Is this dialog editable?
     *
     * \returns returns TRUE, if this dialog was created in an editable manner.
     */
    bool editable() const;

    /**
     * Toggles the form mode.
     * \param mode form mode. For example, if set to QgsAttributeEditorContext::AddFeatureMode, the dialog will be editable even with an invalid feature and
     * will add a new feature when the form is accepted.
     */
    void setMode( QgsAttributeEditorContext::Mode mode );

    /**
     * Sets the edit command message (Undo) that will be used when the dialog is accepted
     *
     * \param message The message
     */
    void setEditCommandMessage( const QString &message );

    void showEvent( QShowEvent *event ) override;

    /**
     * Sets \a extraScope as an additional expression context scope to be used
     * for calculations in this form.
     *
     * \since QGIS 3.16
     */
    void setExtraContextScope( QgsExpressionContextScope *extraScope SIP_TRANSFER );

    QgsMapLayerActionContext createActionContext() override;

  public slots:
    void reject() override;

    //! Show the dialog non-blocking. Reparents this dialog to be a child of the dialog form
    void show();

  private:
    void onPanelRejected();

    QgsAttributePanelWidget *mPanelWidget = nullptr;
    QgsPanelWidgetStack *mWidgetStack = nullptr;

    bool mFirstShow = true;
    bool mPanelWasRejected = false;
    void saveGeometry();
    void restoreGeometry();
};

#endif
