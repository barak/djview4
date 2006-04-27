//C-  -*- C++ -*-
//C- -------------------------------------------------------------------
//C- DjView4
//C- Copyright (c) 2006  Leon Bottou
//C-
//C- This software is subject to, and may be distributed under, the
//C- GNU General Public License. The license should have
//C- accompanied the software or you may obtain a copy of the license
//C- from the Free Software Foundation at http://www.fsf.org .
//C-
//C- This program is distributed in the hope that it will be useful,
//C- but WITHOUT ANY WARRANTY; without even the implied warranty of
//C- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//C- GNU General Public License for more details.
//C-  ------------------------------------------------------------------

// $Id$

#ifndef QDJVIEW_H
#define QDJVIEW_H

#include <QObject>
#include <QMainWindow>
#include <QString>
#include <QUrl>

#include "qdjvu.h"
#include "qdjviewprefs.h"


class QDjViewPrivate;
class QDjViewDialogError;

class QDjView : public QMainWindow
{
  Q_OBJECT

 public:

  enum ViewerMode {
    STANDALONE = 0,
    EMBEDDED_PLUGIN = 1,
    FULLPAGE_PLUGIN = 2
  };

  virtual ~QDjView();

  QDjView(QDjVuContext &context, ViewerMode mode=STANDALONE, QWidget *parent=0);

  void closeDocument();
  void open(QDjVuDocument *document, bool own=true);
  bool open(QString filename);
  bool open(QUrl url);

public slots:  
  void raiseErrorDialog(QString message = QString());
  int  execErrorDialog(QString message = QString());

protected:
  QDjVuContext &djvuContext;
  const QDjView::ViewerMode viewerMode;
  
  QDjVuWidget        *widget;
  QDjVuDocument      *document;
  QDjViewDialogError *errorDialog;

  QDjViewPrefs    preferences;

  QAction *actionNew;
  QAction *actionOpen;
  QAction *actionClose;
  QAction *actionSave;
  QAction *actionPrint;
  QAction *actionSearch;
  QAction *actionZoomIn;
  QAction *actionZoomOut;
  QAction *actionZoomFitWidth;
  QAction *actionZoomFitPage;
  QAction *actionNavFirst;
  QAction *actionNavNext;
  QAction *actionNavPrev;
  QAction *actionNavLast;
  QAction *actionRotateLeft;
  QAction *actionRotateRight;
  QAction *actionPageInfo;
  QAction *actionDocInfo;
  QAction *actionAbout;
  QAction *actionDisplayColor;
  QAction *actionDisplayBW;
  QAction *actionDisplayForeground;
  QAction *actionDisplayBackground;
  QAction *actionPreferences;
  QAction *actionViewToolbar;
  QAction *actionViewSidebar;
  QAction *actionViewStatusbar;
  QAction *actionViewFullScreen;
  QAction *actionLayoutContinuous;
  QAction *actionLayoutSideBySide;
  QAction *actionLayoutPageSettings;

};


#endif
/* -------------------------------------------------------------
   Local Variables:
   c++-font-lock-extra-types: ( "\\sw+_t" "[A-Z]\\sw*[a-z]\\sw*" )
   End:
   ------------------------------------------------------------- */
