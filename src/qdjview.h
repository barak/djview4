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


class QAction;
class QActionGroup;
class QCloseEvent;
class QComboBox;
class QDockWidget;
class QFileDialog;
class QLabel;
class QMenu;
class QMenuBar;
class QStackedLayout;
class QStatusBar;
class QStringList;
class QToolBar;

class QDjViewErrorDialog;
class QDjViewInfoDialog;
class QDjViewMetaDialog;


class QDjView : public QMainWindow
{
  Q_OBJECT

 public:
  enum ViewerMode {
    EMBEDDED_PLUGIN = 0,
    FULLPAGE_PLUGIN = 1,
    STANDALONE      = 2
  };

  QDjView(QDjVuContext &context, ViewerMode mode=STANDALONE, QWidget *parent=0);
  
  QDjVuWidget        *getDjVuWidget()       { return widget; }
  QDjViewErrorDialog *getErrorDialog()      { return errorDialog; }
  QDjVuDocument      *getDocument()         { return document; } 
  QString             getDocumentFileName() { return documentFileName; }
  QUrl                getDocumentUrl()      { return documentUrl; }
  QString             getShortFileName();

  QStringList  parseArgument(QString keyEqualValue);
  QStringList  parseArgument(QString key, QString value);
  void         parseCgiArguments(QUrl url);

  int         pageNum(void);
  QString     pageName(int pageno);
  QString     makeCaption(QString);
  QDjView    *copyWindow(void);
  bool        saveTextFile(QString text, QString filename=QString());
  bool        saveImageFile(QImage image, QString filename=QString());

public slots:
  bool  open(QString filename);
  bool  open(QUrl url);
  void  closeDocument();
  void  goToPage(int pageno);
  void  goToPage(QString name, int from=-1);
  void  addToErrorDialog(QString message);
  void  raiseErrorDialog(QMessageBox::Icon icon, QString caption=QString());
  int   execErrorDialog (QMessageBox::Icon icon, QString caption=QString());
  void  setPageLabelText(QString);
  void  setMouseLabelText(QString);
  
signals:
  void  documentClosed();
  void  documentOpened(QDjVuDocument*);

protected:
  friend class QDjViewInfoDialog;
  friend class QDjViewMetaDialog;

  typedef QDjVuWidget::Position Position;
  typedef QDjVuWidget::PageInfo PageInfo;
  typedef QDjViewPrefs::Options Options;
  typedef QDjViewPrefs::Tools Tools;
  typedef QDjViewPrefs::Saved Saved;

  void     open(QDjVuDocument *document);
  void     fillToolBar(QToolBar *toolBar);
  void     fillZoomCombo(QComboBox *zoomCombo);
  void     fillModeCombo(QComboBox *modeCombo);
  void     fillPageCombo(QComboBox *pageCombo, QString format=QString());
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
  void docinfo();
  void errorCondition(int);
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
  void performAbout(void);
  void performNew(void);
  void performOpen(void);
  void performInformation(void);
  void performMetadata(void);
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
  QDjViewErrorDialog *errorDialog;
  QDjViewInfoDialog  *infoDialog;
  QDjViewMetaDialog  *metaDialog;
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
  // document data
  QDjVuContext           &djvuContext;
  QDjVuDocument          *document;
  QString                 documentFileName;
  QUrl                    documentUrl;
  QList<ddjvu_fileinfo_t> documentPages;
  // delayed settings
  int     pendingPageNo;
  QString pendingPageName;
  QRect   pendingHilite;
  QString pendingSearch;
  // delayed updates
  bool  needToUpdateActions;
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
  QAction *actionExport;
  QAction *actionPrint;
  QAction *actionSearch;
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
