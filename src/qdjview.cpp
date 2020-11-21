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

#if AUTOCONF
# include "config.h"
#else
# define HAVE_STRING_H 1
# define HAVE_STRERROR 1
# ifndef WIN32
#  define HAVE_UNISTD_H 1
# endif
#endif

#if HAVE_STRING_H
# include <string.h>
#endif
#if HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <errno.h>

#include <libdjvu/miniexp.h>
#include <libdjvu/ddjvuapi.h>

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QBoxLayout>
#include <QClipboard>
#include <QCloseEvent>
#include <QComboBox>
#include <QCoreApplication>
#include <QDebug>
#include <QDialog>
#include <QDir>
#include <QDockWidget>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontMetrics>
#include <QBoxLayout>
#include <QFont>
#include <QFrame>
#include <QIcon>
#include <QImageWriter>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QObject>
#include <QPair>
#include <QPalette>
#include <QProcess>
#include <QRegExp>
#include <QRegExpValidator>
#include <QScrollBar>
#include <QSettings>
#include <QShortcut>
#include <QSlider>
#include <QStackedLayout>
#include <QStatusBar>
#include <QString>
#include <QStyle>
#include <QStyleOptionComboBox>
#include <QTextDocument>
#include <QTextStream>
#include <QTimer>
#include <QTabWidget>
#include <QToolBar>
#include <QUrl>
#include <QWhatsThis>
#if QT_VERSION >= 0x40200
# include <QDesktopServices>
#endif
#if QT_VERSION >= 0x50000
# include <QUrlQuery>
#endif

#if QT_VERSION >= 0x50E00
# define tr8 tr
# define swidth(m,s) (m).horizontalAdvance(s)
# define zero(T) T()
#else
# define tr8 trUtf8 
# define swidth(m,s) (m).width(s)
# define zero(T) 0
#endif

#include "qdjvu.h"
#include "qdjvunet.h"
#include "qdjvuwidget.h"
#include "qdjview.h"
#include "qdjviewprefs.h"
#include "qdjviewdialogs.h"
#include "qdjviewsidebar.h"



// ----------------------------------------
// UTILITIES

typedef QList<QPair<QString, QString> > QueryItems;

static QueryItems 
urlQueryItems(const QUrl &url, bool fullydecoded=false)
{
#if QT_VERSION >= 0x50000
  if (fullydecoded)
    return QUrlQuery(url).queryItems(QUrl::FullyDecoded);
  return QUrlQuery(url).queryItems();
#else
  Q_UNUSED(fullydecoded);
  return url.queryItems();
#endif
}

static void 
addQueryItem(QueryItems &items, const QString &k, const QString &v)
{
  items.append(qMakePair(k,v));
}

static bool
hasQueryItem(const QueryItems &items, QString key, bool afterDjvuOpts=true)
{
  key = key.toLower();
  QPair<QString,QString> pair;
  foreach(pair, items)
    if (pair.first.toLower() == "djvuopts")
      afterDjvuOpts = true;
    else if (afterDjvuOpts && pair.first.toLower() == key)
      return true;
  return false;
}

static void 
urlSetQueryItems(QUrl &url, const QueryItems &items)
{
#if QT_VERSION >= 0x50000
  QUrlQuery qitems;
  qitems.setQueryItems(items);
  url.setQuery(qitems);
#else
  url.setQueryItems(items);
#endif
}

static void
growPageCombo(QComboBox *p, QString contents = QString())
{
  QFontMetrics metrics(p->font());
  QSize csize = metrics.size(Qt::TextSingleLine, contents);
  QStyleOptionComboBox options;
  options.initFrom(p);
  options.editable = true;
  QSize osize = p->style()->sizeFromContents(QStyle::CT_ComboBox,&options,csize,p);
  p->setMinimumWidth(qMax(p->minimumWidth(), osize.width()));
}



// ----------------------------------------
// QDJVIEW



/*! \class QDjView
  \brief The main viewer interface.

  Class \a QDjView defines the djvu viewer graphical user interface. It is
  composed of a main window with menubar, toolbar, statusbar and a dockable
  sidebar.  The center is occupied by a \a QDjVuWidget. */



// ----------------------------------------
// FILL USER INTERFACE COMPONENTS


void
QDjView::fillPageCombo(QComboBox *pageCombo)
{
  pageCombo->clear();
  int n = documentPages.size();
  for (int j=0; j<n; j++)
    {
      QString name = pageName(j);
      pageCombo->addItem(name, QVariant(j));
    }
}


void
QDjView::fillZoomCombo(QComboBox *zoomCombo)
{
  zoomCombo->clear();
  zoomCombo->addItem(tr("FitWidth","zoomCombo"), QDjVuWidget::ZOOM_FITWIDTH);
  zoomCombo->addItem(tr("FitPage","zoomCombo"), QDjVuWidget::ZOOM_FITPAGE);
  zoomCombo->addItem(tr("Stretch","zoomCombo"), QDjVuWidget::ZOOM_STRETCH);
  zoomCombo->addItem(tr("1:1","zoomCombo"), QDjVuWidget::ZOOM_ONE2ONE);
  zoomCombo->addItem(tr("300%","zoomCombo"), 300);
  zoomCombo->addItem(tr("200%","zoomCombo"), 200);
  zoomCombo->addItem(tr("150%","zoomCombo"), 150);
  zoomCombo->addItem(tr("100%","zoomCombo"), 100);
  zoomCombo->addItem(tr("75%","zoomCombo"), 75);
  zoomCombo->addItem(tr("50%","zoomCombo"), 50);
}


void
QDjView::fillModeCombo(QComboBox *modeCombo)
{
  modeCombo->clear();
  modeCombo->addItem(tr("Color","modeCombo"), QDjVuWidget::DISPLAY_COLOR);
  modeCombo->addItem(tr("Stencil","modeCombo"), QDjVuWidget::DISPLAY_STENCIL);
  modeCombo->addItem(tr("Foreground","modeCombo"), QDjVuWidget::DISPLAY_FG);
  modeCombo->addItem(tr("Background","modeCombo"), QDjVuWidget::DISPLAY_BG);
  modeCombo->addItem(tr("Hidden Text","modeCombo"), QDjVuWidget::DISPLAY_TEXT);
}  


void
QDjView::fillToolBar(QToolBar *toolBar)
{
  // Hide toolbar
  bool wasHidden = toolBar->isHidden();
  toolBar->hide();
  toolBar->clear();
  // Hide combo boxes
  modeCombo->hide();
  pageCombo->hide();
  zoomCombo->hide();
  // Use options to compose toolbar
  if (viewerMode >= STANDALONE) 
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
  if (tools & QDjViewPrefs::TOOL_FIND)
    {
      toolBar->addAction(actionFind);
    }
  if (tools & QDjViewPrefs::TOOL_SELECT)
    {
      toolBar->addAction(actionSelect);
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
  if (tools & QDjViewPrefs::TOOL_WHATSTHIS)
    {
      toolBar->addAction(actionWhatsThis);
    }
  // Allowed areas
  Qt::ToolBarAreas areas = zero(Qt::ToolBarAreas);
  if (tools & QDjViewPrefs::TOOLBAR_TOP)
    areas |= Qt::TopToolBarArea;
  if (tools & QDjViewPrefs::TOOLBAR_BOTTOM)
    areas |= Qt::BottomToolBarArea;
  if (!areas)
    areas = Qt::TopToolBarArea | Qt::BottomToolBarArea;
  toolBar->setAllowedAreas(areas);
  if (! (toolBarArea(toolBar) & areas))
    {
      removeToolBar(toolBar);
      if (areas & Qt::TopToolBarArea)
        addToolBar(Qt::TopToolBarArea, toolBar);
      else
        addToolBar(Qt::BottomToolBarArea, toolBar);
    }
  // Done
  toolBar->setVisible(!wasHidden);
  toolsCached = tools;
}




// ----------------------------------------
// ACTIONS


QAction *
QDjView::makeAction(QString text)
{
  QAction *action = new QAction(text, this);
  allActions.append(action);
  return action;
}

QAction *
QDjView::makeAction(QString text, Qt::ShortcutContext scontext)
{
  QAction *action = new QAction(text, widget);
  allActions.append(action);
  action->setShortcutContext(scontext);
  return action;
}

QAction *
QDjView::makeAction(QString text, bool value)
{
  QAction *action = new QAction(text, this);
  allActions.append(action);
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
  QList<QKeySequence> shortcuts = action->shortcuts();
  shortcuts.prepend(shortcut);
  action->setShortcuts(shortcuts);
  return action;
}

static inline QAction * 
operator<<(QAction *action, QString string)
{
  if (action->text().isEmpty())
    action->setText(string);
  else if (action->statusTip().isEmpty())
    action->setStatusTip(string);
  else if (action->whatsThis().isEmpty())
    action->setWhatsThis(string);
  return action;
}
  
struct Trigger {
  QObject *object;
  const char *slot;
  Trigger(QObject *object, const char *slot)
    : object(object), slot(slot) { } 
};

static inline QAction *
operator<<(QAction *action, Trigger trigger)
{
  QObject::connect(action, SIGNAL(triggered(bool)),
                   trigger.object, trigger.slot);
  return action;
}

static inline QAction *
operator<<(QAction *action, QVariant variant)
{
  action->setData(variant);
  return action;
}

void
QDjView::createActions()
{
  // Create action groups
  zoomActionGroup = new QActionGroup(this);
  modeActionGroup = new QActionGroup(this);
  rotationActionGroup  = new QActionGroup(this);
  
  // Create all actions
  actionNew = makeAction(tr("&New", "File|"))
    << QKeySequence(tr("Ctrl+N", "File|New"))
    << QIcon(":/images/icon_new.png")
    << tr("Create a new DjView window.")
    << Trigger(this, SLOT(performNew()));

  actionOpen = makeAction(tr("&Open", "File|"))
    << QKeySequence(tr("Ctrl+O", "File|Open"))
    << QIcon(":/images/icon_open.png")
    << tr("Open a DjVu document.")
    << Trigger(this, SLOT(performOpen()));

  actionOpenLocation = makeAction(tr("Open &Location...", "File|"))
    << tr("Open a remote DjVu document.")
    << QIcon(":/images/icon_web.png")
    << Trigger(this, SLOT(performOpenLocation()));

  actionClose = makeAction(tr("&Close", "File|"))
    << QKeySequence(tr("Ctrl+W", "File|Close"))
    << QIcon(":/images/icon_close.png")
    << tr("Close this window.")
    << Trigger(this, SLOT(close()));

  actionQuit = makeAction(tr("&Quit", "File|"))
    << QKeySequence(tr("Ctrl+Q", "File|Quit"))
    << QIcon(":/images/icon_quit.png")
    << tr("Close all windows and quit the application.")
    << Trigger(QCoreApplication::instance(), SLOT(closeAllWindows()));

  actionSave = makeAction(tr("Save &as...", "File|"))
    << QKeySequence(tr("Ctrl+S", "File|SaveAs"))
    << QIcon(":/images/icon_save.png")
    << tr("Save the DjVu document.")
    << Trigger(this, SLOT(saveAs()));
  
  actionExport = makeAction(tr("&Export as...", "File|"))
    << QKeySequence(tr("Ctrl+E", "File|ExportAs"))
    << QIcon(":/images/icon_save.png")
    << tr("Export DjVu page or document to other formats.")
    << Trigger(this, SLOT(exportAs()));

  actionPrint = makeAction(tr("&Print...", "File|"))
    << QKeySequence(tr("Ctrl+P", "File|Print"))
    << QIcon(":/images/icon_print.png")
    << tr("Print the DjVu document.")
    << Trigger(this, SLOT(print()));

  actionFind = makeAction(tr("&Find...", "Edit|"))
    << QKeySequence(tr("Ctrl+F", "Edit|Find"))
    << QIcon(":/images/icon_find.png")
    << tr("Find text in the document.")
    << Trigger(this, SLOT(showFind()));

  actionFindNext = makeAction(tr("Find &Next", "Edit|"))
    << QKeySequence(tr("F3", "Edit|Find Next"))
    << tr("Find next occurrence of search text in the document.")
    << Trigger(findWidget, SLOT(findNext()));

  actionFindPrev = makeAction(tr("Find &Previous", "Edit|"))
    << QKeySequence(tr("Shift+F3", "Edit|Find Previous"))
    << tr("Find previous occurrence of search text in the document.")
    << Trigger(findWidget, SLOT(findPrev()));

  actionSelect = makeAction(tr("&Select", "Edit|"), false)
    << QKeySequence(tr("F2", "Edit|Select"))
    << QIcon(":/images/icon_select.png")
    << tr("Select a rectangle in the document.")
    << Trigger(this, SLOT(performSelect(bool)));
  
  actionZoomIn = makeAction(tr("Zoom &In", "Zoom|"), Qt::WidgetWithChildrenShortcut)
    << QIcon(":/images/icon_zoomin.png")
    << tr("Increase the magnification.")
    << QKeySequence("+")
    << Trigger(widget, SLOT(zoomIn()));

  actionZoomOut = makeAction(tr("Zoom &Out", "Zoom|"), Qt::WidgetWithChildrenShortcut)
    << QIcon(":/images/icon_zoomout.png")
    << tr("Decrease the magnification.")
    << QKeySequence("-")
    << Trigger(widget, SLOT(zoomOut()));

  actionZoomFitWidth = makeAction(tr("Fit &Width", "Zoom|"), false)
    << tr("Set magnification to fit page width.")
    << QVariant(QDjVuWidget::ZOOM_FITWIDTH)
    << Trigger(this, SLOT(performZoom()))
    << *zoomActionGroup;

  actionZoomFitPage = makeAction(tr("Fit &Page", "Zoom|"), false)
    << tr("Set magnification to fit page.")
    << QVariant(QDjVuWidget::ZOOM_FITPAGE)
    << Trigger(this, SLOT(performZoom()))
    << *zoomActionGroup;

  actionZoomOneToOne = makeAction(tr("One &to one", "Zoom|"), false)
    << tr("Set full resolution magnification.")
    << QVariant(QDjVuWidget::ZOOM_ONE2ONE)
    << Trigger(this, SLOT(performZoom()))
    << *zoomActionGroup;

  actionZoom300 = makeAction(tr("&300%", "Zoom|"), false)
    << tr("Magnify 300%")
    << QVariant(300)
    << Trigger(this, SLOT(performZoom()))
    << *zoomActionGroup;

  actionZoom200 = makeAction(tr("&200%", "Zoom|"), false)
    << tr("Magnify 20%")
    << QVariant(200)
    << Trigger(this, SLOT(performZoom()))
    << *zoomActionGroup;

  actionZoom150 = makeAction(tr("&150%", "Zoom|"), false)
    << tr("Magnify 150%")
    << QVariant(200)
    << Trigger(this, SLOT(performZoom()))
    << *zoomActionGroup;

  actionZoom100 = makeAction(tr("&100%", "Zoom|"), false)
    << tr("Magnify 100%")
    << QVariant(100)
    << Trigger(this, SLOT(performZoom()))
    << *zoomActionGroup;

  actionZoom75 = makeAction(tr("&75%", "Zoom|"), false)
    << tr("Magnify 75%")
    << QVariant(75)
    << Trigger(this, SLOT(performZoom()))
    << *zoomActionGroup;

  actionZoom50 = makeAction(tr("&50%", "Zoom|"), false)
    << tr("Magnify 50%")
    << QVariant(50)
    << Trigger(this, SLOT(performZoom()))
    << *zoomActionGroup;

  actionNavFirst = makeAction(tr("&First Page", "Go|"), Qt::WidgetWithChildrenShortcut)
    << QIcon(":/images/icon_first.png")
    << tr("Jump to first document page.")
    << QKeySequence("Ctrl+Home")
    << Trigger(widget, SLOT(firstPage()))
    << Trigger(this, SLOT(updateActionsLater()));

  actionNavNext = makeAction(tr("&Next Page", "Go|"), Qt::WidgetWithChildrenShortcut)
    << QIcon(":/images/icon_next.png")
    << tr("Jump to next document page.")
    << QKeySequence("PgDown")
    << Trigger(widget, SLOT(nextPage()))
    << Trigger(this, SLOT(updateActionsLater()));

  actionNavPrev = makeAction(tr("&Previous Page", "Go|"), Qt::WidgetWithChildrenShortcut)
    << QIcon(":/images/icon_prev.png")
    << tr("Jump to previous document page.")
    << QKeySequence("PgUp")
    << Trigger(widget, SLOT(prevPage()))
    << Trigger(this, SLOT(updateActionsLater()));

  actionNavLast = makeAction(tr("&Last Page", "Go|"), Qt::WidgetWithChildrenShortcut)
    << QIcon(":/images/icon_last.png")
    << tr("Jump to last document page.")
    << QKeySequence("Ctrl+End")
    << Trigger(widget, SLOT(lastPage()))
    << Trigger(this, SLOT(updateActionsLater()));

  actionBack = makeAction(tr("&Backward", "Go|"))
    << QIcon(":/images/icon_back.png")
    << tr("Backward in history.")
    << QKeySequence("Alt+Left")
    << Trigger(this, SLOT(performUndo()))
    << Trigger(this, SLOT(updateActionsLater()));

  actionForw = makeAction(tr("&Forward", "Go|"))
    << QIcon(":/images/icon_forw.png")
    << tr("Forward in history.")
    << QKeySequence("Alt+Right")
    << Trigger(this, SLOT(performRedo()))
    << Trigger(this, SLOT(updateActionsLater()));

  actionRotateLeft = makeAction(tr("Rotate &Left", "Rotate|"), Qt::WidgetWithChildrenShortcut)
    << QIcon(":/images/icon_rotateleft.png")
    << tr("Rotate page image counter-clockwise.")
    << QKeySequence("[")
    << Trigger(widget, SLOT(rotateLeft()))
    << Trigger(this, SLOT(updateActionsLater()));

  actionRotateRight = makeAction(tr("Rotate &Right", "Rotate|"), Qt::WidgetWithChildrenShortcut)
    << QIcon(":/images/icon_rotateright.png")
    << tr("Rotate page image clockwise.")
    << QKeySequence("]")
    << Trigger(widget, SLOT(rotateRight()))
    << Trigger(this, SLOT(updateActionsLater()));

  actionRotate0 = makeAction(tr8("Rotate &0\302\260", "Rotate|"), false)
    << tr("Set natural page orientation.")
    << QVariant(0)
    << Trigger(this, SLOT(performRotation()))
    << *rotationActionGroup;

  actionRotate90 = makeAction(tr8("Rotate &90\302\260", "Rotate|"), false)
    << tr("Turn page on its left side.")
    << QVariant(1)
    << Trigger(this, SLOT(performRotation()))
    << *rotationActionGroup;

  actionRotate180 = makeAction(tr8("Rotate &180\302\260", "Rotate|"), false)
    << tr("Turn page upside-down.")
    << QVariant(2)
    << Trigger(this, SLOT(performRotation()))
    << *rotationActionGroup;

  actionRotate270 = makeAction(tr8("Rotate &270\302\260", "Rotate|"), false)
    << tr("Turn page on its right side.")
    << QVariant(3)
    << Trigger(this, SLOT(performRotation()))
    << *rotationActionGroup;
  
  actionInformation = makeAction(tr("&Information...", "Edit|"))
    << QKeySequence(tr("Ctrl+I", "Edit|Information"))
    << tr("Show information about the document encoding and structure.")
    << Trigger(this, SLOT(performInformation()));

  actionMetadata = makeAction(tr("&Metadata...", "Edit|"))
#ifndef Q_OS_DARWIN
    << QKeySequence(tr("Ctrl+M", "Edit|Metadata"))
#endif
    << tr("Show the document and page meta data.")
    << Trigger(this, SLOT(performMetadata()));

  actionWhatsThis = QWhatsThis::createAction(this);

  actionAbout = makeAction(tr("&About DjView..."))
#ifndef Q_OS_DARWIN
    << QIcon(":/images/djview.png")
#endif
    << tr("Show information about this program.")
    << Trigger(this, SLOT(performAbout()));

  actionDisplayColor = makeAction(tr("&Color", "Display|"), false)
    << tr("Display everything.")
    << Trigger(widget, SLOT(displayModeColor()))
    << Trigger(this, SLOT(updateActionsLater()))
    << *modeActionGroup;

  actionDisplayBW = makeAction(tr("&Stencil", "Display|"), false)
    << tr("Only display the document bitonal stencil.")
    << Trigger(widget, SLOT(displayModeStencil()))
    << Trigger(this, SLOT(updateActionsLater()))
    << *modeActionGroup;

  actionDisplayForeground 
    = makeAction(tr("&Foreground", "Display|"), false)
    << tr("Only display the foreground layer.")
    << Trigger(widget, SLOT(displayModeForeground()))
    << Trigger(this, SLOT(updateActionsLater()))
    << *modeActionGroup;

  actionDisplayBackground
    = makeAction(tr("&Background", "Display|"), false)
    << tr("Only display the background layer.")
    << Trigger(widget, SLOT(displayModeBackground()))
    << Trigger(this, SLOT(updateActionsLater()))
    << *modeActionGroup;
  
  actionDisplayHiddenText
    = makeAction(tr("&Hidden Text", "Display|"), false)
    << tr("Overlay a representation of the hidden text layer.")
    << Trigger(widget, SLOT(displayModeText()))
    << Trigger(this, SLOT(updateActionsLater()))
    << *modeActionGroup;

  actionInvertLuminance
    = makeAction(tr("I&nvert Luminance", "View|"), true)
    << tr("Invert image luminance while preserving hue.")
    << Trigger(widget, SLOT(setInvertLuminance(bool)))
    << Trigger(this, SLOT(updateActionsLater()));
  
  actionPreferences = makeAction(tr("Prefere&nces...", "Settings|")) 
    << QIcon(":/images/icon_prefs.png")
    << tr("Show the preferences dialog.")
    << Trigger(this, SLOT(performPreferences()));

  actionViewSideBar = makeAction(tr("Show &Sidebar", "Settings|"), true)
    << QKeySequence(tr("F9", "Settings|Show sidebar"))
#ifdef Q_OS_DARWIN
    << QKeySequence(tr("Alt+Ctrl+S", "Settings|Show sidebar"))
#endif
    << tr("Show/hide the side bar.")
    << Trigger(this, SLOT(showSideBar(bool)));

  actionViewToolBar = toolBar->toggleViewAction()
    << tr("Show &Toolbar", "Settings|")
    << QKeySequence(tr("F10", "Settings|Show toolbar"))
#ifdef Q_OS_DARWIN
    << QKeySequence(tr("Alt+Ctrl+T", "Settings|Show toolbar"))
#endif
    << tr("Show/hide the standard tool bar.")
    << Trigger(this, SLOT(updateActionsLater()));

  actionViewStatusBar = makeAction(tr("Show Stat&usbar", "Settings|"), true)
    << tr("Show/hide the status bar.")
#ifdef Q_OS_DARWIN
    << QKeySequence(tr("Alt+Ctrl+/", "Settings|Show toolbar"))
#endif
    << Trigger(statusBar,SLOT(setVisible(bool)))
    << Trigger(this, SLOT(updateActionsLater()));

  actionViewFullScreen 
    = makeAction(tr("&Full Screen","View|"), false)
    << QKeySequence(tr("F11","View|FullScreen"))
#ifdef Q_OS_DARWIN
    << QKeySequence(tr("Meta+Ctrl+F","View|FullScreen"))
#endif
    << QIcon(":/images/icon_fullscreen.png")
    << tr("Toggle full screen mode.")
    << Trigger(this, SLOT(performViewFullScreen(bool)));

  actionViewSlideShow
    = makeAction(tr("&Slide Show","View|"), false)
    << QKeySequence(tr("Shift+F11","View|Slideshow"))
#ifdef Q_OS_DARWIN
    << QKeySequence(tr("Shift+Ctrl+F", "Settings|Show toolbar"))
#endif
    << QIcon(":/images/icon_slideshow.png")
    << tr("Toggle slide show mode.")
    << Trigger(this, SLOT(performViewSlideShow(bool)));

  actionLayoutContinuous = makeAction(tr("&Continuous", "Layout|"), false)
    << QIcon(":/images/icon_continuous.png")
    << QKeySequence(tr("F4", "Layout|Continuous"))
    << tr("Toggle continuous layout mode.")
    << Trigger(widget, SLOT(setContinuous(bool)))
    << Trigger(this, SLOT(updateActionsLater()));

  actionLayoutSideBySide = makeAction(tr("Side &by Side", "Layout|"), false)
    << QIcon(":/images/icon_sidebyside.png")
    << QKeySequence(tr("F5", "Layout|SideBySide"))
    << tr("Toggle side-by-side layout mode.")
    << Trigger(widget, SLOT(setSideBySide(bool)))
    << Trigger(this, SLOT(updateActionsLater()));
  
  actionLayoutCoverPage = makeAction(tr("Co&ver Page", "Layout|"), false)
#ifdef Q_OS_DARWIN
    << QIcon(":/images/icon_coverpage.png")
#endif
    << QKeySequence(tr("F6", "Layout|CoverPage"))
    << tr("Show the cover page alone in side-by-side mode.")
    << Trigger(widget, SLOT(setCoverPage(bool)))
    << Trigger(this, SLOT(updateActionsLater()));
  
  actionLayoutRightToLeft = makeAction(tr("&Right to Left", "Layout|"), false)
#ifdef Q_OS_DARWIN
    << QIcon(":/images/icon_righttoleft.png")
#endif
    << QKeySequence(tr("Shift+F6", "Layout|RightToLeft"))
    << tr("Show pages right-to-left in side-by-side mode.")
    << Trigger(widget, SLOT(setRightToLeft(bool)))
    << Trigger(this, SLOT(updateActionsLater()));
  
  actionCopyUrl = makeAction(tr("Copy &URL", "Edit|"))
    << tr("Save an URL for the current page into the clipboard.")
    << QKeySequence(tr("Ctrl+C", "Edit|CopyURL"))
    << Trigger(this, SLOT(performCopyUrl()));
  
  actionCopyOutline = makeAction(tr("Copy &Outline", "Edit|"))
    << tr("Save the djvused code for the outline into the clipboard.")
    << Trigger(this, SLOT(performCopyOutline()));
  
  actionCopyAnnotation = makeAction(tr("Copy &Annotations", "Edit|"))
    << tr("Save the djvused code for the page annotations into the clipboard.")
    << Trigger(this, SLOT(performCopyAnnotation()));
  
  actionAbout->setMenuRole(QAction::AboutRole);
  actionQuit->setMenuRole(QAction::QuitRole);
  actionPreferences->setMenuRole(QAction::PreferencesRole);
}


void
QDjView::createMenus()
{
  // Layout main menu
  QMenu *fileMenu = menuBar->addMenu(tr("&File", "File|"));
  if (viewerMode >= STANDALONE)
    {
      fileMenu->addAction(actionNew);
      fileMenu->addAction(actionOpen);
      fileMenu->addAction(actionOpenLocation);
      recentMenu = fileMenu->addMenu(tr("Open &Recent"));
      recentMenu->menuAction()->setIcon(QIcon(":/images/icon_open.png"));
      connect(recentMenu, SIGNAL(aboutToShow()), this, SLOT(fillRecent()));
      fileMenu->addSeparator();
    }
  fileMenu->addAction(actionSave);
  fileMenu->addAction(actionExport);
  fileMenu->addAction(actionPrint);
  if (viewerMode >= STANDALONE)
    {
      fileMenu->addSeparator();
      fileMenu->addAction(actionClose);
      fileMenu->addAction(actionQuit);
    }
  QMenu *editMenu = menuBar->addMenu(tr("&Edit", "Edit|"));
  editMenu->addAction(actionSelect);
  editMenu->addAction(actionCopyUrl);
  editMenu->addAction(actionCopyOutline);
  editMenu->addAction(actionCopyAnnotation);
  editMenu->addSeparator();
  editMenu->addAction(actionFind);
  editMenu->addAction(actionFindNext);
  editMenu->addAction(actionFindPrev);
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
  modeMenu->addAction(actionDisplayHiddenText);
  viewMenu->addSeparator();
  viewMenu->addAction(actionLayoutContinuous);
  viewMenu->addAction(actionLayoutSideBySide);
  viewMenu->addAction(actionLayoutCoverPage);
  viewMenu->addAction(actionLayoutRightToLeft);
  viewMenu->addSeparator();
  viewMenu->addAction(actionInvertLuminance);
  viewMenu->addSeparator();
  viewMenu->addAction(actionInformation);
  viewMenu->addAction(actionMetadata);
  if (viewerMode >= STANDALONE)
    viewMenu->addSeparator();
  if (viewerMode >= STANDALONE)
    viewMenu->addAction(actionViewFullScreen);
  if (viewerMode >= STANDALONE)
    viewMenu->addAction(actionViewSlideShow);
  QMenu *gotoMenu = menuBar->addMenu(tr("&Go", "Go|"));
  gotoMenu->addAction(actionNavFirst);
  gotoMenu->addAction(actionNavPrev);
  gotoMenu->addAction(actionNavNext);
  gotoMenu->addAction(actionNavLast);
  gotoMenu->addSeparator();
  gotoMenu->addAction(actionBack);
  gotoMenu->addAction(actionForw);
  QMenu *settingsMenu = menuBar->addMenu(tr("&Settings", "Settings|"));
  settingsMenu->addAction(actionViewSideBar);
  settingsMenu->addAction(actionViewToolBar);
  settingsMenu->addAction(actionViewStatusBar);
  settingsMenu->addSeparator();
  settingsMenu->addAction(actionPreferences);
  QMenu *helpMenu = menuBar->addMenu(tr("&Help", "Help|"));
  helpMenu->addAction(actionWhatsThis);
  helpMenu->addSeparator();
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
  modeMenu->addAction(actionDisplayHiddenText);
  contextMenu->addSeparator();
  contextMenu->addAction(actionLayoutContinuous);
  contextMenu->addAction(actionLayoutSideBySide);
  contextMenu->addAction(actionLayoutCoverPage);
  contextMenu->addSeparator();
  contextMenu->addAction(actionFind);
  contextMenu->addAction(actionInformation);
  contextMenu->addAction(actionMetadata);
  contextMenu->addSeparator();
  contextMenu->addAction(actionSave);
  contextMenu->addAction(actionExport);
  contextMenu->addAction(actionPrint);
  contextMenu->addSeparator();
  contextMenu->addAction(actionViewSideBar);
  contextMenu->addAction(actionViewToolBar);
  contextMenu->addAction(actionViewFullScreen);
  contextMenu->addAction(actionViewSlideShow);
  contextMenu->addSeparator();
  contextMenu->addAction(actionPreferences);
  contextMenu->addSeparator();
  contextMenu->addAction(actionWhatsThis);
  contextMenu->addAction(actionAbout);
}


/*! Update graphical interface components.
  This is called whenever something changes
  using function \a QDjView::updateActionsLater().
  It updates the all gui compnents. */

void
QDjView::updateActions()
{
  // Rebuild toolbar if necessary
  if (tools != toolsCached)
    fillToolBar(toolBar);
  
  // Enable all actions
  foreach(QObject *object, allActions)
    if (QAction *action = qobject_cast<QAction*>(object))
      action->setEnabled(true);

  // Some actions are explicitly disabled
  actionSave->setEnabled(savingAllowed || prefs->restrictOverride);
  actionExport->setEnabled(savingAllowed || prefs->restrictOverride);
  actionPrint->setEnabled(printingAllowed || prefs->restrictOverride);
  
  // Some actions are only available in standalone mode
  actionNew->setVisible(viewerMode >= STANDALONE);
  actionOpen->setVisible(viewerMode >= STANDALONE);
  actionOpenLocation->setVisible(viewerMode >= STANDALONE);
  actionClose->setVisible(viewerMode >= STANDALONE);
  actionQuit->setVisible(viewerMode >= STANDALONE);
  actionViewFullScreen->setVisible(viewerMode >= STANDALONE);  
  actionViewSlideShow->setVisible(viewerMode >= STANDALONE);  
  setAcceptDrops(viewerMode >= STANDALONE);

  // Zoom combo and actions
  int zoom = widget->zoom();
  actionZoomIn->setEnabled(zoom < QDjVuWidget::ZOOM_MAX);
  actionZoomOut->setEnabled(zoom < 0 || zoom > QDjVuWidget::ZOOM_MIN);
  actionZoomFitPage->setChecked(zoom == QDjVuWidget::ZOOM_FITPAGE);
  actionZoomFitWidth->setChecked(zoom == QDjVuWidget::ZOOM_FITWIDTH);
  actionZoomOneToOne->setChecked(zoom == QDjVuWidget::ZOOM_ONE2ONE);
  actionZoom300->setChecked(zoom == 300);
  actionZoom200->setChecked(zoom == 200);
  actionZoom150->setChecked(zoom == 150);
  actionZoom100->setChecked(zoom == 100);
  actionZoom75->setChecked(zoom == 75);
  actionZoom50->setChecked(zoom == 50);
  zoomCombo->setEnabled(!!document);
  int zoomIndex = zoomCombo->findData(QVariant(zoom));
  zoomCombo->clearEditText();
  zoomCombo->setCurrentIndex(zoomIndex);
  if (zoomIndex < 0 &&
      zoom >= QDjVuWidget::ZOOM_MIN && 
      zoom <= QDjVuWidget::ZOOM_MAX)
    zoomCombo->setEditText(QString("%1%").arg(zoom));
  else if (zoomIndex >= 0)
    zoomCombo->setEditText(zoomCombo->itemText(zoomIndex));

  // Mode combo and actions
  QDjVuWidget::DisplayMode mode = widget->displayMode();
  actionDisplayColor->setChecked(mode == QDjVuWidget::DISPLAY_COLOR);
  actionDisplayBW->setChecked(mode == QDjVuWidget::DISPLAY_STENCIL);
  actionDisplayBackground->setChecked(mode == QDjVuWidget::DISPLAY_BG);
  actionDisplayForeground->setChecked(mode == QDjVuWidget::DISPLAY_FG);
  actionDisplayHiddenText->setChecked(mode == QDjVuWidget::DISPLAY_TEXT);
  modeCombo->setCurrentIndex(modeCombo->findData(QVariant(mode)));
  modeCombo->setEnabled(!!document);

  // Image inversion
  actionInvertLuminance->setChecked(widget->invertLuminance());

  // Rotations
  int rotation = widget->rotation();
  actionRotate0->setChecked(rotation == 0);
  actionRotate90->setChecked(rotation == 1);
  actionRotate180->setChecked(rotation == 2);
  actionRotate270->setChecked(rotation == 3);
  
  // Page combo and actions
  int pagenum = documentPages.size();
  int pageno = widget->page();
  pageCombo->clearEditText();
  pageCombo->setCurrentIndex(pageCombo->findData(QVariant(pageno)));
  if (pageno >= 0 && pagenum > 0  && !pageCombo->hasFocus())
    {
      QString sNumber = QString("%1 / %2").arg(pageno+1).arg(pagenum);
      QString sName = pageName(pageno,true);
      growPageCombo(pageCombo, sNumber);
      pageCombo->setEditText((sName.isEmpty()) ? sNumber : sName);
      pageCombo->lineEdit()->setCursorPosition(0);
    }
  pageCombo->setEnabled(pagenum > 0);
  actionNavFirst->setEnabled(pagenum>0 && pageno>0);
  actionNavPrev->setEnabled(pagenum>0 && pageno>0);
  actionNavNext->setEnabled(pagenum>0 && pageno<pagenum-1);
  actionNavLast->setEnabled(pagenum>0 && pageno<pagenum-1);

  // Layout actions
  actionLayoutContinuous->setChecked(widget->continuous());  
  actionLayoutSideBySide->setChecked(widget->sideBySide());
  actionLayoutCoverPage->setEnabled(widget->sideBySide());
  actionLayoutCoverPage->setChecked(widget->coverPage());  
  actionLayoutRightToLeft->setEnabled(widget->sideBySide());
  actionLayoutRightToLeft->setChecked(widget->rightToLeft());  
  
  // UndoRedo
  undoTimer->stop();
  undoTimer->start(250);
  actionBack->setEnabled(undoList.size() > 0);
  actionForw->setEnabled(redoList.size() > 0);

  // Misc
  textLabel->setVisible(prefs->showTextLabel);
  textLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Ignored);
  actionCopyOutline->setVisible(prefs->advancedFeatures);
  actionCopyAnnotation->setVisible(prefs->advancedFeatures);
  actionCopyUrl->setEnabled(pagenum > 0);
  actionCopyOutline->setEnabled(pagenum > 0);
  actionCopyAnnotation->setEnabled(pagenum > 0);
  actionViewSideBar->setChecked(hiddenSideBarTabs() == 0);
  actionViewSlideShow->setChecked(viewerMode == STANDALONE_SLIDESHOW);
  actionViewFullScreen->setChecked(viewerMode == STANDALONE_FULLSCREEN);

  // Disable almost everything when document==0
  if (! document)
    {
      foreach(QObject *object, allActions)
        if (QAction *action = qobject_cast<QAction*>(object))
          if (action != actionNew &&
              action != actionOpen &&
              action != actionOpenLocation &&
              action != actionClose &&
              action != actionQuit &&
              action != actionPreferences &&
              action != actionViewToolBar &&
              action != actionViewStatusBar &&
              action != actionViewSideBar &&
              action != actionViewFullScreen &&
              action != actionWhatsThis &&
              action != actionAbout )
            action->setEnabled(false);
    }
  
  // Finished
  updateActionsScheduled = false;

  // Reset slide show timeout
  slideShowTimeout(true);

  // Notify plugin
  if (viewerMode < STANDALONE)  
    emit pluginOnChange();
}



// ----------------------------------------
// WHATSTHIS HELP


void
QDjView::createWhatsThis()
{
#define HELP(x,y) { QString s(x); Help h(s); h y; }
  
  struct Help {
    QString s;
    Help(QString s) : s(s) { }
    Help& operator>>(QWidget *w) { w->setWhatsThis(s); return *this; }
    Help& operator>>(QAction *a) { a->setWhatsThis(s); return *this; }
  };
  
  QString mc, ms, ml;
#ifdef Q_OS_DARWIN
  mc = tr("Control Left Mouse Button");
#else
  mc = tr("Right Mouse Button");
#endif
  ms = prefs->modifiersToString(prefs->modifiersForSelect);
  ms = ms.replace("+"," ");
  ml = prefs->modifiersToString(prefs->modifiersForLens);
  ml = ml.replace("+"," ");
  
  HELP(tr("<html><b>Selecting a rectangle.</b><br/> "
          "Once a rectangular area is selected, a popup menu "
          "lets you copy the corresponding text or image. "
          "Instead of using this tool, you can also hold %1 "
          "and use the Left Mouse Button."
          "</html>").arg(ms),
       >> actionSelect );
  
  HELP(tr("<html><b>Zooming.</b><br/> "
          "Choose a zoom level for viewing the document. "
          "Zoom level 100% displays the document for a 100 dpi screen. "
          "Zoom levels <tt>Fit Page</tt> and <tt>Fit Width</tt> ensure "
          "that the full page or the page width fit in the window. "
          "</html>"),
       >> actionZoomIn >> actionZoomOut
       >> actionZoomFitPage >> actionZoomFitWidth
       >> actionZoom300 >> actionZoom200 >> actionZoom150
       >> actionZoom75 >> actionZoom50
       >> zoomCombo );
  
  HELP(tr("<html><b>Rotating the pages.</b><br/> "
          "Choose to display pages in portrait or landscape mode. "
          "You can also turn them upside down.</html>"),
       >> actionRotateLeft >> actionRotateRight
       >> actionRotate0 >> actionRotate90
       >> actionRotate180 >> actionRotate270 );

  HELP(tr("<html><b>Display mode.</b><br/> "
          "DjVu images compose a background layer and a foreground layer "
          "using a stencil. The display mode specifies with layers "
          "should be displayed.</html>"),
            >> actionDisplayColor >> actionDisplayBW
            >> actionDisplayForeground >> actionDisplayBackground
            >> modeCombo );

  HELP(tr("<html><b>Navigating the document.</b><br/> "
          "The page selector lets you jump to any page by name "
          "and can be activated at any time by pressing Ctrl+G. "
          "The navigation buttons jump to the first page, the previous "
          "page, the next page, or the last page. </html>"),
       >> actionNavFirst >> actionNavPrev 
       >> actionNavNext >> actionNavLast
       >> pageCombo );

  HELP(tr("<html><b>Document and page information.</b><br> "
          "Display a dialog window for viewing "
          "encoding information pertaining to the document "
          "or to a specific page.</html>"),
       >> actionInformation );
  
  HELP(tr("<html><b>Document and page metadata.</b><br> "
          "Display a dialog window for viewing metadata "
          "pertaining to the document "
          "or to a specific page.</html>"),
       >> actionMetadata );

  HELP(tr("<html><b>Continuous layout.</b><br/> "
          "Display all the document pages arranged vertically "
          "inside the scrollable document viewing area.</html>"),
       >> actionLayoutContinuous );
  
  HELP(tr("<html><b>Side by side layout.</b><br/> "
          "Display pairs of pages side by side "
          "inside the scrollable document viewing area.</html>"),
       >> actionLayoutSideBySide );
  
  HELP(tr("<html><b>Page information.</b><br/> "
          "Display information about the page located under the cursor: "
          "the sequential page number, the page size in pixels, "
          "and the page resolution in dots per inch. </html>"),
       >> pageLabel );
  
  HELP(tr("<html><b>Cursor information.</b><br/> "
          "Display the position of the mouse cursor "
          "expressed in page coordinates. </html>"),
       >> mouseLabel );

  HELP(tr("<html><b>Document viewing area.</b><br/> "
          "This is the main display area for the DjVu document. <ul>"
          "<li>Arrows and page keys to navigate the document.</li>"
          "<li>Space and BackSpace to read the document.</li>"
          "<li>Keys <tt>+</tt> <tt>-</tt> <tt>[</tt> <tt>]</tt> "
          "to zoom or rotate the document.</li>"
          "<li>Left Mouse Button for panning and selecting links.</li>"
          "<li>%3 for displaying the contextual menu.</li>"
          "<li>%1 Left Mouse Button for selecting text or images.</li>"
          "<li>%2 for popping the magnification lens.</li>"
          "</ul></html>").arg(ms).arg(ml).arg(mc),
       >> widget );
  
  HELP(tr("<html><b>Document viewing area.</b><br/> "
          "This is the main display area for the DjVu document. "
          "But you must first open a DjVu document to see anything."
          "</html>"),
       >> splash );

#undef HELP    
}



// ----------------------------------------
// APPLY PREFERENCES


QDjViewPrefs::Saved *
QDjView::getSavedConfig(ViewerMode mode)
{
  if (mode == STANDALONE_FULLSCREEN)
    return &fsSavedFullScreen;
  else if (mode == STANDALONE_SLIDESHOW)
    return &fsSavedSlideShow;
  return &fsSavedNormal;
}


QDjViewPrefs::Saved *
QDjView::getSavedPrefs(void)
{
  if (viewerMode==EMBEDDED_PLUGIN)
    return &prefs->forEmbeddedPlugin;
  else if (viewerMode==FULLPAGE_PLUGIN)
    return &prefs->forFullPagePlugin;
  else if (viewerMode==STANDALONE_FULLSCREEN)
    return &prefs->forFullScreen;
  else if (viewerMode==STANDALONE_SLIDESHOW)
    return &prefs->forSlideShow;
  return &prefs->forStandalone;
}


void 
QDjView::enableContextMenu(bool enable)
{
  QMenu *oldContextMenu = widget->contextMenu();
  if (!enable || oldContextMenu != contextMenu)
    {
      // remove context menu
      widget->setContextMenu(0);
      // disable shortcuts
      if (oldContextMenu)
        foreach(QAction *action, widget->actions())
          widget->removeAction(action);
    }
  if (enable && oldContextMenu != contextMenu)
    {
      // setup context menu
      widget->setContextMenu(contextMenu);
      // enable shortcuts
      if (contextMenu)
        {
          // all explicit shortcuts in context menu
          widget->addAction(contextMenu->menuAction());
          // things that do not have a context menu entry
          widget->addAction(actionFindNext);
          widget->addAction(actionFindPrev);
        }
    }
}


void 
QDjView::enableScrollBars(bool enable)
{
  Qt::ScrollBarPolicy policy = Qt::ScrollBarAlwaysOff;
  if (enable) 
    policy = Qt::ScrollBarAsNeeded;
  widget->setHorizontalScrollBarPolicy(policy);
  widget->setVerticalScrollBarPolicy(policy);
}


void 
QDjView::applyOptions(bool remember)
{
  menuBar->setVisible(options & QDjViewPrefs::SHOW_MENUBAR);
  toolBar->setVisible(options & QDjViewPrefs::SHOW_TOOLBAR);
  statusBar->setVisible(options & QDjViewPrefs::SHOW_STATUSBAR);
  enableScrollBars(options & QDjViewPrefs::SHOW_SCROLLBARS);
  widget->setDisplayFrame(options & QDjViewPrefs::SHOW_FRAME);
  widget->setDisplayMapAreas(options & QDjViewPrefs::SHOW_MAPAREAS);
  widget->setContinuous(options & QDjViewPrefs::LAYOUT_CONTINUOUS);
  widget->setSideBySide(options & QDjViewPrefs::LAYOUT_SIDEBYSIDE);
  widget->setCoverPage(options & QDjViewPrefs::LAYOUT_COVERPAGE);
  widget->setRightToLeft(options & QDjViewPrefs::LAYOUT_RIGHTTOLEFT);
  widget->enableMouse(options & QDjViewPrefs::HANDLE_MOUSE);
  widget->enableKeyboard(options & QDjViewPrefs::HANDLE_KEYBOARD);
  widget->enableHyperlink(options & QDjViewPrefs::HANDLE_LINKS);
  enableContextMenu(options & QDjViewPrefs::HANDLE_CONTEXTMENU);
  if (!remember)
    showSideBar(options & QDjViewPrefs::SHOW_SIDEBAR);
}


void 
QDjView::updateOptions(void)
{
  options = zero(QDjViewPrefs::Options);
  if (! menuBar->isHidden())
    options |= QDjViewPrefs::SHOW_MENUBAR;
  if (! toolBar->isHidden())
    options |= QDjViewPrefs::SHOW_TOOLBAR;
  if (! statusBar->isHidden())
    options |= QDjViewPrefs::SHOW_STATUSBAR;
  if (! hiddenSideBarTabs())
    options |= QDjViewPrefs::SHOW_SIDEBAR;    
  if (widget->verticalScrollBarPolicy() != Qt::ScrollBarAlwaysOff)
    options |= QDjViewPrefs::SHOW_SCROLLBARS;
  if (widget->displayFrame())
    options |= QDjViewPrefs::SHOW_FRAME;
  if (widget->displayMapAreas())
    options |= QDjViewPrefs::SHOW_MAPAREAS;
  if (widget->continuous())
    options |= QDjViewPrefs::LAYOUT_CONTINUOUS;
  if (widget->sideBySide())
    options |= QDjViewPrefs::LAYOUT_SIDEBYSIDE;
  if (widget->coverPage())
    options |= QDjViewPrefs::LAYOUT_COVERPAGE;
  if (widget->rightToLeft())
    options |= QDjViewPrefs::LAYOUT_RIGHTTOLEFT;
  if (widget->mouseEnabled())
    options |= QDjViewPrefs::HANDLE_MOUSE;    
  if (widget->keyboardEnabled())
    options |= QDjViewPrefs::HANDLE_KEYBOARD;
  if (widget->hyperlinkEnabled())
    options |= QDjViewPrefs::HANDLE_LINKS;
  if (widget->contextMenu())
    options |= QDjViewPrefs::HANDLE_CONTEXTMENU;
}



void
QDjView::applySaved(Saved *saved)
{
  // default dock geometry
  tabifyDockWidget(thumbnailDock, outlineDock);
  tabifyDockWidget(thumbnailDock, findDock);
  thumbnailDock->raise();
  // main saved states
  options = saved->options;
  if (saved->state.size() > 0)
    restoreState(saved->state);
  applyOptions(saved->remember);
  widget->setZoom(saved->zoom);
}


static const Qt::WindowStates unusualWindowStates = 
       (Qt::WindowMinimized|Qt::WindowMaximized|Qt::WindowFullScreen);

void
QDjView::updateSaved(Saved *saved)
{
  updateOptions();
  if (saved->remember)
    {
      // main saved states
      saved->zoom = widget->zoom();
      saved->state = saveState();
      saved->options = options;
      // avoid confusing options in standalone mode
      if (saved == &prefs->forStandalone &&
          !(saved->options & QDjViewPrefs::HANDLE_CONTEXTMENU) &&
          !(saved->options & QDjViewPrefs::SHOW_MENUBAR) )
        saved->options |= (QDjViewPrefs::SHOW_MENUBAR |
                           QDjViewPrefs::SHOW_SCROLLBARS |
                           QDjViewPrefs::SHOW_STATUSBAR |
                           QDjViewPrefs::SHOW_FRAME |
                           QDjViewPrefs::HANDLE_CONTEXTMENU );
      // main window size in standalone mode
      if (saved == &prefs->forStandalone)
        {
          Qt::WindowStates wstate = windowState();
          prefs->windowMaximized = false;
          if (wstate & Qt::WindowMaximized)
            prefs->windowMaximized = true;
          else if (! (wstate & unusualWindowStates))
            prefs->windowSize = size();
        }
      // non-saved
      saved->nsBorderBrush = widget->borderBrush();
      saved->nsBorderSize = widget->borderSize();
    }
}


void
QDjView::applyPreferences(void)
{
  // Saved preferences
  applySaved(getSavedPrefs());
  
  // Other preferences
  QDjVuNetDocument::setProxy(prefs->proxyUrl);
  djvuContext.setCacheSize(prefs->cacheSize);
  widget->setPixelCacheSize(prefs->pixelCacheSize);
  widget->setModifiersForLens(prefs->modifiersForLens);
  widget->setModifiersForSelect(prefs->modifiersForSelect);
  widget->setModifiersForLinks(prefs->modifiersForLinks);
  widget->setGamma(prefs->gamma);
  widget->setWhite(prefs->white);
  widget->setScreenDpi(prefs->resolution ? prefs->resolution : physicalDpiY());
  widget->setLensSize(prefs->lensSize);
  widget->setLensPower(prefs->lensPower);
  widget->enableAnimation(prefs->enableAnimations);
  widget->setInvertLuminance(prefs->invertLuminance);
  widget->setMouseWheelZoom(prefs->mouseWheelZoom);

  // Thumbnail preferences
  thumbnailWidget->setSize(prefs->thumbnailSize);
  thumbnailWidget->setSmart(prefs->thumbnailSmart);
  
  // Search preferences
  findWidget->setWordOnly(prefs->searchWordsOnly);
  findWidget->setCaseSensitive(prefs->searchCaseSensitive);
  // Too risky:  findWidget->setRegExpMode(prefs->searchRegExpMode);

  // Special preferences for embedded plugins
  if (viewerMode == EMBEDDED_PLUGIN)
    setMinimumSize(QSize(8,8));
  
  // Preload mode change prefs.
  fsSavedNormal = prefs->forStandalone;
  fsSavedFullScreen = prefs->forFullScreen;
  fsSavedSlideShow = prefs->forSlideShow;
}


void
QDjView::updatePreferences(void)
{
  updateSaved(getSavedPrefs());
  prefs->thumbnailSize = thumbnailWidget->size();
  prefs->thumbnailSmart = thumbnailWidget->smart();
  prefs->searchWordsOnly = findWidget->wordOnly();
  prefs->searchCaseSensitive = findWidget->caseSensitive();
  prefs->searchRegExpMode = findWidget->regExpMode();
}


void 
QDjView::preferencesChanged(void)
{
  applyPreferences();
  createWhatsThis();
  updateActions();
}



// ----------------------------------------
// SESSION MANAGEMENT


void
QDjView::saveSession(QSettings *s)
{
  Saved saved;
  updateSaved(&saved);
  s->setValue("name", objectName());
  s->setValue("options", prefs->optionsToString(saved.options));
  s->setValue("state", saved.state);
  s->setValue("tools", prefs->toolsToString(tools));
  s->setValue("documentUrl", getDecoratedUrl().toString());
}

void  
QDjView::restoreSession(QSettings *s)
{
  QUrl du = QUrl(s->value("documentUrl").toString());
  Tools tools = this->tools;
  Saved saved;
  updateSaved(&saved);
  if (s->contains("options"))
    saved.options = prefs->stringToOptions(s->value("options").toString());
  if (s->contains("zoom")) // compat
    saved.zoom = s->value("zoom").toInt();
  if (s->contains("state"))
    saved.state = s->value("state").toByteArray();
  if (s->contains("tools"))
    tools = prefs->stringToTools(s->value("tools").toString());
  if (s->contains("name"))
    setObjectName(s->value("name").toString());
  applySaved(&saved);
  updateActionsLater();
  if (du.isValid())
    open(du);
  if (document && s->contains("pageNo")) // compat
    goToPage(s->value("pageNo", 0).toInt());
}




// ----------------------------------------
// QDJVIEW ARGUMENTS


static bool
string_is_on(QString val)
{
  QString v = val.toLower();
  return v == "yes" || v == "on" || v == "true" || v == "1";
}

static bool
string_is_off(QString val)
{
  QString v = val.toLower();
  return v == "no" || v == "off" || v == "false" || v == "0";
}

static QString
get_boolean(bool b)
{
  return QString(b ? "yes" : "no");
}

static bool
parse_boolean(QString key, QString val, 
              QList<QString> &errors, bool &answer)
{
  answer = false;
  if (string_is_off(val))
    return true;
  answer = true;
  if (string_is_on(val) || val.isNull())
    return true;
  errors << QDjView::tr("Option '%1' requires boolean argument.").arg(key);
  return false;
}

static void
illegal_value(QString key, QString value, QList<QString> &errors)
{
  errors << QDjView::tr("Illegal value '%2' for option '%1'.")
    .arg(key).arg(value);
}

static bool
parse_position(QString value, double &x, double &y)
{
  bool okay0, okay1;
  QStringList val = value.split(",");
  if (val.size() == 2)
    {
      x = val[0].toDouble(&okay0);
      y = val[1].toDouble(&okay1);
      return okay0 && okay1;
    }
  return false;
}

static bool 
parse_highlight(QString value, 
                int &x, int &y, int &w, int &h, 
                QColor &color)
{
  bool okay0, okay1, okay2, okay3;
  QStringList val = value.split(",");
  if (val.size() < 4)
    return false;
  x = val[0].toInt(&okay0);
  y = val[1].toInt(&okay1);
  w = val[2].toInt(&okay2);
  h = val[3].toInt(&okay3);
  if (! (okay0 && okay1 && okay2 && okay3))
    return false;
  if (val.size() < 5)
    return true;
  QString c = val[4];
  if (c[0] != '#')
    c = "#" + c;
  color.setNamedColor(c);
  if (! color.isValid())
    return false;
  if (val.size() <= 5)
    return true;
  return false;
}

template<class T> static inline void
set_reset(QFlags<T> &x, bool plus, bool minus, T y)
{
  if (plus)
    x |= y;
  else if (minus)
    x &= ~y;
}


void
QDjView::parseToolBarOption(QString option, QStringList &errors)
{
  QString str = option.toLower();
  int len = str.size();
  bool wantselect = false;
  bool wantmode = false;
  bool minus = false;
  bool plus = false;
  int npos = 0;
  while (npos < len)
    {
      int pos = npos;
      npos = str.indexOf(QRegExp("[-+,]"), pos);
      if (npos < 0) 
        npos = len;
      QString key = str.mid(pos, npos-pos).trimmed();
      if (string_is_off(key) && !plus && !minus)
        options &= ~QDjViewPrefs::SHOW_TOOLBAR;
      else if (string_is_on(key) && !plus && !minus)
        options |= QDjViewPrefs::SHOW_TOOLBAR;
      else if (key=="bottom" && !plus && !minus) {
        options |= QDjViewPrefs::SHOW_TOOLBAR;
        tools &= ~QDjViewPrefs::TOOLBAR_TOP;
      } else if (key=="top" && !plus && !minus) {
        options |= QDjViewPrefs::SHOW_TOOLBAR;
        tools &= ~QDjViewPrefs::TOOLBAR_BOTTOM;
      } else if (key=="auto" && !plus && !minus)
        tools |= QDjViewPrefs::TOOLBAR_AUTOHIDE;
      else if (key=="always" && !plus && !minus) {
        options |= QDjViewPrefs::SHOW_TOOLBAR;
        tools &= ~QDjViewPrefs::TOOLBAR_AUTOHIDE;
      } else if (key.contains(QRegExp("^(fore|back|color|bw)(_button)?$")))
        wantmode |= plus;
      else if (key=="pan" || key=="zoomsel" || key=="textsel")
        wantselect |= plus;
      else if (key=="modecombo")
        set_reset(tools, plus, minus, QDjViewPrefs::TOOL_MODECOMBO);
      else if (key=="rescombo" || key=="zoomcombo")
        set_reset(tools, plus, minus, QDjViewPrefs::TOOL_ZOOMCOMBO);
      else if (key=="zoom")
        set_reset(tools, plus, minus, QDjViewPrefs::TOOL_ZOOMBUTTONS);
      else if (key=="rotate")
        set_reset(tools, plus, minus, QDjViewPrefs::TOOL_ROTATE);
      else if (key=="search" || key=="find")
        set_reset(tools, plus, minus, QDjViewPrefs::TOOL_FIND);
      else if (key=="print")
        set_reset(tools, plus, minus, QDjViewPrefs::TOOL_PRINT);
      else if (key=="save")
        set_reset(tools, plus, minus, QDjViewPrefs::TOOL_SAVE);
      else if (key=="pagecombo")
        set_reset(tools, plus, minus, QDjViewPrefs::TOOL_PAGECOMBO);
      else if (key=="backforw")
        set_reset(tools, plus, minus, QDjViewPrefs::TOOL_BACKFORW);
      else if (key=="prevnext" || key=="prevnextpage")
        set_reset(tools, plus, minus, QDjViewPrefs::TOOL_PREVNEXT);
      else if (key=="firstlast" || key=="firstlastpage")
        set_reset(tools, plus, minus, QDjViewPrefs::TOOL_FIRSTLAST);
      // lizards
      else if (key=="doublepage")  // lizards!
        set_reset(tools, plus, minus, QDjViewPrefs::TOOL_LAYOUT);
      else if (key=="calibrate" || key=="ruler" || key=="lizard")
        errors << tr("Toolbar option '%1' is not implemented.").arg(key);
      // djview4
      else if (key=="select")
        set_reset(tools, plus, minus, QDjViewPrefs::TOOL_SELECT);
      else if (key=="new")
        set_reset(tools, plus, minus, QDjViewPrefs::TOOL_NEW);
      else if (key=="open")
        set_reset(tools, plus, minus, QDjViewPrefs::TOOL_OPEN);
      else if (key=="layout")
        set_reset(tools, plus, minus, QDjViewPrefs::TOOL_LAYOUT);
      else if (key=="help")
        set_reset(tools, plus, minus, QDjViewPrefs::TOOL_WHATSTHIS);
      else if (key!="")
        errors << tr("Toolbar option '%1' is not recognized.").arg(key);
      // handle + or -
      if (npos < len)
        {
          Tools base = QDjViewPrefs::TOOLBAR_AUTOHIDE |
            QDjViewPrefs::TOOLBAR_TOP | QDjViewPrefs::TOOLBAR_BOTTOM;
          if (str[npos] == '-')
            {
              if (!minus && !plus)
                tools |= (prefs->tools & ~base);
              plus = false;
              minus = true;
            }
          else if (str[npos] == '+')
            {
              if (!minus && !plus)
                tools &= base;
              minus = false;
              plus = true;
            }
          npos += 1;
        }
    }
  if (wantmode)
    tools |= QDjViewPrefs::TOOL_MODECOMBO;
  if (wantselect)
    tools |= QDjViewPrefs::TOOL_SELECT;
}



/*! Parse a qdjview option described by a pair \a key, \a value.
  Such options can be provided on the command line 
  or passed with the document URL as pseudo-CGI arguments.
  Return a list of error strings. */

QStringList
QDjView::parseArgument(QString key, QString value)
{
  bool okay;
  QStringList errors;
  key = key.toLower();
  
  if (key == "fullscreen" || key == "fs")
    {
      if (viewerMode < STANDALONE)
        errors << tr("Option '%1' requires a standalone viewer.").arg(key);
      if (parse_boolean(key, value, errors, okay))
        setViewerMode(okay ? STANDALONE_FULLSCREEN : STANDALONE);
    }
  else if (key == "slideshow")
    {
      if (viewerMode < STANDALONE)
        errors << tr("Option '%1' requires a standalone viewer.").arg(key);
      int delay = value.toInt(&okay);
      if (okay && delay < 0)
        okay = false;
      if (okay || parse_boolean(key, value, errors, okay))
        setViewerMode(okay ? STANDALONE_SLIDESHOW : STANDALONE);
      if (delay > 0)
        setSlideShowDelay(delay);
    }
  else if (key == "toolbar")
    {
      parseToolBarOption(value, errors);
      toolBar->setVisible(options & QDjViewPrefs::SHOW_TOOLBAR);
    }
  else if (key == "page")
    {
      pendingPage = value;
      performPendingLater();
    }
  else if (key == "pageno")
    {
      pendingPage = QString("$%1").arg(value.toInt());
      performPendingLater();
    }
  else if (key == "cache")
    {
      // see QDjVuDocument::setUrl(...)
      parse_boolean(key, value, errors, okay);
    }
  else if (key == "passive" || key == "passivestretch")
    {
      if (parse_boolean(key, value, errors, okay) && okay)
        {
          enableScrollBars(false);
          enableContextMenu(false);
          menuBar->setVisible(false);
          statusBar->setVisible(false);
          thumbnailDock->setVisible(false);
          outlineDock->setVisible(false);
          findDock->setVisible(false);
          toolBar->setVisible(false);
          if (key == "passive")
            widget->setZoom(QDjVuWidget::ZOOM_FITPAGE);
          else
            widget->setZoom(QDjVuWidget::ZOOM_STRETCH);
          widget->setContinuous(false);
          widget->setSideBySide(false);
          widget->enableKeyboard(false);
          widget->enableMouse(false);
        }
    }
  else if (key == "scrollbars")
    {
      if (parse_boolean(key, value, errors, okay))
        enableScrollBars(okay);
    }
  else if (key == "menubar")    // new for djview4
    {
      if (parse_boolean(key, value, errors, okay))
        menuBar->setVisible(okay);
    }
  else if (key == "sidebar" ||  // new for djview4
           key == "navpane" )   // lizards
    {
      showSideBar(value, errors);
    }
  else if (key == "statusbar")   // new for djview4
    {
      if (parse_boolean(key, value, errors, okay))
        statusBar->setVisible(okay);
    }
  else if (key == "continuous")  // new for djview4
    {
      if (parse_boolean(key, value, errors, okay))
        widget->setContinuous(okay);
    }
  else if (key == "side_by_side" ||
           key == "sidebyside")  // new for djview4
    {
      if (parse_boolean(key, value, errors, okay))
        widget->setSideBySide(okay);
    }
  else if (key == "firstpagealone" ||
           key == "first_page_alone" ||
           key == "cover_page" ||
           key == "coverpage")   // new for djview4
    {
      if (parse_boolean(key, value, errors, okay))
        widget->setCoverPage(okay);
    }
  else if (key == "right_to_left" ||
           key == "righttoleft") // new for djview4
    {
      if (parse_boolean(key, value, errors, okay))
        widget->setRightToLeft(okay);
    }
  else if (key == "layout")      // lizards
    {
      bool layout = false;
      bool continuous = false;
      bool sidebyside = false;
      foreach (QString s, value.split(","))
        {
          if (s == "single")
            layout = true, continuous = sidebyside = false;
          else if (s == "double") 
            layout = sidebyside = true;
          else if (s == "continuous")
            layout = continuous = true;
          else if (s == "ltor")
            widget->setRightToLeft(false);
          else if (s == "rtol")
            widget->setRightToLeft(true);
          else if (s == "cover")
            widget->setCoverPage(true);
          else if (s == "nocover")
            widget->setCoverPage(false);
          else if (s == "gap")
            widget->setSeparatorSize(12);
          else if (s == "nogap")
            widget->setSeparatorSize(0);
        }
      if (layout)
        {
          widget->setContinuous(continuous);
          widget->setSideBySide(sidebyside);
        }
    }
  else if (key == "frame")
    {
      if (parse_boolean(key, value, errors, okay))
        widget->setDisplayFrame(okay);
    }
  else if (key == "background")
    {
      QColor color;
      color.setNamedColor((value[0] == '#') ? value : "#" + value);
      if (color.isValid())
        {
          QBrush brush = QBrush(color);
          widget->setBorderBrush(brush);
          fsSavedNormal.nsBorderBrush = brush;
          fsSavedFullScreen.nsBorderBrush = brush;
          fsSavedSlideShow.nsBorderBrush = brush;
        }
      else
        illegal_value(key, value, errors);
    }
  else if (key == "menu")
    {
      if (parse_boolean(key, value, errors, okay))
        enableContextMenu(okay);
    }
  else if (key == "keyboard")
    {
      if (parse_boolean(key, value, errors, okay))
        widget->enableKeyboard(okay);
    }
  else if (key == "mouse")
    {
      if (parse_boolean(key, value, errors, okay))
        widget->enableMouse(okay);
    }
  else if (key == "links")
    {
      if (parse_boolean(key, value, errors, okay))
        widget->enableHyperlink(okay);
    }
  else if (key == "zoom")
    {
      int z = value.toInt(&okay);
      QString val = value.toLower();
      if (val == "one2one")
        widget->setZoom(QDjVuWidget::ZOOM_ONE2ONE);
      else if (val == "width" || val == "fitwidth")
        widget->setZoom(QDjVuWidget::ZOOM_FITWIDTH);
      else if (val == "page" || val == "fitpage")
        widget->setZoom(QDjVuWidget::ZOOM_FITPAGE);
      else if (val == "stretch")
        widget->setZoom(QDjVuWidget::ZOOM_STRETCH);
      else if (okay && z >= QDjVuWidget::ZOOM_MIN 
               && z <= QDjVuWidget::ZOOM_MAX)
        widget->setZoom(z);
      else
        illegal_value(key, value, errors);
    }
  else if (key == "mode")
    {
      QString val = value.toLower();
      if (val == "color")
        widget->setDisplayMode(QDjVuWidget::DISPLAY_COLOR);
      else if (val == "bw" || val == "mask" || val == "stencil")
        widget->setDisplayMode(QDjVuWidget::DISPLAY_STENCIL);
      else if (val == "fore" || val == "foreground" || val == "fg")
        widget->setDisplayMode(QDjVuWidget::DISPLAY_FG);
      else if (val == "back" || val == "background" || val == "bg")
        widget->setDisplayMode(QDjVuWidget::DISPLAY_BG);
      else if (val == "text" || val == "hiddentext")
        widget->setDisplayMode(QDjVuWidget::DISPLAY_TEXT);
      else
        illegal_value(key, value, errors);
    }
  else if (key == "hor_align" || key == "halign")
    {
      QString val = value.toLower();
      if (val == "left")
        widget->setHorizAlign(QDjVuWidget::ALIGN_LEFT);
      else if (val == "center")
        widget->setHorizAlign(QDjVuWidget::ALIGN_CENTER);
      else if (val == "right")
        widget->setHorizAlign(QDjVuWidget::ALIGN_RIGHT);
      else
        illegal_value(key, value, errors);
    }
  else if (key == "ver_align" || key == "valign")
    {
      QString val = value.toLower();
      if (val == "top")
        widget->setVertAlign(QDjVuWidget::ALIGN_TOP);
      else if (val == "center")
        widget->setVertAlign(QDjVuWidget::ALIGN_CENTER);
      else if (val == "bottom")
        widget->setVertAlign(QDjVuWidget::ALIGN_BOTTOM);
      else
        illegal_value(key, value, errors);
    }
  else if (key == "highlight")
    {                           
      int x,y,w,h;
      QColor color;
      if (value.isEmpty())
        widget->clearHighlights(widget->page());
      else if (parse_highlight(value, x, y, w, h, color))
        pendingHilite << StringPair(pendingPage, value);
      else
        illegal_value(key, value, errors);
      performPendingLater();
    }
  else if (key == "rotate")
    {
      if (value == "0")
        widget->setRotation(0);
      else if (value == "90")
        widget->setRotation(1);
      else if (value == "180")
        widget->setRotation(2);
      else if (value == "270")
        widget->setRotation(3);
      else
        illegal_value(key, value, errors);
    }
  else if (key == "print")
    {
      if (parse_boolean(key, value, errors, okay))
        printingAllowed = okay;
    }
  else if (key == "save")
    {
      if (parse_boolean(key, value, errors, okay))
        savingAllowed = okay;
    }
  else if (key == "notoolbar" || 
           key == "noscrollbars" ||
           key == "nomenu" )
    {
      QString msg = tr("Deprecated option '%1'").arg(key);
      qWarning("%s",(const char*)msg.toLocal8Bit());
      if (key == "notoolbar" && value.isNull())
        toolBar->setVisible(false);
      else if (key == "noscrollbars" && value.isNull())
        enableScrollBars(false);
      else if (key == "nomenu" && value.isNull())
        enableContextMenu(false);
    }
  else if (key == "thumbnails")
    {
      QStringList junk;
      if (! showSideBar(value+",thumbnails", junk))
        illegal_value(key, value, errors);
    }
  else if (key == "outline")
    {
      QStringList junk;
      if (! showSideBar(value+",outline", junk))
        illegal_value(key, value, errors);
    }
  else if (key == "find") // new for djview4
    {
      if (findWidget) 
        {
          findWidget->setText(QString());
          findWidget->setRegExpMode(false);
          findWidget->setWordOnly(true);
          findWidget->setCaseSensitive(false);
        }
      pendingFind = value;
      if (! value.isEmpty())
        performPendingLater();
    }
  else if (key == "showposition")
    {
      double x=0, y=0;
      pendingPosition.clear();
      if (! parse_position(value, x, y)) 
        illegal_value(key, value, errors);
      else
        {
          pendingPosition << x << y;
          performPendingLater();
        } 
    }
  else if (key == "logo" || key == "textsel" || key == "search")
    {
      QString msg = tr("Option '%1' is not implemented.").arg(key);
      qWarning("%s",(const char*)msg.toLocal8Bit());
    }
  else
    {
      errors << tr("Option '%1' is not recognized.").arg(key);
    }
  updateActionsLater();
  return errors;
}


/*! Parse a \a QDjView option expressed a a string \a key=value.
  This is very useful for processing command line options. */

QStringList
QDjView::parseArgument(QString keyEqualValue)
{
  int n = keyEqualValue.indexOf("=");
  if (n < 0)
    return parseArgument(keyEqualValue, QString());
  else
    return parseArgument(keyEqualValue.left(n),
                         keyEqualValue.mid(n+1));
}


/*! Parse the \a QDjView options passed via 
  the CGI query arguments of \a url. 
  This is called by \a QDjView::open(QDjVuDocument, QUrl). */

void 
QDjView::parseDjVuCgiArguments(QUrl url)
{
  QStringList errors;
  // parse
  bool djvuopts = false;
  QPair<QString,QString> pair;
  foreach(pair, urlQueryItems(url, true))
    {
      if (pair.first.toLower() == "djvuopts")
        djvuopts = true;
      else if (djvuopts)
        errors << parseArgument(pair.first, pair.second);
    }
  // warning for errors
  if (djvuopts && errors.size() > 0)
    foreach(QString error, errors)
      qWarning("%s",(const char*)error.toLocal8Bit());
}


/*! Return a copy of the url without the
  CGI query arguments corresponding to DjVu options. */

QUrl 
QDjView::removeDjVuCgiArguments(QUrl url)
{
  QList<QPair<QString,QString> > args;
  bool djvuopts = false;
  QPair<QString,QString> pair;
  foreach(pair, urlQueryItems(url))
    {
      if (pair.first.toLower() == "djvuopts")
        djvuopts = true;
      else if (!djvuopts)
        args << pair;
    }
  QUrl newurl = url;
  urlSetQueryItems(newurl, args);
  return newurl;
}


/*! Returns the most adequate value
  that could be passed to \a parseArgument
  for the specified \a key. */

QString      
QDjView::getArgument(QString key)
{
  key = key.toLower();
  if (key == "pages") // readonly
    return QString::number(pageNum());
  else if (key == "page")
    return pageName(widget->page());
  else if (key == "pageno")
    return QString::number(widget->page()+1);
  else if (key == "fullscreen" || key == "fs") 
    return get_boolean(viewerMode == STANDALONE_FULLSCREEN);
  else if (key == "slideshow")
    return get_boolean(viewerMode == STANDALONE_SLIDESHOW);
  else if (key == "scrollbars")
    return get_boolean(widget->horizontalScrollBarPolicy() != 
                       Qt::ScrollBarAlwaysOff );
  else if (key == "menubar")
    return get_boolean(menuBar->isVisibleTo(this));
  else if (key == "statusbar")
    return get_boolean(statusBar->isVisibleTo(this));
  else if (key == "continuous")
    return get_boolean(widget->continuous());
  else if (key == "side_by_side" || key == "sidebyside")
    return get_boolean(widget->sideBySide());
  else if (key == "coverpage" || key == "firstpagealone" || 
           key == "first_page_alone" )
    return get_boolean(widget->coverPage());
  else if (key == "righttoleft" || key == "right_to_left")
    return get_boolean(widget->rightToLeft());
  else if (key == "frame")
    return get_boolean(widget->displayFrame());
  else if (key == "keyboard")
    return get_boolean(widget->keyboardEnabled());
  else if (key == "mouse")
    return get_boolean(widget->mouseEnabled());
  else if (key == "links")
    return get_boolean(widget->hyperlinkEnabled());
  else if (key == "menu")
    return get_boolean(widget->contextMenu() != 0);
  else if (key == "rotate")
    return QString::number(widget->rotation() * 90);
  else if (key == "print")
    return get_boolean(printingAllowed);
  else if (key == "save")
    return get_boolean(savingAllowed);
  else if (key == "background")
    {
      QBrush brush = widget->borderBrush();
      if (brush.style() == Qt::SolidPattern)
        return brush.color().name();
    }
  else if (key == "layout")
    {
      QStringList l;
      if (widget->sideBySide())
        l << QString("double");
      if (widget->continuous())
        l << QString("continuous");
      if (l.isEmpty())
        l << QString("single");
      l << QString(widget->rightToLeft() ? "rtol" : "ltor");
      l << QString(widget->coverPage() ? "cover" : "nocover");
      l << QString(widget->separatorSize() ? "gap" : "nogap");
      return l.join(",");
    }
  else if (key == "zoom")
    {
      int z = widget->zoom();
      if (z == QDjVuWidget::ZOOM_ONE2ONE)
        return QString("one2one");
      else if (z == QDjVuWidget::ZOOM_FITWIDTH)
        return QString("width");
      else if (z == QDjVuWidget::ZOOM_FITPAGE)
        return QString("page");
      else if (z == QDjVuWidget::ZOOM_STRETCH)
        return QString("stretch");
      else if (z >= QDjVuWidget::ZOOM_MIN && z <= QDjVuWidget::ZOOM_MAX)
        return QString::number(z);
    }
  else if (key == "mode")
    {
      QDjVuWidget::DisplayMode m = widget->displayMode();
      if (m == QDjVuWidget::DISPLAY_COLOR)
        return QString("color");
      else if (m == QDjVuWidget::DISPLAY_STENCIL)
        return QString("bw");
      else if (m == QDjVuWidget::DISPLAY_FG)
        return QString("fore");
      else if (m == QDjVuWidget::DISPLAY_BG)
        return QString("back");
      else if (m == QDjVuWidget::DISPLAY_TEXT)
        return QString("text");
    }
  else if (key == "hor_align" || key == "halign")
    {
      QDjVuWidget::Align a = widget->horizAlign();
      if (a == QDjVuWidget::ALIGN_LEFT)
        return QString("left");
      else if (a == QDjVuWidget::ALIGN_CENTER)
        return QString("center");
      else if (a == QDjVuWidget::ALIGN_RIGHT)
        return QString("right");
    }
  else if (key == "ver_align" || key == "valign")
    {
      QDjVuWidget::Align a = widget->vertAlign();
      if (a == QDjVuWidget::ALIGN_TOP)
        return QString("top");
      else if (a == QDjVuWidget::ALIGN_CENTER)
        return QString("center");
      else if (a == QDjVuWidget::ALIGN_BOTTOM)
        return QString("bottom");
    }
  else if (key == "showposition")
    {
      QPoint center = widget->rect().center();
      QDjVuWidget::Position pos = widget->positionWithClosestAnchor(center);
      double ha = pos.hAnchor / 100.0;
      double va = pos.vAnchor / 100.0;
      return QString("%1,%2").arg(ha).arg(va);
    }
  else if (key == "passive" || key == "passivestretch")
    {
      if (widget->horizontalScrollBarPolicy() == Qt::ScrollBarAlwaysOff 
          && !menuBar->isVisibleTo(this) && !statusBar->isVisibleTo(this)
          && !thumbnailDock->isVisibleTo(this) && !outlineDock->isVisibleTo(this)
          && !findDock->isVisibleTo(this) && !toolBar->isVisibleTo(this)
          && !widget->continuous() && !widget->sideBySide()
          && !widget->keyboardEnabled() && !widget->mouseEnabled() 
          && widget->contextMenu() == 0 )
        {
          int z = widget->zoom();
          if (key == "passive" && z == QDjVuWidget::ZOOM_FITPAGE)
            return QString("yes");
          if (key == "passivestretch" && z == QDjVuWidget::ZOOM_STRETCH)
            return QString("yes");
        }
      return QString("no");
    }
  else if (key == "sidebar")
    { // without position 
      QStringList what;
      if (thumbnailDock->isVisibleTo(this))
        what << "thumbnails";
      if (outlineDock->isVisibleTo(this))
        what << "outline";
      if (findDock->isVisibleTo(this))
        what << "find";
      if (what.size() > 0)
        return QString("yes,") + what.join(",");
      else
        return QString("no");
    }
  else if (key == "toolbar")
    { // Currently without toolbar content
      return get_boolean(toolBar->isVisibleTo(this));
    }
  else if (key == "highlight")
    { // Currently not working
      return QString();
    }
  return QString();
}








// ----------------------------------------
// QDJVIEW


/*! \enum QDjView::ViewerMode
  The viewer mode is selected at creation time.
  Mode \a STANDALONE corresponds to a standalone program.
  Modes \a FULLPAGE_PLUGIN and \a EMBEDDED_PLUGIN
  correspond to a netscape plugin. */


QDjView::~QDjView()
{
  closeDocument();
}


/*! Construct a \a QDjView object using
  the specified djvu context and viewer mode. */

QDjView::QDjView(QDjVuContext &context, ViewerMode mode, QWidget *parent)
  : QMainWindow(parent),
    viewerMode(mode < STANDALONE ? mode : STANDALONE),
    djvuContext(context),
    document(0),
    hasNumericalPageTitle(true),
    updateActionsScheduled(false),
    performPendingScheduled(false),
    printingAllowed(true),
    savingAllowed(true)
{
  // Main window setup
  setWindowTitle(tr("DjView"));
  setWindowIcon(QIcon(":/images/djview.png"));
#ifndef Q_OS_DARWIN
  if (QApplication::windowIcon().isNull())
    QApplication::setWindowIcon(windowIcon());
#endif

  // Determine unique object name
  if (mode >= STANDALONE) 
    {
      QWidgetList wl = QApplication::topLevelWidgets();
      static int num = 0;
      QString name;
      while(name.isEmpty()) 
        {
          name = QString("djview%1").arg(num++);
          foreach(QWidget *w, wl)
            if (w->objectName() == name)
              name = QString();
        }
      setObjectName(name);
    }
  
  // Basic preferences
  prefs = QDjViewPrefs::instance();
  options = QDjViewPrefs::defaultOptions;
  tools = prefs->tools;
  toolsCached = zero(Tools);
  
  // Create dialogs
  errorDialog = new QDjViewErrorDialog(this);
  infoDialog = 0;
  metaDialog = 0;

  // Try using GLWidget?
  bool useOpenGL = prefs->openGLAccel;
  QString envOpenGL = QString::fromLocal8Bit(::getenv("DJVIEW_OPENGL"));
  useOpenGL |= string_is_on(envOpenGL);
  useOpenGL &= !string_is_off(envOpenGL);
  if (viewerMode < STANDALONE)
    useOpenGL = false; // GLWidget/XEmbed has focus issues
  
  // Create djvu widget
  QWidget *central = new QWidget(this);
  widget = new QDjVuWidget(useOpenGL, central);
  widget->setFrameShape(QFrame::NoFrame);
  if (viewerMode >= STANDALONE)
    widget->setFrameShadow(QFrame::Sunken);

  widget->viewport()->installEventFilter(this);
  connect(widget, SIGNAL(errorCondition(int)),
          this, SLOT(errorCondition(int)));
  connect(widget, SIGNAL(error(QString,QString,int)),
          errorDialog, SLOT(error(QString,QString,int)));
  connect(widget, SIGNAL(error(QString,QString,int)),
          this, SLOT(error(QString,QString,int)));
  connect(widget, SIGNAL(info(QString)),
          this, SLOT(info(QString)));
  connect(widget, SIGNAL(layoutChanged()),
          this, SLOT(updateActionsLater()));
  connect(widget, SIGNAL(pageChanged(int)),
          this, SLOT(updateActionsLater()));
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

  // Create splash screen
  splash = new QLabel(central);
  splash->setFrameShadow(QFrame::Sunken);
  splash->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
  splash->setPixmap(QPixmap(":/images/splash.png"));
  QPalette palette = splash->palette();
  palette.setColor(QPalette::Background, Qt::white);
  splash->setPalette(palette);
  splash->setAutoFillBackground(true);

  // Create central layout
  layout = new QStackedLayout(central);
  layout->addWidget(widget);
  layout->addWidget(splash);
  layout->setCurrentWidget(splash);
  setCentralWidget(central);

  // Create context menu
  contextMenu = new QMenu(this);
  recentMenu = 0;

  // Create menubar
  menuBar = new QMenuBar(this);
  setMenuBar(menuBar);

  // Create statusbar
  statusBar = new QStatusBar(this);
  QFont font = QApplication::font();
  font.setPointSize((font.pointSize() * 3 + 3) / 4);
  QFontMetrics metric(font);
  textLabelTimer = new QTimer(this);
  textLabelTimer->setSingleShot(true);
  connect(textLabelTimer,SIGNAL(timeout()),this,SLOT(updateTextLabel()));
  textLabel = new QLabel(statusBar);
  textLabel->setFont(font);
  textLabel->setAutoFillBackground(true);
  textLabel->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
  textLabel->setFrameStyle(QFrame::Panel);
  textLabel->setFrameShadow(QFrame::Sunken);
  textLabel->setMinimumWidth(swidth(metric,"M")*48);
  statusBar->addWidget(textLabel, 1);
  pageLabel = new QLabel(statusBar);
  pageLabel->setFont(font);
  pageLabel->setTextFormat(Qt::PlainText);
  pageLabel->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
  pageLabel->setFrameStyle(QFrame::Panel);
  pageLabel->setFrameShadow(QFrame::Sunken);
  pageLabel->setMinimumWidth(swidth(metric," P88/888 8888x8888 888dpi ")); 
  statusBar->addPermanentWidget(pageLabel);
  mouseLabel = new QLabel(statusBar);
  mouseLabel->setFont(font);
  mouseLabel->setTextFormat(Qt::PlainText);
  mouseLabel->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
  mouseLabel->setFrameStyle(QFrame::Panel);
  mouseLabel->setFrameShadow(QFrame::Sunken);
  mouseLabel->setMinimumWidth(swidth(metric," x=888 y=888 "));
  statusBar->addPermanentWidget(mouseLabel);
  setStatusBar(statusBar);

  // Create toolbar  
  toolBar = new QToolBar(this);
  toolBar->setObjectName("toolbar");
  toolBar->setAllowedAreas(Qt::TopToolBarArea|Qt::BottomToolBarArea);
  addToolBar(toolBar);

  // Create mode combo box
  modeCombo = new QComboBox(toolBar);
  fillModeCombo(modeCombo);
  connect(modeCombo, SIGNAL(activated(int)),
          this, SLOT(modeComboActivated(int)) );
  connect(modeCombo, SIGNAL(activated(int)),
          this, SLOT(updateActionsLater()) );

  // Create zoom combo box
  zoomCombo = new QComboBox(toolBar);
  zoomCombo->setEditable(true);
  zoomCombo->setInsertPolicy(QComboBox::NoInsert);
  fillZoomCombo(zoomCombo);
  connect(zoomCombo, SIGNAL(activated(int)),
          this, SLOT(zoomComboActivated(int)) );
  connect(zoomCombo->lineEdit(), SIGNAL(editingFinished()),
          this, SLOT(zoomComboEdited()) );

  // Create page combo box
  pageCombo = new QComboBox(toolBar);
  pageCombo->setEditable(true);
  pageCombo->setInsertPolicy(QComboBox::NoInsert);
  pageCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  pageCombo->installEventFilter(this);
  connect(pageCombo, SIGNAL(activated(int)),
          this, SLOT(pageComboActivated(int)));
  connect(pageCombo->lineEdit(), SIGNAL(editingFinished()),
          this, SLOT(pageComboEdited()) );
  
  // Create sidebar  
  thumbnailWidget = new QDjViewThumbnails(this);
  thumbnailDock = new QDockWidget(this);
  thumbnailDock->setObjectName("thumbnailDock");
  thumbnailDock->setWindowTitle(tr("Thumbnails"));
  thumbnailDock->setWidget(thumbnailWidget);
  thumbnailDock->installEventFilter(this);
  addDockWidget(Qt::LeftDockWidgetArea, thumbnailDock);
  outlineWidget = new QDjViewOutline(this);
  outlineDock = new QDockWidget(this);
  outlineDock->setObjectName("outlineDock");
  outlineDock->setWindowTitle(tr("Outline")); 
  outlineDock->setWidget(outlineWidget);
  outlineDock->installEventFilter(this);
  addDockWidget(Qt::LeftDockWidgetArea, outlineDock);
  findWidget = new QDjViewFind(this);
  findDock = new QDockWidget(this);
  findDock->setObjectName("findDock");
  findDock->setWindowTitle(tr("Find")); 
  findDock->setWidget(findWidget);
  findDock->installEventFilter(this);
  addDockWidget(Qt::LeftDockWidgetArea, findDock);

  // Create escape shortcut for sidebar
  shortcutEscape = new QShortcut(QKeySequence("Esc"), this);
  connect(shortcutEscape, SIGNAL(activated()), this, SLOT(performEscape()));

  // Create escape shortcut for activating page dialog
  shortcutGoPage = new QShortcut(QKeySequence("Ctrl+G"), this);
  connect(shortcutGoPage, SIGNAL(activated()), this, SLOT(performGoPage()));

  // Create MacOS shortcut for minimizing current window
#ifdef Q_OS_DARWIN
  QShortcut *shortcutMinimize = new QShortcut(QKeySequence("Ctrl+M"),this);
  connect(shortcutMinimize, SIGNAL(activated()), this, SLOT(showMinimized()));
#endif

  // Create misc timers
  undoTimer = new QTimer(this);
  undoTimer->setSingleShot(true);
  connect(undoTimer,SIGNAL(timeout()), this, SLOT(saveUndoData()));
  slideShowTimer = new QTimer(this);
  slideShowTimer->setSingleShot(true);
  connect(slideShowTimer, SIGNAL(timeout()), this, SLOT(slideShowTimeout()));
  
  // Actions
  createActions();
  createMenus();
  createWhatsThis();
  enableContextMenu(true);
  
  // Remember geometry and fix viewerMode
  if (mode >= STANDALONE)
    if (! (prefs->windowSize.isNull()))
      resize(prefs->windowSize);
  if (mode >= STANDALONE)
    if (prefs->windowMaximized)
      setWindowState(Qt::WindowMaximized);
  if (mode > STANDALONE)
    setViewerMode(mode);

  // Preferences
  connect(prefs, SIGNAL(updated()), this, SLOT(preferencesChanged()));
  applyPreferences();
  updateActions();

  // Options set so far have default priority
  widget->reduceOptionsToPriority(QDjVuWidget::PRIORITY_DEFAULT);
}


void 
QDjView::closeDocument()
{
  // remember recent document
  QDjVuDocument *doc = document;
  if (doc && viewerMode >= STANDALONE)
    {
      if (documentPages.size() > 0)
        addRecent(getDecoratedUrl());
    }
  // clear undo data
  here.clear();
  undoList.clear();
  redoList.clear();
  // kill one-shot timers
  undoTimer->stop();
  textLabelTimer->stop();
  // close document
  layout->setCurrentWidget(splash);
  if (doc)
    {
      doc->ref();
      disconnect(doc, 0, this, 0);
      disconnect(doc, 0, errorDialog, 0);
      printingAllowed = true;
      savingAllowed = true;
    }
  widget->setDocument(0);
  documentPages.clear();
  documentFileName.clear();
  hasNumericalPageTitle = true;
  documentUrl.clear();
  documentModified = QDateTime();
  document = 0;
  if (doc)
    {
      emit documentClosed(doc);
      doc->deref();
    }
}


/*! Open a document represented by a \a QDjVuDocument object. */

void 
QDjView::open(QDjVuDocument *doc, QUrl url)
{
  closeDocument();
  document = doc;
  if (url.isValid())
    documentUrl = url;
  connect(doc,SIGNAL(destroyed(void)), this, SLOT(closeDocument(void)));
  connect(doc,SIGNAL(docinfo(void)), this, SLOT(docinfo(void)));
  widget->setDocument(document);
  disconnect(document, 0, errorDialog, 0);
  layout->setCurrentWidget(widget);
  updateActions();
  docinfo();
  if (doc)
    emit documentOpened(doc);
  // process url options
  if (url.isValid())
    parseDjVuCgiArguments(url);
  widget->reduceOptionsToPriority(QDjVuWidget::PRIORITY_CGI);
  // set focus
  widget->setFocus();
  // set title
  setWindowTitle(QString("%1[*] - ").arg(getShortFileName()) + tr("DjView"));
#if QT_VERSION > 0x40400
  setWindowFilePath(url.toLocalFile());
#endif
}


/*! Open the djvu document stored in file \a filename. */

bool
QDjView::open(QString filename)
{
  closeDocument();
  QDjVuDocument *doc = new QDjVuDocument(true);
  connect(doc, SIGNAL(error(QString,QString,int)),
          errorDialog, SLOT(error(QString,QString,int)));
  doc->setFileName(&djvuContext, filename);
  if (!doc->isValid())
    {
      delete doc;
      addToErrorDialog(tr("Cannot open file '%1'.").arg(filename));
      raiseErrorDialog(QMessageBox::Critical, tr("Opening DjVu file"));
      return false;
    }
  QFileInfo fileinfo(filename);
  QUrl url = QUrl::fromLocalFile(fileinfo.absoluteFilePath());
  open(doc, url);
  documentFileName = filename;
  documentModified = QFileInfo(filename).lastModified();
  restoreRecentDocument(url);
  return true;
}


/* Helper class to open remote documents */

class QDjView::NetOpen : public QObject
{
  Q_OBJECT
private:
  QDjView *q;
  QDjVuNetDocument *doc;
  QUrl url;
  bool inNewWindow;
  bool maybeInBrowser;
  bool startedBrowser;
public:
  ~NetOpen() {
    if (doc) {
      doc->deref();
      if (! startedBrowser) {
        q->addToErrorDialog(tr("Cannot open URL '%1'.").arg(url.toString()));
        q->raiseErrorDialog(QMessageBox::Critical, tr("Opening DjVu document"));
      }
    }
  }
  NetOpen(QDjView *q, QDjVuNetDocument *d, QUrl u, bool n, bool b)
    : QObject(q),
      q(q), doc(d), url(u),
      inNewWindow(n), maybeInBrowser(b),
      startedBrowser(false) {
    doc->ref();
    connect(doc, SIGNAL(docinfo()),
            this, SLOT(docinfo()) );
    connect(doc, SIGNAL(gotContentType(QString,bool&)),
            this, SLOT(gotContentType(QString,bool&)) );
  }
public slots:
  void docinfo() {
    int status = ddjvu_document_decoding_status(*doc);
    if (status == DDJVU_JOB_OK) {
      disconnect(doc,0,this,0);
      if (inNewWindow) {
        QDjView *other = q->copyWindow();
        other->open(url);
        other->show();
      } else {
        q->open(doc,url);
      }
      doc = 0;
    }
    if (status >= DDJVU_JOB_OK)
      deleteLater();
  }
  void gotContentType(QString type, bool &okay) {
    QRegExp re("image/(x[.]|vnd[.-]|)(djvu|dejavu|iw44)(;.*)?");
    okay = re.exactMatch(type);
    if (maybeInBrowser && !okay) {
      startedBrowser = q->startBrowser(url);
      if (startedBrowser) {
        disconnect(doc,0,this,0);
        deleteLater();
      } else {
        QString msg = tr("Cannot spawn a browser for url '%1'").arg(url.toString());
        qWarning("%s",(const char*)msg.toLocal8Bit());
      }
    }
  }
};


/*! Open the djvu document available at url \a url. */

bool
QDjView::open(QUrl url, bool inNewWindow, bool maybeInBrowser)
{
  QDjVuNetDocument *doc = new QDjVuNetDocument(true);
  connect(doc, SIGNAL(error(QString,QString,int)),
          errorDialog, SLOT(error(QString,QString,int)));
  connect(doc, SIGNAL(authRequired(QString,QString&,QString&)),
          this, SLOT(authRequired(QString,QString&,QString&)) );
  connect(doc, SIGNAL(sslWhiteList(QString,bool&)),
          this, SLOT(sslWhiteList(QString,bool&)) );
  QUrl docurl = removeDjVuCgiArguments(url);
  doc->setUrl(&djvuContext, docurl);
  if (! (url.isValid() && doc->isValid()))
    {
      delete doc;
      addToErrorDialog(tr("Cannot open URL '%1'.").arg(url.toString()));
      raiseErrorDialog(QMessageBox::Critical, tr("Opening DjVu document"));
      return false;
    }
  new NetOpen(this, doc, url, inNewWindow, maybeInBrowser);
  return true;
}


void 
QDjView::sslWhiteList(QString why, bool &okay)
{
#if QT_VERSION >= 0x50000
  QString ewhy = why.toHtmlEscaped();
#else
  QString ewhy = Qt::escape(why);
#endif
  if (QMessageBox::question(this,
        tr("Certificate validation error - DjView", "dialog caption"),
        tr("<html> %1  Do you want to continue anyway? </html>").arg(ewhy),
        QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Ok)
    okay = true;
  else
    closeDocument();
}

void 
QDjView::authRequired(QString why, QString &user, QString &pass)
{
  QDjViewAuthDialog *dialog = new QDjViewAuthDialog(this);
  dialog->setInfo(why);
  dialog->setUser(user);
  dialog->setPass(pass);
  int result = dialog->exec();
  user = dialog->user();
  pass = dialog->pass();
  if (result != QDialog::Accepted)
    closeDocument();
}


/*! Reload the document afresh. */

void
QDjView::reloadDocument()
{
  QDjVuDocument *doc = document;
  QUrl url = getDecoratedUrl();
  if (doc && viewerMode >= STANDALONE && url.isValid())
    {
      closeDocument();
      ddjvu_cache_clear(djvuContext);
      // Opening files more efficiently
      QFileInfo file = url.toLocalFile();
      if (file.exists())
        {
          open(file.absoluteFilePath());
          parseDjVuCgiArguments(url);
          return;
        }
      // Opening the url could do it all in fact.
      open(url);
    }
}


void
QDjView::maybeReloadDocument()
{
  QDjVuDocument *doc = document;
  if (doc && !documentModified.isNull() && !documentFileName.isNull())
    {
      QFileInfo info(documentFileName);
      if (info.exists() && info.lastModified() > documentModified)
        QTimer::singleShot(0, this, SLOT(reloadDocument()));
    }
}


/*! Jump to the page numbered \a pageno. */

void 
QDjView::goToPage(int pageno)
{
  int pagenum = documentPages.size();
  if (!pagenum || !document)
    {
      pendingPage = QString("$%1").arg(pageno+1);
    }
  else
    {
      if (pageno>=0 && pageno<pagenum)
        {
          widget->setPage(pageno);
        }
      else
        {
          QString msg = tr("Cannot find page numbered: %1").arg(pageno+1);
          qWarning("%s",(const char*)msg.toLocal8Bit());
        }
      updateActionsLater();
    }
}


/*! Jump to the page named \a name. 
  Names starting with the "#" character are interpreted
  using the same rules as hyperlinks. In particular,
  names of the form \a "#+n" and \a "#-n" express
  relative displacement from the current page
  or the page specified by argument \a from. */

void 
QDjView::goToPage(QString name, int from)
{
  int pagenum = documentPages.size();
  if (!pagenum || !document)
    {
      pendingPage = name;
    }
  else
    {
      int pageno = pageNumber(name, from);
      if (pageno >= 0 && pageno < pagenum)
        {
          widget->setPage(pageno);
        }
      else
        {
          QString msg = tr("Cannot find page named: %1").arg(name);
          qWarning("%s",(const char*)msg.toLocal8Bit());
        }
      updateActionsLater();
    }
}


/* Jump to the page named \a pagename
   and tries to position the specified point in
   the center of the window. Arguments \a px and \a py
   must be in range 0 to 1 indicating a fraction
   of the page width or height. */

void  
QDjView::goToPosition(QString name, double px, double py)
{
  int pagenum = documentPages.size();
  if (!pagenum || !document)
    {
      pendingPosition.clear();
      pendingPosition << px << py;
      if (! name.isEmpty())
        pendingPage = name;
    }
  else
    {
      int pageno = (name.isEmpty()) ? widget->page() : pageNumber(name);
      if (pageno < 0 || pageno >= pagenum)
        {
          QString msg = tr("Cannot find page named: %1").arg(name);
          qWarning("%s",(const char*)msg.toLocal8Bit());
        }
      else 
        {
          QDjVuWidget::Position pos;
          pos.pageNo = pageno;
          pos.inPage = false;
          pos.posView = QPoint(0,0);
          pos.posPage = QPoint(0,0);
          pos.hAnchor = qBound(0, (int)(px*100), 100);
          pos.vAnchor = qBound(0, (int)(py*100), 100);
          widget->setPosition(pos, widget->rect().center());
        }
      updateActionsLater();
    }
}



/*! Add a message to the error dialog. */

void
QDjView::addToErrorDialog(QString message)
{
  errorDialog->error(message, __FILE__, __LINE__);
}


/*! Show the error dialog. */

void
QDjView::raiseErrorDialog(QMessageBox::Icon icon, QString caption)
{
  errorDialog->prepare(icon, caption);
  errorDialog->show();
  errorDialog->raise();
  errorDialog->activateWindow();
}


/*! Show the error dialog and wait until the user clicks "OK". */

int
QDjView::execErrorDialog(QMessageBox::Icon icon, QString caption)
{
  errorDialog->prepare(icon, caption);
  return errorDialog->exec();
}


/*! Set the content of the page box in the status bar. */

void  
QDjView::setPageLabelText(QString s)
{
  QLabel *m = pageLabel;
  m->setText(s);
  m->setMinimumWidth(qMax(m->minimumWidth(), m->sizeHint().width()));
}


/*! Set the content of the mouse box in the status bar. */

void  
QDjView::setMouseLabelText(QString s)
{
  QLabel *m = mouseLabel;
  m->setText(s);
  m->setMinimumWidth(qMax(m->minimumWidth(), m->sizeHint().width()));
}


/*! Display transient message in the status bar.
  This also emits signal \a pluginStatusMessage
  to mirror the message in the browser status bar. */

void  
QDjView::statusMessage(QString message)
{
  if (message.isEmpty())
    statusBar->clearMessage();
  else
    statusBar->showMessage(message);
  emit pluginStatusMessage(message);
}


/*! Returns a bitmask of the visible sidebar tabs. */

int
QDjView::visibleSideBarTabs()
{
  int tabs = 0;
  if (! thumbnailDock->isHidden())
    tabs |= 1;
  if (! outlineDock->isHidden())
    tabs |= 2;
  if (! findDock->isHidden())
    tabs |= 4;
  return tabs;
}


/*! Returns a bitmask of the hidden sidebar tabs. */

int
QDjView::hiddenSideBarTabs()
{
  int tabs = 0;
  if (thumbnailDock->isHidden())
    tabs |= 1;
  if (outlineDock->isHidden())
    tabs |= 2;
  if (findDock->isHidden())
    tabs |= 4;
  return tabs;
}


/*! Change the position and the visibility of 
    the sidebars specified by bitmask \a tabs. */

bool  
QDjView::showSideBar(Qt::DockWidgetAreas areas, int tabs)
{
  QList<QDockWidget*> allDocks;
  allDocks << thumbnailDock << outlineDock << findDock;
  // find first tab in desired area
  QDockWidget *first = 0;
  foreach(QDockWidget *w, allDocks)
    if (!first && !w->isHidden() && !w->isFloating())
      if (dockWidgetArea(w) & areas)
        first = w;
  // find relevant docks
  QList<QDockWidget*> docks;
  if (tabs & 1)
    docks << thumbnailDock;
  if (tabs & 2)
    docks << outlineDock;
  if (tabs & 4)
    docks << findDock;
  // hide all
  foreach(QDockWidget *w, docks)
    w->hide();
  if (areas)
    {
      Qt::DockWidgetArea area = Qt::LeftDockWidgetArea;
      if (areas & Qt::LeftDockWidgetArea)
        area = Qt::LeftDockWidgetArea;
      else if (areas & Qt::RightDockWidgetArea)
        area = Qt::RightDockWidgetArea;
      else if (areas & Qt::TopDockWidgetArea)
        area = Qt::TopDockWidgetArea;
      else if (areas & Qt::BottomDockWidgetArea)
        area = Qt::BottomDockWidgetArea;
      foreach(QDockWidget *w, docks)
        {
          w->show();
          if (w->isFloating())
            w->setFloating(false);
          if (! (areas & dockWidgetArea(w)))
            {
              removeDockWidget(w);
              addDockWidget(area, w);
              if (first)
                tabifyDockWidget(first, w);
              else 
                first = w;
            }
          w->show();
          w->raise();
        }
    }
  // Okay
  return true;
}


/*! Change the position and composition of the sidebar.
  String \a args contains comma separated keywords:
  \a left, \a right, \a top, \a bottom, \a yes, or \a no,
  \a outline, \a bookmarks, \a thumbnails, \a search, etc.
  Error messages are added to string list \a errors. */

bool
QDjView::showSideBar(QString args, QStringList &errors)
{
  bool yes = true;
  bool ret = true;
  int tabs = 0;
  Qt::DockWidgetAreas areas = zero(Qt::DockWidgetAreas);
  QString arg;
  foreach(arg, args.split(","))
    if (!args.isEmpty())
      {
        arg = arg.toLower();
        if (arg == "no" || arg == "false")
          yes = false;
        else if (arg == "left")
          areas |= Qt::LeftDockWidgetArea;
        else if (arg == "right")
          areas |= Qt::RightDockWidgetArea;
        else if (arg == "top")
          areas |= Qt::TopDockWidgetArea;
        else if (arg == "bottom")
          areas |= Qt::BottomDockWidgetArea;
        else if (arg == "thumbnails" || arg == "thumbnail")
          tabs |= 1;
        else if (arg == "outline" || arg == "bookmarks")
          tabs |= 2;
        else if (arg == "search" || arg == "find")
          tabs |= 4;
        else if (arg != "yes" && arg != "true") {
          errors << tr("Unrecognized sidebar options '%1'.").arg(arg);
          ret = false;
        }
      }
  if (!tabs)
    tabs = ~0;
  if (yes && !areas)
    areas = Qt::AllDockWidgetAreas;
  if (showSideBar(areas, tabs))
    return ret;
  return false;
}


/*! Overloaded version of \a showSideBar for convenience. 
  This version makes great efforts to show docks with
  the exact layout they had before being hidden. */

bool
QDjView::showSideBar(bool show)
{
  if (! show)
    {
      // save everything about dock geometry
      savedDockState = saveState();
    }
  else if (savedDockState.size() > 0) 
    {
      // hack toolbar name to avoid restoring its state
      QString savedToolBarName = toolBar->objectName();
      toolBar->setObjectName(QString());
      restoreState(savedDockState);
      toolBar->setObjectName(savedToolBarName);
      savedDockState.clear();
    }
  // set visibility
  thumbnailDock->setVisible(show);
  outlineDock->setVisible(show);
  findDock->setVisible(show);
  if (! show)
    widget->setFocus(Qt::OtherFocusReason);
  return true;
}


/*! Pops up find sidebar */
void 
QDjView::showFind()
{
  if (! actionViewSideBar->isChecked())
    actionViewSideBar->activate(QAction::Trigger);
  findDock->show();
  findDock->raise();
  findWidget->selectAll();
  findWidget->takeFocus(Qt::ShortcutFocusReason);
}


bool
QDjView::warnAboutPrintingRestrictions()
{
  if (prefs->restrictOverride && !printingAllowed)
    if (QMessageBox::warning(this, 
                             tr("Print - DjView", "dialog caption"),
                             tr("<html> This file was served with "
                                "printing restrictions. " 
                                "Do you want to print it anyway?</html>"),
                             QMessageBox::Yes | QMessageBox::Cancel,
                             QMessageBox::Cancel) != QMessageBox::Yes )
      return false;
  return true;
}


bool
QDjView::warnAboutSavingRestrictions()
{
  if (prefs->restrictOverride && !savingAllowed)
    if (QMessageBox::warning(this, 
                             tr("Save - DjView", "dialog caption"),
                             tr("<html> This file was served with "
                                "saving restrictions. " 
                                "Do you want to save it anyway?</html>"),
                             QMessageBox::Yes | QMessageBox::Cancel,
                             QMessageBox::Cancel) != QMessageBox::Yes )
      return false;
  return true;
}


/*! Pops up a print dialog */
void
QDjView::print()
{
  QDjViewPrintDialog *pd = printDialog;
  if (! pd)
    {
      printDialog = pd = new QDjViewPrintDialog(this);
      pd->setAttribute(Qt::WA_DeleteOnClose);
      pd->setWindowTitle(tr("Print - DjView", "dialog caption"));
    }
  if (warnAboutPrintingRestrictions())
    {
      pd->show();
      pd->raise();
    }
}



/*! Pops up a save dialog */
void
QDjView::saveAs()
{
  QDjViewSaveDialog *sd = saveDialog;
  if (! sd)
    {
      saveDialog = sd = new QDjViewSaveDialog(this);
      sd->setAttribute(Qt::WA_DeleteOnClose);
      sd->setWindowTitle(tr("Save - DjView", "dialog caption"));
    }
  if (warnAboutSavingRestrictions())
    {
      sd->show();
      sd->raise();
    }
}


/*! Pops up export dialog */
void
QDjView::exportAs()
{
  QDjViewExportDialog *sd = exportDialog;
  if (! sd)
    {
      exportDialog = sd = new QDjViewExportDialog(this);
      sd->setAttribute(Qt::WA_DeleteOnClose);
      sd->setWindowTitle(tr("Export - DjView", "dialog caption"));
    }
  if (warnAboutSavingRestrictions())
    {
      sd->show();
      sd->raise();
    }
}



/*! Start searching string \a find. 
    String might be terminated with a slash
    followed by letters "w" (words only),
    "W" (not words only), "c" (case sensitive),
    or "C" (case insentitive). */

void
QDjView::find(QString find)
{
  if (! find.isEmpty())
    {
      QRegExp options("/[wWcCrR]*$");
      if (find.contains(options))
        {
          for (int i=find.lastIndexOf("/"); i<find.size(); i++)
            {
              int c = find[i].toLatin1();
              if (c == 'c')
                findWidget->setCaseSensitive(true); 
              else if (c == 'C')
                findWidget->setCaseSensitive(false); 
              else if (c == 'w')
                findWidget->setWordOnly(true); 
              else if (c == 'W')
                findWidget->setWordOnly(false); 
              else if (c == 'r')
                findWidget->setRegExpMode(true); 
              else if (c == 'R')
                findWidget->setRegExpMode(false); 
            }
          find = find.remove(options);
        }
      findWidget->setText(find);
    }
  findWidget->findNext();
}






// -----------------------------------
// UTILITIES


/*! \fn QDjView::getDjVuWidget()
  Return the \a QDjVuWidget object managed by this window. */

/*! \fn QDjView::getErrorDialog()
  Return the error dialog for this window. */

/*! \fn QDjView::getDocument()
  Return the currently displayed \a QDjVuDocument. */

/*! \fn QDjView::getDocumentFileName()
  Return the filename of the currently displayed \a QDjVuDocument. */

/*! \fn QDjView::getDocumentUrl()
  Return the url of the currently displayed \a QDjVuDocument. */

/*! \fn QDjView::getViewerMode
  Return the current viewer mode. */



/*! Return an url that encodes the current view and page. */

QUrl
QDjView::getDecoratedUrl()
{
  QUrl url = removeDjVuCgiArguments(documentUrl);
  QPoint center = widget->rect().center();
  QDjVuWidget::Position pos = widget->positionWithClosestAnchor(center);
  int pageNo = pos.pageNo;
  if (url.isValid() && pageNo>=0 && pageNo<pageNum())
    {
      QueryItems items = urlQueryItems(url);
      addQueryItem(items, "djvuopts", QString());
      QList<ddjvu_fileinfo_t> &dp = documentPages;
      QString pagestr = QString("%1").arg(pageNo+1);
      if (hasNumericalPageTitle && pageNo<documentPages.size())
        // Only use page ids when confusions are possible.
        // Always using page ids triggers bugs in the celartem plugin :-(
        pagestr = QString::fromUtf8(dp[pageNo].id);
      addQueryItem(items, "page", pagestr);
      int rotation = widget->rotation();
      if (rotation) 
        addQueryItem(items, "rotate", QString::number(90 * rotation));
      QString zoom = getArgument("zoom");
      if (zoom.isEmpty()) 
        zoom = QString::number(widget->zoomFactor());
      addQueryItem(items, "zoom", zoom);
      double ha = pos.hAnchor / 100.0;
      double va = pos.vAnchor / 100.0;
      addQueryItem(items, "showposition", QString("%1,%2").arg(ha).arg(va));
      urlSetQueryItems(url, items);
    }
  return url;
}


/*! Return the base name of the current file. */

QString
QDjView::getShortFileName()
{
  if (! documentFileName.isEmpty())
    return QFileInfo(documentFileName).fileName();
  else if (! documentUrl.isEmpty())
    return documentUrl.path().section('/', -1);
  return QString();
}


/*! Return the number of pages in the document.
  This function returns zero when called before
  fully decoding the document header. */

int 
QDjView::pageNum(void)
{
  return documentPages.size();
}


/*! Return a name for page \a pageno. */

QString 
QDjView::pageName(int pageno, bool titleonly)
{
  // obtain page title
  if (pageno>=0 && pageno<documentPages.size())
    if ( documentPages[pageno].title )
      return QString::fromUtf8(documentPages[pageno].title);
  if (titleonly)
    return QString();
  // generate a name from the page number
  //// if (hasNumericalPageTitle)
  ////  return QString("$%1").arg(pageno + 1);
  return QString("%1").arg(pageno + 1);
}


/*! Return a page number for the page named \a name.
  It first searches a page whose ID matches X.
  Otherwise, if X has the form +N or -N where N is a number,
  this indicates a displacement relative to the current page.
  Otherwise, it searches a page with TITLE X starting
  from the current page and wrapping around.
  Otherwise, if X is numerical and in range, this is the page number.
  Otherwise, it searches a page whose NAME matches X.
*/

int
QDjView::pageNumber(QString name, int from)
{
  int pagenum = documentPages.size();
  if (pagenum <= 0)
    return -1;
  // First search an exact page id match
  QByteArray utf8Name = name.toUtf8();
  for (int i=0; i<pagenum; i++)
    if (documentPages[i].id && 
        !strcmp(utf8Name, documentPages[i].id))
      return i;
  // Then interpret the syntaxes +n, -n.
  // Also recognizes $n as an ordinal page number (obsolete)
  if (from < 0)
    from = widget->page();
  if (from < pagenum && name.contains(QRegExp("^[-+$]\\d+$")) )
    {
      int num = name.mid(1).toInt();
      if (name[0]=='+')
        num = from + 1 + num;
      else if (name[0]=='-')
        num = from + 1 - num;
      return qBound(1, num, pagenum) - 1;
    }
  // Then search a matching page title starting 
  // from the current page and wrapping around
  for (int i=from; i<pagenum; i++)
    if (documentPages[i].title && 
        ! strcmp(utf8Name, documentPages[i].title))
      return i;
  for (int i=0; i<from; i++)
    if (documentPages[i].title &&
        ! strcmp(utf8Name, documentPages[i].title))
      return i;
  // Then process a number in range [1..pagenum]
  if (name.contains(QRegExp("^\\d+$")))
    return qBound(1, name.toInt(), pagenum) - 1;
  // Otherwise search page names in the unlikely
  // case they are different from the page ids
  for (int i=0; i<pagenum; i++)
    if (documentPages[i].name && 
        !strcmp(utf8Name, documentPages[i].name))
      return i;
  // Compatibility with unknown viewers:
  // If name contains space, remove all spaces, and try again
  if (name.contains(" "))
    return pageNumber(name.replace(" ", ""));
  // Give up
  return -1;
}


/*! Create another main window with the same
  contents, zoom and position as the current one. */

QDjView*
QDjView::copyWindow(bool openDocument)
{
  // update preferences
  updatePreferences();
  // create new window
  QDjView *other = new QDjView(djvuContext, STANDALONE);
  QDjVuWidget *otherWidget = other->widget;
  other->setAttribute(Qt::WA_DeleteOnClose);
  // copy window geometry
  if (! (windowState() & unusualWindowStates))
    {
      other->resize( size() );
      other->toolBar->setVisible(!toolBar->isHidden());
      other->statusBar->setVisible(!statusBar->isHidden());
      other->restoreState(saveState());
    }
  // copy essential properties 
  otherWidget->setDisplayMode( widget->displayMode() );
  otherWidget->setContinuous( widget->continuous() );
  otherWidget->setSideBySide( widget->sideBySide() );
  otherWidget->setCoverPage( widget->coverPage() );
  otherWidget->setRotation( widget->rotation() );
  otherWidget->setZoom( widget->zoom() );
  // copy document
  if (document && openDocument)
    {
      other->open(document);
      other->documentFileName = documentFileName;
      other->documentUrl = documentUrl;
      // copy position
      bool saved = otherWidget->animationEnabled();
      otherWidget->enableAnimation(false);
      otherWidget->setPosition( widget->position() );
      otherWidget->enableAnimation(saved);
    }
  return other;
}


/*! Save \a text info the specified file.
  When argument \a filename is omitted, 
  a file dialog is presented to the user. */

bool 
QDjView::saveTextFile(QString text, QString filename)
{
  // obtain filename
  if (filename.isEmpty())
    {
      QString filters;
      filters += tr("Text files", "save filter")+" (*.txt);;";
      filters += tr("All files", "save filter") + " (*)";
      QString caption = tr("Save Text - DjView", "dialog caption");
      filename = QFileDialog::getSaveFileName(this, caption, "", filters);
      if (filename.isEmpty())
        return false;
    }
  // open file
  errno = 0;
  QFile file(filename);
  if (! file.open(QIODevice::WriteOnly|QIODevice::Truncate))
    {
      QString message = file.errorString();
#if HAVE_STRERROR
      if (file.error() == QFile::OpenError && errno > 0)
        message = strerror(errno);
#endif
      QMessageBox::critical(this, 
                            tr("Error - DjView", "dialog caption"),
                            tr("Cannot write file '%1'.\n%2.")
                            .arg(QFileInfo(filename).fileName())
                            .arg(message) );
      file.remove();
      return false;
    }
  // save text in current locale encoding
  QTextStream(&file) << text;
  return true;
}


/*! Save \a image info the specified file
  using a format derived from the filename suffix.
  When argument \a filename is omitted, 
  a file dialog is presented to the user. */

bool 
QDjView::saveImageFile(QImage image, QString filename)
{
  // obtain filename with suitable suffix
  if (filename.isEmpty())
    {
      QString filters;
      QList<QByteArray> formats = QImageWriter::supportedImageFormats(); 
      foreach(QByteArray format, formats)
        filters += tr("%1 files (*.%2);;", "save image filter")
        .arg(QString(format).toUpper())
        .arg(QString(format).toLower());
      filters += tr("All files", "save filter") + " (*)";
      QString caption = tr("Save Image - DjView", "dialog caption");
      filename = QFileInfo(getShortFileName()).baseName();
      if (formats.size())
        filename += "." + QString(formats[0]);
      filename = QFileDialog::getSaveFileName(this, caption, 
                                              filename, filters);
      if (filename.isEmpty())
        return false;
    }
  // suffix
  QString suffix = QFileInfo(filename).suffix();
  if (suffix.isEmpty())
    {
      QMessageBox::critical(this, 
                            tr("Error - DjView", "dialog caption"),
                            tr("Cannot determine file format.\n"
                               "Filename '%1' has no suffix.")
                            .arg(QFileInfo(filename).fileName()) );
      return false;
    }
  // write
  errno = 0;
  QFile file(filename);
  QImageWriter writer(&file, suffix.toLatin1());
  if (! writer.write(image))
    {
      QString message = file.errorString();
      if (writer.error() == QImageWriter::UnsupportedFormatError)
        message = tr("Image format %1 not supported.").arg(suffix.toUpper());
#if HAVE_STRERROR
      else if (file.error() == QFile::OpenError && errno > 0)
        message = strerror(errno);
#endif
      QMessageBox::critical(this,
                            tr("Error - DjView", "dialog caption"),
                            tr("Cannot write file '%1'.\n%2.")
                            .arg(QFileInfo(filename).fileName())
                            .arg(message) );
      file.remove();
      return false;
    }
  return true;
}


/*! Start the preferred browser on \a url. */

bool
QDjView::startBrowser(QUrl url)
{
  // Determine browsers to try
  QStringList browsers;
#ifdef Q_OS_DARWIN
  browsers << "open";
#endif
#ifdef Q_OS_WIN32
# if QT_VERSION < 0x40200
  browsers << "firefox.exe";
  browsers << "iexplore.exe";
# endif
#endif
#ifdef Q_OS_UNIX
  browsers << "x-www-browser" << "firefox" << "konqueror";
  const char *varBrowser = ::getenv("BROWSER");
  if (varBrowser && varBrowser[0])
    browsers = QFile::decodeName(varBrowser).split(":") + browsers;
  browsers << "sensible-browser";
  const char *varPath = ::getenv("PATH");
  QStringList path;
  if (varPath && varPath[0])
    path = QFile::decodeName(varPath).split(":");
  path << ".";
#endif
  if (! prefs->browserProgram.isEmpty())
    browsers.prepend(prefs->browserProgram);
  // Try them
  QStringList args(url.toEncoded());
  foreach (QString browser, browsers)
    {
#ifdef Q_OS_UNIX
      int i;
      for (i=0; i<path.size(); i++)
        if (QFileInfo(QDir(path[i]), browser).exists())
          break;
      if (i >= path.size())
        continue;
#endif
      if (QProcess::startDetached(browser, args))
        return true;
    }
  // fallback
#if QT_VERSION >= 0x40200
  return QDesktopServices::openUrl(url);
#else
  return false;
#endif
}



/*! \fn QDjView::documentClosed(QDjVuDocument *doc)
  This signal is emitted when clearing the current document.
  Argument \a doc is the previous document. */

/*! \fn QDjView::documentOpened(QDjVuDocument *doc)
  This signal is emitted when opening a new document \a doc. */
  
/*! \fn QDjView::documentReady(QDjVuDocument *doc)
  This signal is emitted when the document structure 
  for the current document \a doc is known. */

/*! \fn QDjView::pluginStatusMessage(QString message = QString())
  This signal is emitted when a message is displayed
  in the status bar. This is useful for plugins because
  the message must be replicated in the browser status bar. */

/*! \fn QDjView::pluginGetUrl(QUrl url, QString target)
  This signal is emitted when the user clicks an external
  hyperlink while the viewer is in plugin mode. */



// -----------------------------------
// OVERRIDES


void
QDjView::closeEvent(QCloseEvent *event)
{
  // Close document.
  closeDocument();
  // Save options.
  //  after closing the document in order to 
  //  avoid saving document defined settings.
  updatePreferences();
  prefs->saveRemembered();
  // continue closing the window
  event->accept();
}


void 
QDjView::dragEnterEvent(QDragEnterEvent *event)
{
  if (viewerMode >= STANDALONE)
    {
      const QMimeData *data = event->mimeData();
      if (data->hasUrls() && data->urls().size()==1)
        event->accept();
    }
}


void 
QDjView::dragMoveEvent(QDragMoveEvent *event)
{
#ifdef Q_OS_WIN
  QMainWindow::dragMoveEvent(event);
#else
  QRect rect = centralWidget()->geometry();
  if (event->answerRect().intersects(rect))
    event->accept(rect);
  else
    event->ignore();
#endif
}


void 
QDjView::dropEvent(QDropEvent *event)
{
  if (viewerMode >= STANDALONE)
    {
      const QMimeData *data = event->mimeData();
      if (data->hasUrls() && data->urls().size()==1)
        if (open(data->urls()[0]))
          {
            event->setDropAction(Qt::CopyAction);
            event->accept();
          }
    }
}


bool 
QDjView::eventFilter(QObject *watched, QEvent *event)
{
  switch(event->type())
    {
    case QEvent::Show:
    case QEvent::Hide:
      if (qobject_cast<QDockWidget*>(watched))
        updateActionsLater();
      break;
    case QEvent::Leave:
      if ((watched == widget->viewport()) ||
          (qobject_cast<QDockWidget*>(watched)) )
        {
          mouseLabel->clear();
          textLabel->clear();
          statusMessage();
          return false;
        }
      break;
    case QEvent::Enter:
      if ((watched == widget->viewport()) ||
          (qobject_cast<QDockWidget*>(watched)) )
        maybeReloadDocument();
      break;
    case QEvent::StatusTip:
      if (qobject_cast<QMenu*>(watched))
        return QApplication::sendEvent(this, event);
      break;
    case QEvent::FocusIn:
      if (watched == pageCombo)
        if (widget && documentPages.size())
          pageCombo->setEditText(pageName(widget->page()));
      break;
    default:
      break;
    }
  return false;
}


// -----------------------------------
// PROTECTED SIGNALS


void 
QDjView::info(QString message)
{
  if (! message.contains(QRegExp("^\\[\\d+")))
    statusBar->showMessage(message, 2000);
  qWarning("INFO: %s", (const char*)message.toLocal8Bit());
}


void 
QDjView::error(QString message, QString filename, int lineno)
{
  // just for calling qWarning
  filename = filename.section("/", -1);
  if (filename.isEmpty())
    qWarning("ERROR: %s", (const char*)message.toLocal8Bit());
  else
    qWarning("ERROR (%s:%d): %s", (const char*)filename.toLocal8Bit(), 
             lineno, (const char*)message.toLocal8Bit() );
}


void 
QDjView::errorCondition(int pageno)
{
  QString message;
  if (pageno >= 0)
    message = tr("Cannot decode page %1.").arg(pageno+1);
  else
    message = tr("Cannot decode document.");
  addToErrorDialog(message);
  raiseErrorDialog(QMessageBox::Warning, tr("Decoding DjVu document"));
}


void 
QDjView::docinfo()
{
  if (document && documentPages.size()==0 &&
      ddjvu_document_decoding_status(*document)==DDJVU_JOB_OK)
    {
      // Obtain information about pages.
      int n = ddjvu_document_get_pagenum(*document);
      int m = ddjvu_document_get_filenum(*document);
      for (int i=0; i<m; i++)
        {
          ddjvu_fileinfo_t info;
          if (ddjvu_document_get_fileinfo(*document, i, &info)!=DDJVU_JOB_OK)
            qWarning("Internal(docinfo): ddjvu_document_get_fileinfo fails.");
          if (info.title && info.name && !strcmp(info.title, info.name))
            info.title = 0;  // clear title if equal to name.
          if (info.type == 'P')
            documentPages << info;
        }
      if (documentPages.size() != n)
        qWarning("Internal(docinfo): inconsistent number of pages.");

      // Check for numerical title
      hasNumericalPageTitle = false;
      QRegExp allNumbers("\\d+");
      for (int i=0; i<n; i++)
        if (documentPages[i].title &&
            allNumbers.exactMatch(QString::fromUtf8(documentPages[i].title)) )
          hasNumericalPageTitle = true;
      
      // Fill page combo
      fillPageCombo(pageCombo);
      
      // Continue processing a bit later
      QTimer::singleShot(0, this, SLOT(docinfoExtra()));
    }
}

void 
QDjView::docinfoExtra()
{
  if (document && documentPages.size()>0)
    {
      bool saved = widget->animationEnabled();
      widget->enableAnimation(false);
      performPending();
      widget->enableAnimation(saved);
      updateActionsLater();
      emit documentReady(document);
    }
}


/*! Set options that could not be set before fully decoding
  the document header. For instance, calling \a QDjView::goToPage
  before decoding the djvu document header simply stores
  the page number in a well known variable. Function \a performPending
  peforms the page change as soon as the document information
  is available. */


void
QDjView::performPending()
{
  if (! documentPages.isEmpty())
    {
      if (! pendingPosition.isEmpty())
        {
          if (pendingPosition.size() == 2)
            goToPosition(pendingPage, pendingPosition[0], pendingPosition[1]);
          pendingPosition.clear();
          pendingPage.clear();
        }
      if (! pendingPage.isNull())
        {
          goToPage(pendingPage);
          pendingPage.clear();
        }
      if (pendingHilite.size() > 0)
        {
          StringPair pair;
          foreach(pair, pendingHilite)
            {
              int x, y, w, h;
              QColor color = Qt::blue;
              int pageno = widget->page();
              if (! pair.first.isEmpty())
                pageno = pageNumber(pair.first);
              if (pageno < pageNum() && pageno >= 0 
                    && parse_highlight(pair.second, x, y, w, h, color) 
                    && w > 0 && h > 0 )
                {
                  color.setAlpha(96);
                  widget->addHighlight(pageno, x, y, w, h, color);
                }
            }
          pendingHilite.clear();
        }
      if (pendingFind.size() > 0)
        {
          find(pendingFind);
          pendingFind.clear();
        }
    }
  performPendingScheduled = false;
}


/*! Schedule a call to \a QDjView::performPending(). */

void
QDjView::performPendingLater()
{
  if (! performPendingScheduled)
    {
      performPendingScheduled = true;
      QTimer::singleShot(0, this, SLOT(performPending()));
    }
}


void 
QDjView::pointerPosition(const Position &pos, const PageInfo &info)
{
  // setup page label
  QString p = "";
  QString m = "";
  if (pos.pageNo >= 0)
    {
      p = tr(" P%1/%2 %3x%4 %5dpi ")
        .arg(pos.pageNo+1).arg(documentPages.size()) 
        .arg(info.width).arg(info.height).arg(info.dpi);
    }
  if (pos.inPage || !info.segment.isEmpty())
    {
      if (info.segment.isEmpty())
        m = tr(" x=%1 y=%2 ")
          .arg(pos.posPage.x())
          .arg(pos.posPage.y());
      else
        m = tr(" %3x%4+%1+%2 ")
          .arg(info.segment.left())
          .arg(info.segment.top())
          .arg(info.segment.width())
          .arg(info.segment.height());
    }
  setPageLabelText(p);
  setMouseLabelText(m);
  if (textLabel->isVisibleTo(this))
    {
      textLabelRect = info.selected;
      textLabelTimer->start(25);
    }
}


void 
QDjView::updateTextLabel()
{
  textLabel->clear();
  textLabel->setWordWrap(false);
  textLabel->setTextFormat(Qt::PlainText);
  if (document && textLabel->isVisibleTo(this))
    {
      QString text;
      QFontMetrics m(textLabel->font());
      QString lb = QString::fromUtf8(" \302\253 ");
      QString rb = QString::fromUtf8(" \302\273 ");
      int w = textLabel->width()-2*textLabel->frameWidth()-swidth(m,lb+rb+"MM");
      if (! textLabelRect.isEmpty())
        {
          text = widget->getTextForRect(textLabelRect);
          text = text.replace(QRegExp("\\s+"), " ");
          text = m.elidedText(text, Qt::ElideMiddle, w);
        }
      else
        {
          QString results[3];
          if (widget->getTextForPointer(results))
            {
              results[0] = results[0].simplified();
              results[1] = results[1].simplified();
              results[2] = results[2].simplified();
              if (results[0].size() || results[2].size())
                results[1] = " [" + results[1] + "] ";
              int r1 = swidth(m,results[1]);
              int r2 = swidth(m,results[2]);
              int r0 = qMax(0, qMax( (w-r1)/2, (w-r1-r2) ));
              text = m.elidedText(results[0], Qt::ElideLeft, r0) + results[1];
              text = m.elidedText(text+results[2], Qt::ElideRight, w);
            }
        }
      text = text.trimmed();
      if (text.size())
        textLabel->setText(lb + text + rb);
    }
}


void 
QDjView::pointerEnter(const Position&, miniexp_t)
{
  // Display information message about hyperlink
  QString link = widget->linkUrl();
  if (link.isEmpty())
    return;
  QString target = widget->linkTarget();
  if (target=="_self" || target=="_page")
    target.clear();
  QString message;
  if (link.startsWith("#") &&
      link.contains(QRegExp("^#[-+]\\d+$")) )
    {
      int n = link.mid(2).toInt();
      if (link[1]=='+')
        message = (n==1) ? tr("Go: 1 page forward.") 
          : tr("Go: %n pages forward.", 0, n);
      else
        message = (n==1) ? tr("Go: 1 page backward.")
          : tr("Go: %n pages backward.", 0, n);
    }
  else if (link.startsWith("#$"))
    message = tr("Go: page %1.").arg(link.mid(2));
  else if (link.startsWith("#"))
    message = tr("Go: page %1.").arg(link.mid(1));
  else
    message = tr("Go: %1").arg(link);
  if (!target.isEmpty())
    message = message + tr(" (in other window.)");
  
  statusMessage(message);
}


void 
QDjView::pointerLeave(const Position&, miniexp_t)
{
  statusMessage();
}


void 
QDjView::pointerClick(const Position &pos, miniexp_t)
{
  goToLink(widget->linkUrl(), widget->linkTarget(), pos.pageNo);
}


void  
QDjView::goToLink(QString link, QString target, int fromPage)
{
  bool inPlace = target.isEmpty() || target=="_self" || target=="_page";
  QUrl url = documentUrl;
  // Internal link
  if (link.startsWith("#"))
    {
      QString name = link.mid(1);
      if (inPlace)
        {
          goToPage(name, fromPage);
          return;
        }
      if (viewerMode >= STANDALONE)
        {
          QDjView *other = copyWindow();
          other->goToPage(name, fromPage);
          other->show();
          return;
        }
      // Construct url
      url = removeDjVuCgiArguments(url);
      QueryItems items = urlQueryItems(url);
      addQueryItem(items, "djvuopts", QString());
      int pageno = pageNumber(name, fromPage);
      if (pageno>=0 && pageno<=documentPages.size())
        addQueryItem(items, "page", QString::fromUtf8(documentPages[pageno].id));
      urlSetQueryItems(url, items);
    }
  else if (link.startsWith("?"))
    {
      if (inPlace)
        {
          foreach(QString opt, link.mid(1).split("&"))
            parseArgument(opt);
          return;
        }
      if (viewerMode >= STANDALONE)
        {
          QDjView *other = copyWindow();
          foreach(QString opt, link.mid(1).split("&"))
            other->parseArgument(opt);
          other->show();
          return;
        }
      // Construct url
      QUrl linkUrl = QUrl::fromEncoded("file://d/d" + link.toUtf8());
      url = removeDjVuCgiArguments(url);
      QueryItems items = urlQueryItems(url);
      addQueryItem(items, "djvuopts", "");
      QPair<QString,QString> pair;
      foreach(pair, urlQueryItems(linkUrl))
        addQueryItem(items, pair.first, pair.second);
      urlSetQueryItems(url, items);
    }
  else
    {
      // Decode url
      QUrl linkUrl = QUrl::fromEncoded(link.toUtf8());
      // Resolve url
      QueryItems empty;
      urlSetQueryItems(url, empty);
      url = url.resolved(linkUrl);
      urlSetQueryItems(url, urlQueryItems(linkUrl));
    }
  // Check url
  if (! url.isValid() || url.isRelative())
    {
      QString msg = tr("Cannot resolve link '%1'").arg(link);
      qWarning("%s", (const char*)msg.toLocal8Bit());
      return;
    }
  // Signal browser
  if (viewerMode < STANDALONE)
    {
      emit pluginGetUrl(url, target);
      return;
    }
  // Standalone only: call open(QUrl,...) with adequate flags
  open(url, !inPlace, true);
}


void 
QDjView::pointerSelect(const QPoint &pointerPos, const QRect &rect)
{
  // Collect text
  QString text=widget->getTextForRect(rect);
  int l = text.size();
  int w = rect.width();
  int h = rect.height();
  QString s = tr("%n characters", 0, l);
  if (QApplication::clipboard()->supportsSelection())
    QApplication::clipboard()->setText(text, QClipboard::Selection);
  // Prepare menu
  QMenu *menu = new QMenu(this);
  QAction *copyText = menu->addAction(tr("Copy text (%1)").arg(s));
  QAction *saveText = menu->addAction(tr("Save text as..."));
  copyText->setEnabled(l>0);
  saveText->setEnabled(l>0);
  copyText->setStatusTip(tr("Copy text into the clipboard."));
  saveText->setStatusTip(tr("Save text into a file."));
  menu->addSeparator();
  QString copyImageString = tr("Copy image (%1x%2 pixels)").arg(w).arg(h);
  QAction *copyImage = menu->addAction(copyImageString);
  QAction *saveImage = menu->addAction(tr("Save image as..."));
  copyImage->setStatusTip(tr("Copy image into the clipboard."));
  saveImage->setStatusTip(tr("Save image into a file."));
  menu->addSeparator();
  QAction *zoom = menu->addAction(tr("Zoom to rectangle"));
  zoom->setStatusTip(tr("Zoom the selection to fit the window."));
  QAction *copyUrl = 0;
  QAction *copyMaparea = 0;
  if (prefs->advancedFeatures)
    {
      menu->addSeparator();
      copyUrl = menu->addAction(tr("Copy URL"));
      copyUrl->setStatusTip(tr("Save into the clipboard an URL that "
                               "highlights the selection."));
      copyMaparea = menu->addAction(tr("Copy Maparea"));
      copyMaparea->setStatusTip(tr("Save into the clipboard a maparea "
                                   "annotation expression for program "
                                   "djvused."));
    }
  
  // Make sure that status tips work (hack)
  menu->installEventFilter(this);

  // Execute menu
  QAction *action = menu->exec(pointerPos-QPoint(5,5), copyText);
  if (action == zoom)
    widget->zoomRect(rect);
  else if (action == copyText)
    QApplication::clipboard()->setText(text);
  else if (action == saveText)
    saveTextFile(text);
  else if (action == copyImage)
    QApplication::clipboard()->setImage(widget->getImageForRect(rect));
  else if (action == saveImage)
    saveImageFile(widget->getImageForRect(rect));
  else if (action && action == copyMaparea)
    {
      Position pos = widget->position(pointerPos);
      QRect seg = widget->getSegmentForRect(rect, pos.pageNo);
      if (! rect.isEmpty())
        {
          QString s = QString("(maparea \"url\"\n"
                              "         \"comment\"\n"
                              "         (rect %1 %2 %3 %4))")
            .arg(seg.left()).arg(seg.top())
            .arg(seg.width()).arg(seg.height());
          QApplication::clipboard()->setText(s);
        }
    }
  else if (action && action == copyUrl)
    {
      QUrl url = getDecoratedUrl();
      Position pos = widget->position(pointerPos);
      QRect seg = widget->getSegmentForRect(rect, pos.pageNo);
      if (url.isValid() && pos.pageNo>=0 && pos.pageNo<pageNum())
        {
          QueryItems items = urlQueryItems(url);
          if (! hasQueryItem(items, "djvuopts"))
            addQueryItem(items, "djvuopts", QString());
          QList<ddjvu_fileinfo_t> &dp = documentPages;
          if (! hasQueryItem(items, "page", false))
            if (pos.pageNo>=0 && pos.pageNo<documentPages.size())
              addQueryItem(items, "page", QString::fromUtf8(dp[pos.pageNo].id));
          if (! rect.isEmpty())
            addQueryItem(items, "highlight", QString("%1,%2,%3,%4")
                         .arg(seg.left()).arg(seg.top())
                         .arg(seg.width()).arg(seg.height()) );
          urlSetQueryItems(url, items);
          QApplication::clipboard()->setText(url.toString());
        }
    }
  
  // Cancel select mode.
  updateActionsLater();
  if (actionSelect->isChecked())
    {
      actionSelect->setChecked(false);
      performSelect(false);
    }
}


/*! Schedule a call to \a QDjView::updateActions(). */

void
QDjView::updateActionsLater()
{
  if (! updateActionsScheduled)
    {
      updateActionsScheduled = true;
      QTimer::singleShot(0, this, SLOT(updateActions()));
    }
}


void
QDjView::modeComboActivated(int index)
{
  int mode = modeCombo->itemData(index).toInt();
  widget->setDisplayMode((QDjVuWidget::DisplayMode)mode);
  widget->setFocus();
}


void
QDjView::zoomComboActivated(int index)
{
  int zoom = zoomCombo->itemData(index).toInt();
  widget->setZoom(zoom);
  updateActionsLater();
  widget->setFocus();
}


void 
QDjView::zoomComboEdited(void)
{
  bool okay;
  QString text = zoomCombo->lineEdit()->text();
  int zoom = text.replace(QRegExp("\\s*%?$"),"").trimmed().toInt(&okay);
  if (okay && zoom>0)
    widget->setZoom(zoom);
  updateActionsLater();
  if (okay)
    widget->setFocus();
}


void 
QDjView::pageComboActivated(int index)
{
  goToPage(pageCombo->itemData(index).toInt());
  updateActionsLater();
  widget->setFocus();
}


void 
QDjView::pageComboEdited(void)
{
  QString data = pageCombo->lineEdit()->text().trimmed();
  int pagenum = documentPages.size();
  QRegExp pattern = QRegExp(QString("\\s*(\\d+)\\s*/\\s*%1\\s*").arg(pagenum));
  if (pattern.exactMatch(data))
    goToPage(qMax(0, pattern.cap(1).toInt() - 1));
  else
    goToPage(data);
  updateActionsLater();
  widget->setFocus();
}




// -----------------------------------
// SIGNALS IMPLEMENTING ACTIONS



void
QDjView::performAbout(void)
{
  QStringList version;
#if DDJVUAPI_VERSION >= 20
  version << QString(ddjvu_get_version_string());
#endif
#ifdef QT_VERSION
  version << QString("Qt-%1").arg(qVersion());
#endif
  QString versioninfo = "";
  if (version.size() > 0)
    versioninfo = "(" + version.join(", ") + ")";
  QString html = tr8("<html>"
       "<h2>DjVuLibre DjView %1</h2>%2"
       "<p>"
       "Viewer for DjVu documents<br>"
       "<a href=%3>%3</a><br>"
       "Copyright \302\251 2006-- L\303\251on Bottou."
       "</p>"
       "<p align=justify><small>"
       "This program is free software. "
       "You can redistribute or modify it under the terms of the "
       "GNU General Public License as published "
       "by the Free Software Foundation. "
       "This program is distributed <i>without any warranty</i>. "
       "See the GNU General Public License for more details."
       "</small></p>"
       "</html>")
    .arg(QDjViewPrefs::versionString())
    .arg(versioninfo)
    .arg("http://djvu.sourceforge.net");

  QMessageBox::about(this, tr("About DjView"), html);
}


void
QDjView::performNew(void)
{
  if (viewerMode < STANDALONE) 
    return;
  QDjView *other = copyWindow(false);
  other->show();
}


void
QDjView::performOpen(void)
{
  if (viewerMode < STANDALONE)
    return;
  QString filters;
  filters += tr("DjVu files") + " (*.djvu *.djv);;";
  filters += tr("All files") + " (*)";
  QString caption = tr("Open - DjView", "dialog caption");
  QString dirname = QDir::currentPath();
  QDir dir = QFileInfo(documentFileName).absoluteDir();
  if (dir.exists() && !documentFileName.isEmpty())
    dirname = dir.absolutePath();
  QString fname;
  fname = QFileDialog::getOpenFileName(this, caption, dirname, filters);
  if (! fname.isEmpty())
    open(fname);
}


void
QDjView::performOpenLocation(void)
{
  if (viewerMode < STANDALONE)
    return;
  QString caption = tr("Open Location - DjView", "dialog caption");
  QString label = tr("Enter the URL of a DjVu document:");
  QString text = "http://";
  bool ok;
  QUrl url  = QInputDialog::getText(this, caption, label, 
                                    QLineEdit::Normal, text, &ok);
  if (ok && url.isValid())
	open(url);
}


void 
QDjView::performInformation(void)
{
  if (! documentPages.size())
    return;
  if (! infoDialog)
    infoDialog = new QDjViewInfoDialog(this);
  infoDialog->setWindowTitle(tr("Information - DjView", "dialog caption"));
  infoDialog->setPage(widget->page());
  infoDialog->refresh();
  infoDialog->raise();
  infoDialog->show();
}


void 
QDjView::performMetadata(void)
{
  if (! documentPages.size())
    return;
  if (! metaDialog)
    metaDialog = new QDjViewMetaDialog(this);
  metaDialog->setWindowTitle(tr("Metadata - DjView", "dialog caption"));
  metaDialog->setPage(widget->page());
  metaDialog->refresh();
  metaDialog->raise();
  metaDialog->show();
}


void
QDjView::performPreferences(void)
{
  updatePreferences();
  QDjViewPrefsDialog *dialog = QDjViewPrefsDialog::instance();
  dialog->load(this);
  dialog->show();
  dialog->raise();
}


void 
QDjView::performRotation(void)
{
  QAction *action = qobject_cast<QAction*>(sender());
  int rotation = action->data().toInt();
  widget->setRotation(rotation);
}


void 
QDjView::performZoom(void)
{
  QAction *action = qobject_cast<QAction*>(sender());
  int zoom = action->data().toInt();
  widget->setZoom(zoom);
}


void 
QDjView::performSelect(bool checked)
{
  if (checked)
    widget->setModifiersForSelect(Qt::NoModifier);
  else
    widget->setModifiersForSelect(prefs->modifiersForSelect);
}


bool
QDjView::setViewerMode(ViewerMode mode)
{
  Saved *savedPrefs;
  Saved *savedConfig;
  if (viewerMode == mode)
    return true;
  if (viewerMode < STANDALONE || mode < STANDALONE)
    return false;
  savedPrefs = getSavedPrefs();
  savedConfig = getSavedConfig(viewerMode);
  savedConfig->remember = true;
  updateSaved(savedConfig);
  updateSaved(savedPrefs);
  Qt::WindowStates wstate = windowState();
  wstate &= ~unusualWindowStates;
  if (mode > STANDALONE)
    wstate |= Qt::WindowFullScreen;
  if (mode > STANDALONE)
    setUnifiedTitleAndToolBarOnMac(false);
  savedConfig = getSavedConfig(mode);
  setWindowState(wstate);
  applySaved(savedConfig);
  widget->setBorderBrush(savedConfig->nsBorderBrush);
  widget->setBorderSize(savedConfig->nsBorderSize);
  viewerMode = mode;
  // slideshow delay
  if (mode == STANDALONE_SLIDESHOW)
    setSlideShowDelay(prefs->slideShowDelay);
  // make sure checkboxes are set right.
  updateActionsLater();
  return true;
}

void
QDjView::setSlideShowDelay(int delay)
{
  slideShowCounter = 0;
  slideShowDelay = delay;
  slideShowTimeout(true);
}

void
QDjView::slideShowTimeout(bool reset)
{
  double ratio = 0;
  bool ssmode = (viewerMode == STANDALONE_SLIDESHOW) && (slideShowDelay>0);
  bool active = ssmode && (widget->page() < pageNum()-1);
  bool newpage = (slideShowCounter >= slideShowDelay-1);
  slideShowCounter = (reset||newpage||!active) ? 0 : slideShowCounter+1;
  if (newpage && active)
    widget->nextPage();
  if (active)
    ratio = 1.0 - (double)slideShowCounter / slideShowDelay;
  widget->setHourGlassRatio(ratio);
  slideShowTimer->stop();
  if (ssmode)
    slideShowTimer->start(1000);
}

void 
QDjView::performViewFullScreen(bool checked)
{
  if (checked)
    setViewerMode(STANDALONE_FULLSCREEN);
  else
    setViewerMode(STANDALONE);
}

void 
QDjView::performViewSlideShow(bool checked)
{
  if (checked)
    setViewerMode(STANDALONE_SLIDESHOW);
  else
    setViewerMode(STANDALONE);
}


void 
QDjView::performEscape()
{
  if (actionViewSideBar->isChecked())
    actionViewSideBar->activate(QAction::Trigger);
  else if (viewerMode > STANDALONE && widget->keyboardEnabled())
    setViewerMode(STANDALONE);
}


void 
QDjView::performGoPage()
{
  if (toolBar->isVisibleTo(this) || widget->keyboardEnabled())
    if (toolsCached & QDjViewPrefs::TOOL_PAGECOMBO)
      {
        toolBar->show();
        pageCombo->setFocus();
        QTimer::singleShot(0, pageCombo->lineEdit(), SLOT(selectAll()));
      }
}


void
QDjView::restoreRecentDocument(QUrl url)
{
  url.setPassword(QString());
  QUrl cleanUrl = removeDjVuCgiArguments(url);
  QString prefix = cleanUrl.toString(QUrl::RemoveQuery);
  foreach (QString recent, prefs->recentFiles)
    if (recent.startsWith(prefix))
      {
        QUrl recentUrl = recent;
        QUrl cleanRecentUrl = removeDjVuCgiArguments(recentUrl);
        if (cleanUrl == cleanRecentUrl)
          {
            parseDjVuCgiArguments(recentUrl);
            return;
          }
      }
}


void 
QDjView::addRecent(QUrl url)
{
  prefs->loadRecent();
  // never remember passwords
  url.setPassword(QString());
  // remove matching entries
  QUrl cleanUrl = removeDjVuCgiArguments(url);
  QString prefix = cleanUrl.toString(QUrl::RemoveQuery);
  QStringList::iterator it = prefs->recentFiles.begin();
  while (it != prefs->recentFiles.end())
    {
      if (it->startsWith(prefix) &&
          cleanUrl == removeDjVuCgiArguments(QUrl(*it)))
        it = prefs->recentFiles.erase(it);
      else
        ++it;
    }
  // add new url (max 50)
  QString name = url.toString();
  prefs->recentFiles.prepend(name);
  while(prefs->recentFiles.size() > 50)
    prefs->recentFiles.pop_back();
  // save
  prefs->saveRecent();
}


void 
QDjView::fillRecent()
{
  if (recentMenu)
    {
      recentMenu->clear();
      prefs->loadRecent();
      int n = qMin(prefs->recentFiles.size(), 8);
      for (int i=0; i<n; i++)
        {
          QUrl url = prefs->recentFiles[i];
          QString base = url.path().section("/",-1);
          QString name = url.toLocalFile();
          if (name.isEmpty())
            name = removeDjVuCgiArguments(url).toString();
          QFont defaultFont;
          QFontMetrics metrics(defaultFont);
          name = metrics.elidedText(name, Qt::ElideMiddle, 400);
          name = QString("%1 [%2]").arg(base).arg(name);
          QAction *action = recentMenu->addAction(name);
          action->setData(url);
          connect(action,SIGNAL(triggered()), this, SLOT(openRecent()));
        }
      recentMenu->addSeparator();
    }
  QAction *action = recentMenu->addAction(tr("&Clear History"));
  connect(action, SIGNAL(triggered()), this, SLOT(clearRecent()));
}


void 
QDjView::openRecent()
{
  QAction *action = qobject_cast<QAction*>(sender());
  if (action && viewerMode >= STANDALONE)
    {
      QUrl url = action->data().toUrl();
      QFileInfo file = url.toLocalFile();
      if (file.exists())
        open(file.absoluteFilePath());
      else
        open(url);
    }
}


void 
QDjView::clearRecent()
{
  prefs->recentFiles.clear();
  prefs->saveRecent();
}


QDjView::UndoRedo::UndoRedo()
  : valid(false)
{
}


void
QDjView::UndoRedo::clear()
{
  valid = false;
}


void
QDjView::UndoRedo::set(QDjView *djview)
{
  QDjVuWidget *djvu = djview->getDjVuWidget();
  rotation = djvu->rotation();
  zoom = djvu->zoom();
  hotSpot = djvu->hotSpot();
  position = djvu->position(hotSpot);
  valid = true;
}


void 
QDjView::UndoRedo::apply(QDjView *djview)
{
  if (valid)
    {
      QDjVuWidget *djvu = djview->getDjVuWidget();
      djvu->setZoom(zoom);
      djvu->setRotation(rotation);
      djvu->setPosition(position, hotSpot);
    }
}


bool 
QDjView::UndoRedo::changed(const QDjVuWidget *djvu) const
{
  if (valid)
    {
      if (zoom != djvu->zoom() || 
          rotation != djvu->rotation())
        return true;
      Position curpos = djvu->position(hotSpot);
      if (curpos.pageNo != position.pageNo ||
          curpos.inPage != position.inPage)
        return true;
      if (curpos.inPage)
        return (curpos.posPage != position.posPage);
      else
        return (curpos.posView != position.posView);
    }
  return false;
}


void
QDjView::saveUndoData()
{
  if (QApplication::mouseButtons() == Qt::NoButton
      && widget->pageSizeKnown(widget->page()) )
    {
      if (here.changed(widget))
        {
          undoList.prepend(here);
          while (undoList.size() > 1024)
            undoList.removeLast();
          redoList.clear();
        }
      here.set(this);
      actionBack->setEnabled(undoList.size() > 0);
      actionForw->setEnabled(redoList.size() > 0);
    }
  else
    {
      undoTimer->stop();
      undoTimer->start(250);
    }
}


void 
QDjView::performUndo()
{
  if (undoList.size() > 0)
    {
      UndoRedo target = undoList.takeFirst();
      UndoRedo saved;
      saved.set(this);
      target.apply(this);
      here.clear();
      redoList.prepend(saved);
    }
}


void 
QDjView::performRedo()
{
  if (redoList.size() > 0)
    {
      UndoRedo target = redoList.takeFirst();
      UndoRedo saved;
      saved.set(this);
      target.apply(this);
      here.clear();
      undoList.prepend(saved);
    }
}


void 
QDjView::performCopyUrl()
{
  QUrl url = getDecoratedUrl();
  if (url.isValid())
    QApplication::clipboard()->setText(url.toString());
}


#if DDJVUAPI_VERSION < 21

static QByteArray *qstring_puts_data = 0;
static int qstring_puts(const char *s)
{
  if (qstring_puts_data)
    (*qstring_puts_data) += s;
  return strlen(s);
}

static QString
miniexp_to_string(miniexp_t expr, int width=40, bool octal=false)
{
  QByteArray buffer;
  qstring_puts_data = &buffer;
  int (*saved_puts)(const char*) = minilisp_puts;
  int saved_print_7bits = minilisp_print_7bits;
  minilisp_puts = qstring_puts;
  minilisp_print_7bits = (octal) ? 1 : 0;
  miniexp_pprint(expr, width);
  minilisp_print_7bits = saved_print_7bits;
  minilisp_puts = saved_puts;
  return QString::fromUtf8(buffer.data());
}

#else

static int 
qstring_puts(miniexp_io_t *io, const char *s)
{
  QByteArray *bap = (QByteArray*)io->data[1];
  if (bap) *bap += s;
  return strlen(s);
}

static QString
miniexp_to_string(miniexp_t expr, int width=40, bool octal=false)
{
  QByteArray buffer;
  miniexp_io_t io;
  miniexp_io_init(&io);
  io.fputs = qstring_puts;
  io.data[1] = (void*)&buffer;
#ifdef miniexp_io_print7bits
  static int flags = miniexp_io_print7bits;
  io.p_flags = (octal) ? &flags : 0;
#else
  static int flags = 1;
  io.p_print7bits = (octal) ? &flags : 0;
#endif
  miniexp_pprint_r(&io, expr, width);
  return QString::fromUtf8(buffer.data());
}

#endif


void 
QDjView::performCopyOutline()
{
  if (document)
    {
      QString s;
      minivar_t expr = document->getDocumentOutline();
      if (miniexp_consp(expr))
        s += QString("# This is the existing outline.\n");
      else {
        s += QString("# This is an outline template with all pages.\n");
        expr = miniexp_cons(miniexp_symbol("bookmarks"),miniexp_nil);
        for (int i=0; i<documentPages.size(); i++)
          {
            minivar_t p = miniexp_nil;
            QByteArray ref = documentPages[i].id;
            p = miniexp_cons(miniexp_string(ref.prepend("#").constData()),p);
            QString name = QString("Page %1").arg(pageName(i));
            p = miniexp_cons(miniexp_string(name.toUtf8().constData()),p);
            expr = miniexp_cons(p,expr);
          }
        expr = miniexp_reverse(expr);
      }
      s += QString("# Edit it and store it with command:\n"
                   "#   $ djvused foo.djvu -f thisfile -s\n"
                   "# The following line is the djvused command\n"
                   "# to set the outline and the rest is the outline\n"
                   "set-outline\n\n");
      s += miniexp_to_string(expr);
      // copy to clipboard
      QApplication::clipboard()->setText(s);
    }
}


void 
QDjView::performCopyAnnotation()
{
  int pageNo = widget->page();
  if (document && pageNo>=0 && pageNo<pageNum())
    {
      QString s;
      miniexp_t expr = document->getPageAnnotations(pageNo);
      if (expr == miniexp_nil || expr == miniexp_dummy)
        s += QString("# There were no annotations for page %1.\n");
      else
        s += QString("# These are the annotation for page %1.\n");
      s += QString("# Edit this file and store it with command:\n"
                   "#   $ djvused foo.djvu -f thisfile -s\n"
                   "# Tip: select an area in djview4 and use 'copy maparea'.\n"
                   "# The following line is the djvused command to set\n"
                   "# the annotation and the rest are the annotations\n"
                   "select %2; set-ant\n\n");
      s = s.arg(pageNo+1).arg(pageNo+1);
      while (miniexp_consp(expr))
        {
          s += miniexp_to_string(miniexp_car(expr));
          expr = miniexp_cdr(expr);
        }
      // copy to clipboard
      QApplication::clipboard()->setText(s);
    }
}


// ----------------------------------------
// MOC

#include "qdjview.moc"


/* -------------------------------------------------------------
   Local Variables:
   c++-font-lock-extra-types: ( "\\sw+_t" "[A-Z]\\sw*[a-z]\\sw*" )
   End:
   ------------------------------------------------------------- */
