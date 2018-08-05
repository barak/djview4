//C-  -*- C++ -*-
//C- -------------------------------------------------------------------
//C- DjView4
//C- Copyright (c) 2006-  Leon Bottou
//C-
//C- This software is subject to, and may be distributed under, the
//C- GNU General Public License, either version 2 of the license,
//C- or (at your option) any later version. The license should have
//C- accompanied the software or you may obtain a copy of the license
//C- from the Free Software Foundation at http://www.fsf.org .
//C-
//C- This program is distributed in the hope that it will be useful,
//C- but WITHOUT ANY WARRANTY; without even the implied warranty of
//C- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//C- GNU General Public License for more details.
//C-  ------------------------------------------------------------------

#ifndef QDJVIEW_H
#define QDJVIEW_H

#if AUTOCONF
# include "config.h"
#endif

#include <Qt>
#include <QDateTime>
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
class QDragEnterEvent;
class QDragMoveEvent;
class QDropEvent;
class QFileDialog;
class QFileSystemWatcher;
class QLabel;
class QMenu;
class QMenuBar;
class QShortcut;
class QSettings;
class QStackedLayout;
class QStatusBar;
class QStringList;
class QTimer;
class QTabWidget;
class QToolBar;



#if DDJVUAPI_VERSION < 18
# error "DDJVUAPI_VERSION>=18 is required !"
#endif

#if QT_VERSION < 0x40200
# error "QT_VERSION>=0x40200 is required !"
#endif


class QDjView : public QMainWindow
{
  Q_OBJECT

 public:
  
  
  enum ViewerMode {
    EMBEDDED_PLUGIN       = 0,
    FULLPAGE_PLUGIN       = 1,
    STANDALONE            = 2,
    STANDALONE_FULLSCREEN = 3,
    STANDALONE_SLIDESHOW  = 4
  };
  
  QDjView(QDjVuContext &context, 
          ViewerMode mode=STANDALONE, 
          QWidget *parent=0);
  
  ~QDjView();

  QDjVuContext       &getDjVuContext()      { return djvuContext; }
  QDjVuWidget        *getDjVuWidget()       { return widget; }
  QDjViewErrorDialog *getErrorDialog()      { return errorDialog; }
  QDjVuDocument      *getDocument()         { return document; } 
  QString             getDocumentFileName() { return documentFileName; }
  QUrl                getDocumentUrl()      { return documentUrl; }
  QUrl                getDecoratedUrl();
  QString             getShortFileName();
  ViewerMode          getViewerMode()       { return viewerMode; }

  QString      getArgument(QString key);
  QStringList  parseArgument(QString key, QString val);
  QStringList  parseArgument(QString keyEqualVal);
  void         parseDjVuCgiArguments(QUrl url);
  static QUrl  removeDjVuCgiArguments(QUrl url);

  int         pageNum(void);
  QString     pageName(int pageno, bool titleonly=false);
  int         pageNumber(QString name, int from = -1);
  QDjView    *copyWindow(bool openDocument=true);
  int         visibleSideBarTabs();
  int         hiddenSideBarTabs();
  bool        saveTextFile(QString text, QString filename=QString());
  bool        saveImageFile(QImage image, QString filename=QString());
  bool        startBrowser(QUrl url);
  void        fillZoomCombo(QComboBox *zoomCombo);
  void        fillModeCombo(QComboBox *modeCombo);
  void        fillPageCombo(QComboBox *pageCombo);

public slots:
  bool  open(QString filename);
  bool  open(QUrl url, bool inNewWindow=false, bool maybeInBrowser=false);
  void  open(QDjVuDocument *document, QUrl url = QUrl());
  void  closeDocument();
  void  reloadDocument();
  void  goToPage(int pageno);
  void  goToPage(QString name, int from=-1);
  void  goToPosition(QString pagename, double px, double py);
  void  goToLink(QString link, QString target=QString(), int from=-1);
  void  addToErrorDialog(QString message);
  void  raiseErrorDialog(QMessageBox::Icon icon, QString caption=QString());
  int   execErrorDialog (QMessageBox::Icon icon, QString caption=QString());
  void  setPageLabelText(QString s = QString());
  void  setMouseLabelText(QString s = QString());
  void  statusMessage(QString s = QString());
  bool  showSideBar(Qt::DockWidgetAreas areas, int tabs);
  bool  showSideBar(QString args, QStringList &errors);
  bool  showSideBar(bool);
  void  showFind(void);
  void  print(void);
  void  saveAs(void);
  void  exportAs(void);
  void  find(QString find = QString());
  void  saveSession(QSettings *s);
  void  restoreSession(QSettings *s);
  bool  setViewerMode(ViewerMode);
  void  setSlideShowDelay(int delay);
  
signals:
  void  documentClosed(QDjVuDocument *doc);
  void  documentOpened(QDjVuDocument *doc);
  void  documentReady(QDjVuDocument *doc);
  void  pluginStatusMessage(QString message = QString());
  void  pluginGetUrl(QUrl url, QString target);
  void  pluginOnChange();

protected:
  typedef QDjVuWidget::Position Position;
  typedef QDjVuWidget::PageInfo PageInfo;
  typedef QDjViewPrefs::Options Options;
  typedef QDjViewPrefs::Tools Tools;
  typedef QDjViewPrefs::Saved Saved;

  void     fillToolBar(QToolBar *toolBar);
  QAction *makeAction(QString text);
  QAction *makeAction(QString text, Qt::ShortcutContext sc);
  QAction *makeAction(QString text, bool value);
  void     createActions(void);
  void     createMenus(void);
  void     createWhatsThis(void);
  Saved   *getSavedPrefs(void);
  Saved   *getSavedConfig(ViewerMode mode);
  void     enableContextMenu(bool);
  void     enableScrollBars(bool);
  void     applyOptions(bool remember=true);
  void     updateOptions(void);
  void     applySaved(Saved *saved);
  void     updateSaved(Saved *saved);
  void     applyPreferences(void);
  void     updatePreferences(void);
  void     parseToolBarOption(QString option, QStringList &errors);
  bool     warnAboutPrintingRestrictions();
  bool     warnAboutSavingRestrictions();
  void     restoreRecentDocument(QUrl);

  virtual bool eventFilter(QObject *watched, QEvent *event);
  virtual void closeEvent(QCloseEvent *event);
  virtual void dragEnterEvent(QDragEnterEvent *event);
  virtual void dragMoveEvent(QDragMoveEvent *event);
  virtual void dropEvent(QDropEvent *event);

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
  void updateTextLabel(void);
  void modeComboActivated(int);
  void zoomComboActivated(int);
  void zoomComboEdited(void);
  void pageComboActivated(int);
  void pageComboEdited(void);
  void preferencesChanged(void);
  void performAbout(void);
  void performNew(void);
  void performOpen(void);
  void performOpenLocation(void);
  void performInformation(void);
  void performMetadata(void);
  void performPreferences(void);
  void performRotation(void);
  void performZoom(void);
  void performSelect(bool);
  void performViewFullScreen(bool);
  void performViewSlideShow(bool);
  void performEscape();
  void performGoPage();
  void addRecent(QUrl);
  void fillRecent();
  void openRecent();
  void clearRecent();
  void performUndo();
  void performRedo();
  void saveUndoData();
  void performCopyUrl();
  void performCopyOutline();
  void performCopyAnnotation();
  void maybeReloadDocument();
  void sslWhiteList(QString why, bool &okay);
  void authRequired(QString why, QString &user, QString &pass);
  void slideShowTimeout(bool reset=false);

protected:
  // mode
  ViewerMode   viewerMode;
  // preferences
  QDjViewPrefs  *prefs;
  Options        options;
  Tools          tools;
  Tools          toolsCached;
  // dialogs
  QPointer<QDjViewErrorDialog>  errorDialog;
  QPointer<QDjViewInfoDialog>   infoDialog;
  QPointer<QDjViewMetaDialog>   metaDialog;
  QPointer<QDjViewSaveDialog>   saveDialog;
  QPointer<QDjViewExportDialog> exportDialog;
  QPointer<QDjViewPrintDialog>  printDialog;
  // widgets
  QLabel             *splash;
  QDjVuWidget        *widget;
  QStackedLayout     *layout;
  QMenu              *contextMenu;
  QMenu              *recentMenu;
  QMenuBar           *menuBar;
  QStatusBar         *statusBar;
  QLabel             *pageLabel;
  QLabel             *mouseLabel;
  QLabel             *textLabel;
  QToolBar           *toolBar;
  QComboBox          *modeCombo;
  QComboBox          *zoomCombo;
  QComboBox          *pageCombo;
  QDjViewOutline     *outlineWidget;
  QDockWidget        *outlineDock;
  QDjViewThumbnails  *thumbnailWidget;
  QDockWidget        *thumbnailDock;
  QDjViewFind        *findWidget;
  QDockWidget        *findDock;
  // document data
  QDjVuContext           &djvuContext;
  QDjVuDocument          *document;
  QString                 documentFileName;
  QUrl                    documentUrl;
  QList<ddjvu_fileinfo_t> documentPages;
  QDateTime               documentModified;
  bool                    hasNumericalPageTitle;
  // delayed settings
  typedef QPair<QString,QString> StringPair;
  QString           pendingPage;
  QList<double>     pendingPosition;
  QList<StringPair> pendingHilite;
  QString           pendingFind;
  // delayed updates
  bool  updateActionsScheduled;
  bool  performPendingScheduled;
  QTimer *textLabelTimer;
  QRect   textLabelRect;
  // action lists
  QActionGroup *zoomActionGroup;
  QActionGroup *modeActionGroup;
  QActionGroup *rotationActionGroup;
  // all actions
  QObjectList allActions;
  QAction *actionNew;
  QAction *actionOpen;
  QAction *actionOpenLocation;
  QAction *actionClose;
  QAction *actionQuit;
  QAction *actionSave;
  QAction *actionExport;
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
  QAction *actionDisplayHiddenText;
  QAction *actionInvertLuminance;
  QAction *actionPreferences;
  QAction *actionViewToolBar;
  QAction *actionViewSideBar;
  QAction *actionViewStatusBar;
  QAction *actionViewFullScreen;
  QAction *actionViewSlideShow;
  QAction *actionLayoutContinuous;
  QAction *actionLayoutSideBySide;
  QAction *actionLayoutCoverPage;
  QAction *actionLayoutRightToLeft;
  QShortcut *shortcutEscape;
  QShortcut *shortcutGoPage;
  QAction *actionCopyUrl;
  QAction *actionCopyOutline;
  QAction *actionCopyAnnotation;
  // permission
  bool printingAllowed;
  bool savingAllowed;
  // mode changes
  Saved fsSavedNormal;
  Saved fsSavedFullScreen;
  Saved fsSavedSlideShow;
  QByteArray savedDockState;
  QTimer *slideShowTimer;
  int    slideShowDelay;
  int    slideShowCounter;
  // undo/redo
  class UndoRedo 
  {
  public:
    UndoRedo();
    void clear();
    void set(QDjView *djview);
    void apply(QDjView *djview);
    bool changed(const QDjVuWidget *widget) const;
  protected:
    bool valid;
    QPoint hotSpot;
    QDjVuWidget::Position position;
    int rotation;
    int zoom;
  };
  UndoRedo here;
  QTimer *undoTimer;
  QList<UndoRedo> undoList;
  QList<UndoRedo> redoList;
  // netopen
  class NetOpen;
};

#endif

/* -------------------------------------------------------------
   Local Variables:
   c++-font-lock-extra-types: ( "\\sw+_t" "[A-Z]\\sw*[a-z]\\sw*" )
   End:
   ------------------------------------------------------------- */
