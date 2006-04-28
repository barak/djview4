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
#include <QMessageBox>
#include <QString>
#include <QList>
#include <QUrl>

#include <libdjvu/miniexp.h>
#include <libdjvu/ddjvuapi.h>

#include "qdjvu.h"
#include "qdjvuwidget.h"
#include "qdjviewprefs.h"


class QLabel;
class QAction;
class QActionGroup;
class QStackedLayout;
class QDjViewDialogError;
class QMenu;
class QMenuBar;
class QToolBar;
class QDockWidget;
class QStatusBar;
class QComboBox;



class QDjView : public QMainWindow
{
  Q_OBJECT

 public:

  enum ViewerMode {
    STANDALONE = 0,
    EMBEDDED_PLUGIN = 1,
    FULLPAGE_PLUGIN = 2
  };

  QDjView(QDjVuContext &context, ViewerMode mode=STANDALONE, QWidget *parent=0);

  void open(QDjVuDocument *document, bool own=true);
  bool open(QString filename);
  bool open(QUrl url);
  void closeDocument();
  QDjVuWidget *djvuWidget();
  

public slots:
  void raiseErrorDialog(QMessageBox::Icon icon, 
                        QString caption="", QString message="");
  int  execErrorDialog (QMessageBox::Icon icon,
                        QString caption="", QString message="");
  
protected:
  typedef QDjVuWidget::Position Position;
  typedef QDjVuWidget::PageInfo PageInfo;
  QAction *makeAction(QString text);
  QAction *makeAction(QString text, bool value);
  void createActions(void);
  void updateActions(void);
  bool eventFilter(QObject *watched, QEvent *event);

  QString pageName(int pageno);

protected slots:
  void docinfo();
  void layoutChanged();
  void pageChanged(int pageno);
  void errorCondition(int);
  void pointerPosition(const Position &pos, const PageInfo &page);
  void pointerEnter(const Position &pos, miniexp_t maparea);
  void pointerLeave(const Position &pos, miniexp_t maparea);
  void pointerClick(const Position &pos, miniexp_t maparea);
  void pointerSelect(const QPoint &pointerPos, const QRect &rect);

protected:
  const ViewerMode          viewerMode;
  QDjVuContext             &djvuContext;
  QDjViewPrefs             *generalPrefs;
  QDjViewPrefs::Appearance *appearancePrefs;
  int                       appearanceFlags;
  
  QLabel             *splash;
  QDjVuWidget        *widget;
  QStackedLayout     *layout;
  QDjViewDialogError *errorDialog;
  QMenu              *contextMenu;
  QMenuBar           *menuBar;
  QStatusBar         *statusBar;
  QLabel             *pageLabel;
  QLabel             *mouseLabel;
  QToolBar           *toolBar;
  QComboBox          *modeCombo;
  QComboBox          *zoomCombo;
  QComboBox          *pageCombo;
  QToolBar           *searchBar;
  QDockWidget        *sideBar;
  
  QDjVuDocument          *document;
  QString                 documentFileName;
  QUrl                    documentUrl;
  QList<ddjvu_fileinfo_t> documentPages;

  QActionGroup *zoomActionGroup;
  QActionGroup *modeActionGroup;
  QActionGroup *rotationActionGroup;

  QAction *actionNew;
  QAction *actionOpen;
  QAction *actionClose;
  QAction *actionQuit;
  QAction *actionSave;
  QAction *actionExport;
  QAction *actionPrint;
  QAction *actionSearch;
  QAction *actionZoomIn;
  QAction *actionZoomOut;
  QAction *actionZoomFitWidth;
  QAction *actionZoomFitPage;
  QAction *actionZoomOneToOne;
  QAction *actionZoom300;
  QAction *actionZoom200;
  QAction *actionZoom150;
  QAction *actionZoom100;
  QAction *actionZoom75;
  QAction *actionZoom50;
  QAction *actionNavFirst;
  QAction *actionNavNext;
  QAction *actionNavPrev;
  QAction *actionNavLast;
  QAction *actionRotateLeft;
  QAction *actionRotateRight;
  QAction *actionRotate0;
  QAction *actionRotate90;
  QAction *actionRotate180;
  QAction *actionRotate270;
  QAction *actionPageInfo;
  QAction *actionDocInfo;
  QAction *actionAbout;
  QAction *actionDisplayColor;
  QAction *actionDisplayBW;
  QAction *actionDisplayForeground;
  QAction *actionDisplayBackground;
  QAction *actionPreferences;
  QAction *actionViewToolBar;
  QAction *actionViewSearchBar;
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
