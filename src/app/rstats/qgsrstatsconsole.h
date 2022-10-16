/***************************************************************************
                             qgsrstatsconsole.h
                             --------------
    begin                : September 2022
    copyright            : (C) 2022 Nyall Dawson
    email                : nyall dot dawson at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef QGSRSTATSCONSOLE_H
#define QGSRSTATSCONSOLE_H

#include <QWidget>

#include "qgscodeeditorr.h"

class QgsRStatsRunner;
class QLineEdit;
class QTextBrowser;
class QgsDockableWidgetHelper;

class QgsInteractiveRWidget : public QgsCodeEditorR
{
    Q_OBJECT
  public:

    QgsInteractiveRWidget( QWidget *parent = nullptr );

  signals:

    void execCommand( const QString &command );

  protected:

    void keyPressEvent( QKeyEvent *event ) override;
    void runCommandImpl( const QString &command ) override;

    void initializeLexer() override;

};

class QgsRStatsConsole: public QWidget
{
  public:
    QgsRStatsConsole( QWidget *parent, QgsRStatsRunner *runner );
    ~QgsRStatsConsole() override;

  private:

    QgsRStatsRunner *mRunner = nullptr;
    QgsInteractiveRWidget *mInputEdit = nullptr;
    QgsCodeEditorR *mOutput = nullptr;
    QgsDockableWidgetHelper *mDockableWidgetHelper = nullptr;

};

#endif // QGSRSTATSCONSOLE_H
