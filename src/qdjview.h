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

#include <Qt>
#include <QObject>
#include <QMainWindow>
#include <QMessageBox>
#include <QPair>
#include <QPointer>
#include <QString>
#include <QList>
#include <QUrl>

#include <libdjvu/miniexp.h>
#include <libdjvu/ddjvuapi.h>

#include "qdjvu.h"
#include "qdjvuwidget.h"
#include "qdjviewprefs.h"
#include "qdjviewdialogs.h"


class QAction;
class QActionGroup;
class QBoxLayout;
class QCloseEvent;
class QComboBox;
class QDockWidget;
class QDjViewOutline;
class QDjViewThumbnails;
class QDjViewFind;
class QFileDialog;
class QLabel;
class QMenu;
class QMenuBar;
class QStackedLayout;
class QStatusBar;
class QStringList;
class QToolBar;
class QToolBox;


class QDjView : public QMainWindow
{
  Q_OBJECT

 public:

  enum ViewerMode {
    EMBEDDED_PLUGIN = 0,
    FULLPAGE_PLUGIN = 1,
    STANDALONE      = 2
  };

  QDjView(QDjVuContext &context, 
          ViewerMode mode=STANDALONE, QWidget *parent=0);

  QDjVuContext       &getDjVuContext()      { return djvuContext; }
  QDjVuWidget        *getDjVuWidget()       { return widget; }
  QDjViewErrorDialog *getErrorDialog()      { return errorDialog; }
  QDjVuDocument      *getDocument()         { return document; } 
  QString             getDocumentFileName() { return documentFileName; }
  QUrl                getDocumentUrl()      { return documentUrl; }
  QString             getShortFileName();
  
  QStringList  parseArgument(QString key, QString val);
  QStringList  parseArgument(QString keyEqualVal);
  void         parseDjVuCgiArguments(QUrl url);
  static QUrl  removeDjVuCgiArguments(QUrl url);
  
  int         pageNum(void);
  QString     pageName(int pageno, bool titleonly=false);
  int         pageNumber(QString name, int from = -1);
  QDjView    *copyWindow(void);
  bool        saveTextFile(QString text, QString filename=QString());
  bool        saveImageFile(QImage image, QString filename=QString());
  bool        startBrowser(QUrl url);
  void        fillZoomCombo(QComboBox *zoomCombo);
  void        fillModeCombo(QComboBox *modeCombo);
  void        fillPageCombo(QComboBox *pageCombo);

public slots:
  bool  open(QString filename);
  bool  open(QUrl url);
  void  open(QDjVuDocument *document, QUrl url = QUrl());
  void  closeDocument();
  void  goToPage(int pageno);
  void  goToPage(QString name, int from=-1);
  void  addToErrorDialog(QString message);
  void  raiseErrorDialog(QMessageBox::Icon icon, QString caption=QString());
  int   execErrorDialog (QMessageBox::Icon icon, QString caption=QString());
  void  setPageLabelText(QString s = QString());
  void  setMouseLabelText(QString s = QString());
  void  statusMessage(QString s = QString());
  bool  showSideBar(Qt::DockWidgetAreas areas, int tab=-1);
  bool  showSideBar(QString args, QStringList &errors);
  bool  showSideBar(QString args);
  void  print(void);
  void  save(void);
  void  find(QString find = QString());
  
signals:
  void  documentClosed(QDjVuDocument *doc);
  void  documentOpened(QDjVuDocument *doc);
  void  documentReady(QDjVuDocument *doc);
  void  pluginStatusMessage(QString message = QString());
  void  pluginGetUrl(QUrl url, QString target);

protected:
  typedef QDjVuWidget::Position Position;
  typedef QDjVuWidget::PageInfo PageInfo;
  typedef QDjViewPrefs::Options Options;
  typedef QDjViewPrefs::Tools Tools;
  typedef QDjViewPrefs::Saved Saved;

  void     fillToolBar(QToolBar *toolBar);
  QAction *makeAction(QString text);
  QAction *makeAction(QString text, bool value);
  void     createActions(void);
  void     createMenus(void);
  void     createWhatsThis(void);
  Saved   *getSavedPrefs(void);
  void     enableContextMenu(bool);
  void     enableScrollBars(bool);
  void     applyOptions(void);
  void     updateOptions(void);
  void     applySaved(Saved *saved);
  void     updateSaved(Saved *saved);
  void     applyPreferences(void);
  void     parseToolBarOption(QString option, QStringList &errors);

  virtual bool eventFilter(QObject *watched, QEvent *event);
  virtual void closeEvent(QCloseEvent *event);

protected slots:
  void info(QString);
  void error(QString, QString, int);
  void errorCondition(int);
  void docinfo();
  void docinfoExtra();
  void performPending();
  void performPendingLater();
  void pointerPosition(const Position &pos, const PageInfo &page);
  void pointerEnter(const Position &pos, miniexp_t maparea);
  void pointerLeave(const Position &pos, miniexp_t maparea);
  void pointerClick(const Position &pos, miniexp_t maparea);
  void pointerSelect(const QPoint &pointerPos, const QRect &rect);
  void updateActions(void);
  void updateActionsLater(void);
  void modeComboActivated(int);
  void zoomComboActivated(int);
  void zoomComboEdited(void);
  void pageComboActivated(int);
  void pageComboEdited(void);
  void preferencesChanged(void);
  void performAbout(void);
  void performNew(void);
  void performOpen(void);
  void performInformation(void);
  void performMetadata(void);
  void performPreferences(void);
  void performRotation(void);
  void performZoom(void);
  void performSelect(bool);
  void performViewFullScreen(bool);

protected:
  // mode
  const ViewerMode   viewerMode;
  // preferences
  QDjViewPrefs  *prefs;
  Options        options;
  Tools          tools;
  Tools          toolsCached;
  // dialogs
  QPointer<QDjViewErrorDialog> errorDialog;
  QPointer<QDjViewInfoDialog>  infoDialog;
  QPointer<QDjViewMetaDialog>  metaDialog;
  QPointer<QDjViewSaveDialog>  saveDialog;
  QPointer<QDjViewPrintDialog> printDialog;
  // widgets
  QLabel             *splash;
  QDjVuWidget        *widget;
  QStackedLayout     *layout;
  QMenu              *contextMenu;
  QMenuBar           *menuBar;
  QStatusBar         *statusBar;
  QLabel             *pageLabel;
  QLabel             *mouseLabel;
  QToolBar           *toolBar;
  QComboBox          *modeCombo;
  QComboBox          *zoomCombo;
  QComboBox          *pageCombo;
  QDockWidget        *sideBar;
  QToolBox           *sideToolBox;
  QDjViewOutline     *outlineWidget;
  QDjViewThumbnails  *thumbnailWidget;
  QDjViewFind        *findWidget;
  // document data
  QDjVuContext           &djvuContext;
  QDjVuDocument          *document;
  QString                 documentFileName;
  QUrl                    documentUrl;
  QList<ddjvu_fileinfo_t> documentPages;
  bool                    hasNumericalPageTitle;
  // delayed settings
  typedef QPair<QString,QString> StringPair;
  QString           pendingUrl;
  QString           pendingPage;
  QList<StringPair> pendingHilite;
  QString           pendingFind;
  // delayed updates
  bool  updateActionsScheduled;
  bool  performPendingScheduled;
  // action lists
  QList<QAction*> allActions;
  QActionGroup *zoomActionGroup;
  QActionGroup *modeActionGroup;
  QActionGroup *rotationActionGroup;
  // all actions
  QAction *actionNew;
  QAction *actionOpen;
  QAction *actionClose;
  QAction *actionQuit;
  QAction *actionSave;
  QAction *actionPrint;
  QAction *actionFind;
  QAction *actionFindNext;
  QAction *actionFindPrev;
  QAction *actionSelect;
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
  QAction *actionBack;
  QAction *actionForw;
  QAction *actionRotateLeft;
  QAction *actionRotateRight;
  QAction *actionRotate0;
  QAction *actionRotate90;
  QAction *actionRotate180;
  QAction *actionRotate270;
  QAction *actionInformation;
  QAction *actionMetadata;
  QAction *actionAbout;
  QAction *actionWhatsThis;
  QAction *actionDisplayColor;
  QAction *actionDisplayBW;
  QAction *actionDisplayForeground;
  QAction *actionDisplayBackground;
  QAction *actionPreferences;
  QAction *actionViewToolBar;
  QAction *actionViewSideBar;
  QAction *actionViewStatusBar;
  QAction *actionViewFullScreen;
  QAction *actionLayoutContinuous;
  QAction *actionLayoutSideBySide;
  // permission
  bool printingAllowed;
  bool savingAllowed;
  // fullscreen stuff
  Saved fsSavedNormal;
  Saved fsSavedFullScreen;
  Qt::WindowStates fsWindowState;
};


#endif
/* -------------------------------------------------------------
   Local Variables:
   c++-font-lock-extra-types: ( "\\sw+_t" "[A-Z]\\sw*[a-z]\\sw*" )
   End:
   ------------------------------------------------------------- */
