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

#include <string.h>
#include <libdjvu/miniexp.h>
#include <libdjvu/ddjvuapi.h>

#include <QDebug>

#include <QObject>
#include <QCoreApplication>
#include <QApplication>
#include <QMainWindow>
#include <QDockWidget>
#include <QMenuBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QStackedLayout>
#include <QComboBox>
#include <QFrame>
#include <QLabel>
#include <QToolBar>
#include <QAction>
#include <QActionGroup>
#include <QIcon>
#include <QFont>
#include <QMenu>
#include <QString>
#include <QRegExp>
#include <QList>
#include <QMessageBox>
#include <QScrollBar>
#include <QPalette>
#include <QRegExp>

#include "qdjvu.h"
#include "qdjvuhttp.h"
#include "qdjvuwidget.h"
#include "qdjview.h"
#include "qdjviewprefs.h"
#include "qdjviewdialogs.h"








// ----------------------------------------
// ACTIONS


QAction *
QDjView::makeAction(QString text)
{
  return new QAction(text, this);
}

QAction *
QDjView::makeAction(QString text, bool value)
{
  QAction *action = new QAction(text, this);
  action->setCheckable(true);
  action->setChecked(value);
  return action;
}

static inline QAction * 
operator<<(QAction *action, QIcon icon)
{
  action->setIcon(icon);
  return action;
}

static inline QAction * 
operator<<(QAction *action, QActionGroup &group)
{
  action->setActionGroup(&group);
  return action;
}

static inline QAction * 
operator<<(QAction *action, QKeySequence shortcut)
{
  action->setShortcut(shortcut);
  return action;
}

static inline QAction * 
operator<<(QAction *action, QString string)
{
  if (action->statusTip().isEmpty())
    action->setStatusTip(string);
  else if (action->toolTip().isEmpty())
    action->setToolTip(string);
  return action;
}

void
QDjView::createActions()
{
  // Create action groups
  zoomActionGroup = new QActionGroup(this);
  modeActionGroup = new QActionGroup(this);
  rotationActionGroup  = new QActionGroup(this);
  
  // Create actions
  actionNew = makeAction(tr("&New", "File|New"))
    << QKeySequence(tr("Ctrl+N", "File|New"))
    << QIcon(":/images/icon_new.png")
    << tr("Create a new DjView window.");
  actionOpen = makeAction(tr("&Open", "File|Open"))
    << QKeySequence(tr("Ctrl+O", "File|Open"))
    << QIcon(":/images/icon_open.png")
    << tr("Open a DjVu document.");
  actionClose = makeAction(tr("&Close", "File|Close"))
    << QKeySequence(tr("Ctrl+W", "File|Close"))
    << QIcon(":/images/icon_close.png")
    << tr("Close this window.");
  actionQuit = makeAction(tr("&Quit", "File|Quit"))
    << QKeySequence(tr("Ctrl+Q", "File|Quit"))
    << QIcon(":/images/icon_quit.png")
    << tr("Close all windows and quit the application.");
  actionSave = makeAction(tr("&Save", "File|Save"))
    << QKeySequence(tr("Ctrl+S", "File|Save"))
    << QIcon(":/images/icon_save.png")
    << tr("Save the DjVu document.");
  actionExport = makeAction(tr("&Export", "File|Export"))
    << tr("Export the DjVu document under another format.");
  actionPrint = makeAction(tr("&Print", "File|Print"))
    << QKeySequence(tr("Ctrl+P", "File|Print"))
    << QIcon(":/images/icon_print.png")
    << tr("Print the DjVu document.");
  actionSearch = makeAction(tr("&Find"))
    << QKeySequence(tr("Ctrl+F", "Find"))
    << QIcon(":/images/icon_find.png")
    << tr("Find text in the DjVu document.");
  actionZoomIn = makeAction(tr("Zoom &In"))
    << QIcon(":/images/icon_zoomin.png")
    << tr("Increase the magnification.");
  actionZoomOut = makeAction(tr("Zoom &Out"))
    << QIcon(":/images/icon_zoomout.png")
    << tr("Decrease the magnification.");
  actionZoomFitWidth = makeAction(tr("Fit &Width", "Zoom|Fitwith"),false)
    << tr("Set magnification to fit page width.")
    << *zoomActionGroup;
  actionZoomFitPage = makeAction(tr("Fit &Page", "Zoom|Fitpage"),false)
    << tr("Set magnification to fit page.")
    << *zoomActionGroup;
  actionZoomOneToOne = makeAction(tr("One &to one", "Zoom|1:1"),false)
    << tr("Set full resolution magnification.")
    << *zoomActionGroup;
  actionZoom300 = makeAction(tr("&300%", "Zoom|300%"), false)
    << tr("Magnify 300%")
    << *zoomActionGroup;
  actionZoom200 = makeAction(tr("&200%", "Zoom|200%"), false)
    << tr("Magnify 300%")
    << *zoomActionGroup;
  actionZoom150 = makeAction(tr("150%", "Zoom|150%"), false)
    << tr("Magnify 300%")
    << *zoomActionGroup;
  actionZoom100 = makeAction(tr("&100%", "Zoom|100%"), false)
    << tr("Magnify 300%")
    << *zoomActionGroup;
  actionZoom75 = makeAction(tr("&75%", "Zoom|75%"), false)
    << tr("Magnify 300%")
    << *zoomActionGroup;
  actionZoom50 = makeAction(tr("&50%", "Zoom|50%"), false)
    << tr("Magnify 300%")
    << *zoomActionGroup;
  actionNavFirst = makeAction(tr("&First Page"))
    << QIcon(":/images/icon_first.png")
    << tr("Jump to first document page.");
  actionNavNext = makeAction(tr("&Next Page"))
    << QIcon(":/images/icon_next.png")
    << tr("Jump to next document page.");
  actionNavPrev = makeAction(tr("&Previous Page"))
    << QIcon(":/images/icon_prev.png")
    << tr("Jump to previous document page.");
  actionNavLast = makeAction(tr("&Last Page"))
    << QIcon(":/images/icon_last.png")
    << tr("Jump to last document page.");
  actionBack = makeAction(tr("&Backward"))
    << QIcon(":/images/icon_back.png")
    << tr("Backward in history.");
  actionForw = makeAction(tr("&Forward"))
    << QIcon(":/images/icon_forw.png")
    << tr("Forward in history.");
  actionRotateLeft = makeAction(tr("Rotate &Left"))
    << QIcon(":/images/icon_rotateleft.png")
    << tr("Rotate page image counter-clockwise.");
  actionRotateRight = makeAction(tr("Rotate &Right"))
    << QIcon(":/images/icon_rotateright.png")
    << tr("Rotate page image clockwise.");
  actionRotate0 = makeAction(tr("Rotate &0\260"), false)
    << tr("Set natural page orientation.")
    << *rotationActionGroup;
  actionRotate90 = makeAction(tr("Rotate &90\260"), false)
    << tr("Turn page on its left side.")
    << *rotationActionGroup;
  actionRotate180 = makeAction(tr("Rotate &180\260"), false)
    << tr("Turn page upside-down.")
    << *rotationActionGroup;
  actionRotate270 = makeAction(tr("Rotate &270\260"), false)
    << tr("Turn page on its right side.")
    << *rotationActionGroup;
  actionPageInfo = makeAction(tr("Page &Information"))
    << tr("Show DjVu encoding information for the current page.");
  actionDocInfo = makeAction(tr("&Document Information"))
    << tr("Show DjVu encoding information for the document.");
  actionAbout = makeAction(tr("&About DjView"))
    << QIcon(":/images/icon_djvu.png")
    << tr("Show information about this program.");
  actionDisplayColor = makeAction(tr("&Color", "Display|Color"), false)
    << tr("Display document in full colors.")
    << *modeActionGroup;
  actionDisplayBW = makeAction(tr("&Mask", "Display|BW"), false)
    << tr("Only display the document black&white stencil.")
    << *modeActionGroup;
  actionDisplayForeground = makeAction(tr("&Foreground", "Display|Foreground"), false)
    << tr("Only display the foreground layer.")
    << *modeActionGroup;
  actionDisplayBackground = makeAction(tr("&Background", "Display|Background"), false)
    << tr("Only display the background layer.")
    << *modeActionGroup;
  actionPreferences = makeAction(tr("Prefere&nces")) 
    << QIcon(":/images/icon_prefs.png")
    << tr("Show the preferences dialog.");
  actionViewToolBar = makeAction(tr("Show &tool bar"),true)
    << tr("Show/hide the standard tool bar.");
  actionViewStatusbar = makeAction(tr("Show &status bar"), true)
    << tr("Show/hide the status bar.");
  actionViewSidebar = makeAction(tr("Show s&ide bar"),true)
    << QKeySequence("F9")
    << QIcon(":/images/icon_sidebar.png")
    << tr("Show/hide the side bar.");
  actionViewFullScreen = makeAction(tr("&Full Screen","View|FullScreen"), false)
    << QKeySequence("F11")
    << QIcon(":/images/icon_fullscreen.png")
    << tr("Toggle full screen mode.");
  actionLayoutContinuous = makeAction(tr("&Continuous","Layout"), false)
    << QIcon(":/images/icon_continuous.png")
    << QKeySequence("F2")
    << tr("Toggle continuous layout mode.");
  actionLayoutSideBySide = makeAction(tr("&Side by side","Layout"), false)
    << QIcon(":/images/icon_sidebyside.png")
    << QKeySequence("F3")
    << tr("Toggle side-by-side layout mode.");
  actionLayoutPageSettings = makeAction(tr("Obey &annotations"), false)
    << tr("Obey/ignore layout instructions from the page data.");

  
  // temporary connections
  connect(actionQuit, SIGNAL(triggered()),
          QCoreApplication::instance(), SLOT(quit()));
  connect(actionClose, SIGNAL(triggered()),
          this, SLOT(close()));
  connect(actionLayoutContinuous,SIGNAL(toggled(bool)),
          widget, SLOT(setContinuous(bool)));
  connect(actionLayoutSideBySide,SIGNAL(toggled(bool)),
          widget, SLOT(setSideBySide(bool)));
  
}

void
QDjView::updateActions()
{
}



// ----------------------------------------
// MENUS

void
QDjView::createMenus()
{

  // Layout main menu
  QMenu *fileMenu = menuBar->addMenu(tr("&File", "File|"));
  if (viewerMode == STANDALONE)
    fileMenu->addAction(actionNew);
  if (viewerMode == STANDALONE)
    fileMenu->addAction(actionOpen);
  if (viewerMode == STANDALONE)
    fileMenu->addSeparator();
  fileMenu->addAction(actionSave);
  fileMenu->addAction(actionExport);
  fileMenu->addAction(actionPrint);
  if (viewerMode == STANDALONE)
    fileMenu->addSeparator();
  if (viewerMode == STANDALONE)
    fileMenu->addAction(actionClose);
  if (viewerMode == STANDALONE)
    fileMenu->addAction(actionQuit);
  QMenu *editMenu = menuBar->addMenu(tr("&Edit", "Edit|"));
  editMenu->addAction(actionSearch);
  editMenu->addSeparator();
  editMenu->addAction(actionDocInfo);
  editMenu->addAction(actionPageInfo);
  QMenu *viewMenu = menuBar->addMenu(tr("&View", "View|"));
  QMenu *zoomMenu = viewMenu->addMenu(tr("&Zoom","View|Zoom"));
  zoomMenu->addAction(actionZoomIn);
  zoomMenu->addAction(actionZoomOut);
  zoomMenu->addSeparator();
  zoomMenu->addAction(actionZoomOneToOne);
  zoomMenu->addAction(actionZoomFitWidth);
  zoomMenu->addAction(actionZoomFitPage);
  zoomMenu->addSeparator();
  zoomMenu->addAction(actionZoom300);
  zoomMenu->addAction(actionZoom200);
  zoomMenu->addAction(actionZoom150);
  zoomMenu->addAction(actionZoom100);
  zoomMenu->addAction(actionZoom75);
  zoomMenu->addAction(actionZoom50);
  QMenu *rotationMenu = viewMenu->addMenu(tr("&Rotate","View|Rotate"));
  rotationMenu->addAction(actionRotateLeft);
  rotationMenu->addAction(actionRotateRight);
  rotationMenu->addSeparator();
  rotationMenu->addAction(actionRotate0);
  rotationMenu->addAction(actionRotate90);
  rotationMenu->addAction(actionRotate180);
  rotationMenu->addAction(actionRotate270);
  QMenu *modeMenu = viewMenu->addMenu(tr("&Display","View|Display"));
  modeMenu->addAction(actionDisplayColor);
  modeMenu->addAction(actionDisplayBW);
  modeMenu->addAction(actionDisplayForeground);
  modeMenu->addAction(actionDisplayBackground);
  viewMenu->addSeparator();
  viewMenu->addAction(actionLayoutContinuous);
  viewMenu->addAction(actionLayoutSideBySide);
  if (viewerMode == STANDALONE)
    viewMenu->addSeparator();
  if (viewerMode == STANDALONE)
    viewMenu->addAction(actionViewFullScreen);
  QMenu *gotoMenu = menuBar->addMenu(tr("&Go", "Go|"));
  gotoMenu->addAction(actionNavFirst);
  gotoMenu->addAction(actionNavPrev);
  gotoMenu->addAction(actionNavNext);
  gotoMenu->addAction(actionNavLast);
  QMenu *settingsMenu = menuBar->addMenu(tr("&Settings", "Settings|"));
  settingsMenu->addAction(actionViewSidebar);
  settingsMenu->addAction(actionViewToolBar);
  settingsMenu->addAction(actionViewStatusbar);
  settingsMenu->addSeparator();
  settingsMenu->addAction(actionLayoutPageSettings);
  settingsMenu->addSeparator();
  settingsMenu->addAction(actionPreferences);
  QMenu *helpMenu = menuBar->addMenu(tr("&Help", "Help|"));
  helpMenu->addAction(actionAbout);

  // Layout context menu
  gotoMenu = contextMenu->addMenu(tr("&Go", "Go|"));
  gotoMenu->addAction(actionNavFirst);
  gotoMenu->addAction(actionNavPrev);
  gotoMenu->addAction(actionNavNext);
  gotoMenu->addAction(actionNavLast);
  zoomMenu = contextMenu->addMenu(tr("&Zoom","View|Zoom"));
  zoomMenu->addAction(actionZoomIn);
  zoomMenu->addAction(actionZoomOut);
  zoomMenu->addSeparator();
  zoomMenu->addAction(actionZoomOneToOne);
  zoomMenu->addAction(actionZoomFitWidth);
  zoomMenu->addAction(actionZoomFitPage);
  zoomMenu->addSeparator();
  zoomMenu->addAction(actionZoom300);
  zoomMenu->addAction(actionZoom200);
  zoomMenu->addAction(actionZoom150);
  zoomMenu->addAction(actionZoom100);
  zoomMenu->addAction(actionZoom75);
  zoomMenu->addAction(actionZoom50);
  rotationMenu = contextMenu->addMenu(tr("&Rotate","View|Rotate"));
  rotationMenu->addAction(actionRotateLeft);
  rotationMenu->addAction(actionRotateRight);
  rotationMenu->addSeparator();
  rotationMenu->addAction(actionRotate0);
  rotationMenu->addAction(actionRotate90);
  rotationMenu->addAction(actionRotate180);
  rotationMenu->addAction(actionRotate270);
  modeMenu = contextMenu->addMenu(tr("&Display","View|Display"));
  modeMenu->addAction(actionDisplayColor);
  modeMenu->addAction(actionDisplayBW);
  modeMenu->addAction(actionDisplayForeground);
  modeMenu->addAction(actionDisplayBackground);
  contextMenu->addSeparator();
  contextMenu->addAction(actionLayoutContinuous);
  contextMenu->addAction(actionLayoutSideBySide);
  contextMenu->addSeparator();
  contextMenu->addAction(actionSearch);
  contextMenu->addSeparator();
  contextMenu->addAction(actionSave);
  contextMenu->addAction(actionExport);
  contextMenu->addAction(actionPrint);
  contextMenu->addSeparator();
  contextMenu->addAction(actionPreferences);
  contextMenu->addAction(actionAbout);
}



// ----------------------------------------
// TOOLBAR


void
QDjView::updateToolBar(void)
{
  bool wasHidden = toolBar->isHidden();
  toolBar->hide();
  toolBar->clear();

  modeCombo->hide();
  pageCombo->hide();
  zoomCombo->hide();
  
  if (viewerMode == STANDALONE) 
    {
      if (tools & QDjViewPrefs::TOOL_NEW)
        toolBar->addAction(actionNew);
      if (tools & QDjViewPrefs::TOOL_OPEN)
        toolBar->addAction(actionOpen);
    }
  if (tools & QDjViewPrefs::TOOL_SAVE)
    {
      toolBar->addAction(actionSave);
    }
  if (tools & QDjViewPrefs::TOOL_PRINT)
    {
      toolBar->addAction(actionPrint);
    }
  if (tools & QDjViewPrefs::TOOL_SEARCH)
    {
      toolBar->addAction(actionSearch);
    }
  if (tools & QDjViewPrefs::TOOL_LAYOUT)
    {
      toolBar->addAction(actionLayoutContinuous);
      toolBar->addAction(actionLayoutSideBySide);
    }
  if ((tools & QDjViewPrefs::TOOL_MODECOMBO) ||
      (tools & QDjViewPrefs::TOOL_MODEBUTTONS) )
    {
      modeCombo->show();
      toolBar->addWidget(modeCombo);
    }
  if (tools & QDjViewPrefs::TOOL_ZOOMCOMBO)
    {
      zoomCombo->show();
      toolBar->addWidget(zoomCombo);
    }
  if (tools & QDjViewPrefs::TOOL_ZOOMBUTTONS)
    {
      toolBar->addAction(actionZoomIn);
      toolBar->addAction(actionZoomOut);
    }
  if (tools & QDjViewPrefs::TOOL_ROTATE)
    {
      toolBar->addAction(actionRotateRight);
      toolBar->addAction(actionRotateLeft);
    }
  if (tools & QDjViewPrefs::TOOL_PAGECOMBO)
    {
      pageCombo->show();
      toolBar->addWidget(pageCombo);
    }
  if (tools & QDjViewPrefs::TOOL_FIRSTLAST)
    {
      toolBar->addAction(actionNavFirst);
    }
  if (tools & QDjViewPrefs::TOOL_PREVNEXT)
    {
      toolBar->addAction(actionNavPrev);
      toolBar->addAction(actionNavNext);
    }
  if (tools & QDjViewPrefs::TOOL_FIRSTLAST)
    {
      toolBar->addAction(actionNavLast);
    }
  if (tools & QDjViewPrefs::TOOL_BACKFORW)
    {
      toolBar->addAction(actionBack);
      toolBar->addAction(actionForw);
    }

  toolBar->setVisible(!wasHidden);
}


// ----------------------------------------
// CONSTRUCTOR


QDjView::QDjView(QDjVuContext &context, ViewerMode mode, QWidget *parent)
  : QMainWindow(parent),
    viewerMode(mode),
    djvuContext(context),
    document(0)
{
  // obtain preferences
  generalPrefs = QDjViewPrefs::create();
  if (viewerMode == EMBEDDED_PLUGIN)
    appearancePrefs = &generalPrefs->forEmbeddedPlugin;
  else if (viewerMode == FULLPAGE_PLUGIN)
    appearancePrefs = &generalPrefs->forFullPagePlugin;
  else
    appearancePrefs = &generalPrefs->forStandalone;
  options = appearancePrefs->options;
  tools = generalPrefs->tools;
  
  // create dialogs
  // - error dialog
  errorDialog = new QDjViewDialogError(this);
  
  // create widgets
  QWidget *central = new QWidget(this);
  // - djvu widget
  widget = new QDjVuWidget(central);
  widget->setFrameShape(QFrame::Panel);
  widget->setFrameShadow(QFrame::Sunken);
  widget->viewport()->installEventFilter(this);
  connect(widget, SIGNAL(errorCondition(int)),
          this, SLOT(errorCondition(int)));
  connect(widget, SIGNAL(error(QString,QString,int)),
          errorDialog, SLOT(error(QString,QString,int)));
  connect(widget, SIGNAL(layoutChanged()),
          this, SLOT(layoutChanged()));
  connect(widget, SIGNAL(pageChanged(int)),
          this, SLOT(pageChanged(int)));
  connect(widget, SIGNAL(pointerPosition(const Position&,const PageInfo&)),
          this, SLOT(pointerPosition(const Position&,const PageInfo&)));
  connect(widget, SIGNAL(pointerEnter(const Position&,miniexp_t)),
          this, SLOT(pointerEnter(const Position&,miniexp_t)));
  connect(widget, SIGNAL(pointerLeave(const Position&,miniexp_t)),
          this, SLOT(pointerLeave(const Position&,miniexp_t)));
  connect(widget, SIGNAL(pointerClick(const Position&,miniexp_t)),
          this, SLOT(pointerClick(const Position&,miniexp_t)));
  connect(widget, SIGNAL(pointerSelect(const QPoint&,const QRect&)),
          this, SLOT(pointerSelect(const QPoint&,const QRect&)));

  // - splash screen
  splash = new QLabel(central);
  splash->setFrameShape(QFrame::Box);
  splash->setFrameShadow(QFrame::Sunken);
  splash->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
  splash->setPixmap(QPixmap(":/images/splash.png"));
  QPalette palette = splash->palette();
  palette.setColor(QPalette::Window, Qt::white);
  splash->setPalette(palette);
#if QT_VERSION >= 0x040100
  splash->setAutoFillBackground(true);
#endif
  // - central layout
  layout = new QStackedLayout(central);
  layout->addWidget(widget);
  layout->addWidget(splash);
  layout->setCurrentWidget(splash);
  setCentralWidget(central);
  // - context menu
  contextMenu = new QMenu(this);
  widget->setContextMenu(contextMenu);
  // - menubar
  menuBar = new QMenuBar(this);
  setMenuBar(menuBar);
  // - statusbar
  statusBar = new QStatusBar(this);
  QFont font = QApplication::font();
  font.setPointSize(font.pointSize() - 3);
  QFontMetrics metric(font);
  pageLabel = new QLabel(statusBar);
  pageLabel->setFont(font);
  pageLabel->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
  pageLabel->setFrameStyle(QFrame::Panel);
  pageLabel->setFrameShadow(QFrame::Sunken);
  pageLabel->setMinimumWidth(metric.width(" 88 (8888x8888@888dpi) ")); 
  statusBar->addPermanentWidget(pageLabel);
  mouseLabel = new QLabel(statusBar);
  mouseLabel->setFont(font);
  mouseLabel->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
  mouseLabel->setFrameStyle(QFrame::Panel);
  mouseLabel->setFrameShadow(QFrame::Sunken);
  mouseLabel->setMinimumWidth(metric.width(" x=888 y=888 "));
  statusBar->addPermanentWidget(mouseLabel);
  setStatusBar(statusBar);
  // - toolBar  
  toolBar = new QToolBar(this);
  toolBar->setObjectName("toolbar");
  toolBar->setWindowTitle("Toolbar");
  modeCombo = new QComboBox(toolBar);
  zoomCombo = new QComboBox(toolBar);
  pageCombo = new QComboBox(toolBar);
  addToolBar(toolBar);
  // - sidebar  
  sideBar = new QDockWidget(this);  // for now
  sideBar->setObjectName("sidebar");
  sideBar->setWindowTitle("Sidebar");
  addDockWidget(Qt::LeftDockWidgetArea, sideBar);
  
  // create actions
  createActions();
  createMenus();
  updateToolBar();
  updateActions();

  // setup
  setWindowTitle(tr("DjView"));
  setWindowIcon(QIcon(":/images/icon32_djvu.png"));
  if (QApplication::windowIcon().isNull())
    QApplication::setWindowIcon(windowIcon());
  
  // connections
}

QDjVuWidget *
QDjView::djvuWidget()
{
  return widget;
}

void 
QDjView::closeDocument()
{
  layout->setCurrentWidget(splash);
  if (document)
    disconnect(document, 0, this, 0);
  documentPages.clear();
  documentFileName.clear();
  documentUrl.clear();
  document = 0;
  widget->setDocument(0);
}

void 
QDjView::open(QDjVuDocument *doc, bool own)
{
  closeDocument();
  document = doc;
  widget->setDocument(document, own);
  docinfo();
  connect(document,SIGNAL(docinfo(void)), this, SLOT(docinfo(void)));
  layout->setCurrentWidget(widget);
}

bool
QDjView::open(QString filename)
{
  closeDocument();
  QDjVuDocument *doc = new QDjVuDocument(this);
  connect(doc, SIGNAL(error(QString,QString,int)),
          errorDialog, SLOT(error(QString,QString,int)));
  doc->setFileName(&djvuContext, filename);
  if (!doc->isValid())
    {
      raiseErrorDialog(QMessageBox::Critical,
                       tr("Open DjVu file ..."),
                       tr("Cannot open file '%1'.").arg(filename) );
      return false;
    }
  setWindowTitle(tr("Djview - %1").arg(filename));
  open(doc, true);
  documentFileName = filename;
  return true;
}

bool
QDjView::open(QUrl url)
{
  closeDocument();
  QDjVuHttpDocument *doc = new QDjVuHttpDocument(this);
  connect(doc, SIGNAL(error(QString,QString,int)),
          errorDialog, SLOT(error(QString,QString,int)));
  doc->setUrl(&djvuContext, url);
  if (!doc->isValid())
    {
      raiseErrorDialog(QMessageBox::Critical,
                       tr("Open DjVu document ..."),
                       tr("Cannot open URL '%1'.").arg(url.toString()) );
      return false;
    }
  setWindowTitle(tr("Djview - %1").arg(url.toString()));
  open(doc, true);
  documentUrl = url;
  return true;
}

void
QDjView::raiseErrorDialog(QMessageBox::Icon icon, 
                          QString caption, QString message)
{
  errorDialog->error(message, QString(), 0);
  errorDialog->prepare(icon, caption);
  errorDialog->show();
  errorDialog->raise();
}

int
QDjView::execErrorDialog(QMessageBox::Icon icon, 
                         QString caption, QString message)
{
  errorDialog->error(message, QString(), 0);
  errorDialog->prepare(icon, caption);
  return errorDialog->exec();
}


// -----------------------------------
// UTILITIES


QString 
QDjView::pageName(int pageno)
{
  if (pageno>=0 && pageno<documentPages.size())
    {
      ddjvu_fileinfo_t &info = documentPages[pageno];
      if (strcmp(info.name, info.title))
        return QString::fromUtf8(info.title);
    }
  return QString::number(pageno + 1);
}


// -----------------------------------
// EVENT FILTER


bool 
QDjView::eventFilter(QObject *watched, QEvent *event)
{
  switch(event->type())
    {
    case QEvent::Leave:
      if (watched == widget->viewport())
        {
          pageLabel->clear();
          mouseLabel->clear();
          statusBar->clearMessage();
          return false;
        }
      break;
    default:
      break;
    }
  return false;
}


// -----------------------------------
// PROTECTED SIGNALS


void 
QDjView::docinfo()
{
  if (document &&
      ddjvu_document_decoding_status(*document) == DDJVU_JOB_OK)
    {
      // Obtain information about pages.
      int n = ddjvu_document_get_filenum(*document);
      for (int i=0; i<n; i++)
        {
          ddjvu_fileinfo_t info;
          if (ddjvu_document_get_fileinfo(*document, i, &info) != DDJVU_JOB_OK)
            qWarning("Internal (docinfo): ddjvu_document_get_fileinfo failed.");
          if (info.type == 'P')
            documentPages << info;
        }
      if (documentPages.size() != ddjvu_document_get_pagenum(*document))
        qWarning("Internal (docinfo): inconsistent number of pages.");
      
      // Update actions
      updateActions();
    }
}


void 
QDjView::layoutChanged()
{
}

void 
QDjView::pageChanged(int pageno)
{
}

void 
QDjView::pointerPosition(const Position &pos, const PageInfo &page)
{
  // setup page label
  QLabel *p = pageLabel;
  p->setText(tr(" %1 (%2x%3@%4dpi) ").arg(pageName(pos.pageNo)).
             arg(page.width).arg(page.height).arg(page.dpi) );
  p->setMinimumWidth(qMax(p->minimumWidth(), p->sizeHint().width()));
  // setup mouse label
  QLabel *m = mouseLabel;
  if (pos.inPage)
    m->setText(tr(" x=%1 y=%2 ").arg(pos.posPage.x()).arg(pos.posPage.y()));
  else
  m->clear();
  m->setMinimumWidth(qMax(m->minimumWidth(), m->sizeHint().width()));
}

void 
QDjView::pointerEnter(const Position &pos, miniexp_t maparea)
{
  QString link = widget->linkUrl();
  if (link.isEmpty())
    return;
  if (link == "#+1")
    link = tr("Turn 1 page forward.");
  else if (link == "#-1")
    link = tr("Turn 1 page backward.");
  else if (link.contains(QRegExp("^#\\+\\d+$")))
    link = tr("Turn %1 pages forward.").arg(link.mid(2).toInt());
  else if (link.contains(QRegExp("^#-\\d+$")))
    link = tr("Turn %1 pages backward.").arg(link.mid(2).toInt());
  else if (link.startsWith("#="))
    link = tr("Go to page: %1.").arg(link.mid(2));
  else if (link.startsWith("#"))
    link = tr("Go to page: %1.").arg(link.mid(1));
  else
    link = tr("Link: %1.").arg(link);
  statusBar->showMessage(link);
}

void 
QDjView::pointerLeave(const Position &pos, miniexp_t maparea)
{
  statusBar->clearMessage();
}

void 
QDjView::pointerClick(const Position &pos, miniexp_t maparea)
{
}

void 
QDjView::pointerSelect(const QPoint &pointerPos, const QRect &rect)
{
}

void 
QDjView::errorCondition(int pageno)
{
  QString message;
  if (pageno >= 0)
    message = tr("Cannot decode page %1.").arg(pageno);
  else
    message = tr("Cannot decode document.");
  raiseErrorDialog(QMessageBox::Warning,
                   tr("Decoding DjVu document ..."),
                   message);
}



/* -------------------------------------------------------------
   Local Variables:
   c++-font-lock-extra-types: ( "\\sw+_t" "[A-Z]\\sw*[a-z]\\sw*" )
   End:
   ------------------------------------------------------------- */
