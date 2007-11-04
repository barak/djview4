//C-  -*- C++ -*-
//C- -------------------------------------------------------------------
//C- DjView4
//C- Copyright (c) 2006  Leon Bottou
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

// $Id$

#if AUTOCONF
# include "config.h"
#else
# define HAVE_STRING_H 1
# define HAVE_UNISTD_H 1
# define HAVE_STRERROR 1
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
#include <QObject>
#include <QPair>
#include <QPalette>
#include <QProcess>
#include <QRegExp>
#include <QRegExp>
#include <QRegExpValidator>
#include <QScrollBar>
#include <QShortcut>
#include <QSlider>
#include <QStackedLayout>
#include <QStatusBar>
#include <QString>
#include <QTextStream>
#include <QTimer>
#include <QToolBar>
#include <QToolBox>
#include <QUrl>
#include <QWhatsThis>

#include "qdjvu.h"
#include "qdjvuhttp.h"
#include "qdjvuwidget.h"
#include "qdjview.h"
#include "qdjviewprefs.h"
#include "qdjviewdialogs.h"
#include "qdjviewsidebar.h"

#if DDJVUAPI_VERSION < 18
# error "DDJVUAPI_VERSION>=18 is required !"
#endif



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
  modeCombo->addItem(tr("Background","modeCombo"), QDjVuWidget::DISPLAY_BG );
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
  Qt::ToolBarAreas areas = 0;
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
#if QT_VERSION >= 0x040200
  QList<QKeySequence> shortcuts = action->shortcuts();
# ifdef Q_WS_MAC
  shortcuts.append(shortcut);
# else
  shortcuts.prepend(shortcut);
# endif
  action->setShortcuts(shortcuts);
#else
  action->setShortcut(shortcut);
#endif
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
    << Trigger(this, SLOT(find()));

  actionFindNext = makeAction(tr("Find &Next", "Edit|"))
    << QKeySequence(tr("Ctrl+F3", "Edit|Find Next"))
    << QKeySequence(tr("F3", "Edit|Find Next"))
    << tr("Find next occurence of search text in the document.")
    << Trigger(findWidget, SLOT(findNext()));

  actionFindPrev = makeAction(tr("Find &Previous", "Edit|"))
    << QKeySequence(tr("Shift+F3", "Edit|Find Previous"))
    << tr("Find previous occurence of search text in the document.")
    << Trigger(findWidget, SLOT(findPrev()));

  actionSelect = makeAction(tr("&Select", "Edit|"), false)
    << QKeySequence(tr("Ctrl+F2", "Edit|Select"))
    << QKeySequence(tr("F2", "Edit|Select"))
    << QIcon(":/images/icon_select.png")
    << tr("Select a rectangle in the document.")
    << Trigger(this, SLOT(performSelect(bool)));
  
  actionZoomIn = makeAction(tr("Zoom &In", "Zoom|"))
    << QIcon(":/images/icon_zoomin.png")
    << tr("Increase the magnification.")
    << Trigger(widget, SLOT(zoomIn()));

  actionZoomOut = makeAction(tr("Zoom &Out", "Zoom|"))
    << QIcon(":/images/icon_zoomout.png")
    << tr("Decrease the magnification.")
    << Trigger(widget, SLOT(zoomOut()));

  actionZoomFitWidth = makeAction(tr("Fit &Width", "Zoom|"),false)
    << tr("Set magnification to fit page width.")
    << QVariant(QDjVuWidget::ZOOM_FITWIDTH)
    << Trigger(this, SLOT(performZoom()))
    << *zoomActionGroup;

  actionZoomFitPage = makeAction(tr("Fit &Page", "Zoom|"),false)
    << tr("Set magnification to fit page.")
    << QVariant(QDjVuWidget::ZOOM_FITPAGE)
    << Trigger(this, SLOT(performZoom()))
    << *zoomActionGroup;

  actionZoomOneToOne = makeAction(tr("One &to one", "Zoom|"),false)
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

  actionZoom150 = makeAction(tr("150%", "Zoom|"), false)
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

  actionNavFirst = makeAction(tr("&First Page", "Go|"))
    << QIcon(":/images/icon_first.png")
    << tr("Jump to first document page.")
    << Trigger(widget, SLOT(firstPage()))
    << Trigger(this, SLOT(updateActionsLater()));

  actionNavNext = makeAction(tr("&Next Page", "Go|"))
    << QIcon(":/images/icon_next.png")
    << tr("Jump to next document page.")
    << Trigger(widget, SLOT(nextPage()))
    << Trigger(this, SLOT(updateActionsLater()));

  actionNavPrev = makeAction(tr("&Previous Page", "Go|"))
    << QIcon(":/images/icon_prev.png")
    << tr("Jump to previous document page.")
    << Trigger(widget, SLOT(prevPage()))
    << Trigger(this, SLOT(updateActionsLater()));

  actionNavLast = makeAction(tr("&Last Page", "Go|"))
    << QIcon(":/images/icon_last.png")
    << tr("Jump to last document page.")
    << Trigger(widget, SLOT(lastPage()))
    << Trigger(this, SLOT(updateActionsLater()));

  actionBack = makeAction(tr("&Backward", "Go|"))
    << QIcon(":/images/icon_back.png")
    << tr("Backward in history.")
    << Trigger(this, SLOT(performUndo()))
    << Trigger(this, SLOT(updateActionsLater()));

  actionForw = makeAction(tr("&Forward", "Go|"))
    << QIcon(":/images/icon_forw.png")
    << tr("Forward in history.")
    << Trigger(this, SLOT(performRedo()))
    << Trigger(this, SLOT(updateActionsLater()));

  actionRotateLeft = makeAction(tr("Rotate &Left", "Rotate|"))
    << QIcon(":/images/icon_rotateleft.png")
    << tr("Rotate page image counter-clockwise.")
    << Trigger(widget, SLOT(rotateLeft()))
    << Trigger(this, SLOT(updateActionsLater()));

  actionRotateRight = makeAction(tr("Rotate &Right", "Rotate|"))
    << QIcon(":/images/icon_rotateright.png")
    << tr("Rotate page image clockwise.")
    << Trigger(widget, SLOT(rotateRight()))
    << Trigger(this, SLOT(updateActionsLater()));

  actionRotate0 = makeAction(tr("Rotate &0\260", "Rotate|"), false)
    << tr("Set natural page orientation.")
    << QVariant(0)
    << Trigger(this, SLOT(performRotation()))
    << *rotationActionGroup;

  actionRotate90 = makeAction(tr("Rotate &90\260", "Rotate|"), false)
    << tr("Turn page on its left side.")
    << QVariant(1)
    << Trigger(this, SLOT(performRotation()))
    << *rotationActionGroup;

  actionRotate180 = makeAction(tr("Rotate &180\260", "Rotate|"), false)
    << tr("Turn page upside-down.")
    << QVariant(2)
    << Trigger(this, SLOT(performRotation()))
    << *rotationActionGroup;

  actionRotate270 = makeAction(tr("Rotate &270\260", "Rotate|"), false)
    << tr("Turn page on its right side.")
    << QVariant(3)
    << Trigger(this, SLOT(performRotation()))
    << *rotationActionGroup;
  
  actionInformation = makeAction(tr("&Information...", "Edit|"))
    << QKeySequence(tr("Ctrl+I", "Edit|Information"))
    << tr("Show information about the document encoding and structure.")
    << Trigger(this, SLOT(performInformation()));

  actionMetadata = makeAction(tr("&Metadata...", "Edit|"))
    << QKeySequence(tr("Ctrl+M", "Edit|Metadata"))
    << tr("Show the document and page meta data.")
    << Trigger(this, SLOT(performMetadata()));

  actionWhatsThis = QWhatsThis::createAction(this);

  actionAbout = makeAction(tr("&About DjView..."))
#ifndef Q_WS_MAC
    << QIcon(":/images/icon_djvu.png")
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
  
  actionPreferences = makeAction(tr("Prefere&nces...", "Settings|")) 
    << QIcon(":/images/icon_prefs.png")
    << tr("Show the preferences dialog.")
    << Trigger(this, SLOT(performPreferences()));

  actionViewSideBar = sideBar->toggleViewAction() 
    << tr("Show &Sidebar", "Settings|")
    << QKeySequence(tr("Ctrl+F9", "Settings|Show sidebar"))
    << QKeySequence(tr("F9", "Settings|Show sidebar"))
    // << QIcon(":/images/icon_sidebar.png")
    << tr("Show/hide the side bar.")
    << Trigger(this, SLOT(updateActionsLater()));

  actionViewToolBar = toolBar->toggleViewAction()
    << tr("Show &Toolbar", "Settings|")
    << QKeySequence(tr("Ctrl+F10", "Settings|Show toolbar"))
    << QKeySequence(tr("F10", "Settings|Show toolbar"))
    << tr("Show/hide the standard tool bar.")
    << Trigger(this, SLOT(updateActionsLater()));

  actionViewStatusBar = makeAction(tr("Show Stat&usbar", "Settings|"), true)
    << tr("Show/hide the status bar.")
    << Trigger(statusBar,SLOT(setVisible(bool)))
    << Trigger(this, SLOT(updateActionsLater()));

  actionViewFullScreen 
    = makeAction(tr("F&ull Screen","View|"), false)
    << QKeySequence(tr("Ctrl+F11","View|FullScreen"))
    << QKeySequence(tr("F11","View|FullScreen"))
    << QIcon(":/images/icon_fullscreen.png")
    << tr("Toggle full screen mode.")
    << Trigger(this, SLOT(performViewFullScreen(bool)));

  actionLayoutContinuous = makeAction(tr("&Continuous", "Layout|"), false)
    << QIcon(":/images/icon_continuous.png")
    << QKeySequence(tr("Ctrl+F4", "Layout|Continuous"))
    << QKeySequence(tr("F4", "Layout|Continuous"))
    << tr("Toggle continuous layout mode.")
    << Trigger(widget, SLOT(setContinuous(bool)))
    << Trigger(this, SLOT(updateActionsLater()));

  actionLayoutSideBySide = makeAction(tr("Side &by side", "Layout|"), false)
    << QIcon(":/images/icon_sidebyside.png")
    << QKeySequence(tr("Ctrl+F5", "Layout|SideBySide"))
    << QKeySequence(tr("F5", "Layout|SideBySide"))
    << tr("Toggle side-by-side layout mode.")
    << Trigger(widget, SLOT(setSideBySide(bool)))
    << Trigger(this, SLOT(updateActionsLater()));

  // Enumerate all actions
  QAction *a;
  QObject *o;
  allActions.clear();
  foreach(o, children())
    if ((a = qobject_cast<QAction*>(o)))
      allActions << a;
}


void
QDjView::createMenus()
{
  // Layout main menu
  QMenu *fileMenu = menuBar->addMenu(tr("&File", "File|"));
  if (viewerMode == STANDALONE)
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
  if (viewerMode == STANDALONE)
    {
      fileMenu->addSeparator();
      fileMenu->addAction(actionClose);
      fileMenu->addAction(actionQuit);
    }
  QMenu *editMenu = menuBar->addMenu(tr("&Edit", "Edit|"));
  editMenu->addAction(actionSelect);
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
  viewMenu->addSeparator();
  viewMenu->addAction(actionLayoutContinuous);
  viewMenu->addAction(actionLayoutSideBySide);
  viewMenu->addSeparator();
  viewMenu->addAction(actionInformation);
  viewMenu->addAction(actionMetadata);
  if (viewerMode == STANDALONE)
    viewMenu->addSeparator();
  if (viewerMode == STANDALONE)
    viewMenu->addAction(actionViewFullScreen);
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
  contextMenu->addSeparator();
  contextMenu->addAction(actionLayoutContinuous);
  contextMenu->addAction(actionLayoutSideBySide);
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
  foreach(QAction *action, allActions)
    action->setEnabled(true);

  // Some actions are explicitly disabled
  actionSave->setEnabled(savingAllowed);
  actionExport->setEnabled(savingAllowed);
  actionPrint->setEnabled(printingAllowed);
  
  // Some actions are only available in standalone mode
  actionNew->setVisible(viewerMode == STANDALONE);
  actionOpen->setVisible(viewerMode == STANDALONE);
  actionOpenLocation->setVisible(viewerMode == STANDALONE);
  actionClose->setVisible(viewerMode == STANDALONE);
  actionQuit->setVisible(viewerMode == STANDALONE);
  actionViewFullScreen->setVisible(viewerMode == STANDALONE);  

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
  modeCombo->setCurrentIndex(modeCombo->findData(QVariant(mode)));
  modeCombo->setEnabled(!!document);

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
  if (pageno >= 0 && pagenum > 0)
    pageCombo->setEditText(pageName(pageno));
  pageCombo->setEnabled(pagenum > 0);
  actionNavFirst->setEnabled(pagenum>0 && pageno>0);
  actionNavPrev->setEnabled(pagenum>0 && pageno>0);
  actionNavNext->setEnabled(pagenum>0 && pageno<pagenum-1);
  actionNavLast->setEnabled(pagenum>0 && pageno<pagenum-1);

  // Layout actions
  actionLayoutContinuous->setChecked(widget->continuous());  
  actionLayoutSideBySide->setChecked(widget->sideBySide());
  
  // UndoRedo
  undoTimer->stop();
  undoTimer->start(250);
  actionBack->setEnabled(undoList.size() > 0);
  actionForw->setEnabled(redoList.size() > 0);
  
  // Disable almost everything when document==0
  if (! document)
    {
      foreach(QAction *action, allActions)
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
}



// ----------------------------------------
// WHATSTHIS HELP


struct Help {
  QString s;
  Help(QString s) : s(s) { }
  Help& operator>>(QWidget *w) { w->setWhatsThis(s); return *this; }
  Help& operator>>(QAction *a) { a->setWhatsThis(s); return *this; }
};


void
QDjView::createWhatsThis()
{
  QString mc, ms, ml;
#ifdef Q_WS_MAC
  mc = tr("Control Left Mouse Button");
#else
  mc = tr("Right Mouse Button");
#endif
  ms = prefs->modifiersToString(prefs->modifiersForSelect);
  ms = ms.replace("+"," ");
  ml = prefs->modifiersToString(prefs->modifiersForLens);
  ml = ml.replace("+"," ");
  
  Help(tr("<html><b>Selecting a rectangle.</b><br/> "
          "Once a rectangular area is selected, a popup menu "
          "lets you copy the corresponding text or image. "
          "Instead of using this tool, you can also hold %1 "
          "and use the Left Mouse Button."
          "</html>").arg(ms))
            >> actionSelect;
  
  Help(tr("<html><b>Zooming.</b><br/> "
          "Choose a zoom level for viewing the document. "
          "Zoom level 100% displays the document for a 100 dpi screen. "
          "Zoom levels <tt>Fit Page</tt> and <tt>Fit Width</tt> ensure "
          "that the full page or the page width fit in the window. "
          "</html>"))
            >> actionZoomIn >> actionZoomOut
            >> actionZoomFitPage >> actionZoomFitWidth
            >> actionZoom300 >> actionZoom200 >> actionZoom150
            >> actionZoom75 >> actionZoom50
            >> zoomCombo;
  
  Help(tr("<html><b>Rotating the pages.</b><br/> "
          "Choose to display pages in portrait or landscape mode. "
          "You can also turn them upside down.</html>"))
            >> actionRotateLeft >> actionRotateRight
            >> actionRotate0 >> actionRotate90
            >> actionRotate180 >> actionRotate270;

  Help(tr("<html><b>Display mode.</b><br/> "
          "DjVu images compose a background layer and a foreground layer "
          "using a stencil. The display mode specifies with layers "
          "should be displayed.</html>"))
            >> actionDisplayColor >> actionDisplayBW
            >> actionDisplayForeground >> actionDisplayBackground
            >> modeCombo;

  Help(tr("<html><b>Navigating the document.</b><br/> "
          "The page selector lets you jump to any page by name. "
          "The navigation buttons jump to the first page, the previous "
          "page, the next page, or the last page. </html>"))
            >> actionNavFirst >> actionNavPrev 
            >> actionNavNext >> actionNavLast
            >> pageCombo;

  Help(tr("<html><b>Document and page information.</b><br> "
          "Display a dialog window for viewing "
          "encoding information pertaining to the document "
          "or to a specific page.</html>"))
            >> actionInformation;
  
  Help(tr("<html><b>Document and page metadata.</b><br> "
          "Display a dialog window for viewing metadata "
          "pertaining to the document "
          "or to a specific page.</html>"))
            >> actionMetadata;

  Help(tr("<html><b>Continuous layout.</b><br/> "
          "Display all the document pages arranged vertically "
          "inside the scrollable document viewing area.</html>"))
            >> actionLayoutContinuous;
  
  Help(tr("<html><b>Side by side layout.</b><br/> "
          "Display pairs of pages side by side "
          "inside the scrollable document viewing area.</html>"))
            >> actionLayoutSideBySide;
  
  Help(tr("<html><b>Page information.</b><br/> "
          "Display information about the page located under the cursor: "
          "the sequential page number, the page size in pixels, "
          "and the page resolution in dots per inch. </html>"))
            >> pageLabel;
  
  Help(tr("<html><b>Cursor information.</b><br/> "
          "Display the position of the mouse cursor "
          "expressed in page coordinates. </html>"))
            >> mouseLabel;

  Help(tr("<html><b>Document viewing area.</b><br/> "
          "This is the main display area for the DjVu document. <ul>"
          "<li>Arrows and page keys to navigate the document.</li>"
          "<li>Space and BackSpace to read the document.</li>"
          "<li>Keys <tt>+</tt> <tt>-</tt> <tt>[</tt> <tt>]</tt> "
          "to zoom or rotate the document.</li>"
          "<li>Left Mouse Button for panning and selecting links.</li>"
          "<li>%3 for displaying the contextual menu.</li>"
          "<li>%1 Left Mouse Button for selecting text or images.</li>"
          "<li>%2 for popping the magnification lens.</li>"
          "</ul></html>").arg(ms).arg(ml).arg(mc))
            >> widget;
  
  Help(tr("<html><b>Document viewing area.</b><br/> "
          "This is the main display area for the DjVu document. "
          "But you must first open a DjVu document to see anything."
          "</html>"))
            >> splash;
    
}



// ----------------------------------------
// APPLY PREFERENCES



QDjViewPrefs::Saved *
QDjView::getSavedPrefs(void)
{
  if (viewerMode==EMBEDDED_PLUGIN)
    return &prefs->forEmbeddedPlugin;
  if (viewerMode==FULLPAGE_PLUGIN)
    return &prefs->forFullPagePlugin;
  if (actionViewFullScreen->isChecked())
    return &prefs->forFullScreen;
  else
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
QDjView::applyOptions(void)
{
  menuBar->setVisible(options & QDjViewPrefs::SHOW_MENUBAR);
  toolBar->setVisible(options & QDjViewPrefs::SHOW_TOOLBAR);
  sideBar->setVisible(options & QDjViewPrefs::SHOW_SIDEBAR);
  statusBar->setVisible(options & QDjViewPrefs::SHOW_STATUSBAR);
  enableScrollBars(options & QDjViewPrefs::SHOW_SCROLLBARS);
  widget->setDisplayFrame(options & QDjViewPrefs::SHOW_FRAME);
  widget->setDisplayMapAreas(options & QDjViewPrefs::SHOW_MAPAREAS);
  widget->setContinuous(options & QDjViewPrefs::LAYOUT_CONTINUOUS);
  widget->setSideBySide(options & QDjViewPrefs::LAYOUT_SIDEBYSIDE);
  widget->enableMouse(options & QDjViewPrefs::HANDLE_MOUSE);
  widget->enableKeyboard(options & QDjViewPrefs::HANDLE_KEYBOARD);
  widget->enableHyperlink(options & QDjViewPrefs::HANDLE_LINKS);
  enableContextMenu(options & QDjViewPrefs::HANDLE_CONTEXTMENU);
}


void 
QDjView::updateOptions(void)
{
  options = 0;
  if (! menuBar->isHidden())
    options |= QDjViewPrefs::SHOW_MENUBAR;
  if (! toolBar->isHidden())
    options |= QDjViewPrefs::SHOW_TOOLBAR;
  if (! sideBar->isHidden())
    options |= QDjViewPrefs::SHOW_SIDEBAR;
  if (! statusBar->isHidden())
    options |= QDjViewPrefs::SHOW_STATUSBAR;
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
  // main saved states
  options = saved->options;
  if (saved->state.size() > 0)
    restoreState(saved->state);
  applyOptions();
  widget->setZoom(saved->zoom);
  // global window size in standalone mode
  if (saved == &prefs->forStandalone)
    if (! (prefs->windowSize.isNull()))
      resize(prefs->windowSize);
  // sidebar tab
  if (saved->sidebarTab >= 0 
      && saved->sidebarTab < sideToolBox->count())
    sideToolBox->setCurrentIndex(saved->sidebarTab);
}


static const Qt::WindowStates 
unusualWindowStates = (Qt::WindowMinimized |
                       Qt::WindowMaximized |
                       Qt::WindowFullScreen );


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
                           QDjViewPrefs::SHOW_FRAME |
                           QDjViewPrefs::HANDLE_CONTEXTMENU );
      // main window size in standalone mode
      if (saved == &prefs->forStandalone)
        if (! (windowState() & unusualWindowStates))
          prefs->windowSize = size();
      // sidebar tab
      saved->sidebarTab = sideToolBox->currentIndex();
    }
}


void
QDjView::applyPreferences(void)
{
  // Saved preferences
  applySaved(getSavedPrefs());
  
  // Other preferences
  djvuContext.setCacheSize(prefs->cacheSize);
  widget->setPixelCacheSize(prefs->pixelCacheSize);
  widget->setModifiersForLens(prefs->modifiersForLens);
  widget->setModifiersForSelect(prefs->modifiersForSelect);
  widget->setModifiersForLinks(prefs->modifiersForLinks);
  widget->setGamma(prefs->gamma);
  widget->setLensSize(prefs->lensSize);
  widget->setLensPower(prefs->lensPower);

  // Thumbnail preferences
  thumbnailWidget->setSize(prefs->thumbnailSize);
  thumbnailWidget->setSmart(prefs->thumbnailSmart);
  
  // Search preferences
  findWidget->setWordOnly(prefs->searchWordsOnly);
  findWidget->setCaseSensitive(prefs->searchCaseSensitive);

  // Special preferences for embedded plugins
  if (viewerMode == EMBEDDED_PLUGIN)
    {
      setMinimumSize(QSize(8,8));
      widget->setBorderBrush(QBrush(Qt::white));
      widget->setBorderSize(0);
    }
  
  // Preload full screen prefs.
  fsSavedNormal = prefs->forStandalone;
  fsSavedFullScreen = prefs->forFullScreen;
}


void 
QDjView::preferencesChanged(void)
{
  applyPreferences();
  updateActions();
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
          if (str[npos] == '-')
            {
              plus = false;
              minus = true;
            }
          else if (str[npos] == '+')
            {
              if (!minus && !plus)
                tools &= (QDjViewPrefs::TOOLBAR_TOP |
                          QDjViewPrefs::TOOLBAR_BOTTOM |
                          QDjViewPrefs::TOOLBAR_AUTOHIDE );
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
      if (viewerMode != STANDALONE)
        errors << tr("Option '%1' requires a standalone viewer.").arg(key);
      if (parse_boolean(key, value, errors, okay))
        if (actionViewFullScreen->isChecked() != okay)
          actionViewFullScreen->activate(QAction::Trigger);
    }
  else if (key == "toolbar")
    {
      parseToolBarOption(value, errors);
      toolBar->setVisible(options & QDjViewPrefs::SHOW_TOOLBAR);
    }
  else if (key == "page")
    {
      goToPage(value);
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
          sideBar->setVisible(false);
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
  else if (key == "statusbar")  // new for djview4
    {
      if (parse_boolean(key, value, errors, okay))
        statusBar->setVisible(okay);
    }
  else if (key == "continuous") // new for djview4
    {
      if (parse_boolean(key, value, errors, okay))
        widget->setContinuous(okay);
    }
  else if (key == "side_by_side" ||
           key == "sidebyside") // new for djview4
    {
      if (parse_boolean(key, value, errors, okay))
        widget->setSideBySide(okay);
    }
  else if (key == "frame")
    {
      if (parse_boolean(key, value, errors, okay))
        widget->setDisplayFrame(okay);
    }
  else if (key == "scrollbars")
    {
      if (parse_boolean(key, value, errors, okay))
        enableScrollBars(okay);
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
      if (! parse_highlight(value, x, y, w, h, color))
        illegal_value(key, value, errors);
      pendingHilite << StringPair(pendingPage, value);
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
      qWarning((const char*)msg.toLocal8Bit());
      if (key == "notoolbar" && value.isNull())
        toolBar->setVisible(false);
      else if (key == "noscrollbars" && value.isNull())
        enableScrollBars(false);
      else if (key == "nomenu" && value.isNull())
        enableContextMenu(false);
    }
  else if (key == "thumbnails")
    {
      if (! showSideBar(value+",thumbnails"))
        illegal_value(key, value, errors);
    }
  else if (key == "outline")
    {
      if (! showSideBar(value+",outline"))
        illegal_value(key, value, errors);
    }
  else if (key == "find") // new for djview4
    {
      pendingFind = value;
    }
  else if (key == "src" && viewerMode != STANDALONE)
    {
      // parseArgument("src", someurl) redirects the viewer.
      // This could be used to set javascript properties.
      pendingUrl = value;        
      if (! pendingUrl.isEmpty())
        performPendingLater();
    }
  else if (key == "logo" || key == "textsel" || key == "search")
    {
      QString msg = tr("Option '%1' is not implemented.").arg(key);
      qWarning((const char*)msg.toLocal8Bit());
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
  foreach(pair, url.queryItems())
    {
      if (pair.first.toLower() == "djvuopts")
        djvuopts = true;
      else if (djvuopts)
        errors << parseArgument(pair.first, pair.second);
    }
  // warning for errors
  if (djvuopts && errors.size() > 0)
    foreach(QString error, errors)
      qWarning((const char*)error.toLocal8Bit());
}


/*! Return a copy of the url without the
  CGI query arguments corresponding to DjVu options. */

QUrl 
QDjView::removeDjVuCgiArguments(QUrl url)
{
  QList<QPair<QString,QString> > args;
  bool djvuopts = false;
  QPair<QString,QString> pair;
  foreach(pair, url.queryItems())
    {
      if (pair.first.toLower() == "djvuopts")
        djvuopts = true;
      else if (!djvuopts)
        args << pair;
    }
  QUrl newurl = url;
  newurl.setQueryItems(args);
  return newurl;
}




// ----------------------------------------
// QDJVIEW


/*! \enum QDjView::ViewerMode
  The viewer mode is selected at creation time.
  Mode \a STANDALONE corresponds to a standalone program.
  Modes \a FULLPAGE_PLUGIN and \a EMBEDDED_PLUGIN
  correspond to a netscape plugin. */


/*! Construct a \a QDjView object using
  the specified djvu context and viewer mode. */

QDjView::QDjView(QDjVuContext &context, ViewerMode mode, QWidget *parent)
  : QMainWindow(parent),
    viewerMode(mode),
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
  setWindowIcon(QIcon(":/images/djvu.png"));
#ifndef Q_WS_MAC
  if (QApplication::windowIcon().isNull())
    QApplication::setWindowIcon(windowIcon());
#endif

  // Basic preferences
  prefs = QDjViewPrefs::instance();
  options = QDjViewPrefs::defaultOptions;
  tools = prefs->tools;
  toolsCached = 0;
  fsWindowState = 0;
  
  // Create dialogs
  errorDialog = new QDjViewErrorDialog(this);
  infoDialog = 0;
  metaDialog = 0;
  
  // Create djvu widget
  QWidget *central = new QWidget(this);
  widget = new QDjVuWidget(central);
  widget->setFrameShape(QFrame::NoFrame);
  if (viewerMode == STANDALONE)
    {
      widget->setFrameShadow(QFrame::Sunken);
      widget->setFrameShape(QFrame::Box);
    }
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
  splash->setFrameShape(QFrame::Box);
  splash->setFrameShadow(QFrame::Sunken);
  splash->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
  splash->setPixmap(QPixmap(":/images/splash.png"));
  QPalette palette = splash->palette();
  palette.setColor(QPalette::Background, Qt::white);
  splash->setPalette(palette);
#if QT_VERSION >= 0x040100
  splash->setAutoFillBackground(true);
#endif

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
  pageLabel = new QLabel(statusBar);
  pageLabel->setFont(font);
  pageLabel->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
  pageLabel->setFrameStyle(QFrame::Panel);
  pageLabel->setFrameShadow(QFrame::Sunken);
  pageLabel->setMinimumWidth(metric.width(" P88 8888x8888 888dpi ")); 
  statusBar->addPermanentWidget(pageLabel);
  mouseLabel = new QLabel(statusBar);
  mouseLabel->setFont(font);
  mouseLabel->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
  mouseLabel->setFrameStyle(QFrame::Panel);
  mouseLabel->setFrameShadow(QFrame::Sunken);
  mouseLabel->setMinimumWidth(metric.width(" x=888 y=888 "));
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
  pageCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  pageCombo->setMinimumWidth(80);
  pageCombo->setEditable(true);
  pageCombo->setInsertPolicy(QComboBox::NoInsert);
  connect(pageCombo, SIGNAL(activated(int)),
          this, SLOT(pageComboActivated(int)));
  connect(pageCombo->lineEdit(), SIGNAL(editingFinished()),
          this, SLOT(pageComboEdited()) );
  
  // Create sidebar  
  sideBar = new QDockWidget(this); 
  sideBar->setObjectName("sidebar");
  sideBar->setAllowedAreas(Qt::AllDockWidgetAreas);
  addDockWidget(Qt::LeftDockWidgetArea, sideBar);
  sideBar->installEventFilter(this);
  sideToolBox = new QToolBox(sideBar);
  sideToolBox->setBackgroundRole(QPalette::Background);
  sideBar->setWidget(sideToolBox);

  // Create escape shortcut for sidebar
  shortcutEscape = new QShortcut(QKeySequence("Esc"), this);
  connect(shortcutEscape, SIGNAL(activated()), this, SLOT(performEscape()));
  
  // Create sidebar components
  thumbnailWidget = new QDjViewThumbnails(this);
  sideToolBox->addItem(thumbnailWidget, tr("&Thumbnails"));
  outlineWidget = new QDjViewOutline(this);
  sideToolBox->addItem(outlineWidget, tr("&Outline")); 
  findWidget = new QDjViewFind(this);
  sideToolBox->addItem(findWidget, tr("&Find")); 

  // Create undoTimer.
  undoTimer = new QTimer(this);
  undoTimer->setSingleShot(true);
  connect(undoTimer,SIGNAL(timeout()), this, SLOT(saveUndoData()));

  // Actions
  createActions();
  createMenus();
  createWhatsThis();
  enableContextMenu(true);
  
  // Preferences
  connect(prefs, SIGNAL(updated()), this, SLOT(preferencesChanged()));
  applyPreferences();
  updateActions();
}


void 
QDjView::closeDocument()
{
  here.clear();
  undoList.clear();
  redoList.clear();
  layout->setCurrentWidget(splash);
  QDjVuDocument *doc = document;
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
  // options set so far get default priority
  widget->reduceOptionsToPriority(QDjVuWidget::PRIORITY_DEFAULT);
  // process url options
  if (url.isValid())
    parseDjVuCgiArguments(url);
  // newly set options get cgi priority
  widget->reduceOptionsToPriority(QDjVuWidget::PRIORITY_CGI);
  widget->setFocus();
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
  addRecent(url);
  setWindowTitle(tr("Djview - %1[*]").arg(getShortFileName()));
  return true;
}


/*! Open the djvu document available at url \a url.
  Only the \a http: and \a file: protocols are supported. */

bool
QDjView::open(QUrl url)
{
  closeDocument();
  QDjVuHttpDocument *doc = new QDjVuHttpDocument(true);
  connect(doc, SIGNAL(error(QString,QString,int)),
          errorDialog, SLOT(error(QString,QString,int)));
  if (prefs->proxyUrl.isValid() && prefs->proxyUrl.scheme() == "http")
    {
      QUrl proxyUrl = prefs->proxyUrl;
      QString host =  proxyUrl.host();
#if QT_VERSION >= 0x040100
      int port = proxyUrl.port(8080);
#else
      int port = proxyUrl.port();
#endif
      QString user = proxyUrl.userName();
      QString pass = proxyUrl.password();
      if (!host.isEmpty() && proxyUrl.path().isEmpty() && port >=0 )
        doc->setProxy(host, port, user, pass);
    }
  
  QUrl docurl = removeDjVuCgiArguments(url);
  doc->setUrl(&djvuContext, docurl);
  if (!doc->isValid())
    {
      delete doc;
      addToErrorDialog(tr("Cannot open URL '%1'.").arg(url.toString()));
      raiseErrorDialog(QMessageBox::Critical, tr("Opening DjVu document"));
      return false;
    }
  open(doc, url);
  addRecent(url);
  setWindowTitle(tr("Djview - %1[*]").arg(getShortFileName()));
  return true;
}


/*! Jump to the page numbered \a pageno. */

void 
QDjView::goToPage(int pageno)
{
  int pagenum = documentPages.size();
  if (!pagenum || !document)
    {
      pendingPage = QString("#%1").arg(pageno+1);
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
          qWarning((const char*)msg.toLocal8Bit());
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
          qWarning((const char*)msg.toLocal8Bit());
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
  statusBar->showMessage(message);
  emit pluginStatusMessage(message);
}


/*! Change the position and composition of the sidebar. */

bool  
QDjView::showSideBar(Qt::DockWidgetAreas areas, int tab)
{
  // Position
  if (areas && !(dockWidgetArea(sideBar) & areas))
    {
      removeDockWidget(sideBar);
      if (areas & Qt::LeftDockWidgetArea)
        addDockWidget(Qt::LeftDockWidgetArea, sideBar);
      else if (areas & Qt::RightDockWidgetArea)
        addDockWidget(Qt::RightDockWidgetArea, sideBar);
      else if (areas & Qt::TopDockWidgetArea)
        addDockWidget(Qt::TopDockWidgetArea, sideBar);
      else if (areas & Qt::BottomDockWidgetArea)
        addDockWidget(Qt::BottomDockWidgetArea, sideBar);
      else
        addDockWidget(Qt::LeftDockWidgetArea, sideBar);
    }
  sideBar->setVisible(areas != 0);
  // Tab
  if (tab >= 0 && tab < sideToolBox->count())
    sideToolBox->setCurrentIndex(tab);
  if (tab == 2 && findWidget)
    findWidget->takeFocus(Qt::ShortcutFocusReason);
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
  bool no = false;
  bool ret = true;
  int tab = -1;
  Qt::DockWidgetAreas areas = 0;
  QString arg;
  foreach(arg, args.split(",", QString::SkipEmptyParts))
    {
      arg = arg.toLower();
      if (arg == "no" || arg == "false")
        no = true;
      else if (arg == "left")
        areas |= Qt::LeftDockWidgetArea;
      else if (arg == "right")
        areas |= Qt::RightDockWidgetArea;
      else if (arg == "top")
        areas |= Qt::TopDockWidgetArea;
      else if (arg == "bottom")
        areas |= Qt::BottomDockWidgetArea;
      else if (arg == "thumbnails" || arg == "thumbnail")
        tab = 0;
      else if (arg == "outline" || arg == "bookmarks")
        tab = 1;
      else if (arg == "search" || arg == "find")
        tab = 2;
      else if (arg != "yes" && arg != "true") {
        errors << tr("Unrecognized sidebar options '%1'.").arg(arg);
        ret = false;
      }
    }
  if (no)
    areas = 0;
  else if (! areas)
    areas |= Qt::AllDockWidgetAreas;
  if (showSideBar(areas, tab))
    return ret;
  return false;
}


/* Overloaded version of \a showSideBar for convenience. */

bool
QDjView::showSideBar(QString args)
{
  QStringList errors;
  return showSideBar(args, errors);
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
  pd->show();
  pd->raise();
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
  sd->show();
  sd->raise();
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
  sd->show();
  sd->raise();
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
      QRegExp options("/[wWcC]*$");
      if (find.contains(options))
        {
          for (int i=find.lastIndexOf("/"); i<find.size(); i++)
            {
              int c = find[i].toAscii();
              if (c == 'c')
                findWidget->setCaseSensitive(true); 
              else if (c == 'C')
                findWidget->setCaseSensitive(false); 
              else if (c == 'w')
                findWidget->setWordOnly(true); 
              else if (c == 'W')
                findWidget->setWordOnly(false); 
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


/*! Return true if full screen mode is active */
bool
QDjView::getFullScreen()
{
  if (viewerMode == STANDALONE)
    return actionViewFullScreen->isChecked();
  return false;
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
  if (hasNumericalPageTitle)
    return QString("%1").arg(pageno + 1);
  return QString("%1").arg(pageno + 1);
}


/*! Return a page number for the page named \a name.
  Names of the form the "#+n", "#-n" express
  relative displacement from the current page
  or the page specified by argument \a from.
  Names of the form "#$n" or "$n" express an absolute 
  page number starting from 1.  Other names
  are matched against page titles, page numbers,
  page names, and page ids. */

int
QDjView::pageNumber(QString name, int from)
{
  int pagenum = documentPages.size();
  if (pagenum <= 0)
    return -1;
  if (from < 0)
    from = widget->page();
  if (name.startsWith("#") && 
      name.contains(QRegExp("^#[-+$]\\d+$")))
    {
      int num = name.mid(2).toInt();
      if (name[1]=='+')
        num = from + 1 + num;
      else if (name[1]=='-')
        num = from + 1 - num;
      return qBound(1, num, pagenum) - 1;
    }
  else if (name.startsWith("$") &&
           name.contains(QRegExp("^\\$\\d+$")) )
    {
      int num = name.mid(1).toInt();
      return qBound(1, num, pagenum) - 1;
    }
  else if (name.startsWith("#"))
    name = name.mid(1);
  // Search exact name starting from current page
  QByteArray utf8Name= name.toUtf8();
  for (int i=from; i<pagenum; i++)
    if (documentPages[i].title && 
        ! strcmp(utf8Name, documentPages[i].title))
      return i;
  for (int i=0; i<from; i++)
    if (documentPages[i].title &&
        ! strcmp(utf8Name, documentPages[i].title))
      return i;
  // Otherwise try a number in range [1..pagenum]
  int pageno = name.toInt() - 1;
  if (pageno >= 0 && pageno < pagenum)
    return pageno;
  // Otherwise search page names and ids
  for (int i=0; i<pagenum; i++)
    if (documentPages[i].name && 
        !strcmp(utf8Name, documentPages[i].name))
      return i;
  if (pageno < 0 || pageno >= pagenum)
    for (int i=0; i<pagenum; i++)
      if (documentPages[i].id && 
          !strcmp(utf8Name, documentPages[i].id))
        return i;
  // Failed
  return -1;
}


/*! Create another main window with the same
  contents, zoom and position as the current one. */

QDjView*
QDjView::copyWindow(void)
{
  // update preferences
  if (viewerMode == STANDALONE)
    updateSaved(getSavedPrefs());
  // create new window
  QDjView *other = new QDjView(djvuContext, STANDALONE);
  QDjVuWidget *otherWidget = other->widget;
  other->setAttribute(Qt::WA_DeleteOnClose);
  other->setWindowTitle(windowTitle());
  // copy window geometry
  if (! (windowState() & unusualWindowStates))
    {
      other->resize( size() );
      other->toolBar->setVisible(!toolBar->isHidden());
      other->sideBar->setVisible(!sideBar->isHidden());
      other->statusBar->setVisible(!statusBar->isHidden());
    }
  // copy essential properties 
  otherWidget->setDisplayMode( widget->displayMode() );
  otherWidget->setContinuous( widget->continuous() );
  otherWidget->setSideBySide( widget->sideBySide() );
  otherWidget->setRotation( widget->rotation() );
  otherWidget->setZoom( widget->zoom() );
  // copy document
  if (document)
    {
      other->open(document);
      other->documentFileName = documentFileName;
      other->documentUrl = documentUrl;
      // copy position
      otherWidget->setPosition( widget->position() );
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
      QStringList patterns;
      foreach(QByteArray format, QImageWriter::supportedImageFormats())
        patterns << "*." + QString(format).toLower();
      QString filters;
      filters += tr("All supported files", "save filter") + " (%1);;";
      filters += tr("All files", "save filter") + " (*)";
      QString caption = tr("Save Image - DjView", "dialog caption");
      filters = filters.arg(patterns.join(" "));
      filename = QFileDialog::getSaveFileName(this, caption, "", filters);
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
#ifdef Q_OS_WIN32
  browsers << "firefox.exe";
  browsers << "iexplore.exe";
#endif
#ifdef Q_WS_MAC
  browsers << "open";
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
  return false;
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
  updateSaved(getSavedPrefs());
  prefs->thumbnailSize = thumbnailWidget->size();
  prefs->thumbnailSmart = thumbnailWidget->smart();
  prefs->searchWordsOnly = findWidget->wordOnly();
  prefs->searchCaseSensitive = findWidget->caseSensitive();
  prefs->save();
  // continue closing the window
  event->accept();
}


bool 
QDjView::eventFilter(QObject *watched, QEvent *event)
{
  switch(event->type())
    {
    case QEvent::Leave:
      if (watched == widget->viewport() || watched == sideBar)
        {
          pageLabel->clear();
          mouseLabel->clear();
          statusMessage();
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
QDjView::info(QString message)
{
  // just for calling qWarning
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
      performPending();
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
  if (! pendingUrl.isEmpty())
    {
      QUrl url = pendingUrl;
      pendingUrl.clear();
      pendingPage.clear();
      pendingHilite.clear();
      open(url);
    }
  else if (! documentPages.isEmpty())
    {
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
              if (pageno >= 0 && pageno < pageNum() &&
                  parse_highlight(pair.second, x, y, w, h, color) &&
                  w > 0 && h > 0 )
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
QDjView::pointerPosition(const Position &pos, const PageInfo &page)
{
  // setup page label
  QString p = "";
  QString m = "";
  if (pos.inPage)
    {
      p = tr(" P%1 %2x%3 %4dpi ").arg(pos.pageNo+1) 
             .arg(page.width).arg(page.height).arg(page.dpi);
      m = tr(" x=%1 y=%2 ").
             arg(pos.posPage.x()).arg(pos.posPage.y());
    }
  setPageLabelText(p);
  setMouseLabelText(m);
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
      if (link[1]=='+')  // i18n: fix plural forms.
        message = (n>1) ? tr("Go: %1 pages forward.") 
          : tr("Go: %1 page forward.");
      else
        message = (n>1) ? tr("Go: %1 pages backward.") 
          : tr("Go: %1 page backward.");
      message = message.arg(n);
    }
  else if (link.startsWith("#$"))
    message = tr("Go: page %1.").arg(link.mid(2));
  else if (link.startsWith("#"))
    message = tr("Go: page %1.").arg(link.mid(1));
  else
    message = tr("Link: %1").arg(link);
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
  // Obtain link information
  QList<QPair<QString, QString> > query;
  QString link = widget->linkUrl();
  QString target = widget->linkTarget();
  bool inPlace = target.isEmpty() || target=="_self" || target=="_page";
  QUrl url = documentUrl;
  // Internal link
  if (link.startsWith("#"))
    {
      if (inPlace)
        {
          goToPage(link, pos.pageNo);
          return;
        }
      if (viewerMode == STANDALONE)
        {
          QDjView *other = copyWindow();
          other->goToPage(link, pos.pageNo);
          other->show();
          return;
        }
      // Construct url
      QPair<QString,QString> pair;
      foreach(pair, url.queryItems())
        if (pair.first.toLower() != "djvuopts")
          query << pair;
        else
          break;
      url.setQueryItems(query);
      url.addQueryItem("djvuopts", "");
      url.addQueryItem("page", QString("#$%1").arg(pageNumber(link)+1));
    }
  else
    {
      // Resolve url
      QUrl linkUrl(link);
      url.setQueryItems(query); // empty!
      url = url.resolved(linkUrl);
      url.setQueryItems(linkUrl.queryItems());
    }
  // Check url
  if (! url.isValid() || url.isRelative())
    {
      QString msg = tr("Cannot resolve link '%1'").arg(link);
      qWarning(msg.toLocal8Bit());
      return;
    }
  // Signal browser
  if (viewerMode != STANDALONE)
    {
      emit pluginGetUrl(url, target);
      return;
    }
  // Standalone can open djvu documents
  QFileInfo file = url.toLocalFile();
  QString suffix = file.suffix().toLower();
  if (file.exists() && (suffix=="djvu" || suffix=="djv"))
    {
      if (inPlace)
        open(url);
      else
        {
          QDjView *other = copyWindow();
          other->open(url);
          other->show();
        }
      return;
    }
  // Open a browser
  if (! startBrowser(url))
    {
      QString msg = tr("Cannot spawn a browser for url '%1'").arg(link);
      qWarning((const char*)msg.toLocal8Bit());
    }
}

  
void 
QDjView::pointerSelect(const QPoint &pointerPos, const QRect &rect)
{
  // Collect text
  QString text=widget->getTextForRect(rect);
  int l = text.size();
  int w = rect.width();
  int h = rect.height();
  QString s = tr("%1 characters").arg(l);
  
  // Prepare menu
  QMenu *menu = new QMenu(this);
  QAction *copyText = menu->addAction(tr("Copy text (%1)").arg(s));
  QAction *saveText = menu->addAction(tr("Save text as..."));
  copyText->setEnabled(l>0);
  saveText->setEnabled(l>0);
  menu->addSeparator();
  QString copyImageString = tr("Copy image (%1x%2 pixels)").arg(w).arg(h);
  QAction *copyImage = menu->addAction(copyImageString);
  QAction *saveImage = menu->addAction(tr("Save image as..."));
  menu->addSeparator();
  QAction *zoom = menu->addAction(tr("Zoom to rectangle"));
  
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
}


void
QDjView::zoomComboActivated(int index)
{
  int zoom = zoomCombo->itemData(index).toInt();
  widget->setZoom(zoom);
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
  goToPage(pageCombo->lineEdit()->text().trimmed());
  updateActionsLater();
  widget->setFocus();
}




// -----------------------------------
// SIGNALS IMPLEMENTING ACTIONS



void
QDjView::performAbout(void)
{
  QString html = 
    tr("<html>"
       "<h2>DjVuLibre DjView %1</h2>"
       "<p>"
       "Viewer for DjVu documents<br>"
       "<a href=http://djvulibre.djvuzone.org>"
       "http://djvulibre.djvuzone.org</a><br>"
       "Copyright \251 2006-- L\351on Bottou."
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
    .arg(QDjViewPrefs::versionString());

  QMessageBox::about(this, tr("About DjView"), html);
}


void
QDjView::performNew(void)
{
  if (viewerMode != STANDALONE)
    return;
  QDjView *other = copyWindow();
  other->show();
}


void
QDjView::performOpen(void)
{
  if (viewerMode != STANDALONE)
    return;
  QString caption = tr("Open - DjView", "dialog caption");
  QString filters = tr("DjVu files") + " (*.djvu *.djv)";
  QString dirname = QDir::currentPath();
  QDir dir = QFileInfo(documentFileName).absoluteDir();
  if (dir.exists() && !documentFileName.isEmpty())
    dirname = dir.absolutePath();
  qDebug() << dirname;
  QString filename = QFileDialog::getOpenFileName(this, caption, dirname, filters);
  if (! filename.isEmpty())
    open(filename);
}


void
QDjView::performOpenLocation(void)
{
  if (viewerMode != STANDALONE)
    return;
  QString caption = tr("Open Location - DjView", "dialog caption");
  QString label = tr("Enter the URL of a DjVu document.");
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
  QDjViewPrefsDialog *dialog = QDjViewPrefsDialog::instance();
  updateSaved(getSavedPrefs());
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


void 
QDjView::performViewFullScreen(bool checked)
{
  if (viewerMode != STANDALONE)
    return;
  if (checked)
    {
      fsSavedNormal.remember = true;
      updateSaved(&fsSavedNormal);
      updateSaved(&prefs->forStandalone);
      Qt::WindowStates wstate = windowState();
      fsWindowState = wstate;
      wstate &= ~unusualWindowStates;
      wstate |= Qt::WindowFullScreen;
      setWindowState(wstate);
      applySaved(&fsSavedFullScreen);
      // Make sure full screen action remains accessible (F11)
      if (! actions().contains(actionViewFullScreen))
        addAction(actionViewFullScreen);
    }
  else
    {
      fsSavedFullScreen.remember = true;
      updateSaved(&fsSavedFullScreen);
      updateSaved(&prefs->forFullScreen);
      Qt::WindowStates wstate = windowState();
      wstate &= ~unusualWindowStates;
      wstate |= fsWindowState & unusualWindowStates;
      wstate &= ~Qt::WindowFullScreen;
      setWindowState(wstate);
      applySaved(&fsSavedNormal);
      // Demote full screen action
      if (actions().contains(actionViewFullScreen))
        removeAction(actionViewFullScreen);
    }
}


void 
QDjView::performEscape()
{
  if (sideBar && !sideBar->isHidden())
    sideBar->hide();
  else if (actionViewFullScreen->isChecked())
    actionViewFullScreen->activate(QAction::Trigger);
}


void 
QDjView::addRecent(QUrl url)
{
  QString name = url.toString();
  prefs->recentFiles.removeAll(name);
  prefs->recentFiles.prepend(name);
  while(prefs->recentFiles.size() > 6)
    prefs->recentFiles.pop_back();
}


void 
QDjView::fillRecent()
{
  if (recentMenu)
    {
      int n = prefs->recentFiles.size();
      recentMenu->clear();
      for (int i=0; i<n; i++)
        {
          QUrl url = prefs->recentFiles[i];
          QString base = url.path().section("/",-1);
          QString name = url.toLocalFile();
          if (name.isEmpty())
            name = url.toString();
#if QT_VERSION >= 0x040200
          QFontMetrics metrics = QFont();
          name = metrics.elidedText(name, Qt::ElideMiddle, 400);
#endif
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
  if (action && viewerMode == STANDALONE)
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
  prefs->save();
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



/* -------------------------------------------------------------
   Local Variables:
   c++-font-lock-extra-types: ( "\\sw+_t" "[A-Z]\\sw*[a-z]\\sw*" )
   End:
   ------------------------------------------------------------- */
