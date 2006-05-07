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
#include <errno.h>
#include <libdjvu/miniexp.h>
#include <libdjvu/ddjvuapi.h>

#include <QDebug>

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QDockWidget>
#include <QDockWidget>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QFrame>
#include <QIcon>
#include <QImageWriter>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QObject>
#include <QPalette>
#include <QRegExp>
#include <QRegExp>
#include <QRegExpValidator>
#include <QScrollBar>
#include <QStackedLayout>
#include <QStatusBar>
#include <QString>
#include <QTextStream>
#include <QTimer>
#include <QToolBar>
#include <QUrl>
#include <QWhatsThis>

#include "qdjvu.h"
#include "qdjvuhttp.h"
#include "qdjvuwidget.h"
#include "qdjview.h"
#include "qdjviewprefs.h"
#include "qdjviewdialogs.h"

#if DDJVUAPI_VERSION < 17
# error "DDJVUAPI_VERSION>=17 is required !"
#endif
#if DDJVUAPI_VERSION < 18
# define WORKAROUND_FOR_DDJVUAPI_17 1
#endif

// ----------------------------------------
// USEFUL CONSTANTS

static const Qt::WindowStates 
unusualWindowStates = (Qt::WindowMinimized |
                       Qt::WindowMaximized |
                       Qt::WindowFullScreen );




// ----------------------------------------
// FILL USER INTERFACE COMPONENTS



void
QDjView::fillPageCombo(QComboBox *pageCombo, QString format)
{
  pageCombo->clear();
  int n = documentPages.size();
  for (int j=0; j<n; j++)
    {
      QString name = pageName(j);
      if (!format.isEmpty())
        name = format.arg(name);
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
  if (tools & QDjViewPrefs::TOOL_SEARCH)
    {
      toolBar->addAction(actionSearch);
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
  if (areas)
    toolBar->setAllowedAreas(areas);
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
  action->setShortcut(shortcut);
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
  
  // Create actions
  actionNew = makeAction(tr("&New", "File|New"))
    << QKeySequence(tr("Ctrl+N", "File|New"))
    << QIcon(":/images/icon_new.png")
    << tr("Create a new DjView window.")
    << Trigger(this, SLOT(performNew()));

  actionOpen = makeAction(tr("&Open", "File|Open"))
    << QKeySequence(tr("Ctrl+O", "File|Open"))
    << QIcon(":/images/icon_open.png")
    << tr("Open a DjVu document.")
    << Trigger(this, SLOT(performOpen()));

  actionClose = makeAction(tr("&Close", "File|Close"))
    << QKeySequence(tr("Ctrl+W", "File|Close"))
    << QIcon(":/images/icon_close.png")
    << tr("Close this window.")
    << Trigger(this, SLOT(close()));

  actionQuit = makeAction(tr("&Quit", "File|Quit"))
    << QKeySequence(tr("Ctrl+Q", "File|Quit"))
    << QIcon(":/images/icon_quit.png")
    << tr("Close all windows and quit the application.")
    << Trigger(QCoreApplication::instance(), SLOT(closeAllWindows()));

  actionSave = makeAction(tr("Save &as...", "File|Save"))
    << QKeySequence(tr("Ctrl+S", "File|Save"))
    << QIcon(":/images/icon_save.png")
    << tr("Save the DjVu document.");

  actionExport = makeAction(tr("&Export as...", "File|Export"))
    << tr("Export the DjVu document under another format.");

  actionPrint = makeAction(tr("&Print...", "File|Print"))
    << QKeySequence(tr("Ctrl+P", "File|Print"))
    << QIcon(":/images/icon_print.png")
    << tr("Print the DjVu document.");

  actionSearch = makeAction(tr("&Find..."))
    << QKeySequence(tr("Ctrl+F", "Find"))
    << QIcon(":/images/icon_find.png")
    << tr("Find text in the document.");

  actionSelect = makeAction(tr("&Select"), false)
    << QIcon(":/images/icon_select.png")
    << tr("Select a rectangle in the document.")
    << Trigger(this, SLOT(performSelect(bool)));
  
  actionZoomIn = makeAction(tr("Zoom &In"))
    << QIcon(":/images/icon_zoomin.png")
    << tr("Increase the magnification.")
    << Trigger(widget, SLOT(zoomIn()));

  actionZoomOut = makeAction(tr("Zoom &Out"))
    << QIcon(":/images/icon_zoomout.png")
    << tr("Decrease the magnification.")
    << Trigger(widget, SLOT(zoomOut()));

  actionZoomFitWidth = makeAction(tr("Fit &Width", "Zoom|Fitwith"),false)
    << tr("Set magnification to fit page width.")
    << QVariant(QDjVuWidget::ZOOM_FITWIDTH)
    << Trigger(this, SLOT(performZoom()))
    << *zoomActionGroup;

  actionZoomFitPage = makeAction(tr("Fit &Page", "Zoom|Fitpage"),false)
    << tr("Set magnification to fit page.")
    << QVariant(QDjVuWidget::ZOOM_FITPAGE)
    << Trigger(this, SLOT(performZoom()))
    << *zoomActionGroup;

  actionZoomOneToOne = makeAction(tr("One &to one", "Zoom|1:1"),false)
    << tr("Set full resolution magnification.")
    << QVariant(QDjVuWidget::ZOOM_ONE2ONE)
    << Trigger(this, SLOT(performZoom()))
    << *zoomActionGroup;

  actionZoom300 = makeAction(tr("&300%", "Zoom|300%"), false)
    << tr("Magnify 300%")
    << QVariant(300)
    << Trigger(this, SLOT(performZoom()))
    << *zoomActionGroup;

  actionZoom200 = makeAction(tr("&200%", "Zoom|200%"), false)
    << tr("Magnify 20%")
    << QVariant(200)
    << Trigger(this, SLOT(performZoom()))
    << *zoomActionGroup;

  actionZoom150 = makeAction(tr("150%", "Zoom|150%"), false)
    << tr("Magnify 150%")
    << QVariant(200)
    << Trigger(this, SLOT(performZoom()))
    << *zoomActionGroup;

  actionZoom100 = makeAction(tr("&100%", "Zoom|100%"), false)
    << tr("Magnify 100%")
    << QVariant(100)
    << Trigger(this, SLOT(performZoom()))
    << *zoomActionGroup;

  actionZoom75 = makeAction(tr("&75%", "Zoom|75%"), false)
    << tr("Magnify 75%")
    << QVariant(75)
    << Trigger(this, SLOT(performZoom()))
    << *zoomActionGroup;

  actionZoom50 = makeAction(tr("&50%", "Zoom|50%"), false)
    << tr("Magnify 0%")
    << QVariant(50)
    << Trigger(this, SLOT(performZoom()))
    << *zoomActionGroup;

  actionNavFirst = makeAction(tr("&First Page"))
    << QIcon(":/images/icon_first.png")
    << tr("Jump to first document page.")
    << Trigger(widget, SLOT(firstPage()))
    << Trigger(this, SLOT(updateActionsLater()));

  actionNavNext = makeAction(tr("&Next Page"))
    << QIcon(":/images/icon_next.png")
    << tr("Jump to next document page.")
    << Trigger(widget, SLOT(nextPage()))
    << Trigger(this, SLOT(updateActionsLater()));

  actionNavPrev = makeAction(tr("&Previous Page"))
    << QIcon(":/images/icon_prev.png")
    << tr("Jump to previous document page.")
    << Trigger(widget, SLOT(prevPage()))
    << Trigger(this, SLOT(updateActionsLater()));

  actionNavLast = makeAction(tr("&Last Page"))
    << QIcon(":/images/icon_last.png")
    << tr("Jump to last document page.")
    << Trigger(widget, SLOT(lastPage()))
    << Trigger(this, SLOT(updateActionsLater()));

  actionBack = makeAction(tr("&Backward"))
    << QIcon(":/images/icon_back.png")
    << tr("Backward in history.");

  actionForw = makeAction(tr("&Forward"))
    << QIcon(":/images/icon_forw.png")
    << tr("Forward in history.");

  actionRotateLeft = makeAction(tr("Rotate &Left"))
    << QIcon(":/images/icon_rotateleft.png")
    << tr("Rotate page image counter-clockwise.")
    << Trigger(widget, SLOT(rotateLeft()))
    << Trigger(this, SLOT(updateActionsLater()));

  actionRotateRight = makeAction(tr("Rotate &Right"))
    << QIcon(":/images/icon_rotateright.png")
    << tr("Rotate page image clockwise.")
    << Trigger(widget, SLOT(rotateRight()))
    << Trigger(this, SLOT(updateActionsLater()));

  actionRotate0 = makeAction(tr("Rotate &0\260"), false)
    << tr("Set natural page orientation.")
    << QVariant(0)
    << Trigger(this, SLOT(performRotation()))
    << *rotationActionGroup;

  actionRotate90 = makeAction(tr("Rotate &90\260"), false)
    << tr("Turn page on its left side.")
    << QVariant(1)
    << Trigger(this, SLOT(performRotation()))
    << *rotationActionGroup;

  actionRotate180 = makeAction(tr("Rotate &180\260"), false)
    << tr("Turn page upside-down.")
    << QVariant(2)
    << Trigger(this, SLOT(performRotation()))
    << *rotationActionGroup;

  actionRotate270 = makeAction(tr("Rotate &270\260"), false)
    << tr("Turn page on its right side.")
    << QVariant(3)
    << Trigger(this, SLOT(performRotation()))
    << *rotationActionGroup;
  
  actionInformation = makeAction(tr("&Information..."))
    << QKeySequence("Ctrl+I")
    << tr("Show the document and page properties.")
    << Trigger(this, SLOT(performInformation()));

  actionWhatsThis = QWhatsThis::createAction(this);

  actionAbout = makeAction(tr("&About DjView..."))
    << QIcon(":/images/icon_djvu.png")
    << tr("Show information about this program.")
    << Trigger(this, SLOT(performAbout()));

  actionDisplayColor = makeAction(tr("&Color", "Display|Color"), false)
    << tr("Display everything.")
    << Trigger(widget, SLOT(displayModeColor()))
    << Trigger(this, SLOT(updateActionsLater()))
    << *modeActionGroup;

  actionDisplayBW = makeAction(tr("&Stencil", "Display|BW"), false)
    << tr("Only display the document bitonal stencil.")
    << Trigger(widget, SLOT(displayModeStencil()))
    << Trigger(this, SLOT(updateActionsLater()))
    << *modeActionGroup;

  actionDisplayForeground = makeAction(tr("&Foreground", "Display|Foreground"), false)
    << tr("Only display the foreground layer.")
    << Trigger(widget, SLOT(displayModeForeground()))
    << Trigger(this, SLOT(updateActionsLater()))
    << *modeActionGroup;

  actionDisplayBackground = makeAction(tr("&Background", "Display|Background"), false)
    << tr("Only display the background layer.")
    << Trigger(widget, SLOT(displayModeBackground()))
    << Trigger(this, SLOT(updateActionsLater()))
    << *modeActionGroup;

  actionPreferences = makeAction(tr("Prefere&nces...")) 
    << QIcon(":/images/icon_prefs.png")
    << tr("Show the preferences dialog.");

  actionViewSideBar = sideBar->toggleViewAction() 
    << tr("Show &side bar")
    << QKeySequence("F9")
    << QIcon(":/images/icon_sidebar.png")
    << tr("Show/hide the side bar.")
    << Trigger(this, SLOT(updateActionsLater()));

  actionViewToolBar = toolBar->toggleViewAction()
    << tr("Show &tool bar")
    << QKeySequence("F10")
    << tr("Show/hide the standard tool bar.")
    << Trigger(this, SLOT(updateActionsLater()));

  actionViewStatusBar = makeAction(tr("Show stat&us bar"), true)
    << tr("Show/hide the status bar.")
    << Trigger(statusBar,SLOT(setVisible(bool)))
    << Trigger(this, SLOT(updateActionsLater()));

  actionViewFullScreen = makeAction(tr("F&ull Screen","View|FullScreen"), false)
    << QKeySequence("F11")
    << QIcon(":/images/icon_fullscreen.png")
    << tr("Toggle full screen mode.")
    << Trigger(this, SLOT(performViewFullScreen(bool)));

  actionLayoutContinuous = makeAction(tr("&Continuous","Layout"), false)
    << QIcon(":/images/icon_continuous.png")
    << QKeySequence("F2")
    << tr("Toggle continuous layout mode.")
    << Trigger(widget, SLOT(setContinuous(bool)))
    << Trigger(this, SLOT(updateActionsLater()));

  actionLayoutSideBySide = makeAction(tr("Side &by side","Layout"), false)
    << QIcon(":/images/icon_sidebyside.png")
    << QKeySequence("F3")
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
  editMenu->addAction(actionSelect);
  editMenu->addAction(actionSearch);
  editMenu->addSeparator();
  editMenu->addAction(actionInformation);
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
  contextMenu->addAction(actionSearch);
  contextMenu->addAction(actionInformation);
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


void
QDjView::updateActions()
{
  // Rebuild toolbar if necessary
  if (tools != toolsCached)
    fillToolBar(toolBar);
  
  // Enable all actions
  foreach(QAction *action, allActions)
    action->setEnabled(true);

  // Some actions are not yet implemented
  actionExport->setVisible(false);
  
  // Some actions are only available in standalone mode
  actionNew->setVisible(viewerMode == STANDALONE);
  actionOpen->setVisible(viewerMode == STANDALONE);
  actionClose->setVisible(viewerMode == STANDALONE);
  actionQuit->setVisible(viewerMode == STANDALONE);
  actionViewFullScreen->setVisible(viewerMode == STANDALONE);
  
  // - zoom combo and actions
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

  // - mode combo and actions
  QDjVuWidget::DisplayMode mode = widget->displayMode();
  actionDisplayColor->setChecked(mode == QDjVuWidget::DISPLAY_COLOR);
  actionDisplayBW->setChecked(mode == QDjVuWidget::DISPLAY_STENCIL);
  actionDisplayBackground->setChecked(mode == QDjVuWidget::DISPLAY_BG);
  actionDisplayForeground->setChecked(mode == QDjVuWidget::DISPLAY_FG);
  modeCombo->setCurrentIndex(modeCombo->findData(QVariant(mode)));
  modeCombo->setEnabled(!!document);

  // - rotations
  int rotation = widget->rotation();
  actionRotate0->setChecked(rotation == 0);
  actionRotate90->setChecked(rotation == 1);
  actionRotate180->setChecked(rotation == 2);
  actionRotate270->setChecked(rotation == 3);
  
  // - page combo and actions
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

  // - layout actions
  actionLayoutContinuous->setChecked(widget->continuous());  
  actionLayoutSideBySide->setChecked(widget->sideBySide());

  // Disable almost everything when document==0
  if (! document)
    {
      foreach(QAction *action, allActions)
        if (action != actionNew &&
            action != actionOpen &&
            action != actionClose &&
            action != actionQuit &&
            action != actionViewToolBar &&
            action != actionViewStatusBar &&
            action != actionViewSideBar &&
            action != actionViewFullScreen &&
            action != actionWhatsThis &&
            action != actionAbout )
          action->setEnabled(false);
    }
  
  // Finished
  needToUpdateActions = false;
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
  QString ms, ml;
  ms = prefs->modifiersToString(prefs->modifiersForSelect);
  ml = prefs->modifiersToString(prefs->modifiersForLens);

  Help(tr("<html><b>Selecting a rectangle...</b><br/> "
          "Once a rectanglular area is selected, a popup menu "
          "lets you copy the corresponding text or image. "
          "Instead of using this tool, you can also hold %1 "
          "and use the Left Mouse Button."
          "</html>").arg(ms))
            >> actionSelect;
  
  Help(tr("<html><b>Zooming...</b><br/> "
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
  
  Help(tr("<html><b>Rotating the pages...</b><br/> "
          "Choose to display pages in portrait or landscape mode. "
          "You can also turn them upside down.</html>"))
            >> actionRotateLeft >> actionRotateRight
            >> actionRotate0 >> actionRotate90
            >> actionRotate180 >> actionRotate270;

  Help(tr("<html><b>Display mode...</b><br/> "
          "DjVu images compose a background layer and a foreground layer "
          "using a stencil. The display mode specifies with layers "
          "should be displayed.</html>"))
            >> actionDisplayColor >> actionDisplayBW
            >> actionDisplayForeground >> actionDisplayBackground
            >> modeCombo;

  Help(tr("<html><b>Navigating the document...</b><br/> "
          "The page selector lets you jump to any page by name. "
          "The navigation buttons jump to the first page, the previous "
          "page, the next page, or the last page. </html>"))
            >> actionNavFirst >> actionNavPrev >> actionNavNext >> actionNavLast
            >> pageCombo;

  Help(tr("<html><b>Document and page infromation.</b><br> "
          "Display a dialog window for viewing metadata and "
          "encoding information pertaining to the document "
          "or to a specific page."))
            >> actionInformation;
  
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
          "<li>Keys <tt>+</tt>, <tt>-</tt>, <tt>[</tt>, <tt>]</tt> "
          "    to zoom or rotate the document.</li>"
          "<li>Left Mouse Button for panning and selecting links.</li>"
          "<li>Right Mouse Button for displaying the contextual menu.</li>"
          "<li><tt>%1</tt> Left Mouse Button for selecting text or images.</li>"
          "<li><tt>%2</tt> for popping the magnification lens.</li>"
          "</ul></html>").arg(ms).arg(ml))
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
      if (oldContextMenu)
        widget->removeAction(oldContextMenu->menuAction());
      widget->setContextMenu(0);
    }
  if (enable && oldContextMenu != contextMenu)
    {
      if (contextMenu)
        widget->addAction(contextMenu->menuAction());
      widget->setContextMenu(contextMenu);
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
}


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
      // safety feature
      saved->options |= QDjViewPrefs::HANDLE_MOUSE;
      saved->options |= QDjViewPrefs::HANDLE_KEYBOARD;
      saved->options |= QDjViewPrefs::HANDLE_LINKS;
      saved->options |= QDjViewPrefs::HANDLE_CONTEXTMENU;
      // global window size in standalone mode
      if (saved == &prefs->forStandalone)
        if (! (windowState() & unusualWindowStates))
          prefs->windowSize = size();
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

  // Preload full screen prefs.
  fsSavedNormal = prefs->forStandalone;
  fsSavedFullScreen = prefs->forFullScreen;
}



// ----------------------------------------
// QDJVIEW



QDjView::QDjView(QDjVuContext &context, ViewerMode mode, QWidget *parent)
  : QMainWindow(parent),
    viewerMode(mode),
    djvuContext(context),
    document(0),
    pendingPageNo(-1),
    needToUpdateActions(false)
{
  // Main window setup
  setWindowTitle(tr("DjView"));
  setWindowIcon(QIcon(":/images/djvu.png"));
  if (QApplication::windowIcon().isNull())
    QApplication::setWindowIcon(windowIcon());

  // Basic preferences
  prefs = QDjViewPrefs::create();
  options = QDjViewPrefs::defaultOptions;
  tools = prefs->tools;
  toolsCached = 0;
  fsWindowState = 0;
  
  // Create dialogs

  errorDialog = new QDjViewErrorDialog(this);
  infoDialog = 0;
  
  // Create widgets

  // - djvu widget
  QWidget *central = new QWidget(this);
  widget = new QDjVuWidget(central);
  widget->setFrameShape(QFrame::Panel);
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

  // - splash screen
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

  // - central layout
  layout = new QStackedLayout(central);
  layout->addWidget(widget);
  layout->addWidget(splash);
  layout->setCurrentWidget(splash);
  setCentralWidget(central);

  // - context menu
  contextMenu = new QMenu(this);
  enableContextMenu(true);

  // - menubar
  menuBar = new QMenuBar(this);
  setMenuBar(menuBar);

  // - statusbar
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

  // - toolbar  
  toolBar = new QToolBar(this);
  toolBar->setObjectName("toolbar");
  toolBar->setAllowedAreas(Qt::TopToolBarArea|Qt::BottomToolBarArea);
  addToolBar(toolBar);

  // - mode combo box
  modeCombo = new QComboBox(toolBar);
  fillModeCombo(modeCombo);
  connect(modeCombo, SIGNAL(activated(int)),
          this, SLOT(modeComboActivated(int)) );
  connect(modeCombo, SIGNAL(activated(int)),
          this, SLOT(updateActionsLater()) );

  // - zoom combo box
  zoomCombo = new QComboBox(toolBar);
  zoomCombo->setEditable(true);
  zoomCombo->setInsertPolicy(QComboBox::NoInsert);
  fillZoomCombo(zoomCombo);
  connect(zoomCombo, SIGNAL(activated(int)),
          this, SLOT(zoomComboActivated(int)) );
  connect(zoomCombo->lineEdit(), SIGNAL(editingFinished()),
          this, SLOT(zoomComboEdited()) );

  // - page combo box
  pageCombo = new QComboBox(toolBar);
  pageCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  pageCombo->setMinimumWidth(80);
  pageCombo->setEditable(true);
  pageCombo->setInsertPolicy(QComboBox::NoInsert);
  connect(pageCombo, SIGNAL(activated(int)),
          this, SLOT(pageComboActivated(int)));
  connect(pageCombo->lineEdit(), SIGNAL(editingFinished()),
          this, SLOT(pageComboEdited()) );
  
  // - sidebar  
  sideBar = new QDockWidget(this);  // for now
  sideBar->setObjectName("sidebar");
  sideBar->setAllowedAreas(Qt::LeftDockWidgetArea|Qt::RightDockWidgetArea);
  addDockWidget(Qt::LeftDockWidgetArea, sideBar);
  sideBar->installEventFilter(this);

  // Actions
  createActions();
  createMenus();
  createWhatsThis();
  
  // Preferences
  applyPreferences();
  updateActions();
}


QString
QDjView::getShortFileName()
{
  if (! documentFileName.isEmpty())
    return QFileInfo(documentFileName).fileName();
  else if (! documentUrl.isEmpty())
    return documentUrl.path().section('/', -1);
  return QString();
}


void 
QDjView::closeDocument()
{
  layout->setCurrentWidget(splash);
  QDjVuDocument *doc = document;
  if (doc)
    {
      doc->ref();
      disconnect(doc, 0, this, 0);
    }
  widget->setDocument(0);
  documentPages.clear();
  documentFileName.clear();
  documentUrl.clear();
  document = 0;
  if (doc)
    {
      emit documentClosed();
      doc->deref();
    }
}


void 
QDjView::open(QDjVuDocument *doc)
{
  closeDocument();
  widget->makeDefaults();
  document = doc;
  connect(doc,SIGNAL(destroyed(void)), this, SLOT(closeDocument(void)));
  connect(doc,SIGNAL(docinfo(void)), this, SLOT(docinfo(void)));
  widget->setDocument(document);
  disconnect(document, 0, errorDialog, 0);
  layout->setCurrentWidget(widget);
  updateActions();
  docinfo();
}


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
      raiseErrorDialog(QMessageBox::Critical, tr("Opening DjVu file..."));
      return false;
    }
  open(doc);
  QFileInfo fileinfo(filename);
  documentUrl = QUrl::fromLocalFile(fileinfo.absoluteFilePath());
  documentFileName = filename;
  setWindowTitle(tr("Djview - %1[*]").arg(getShortFileName()));
  return true;
}


bool
QDjView::open(QUrl url)
{
  closeDocument();
  QDjVuHttpDocument *doc = new QDjVuHttpDocument(true);
  connect(doc, SIGNAL(error(QString,QString,int)),
          errorDialog, SLOT(error(QString,QString,int)));
  doc->setUrl(&djvuContext, url);
  if (!doc->isValid())
    {
      delete doc;
      addToErrorDialog(tr("Cannot open URL '%1'.").arg(url.toString()));
      raiseErrorDialog(QMessageBox::Critical, tr("Opening DjVu document..."));
      return false;
    }
  open(doc);
  documentUrl = url;
  parseCgiArguments(url);
  setWindowTitle(tr("Djview - %1[*]").arg(getShortFileName()));
  return true;
}


void 
QDjView::goToPage(int pageno)
{
  int pagenum = documentPages.size();
  if (!pagenum || !document)
    {
      pendingPageNo = pageno;
      pendingPageName.clear();
    }
  else
    {
      widget->setPage(pageno);
      updateActionsLater();
    }
}


void 
QDjView::goToPage(QString name, int from)
{
  int pagenum = documentPages.size();
  if (!pagenum || !document)
    {
      pendingPageNo = -1;
      pendingPageName = name;
    }
  else
    {
      int pageno = -1;
      if (from < 0)
        from = widget->page();
      // Handle names starting with hash mark
      if (name.startsWith("#") &&
          name.contains(QRegExp("^#[-+]?\\d+$")))
        {
          if (name[1]=='+')
            pageno = qMin(from + name.mid(2).toInt(), pagenum-1);
          else if (name[1]=='-')
            pageno = qMax(from - name.mid(2).toInt(), 0);
          else
            pageno = qBound(1, name.mid(1).toInt(), pagenum) - 1;
        }
      else if (name.startsWith("#="))
        name = name.mid(2);
      // Search exact name starting from current page
      QByteArray utf8Name= name.toUtf8();
      if (pageno < 0 || pageno >= pagenum)
        for (int i=from; i<pagenum; i++)
          if (documentPages[i].title && 
              ! strcmp(utf8Name, documentPages[i].title))
            { pageno = i; break; }
      if (pageno < 0 || pageno >= pagenum)
        for (int i=0; i<from; i++)
          if (documentPages[i].title &&
              ! strcmp(utf8Name, documentPages[i].title))
            { pageno = i; break; }
      // Otherwise try a number in range [1..pagenum]
      if (pageno < 0 || pageno >= pagenum)
        pageno = name.toInt() - 1;
      // Otherwise search page names and ids
      if (pageno < 0 || pageno >= pagenum)
        for (int i=0; i<pagenum; i++)
          if (documentPages[i].name && 
              !strcmp(utf8Name, documentPages[i].name))
            { pageno = i; break; }
      if (pageno < 0 || pageno >= pagenum)
        for (int i=0; i<pagenum; i++)
          if (documentPages[i].id && 
              !strcmp(utf8Name, documentPages[i].id))
            { pageno = i; break; }
      // Done
      if (pageno >= 0 && pageno < pagenum)
        widget->setPage(pageno);
    }
}


void
QDjView::addToErrorDialog(QString message)
{
  errorDialog->error(message, __FILE__, __LINE__);
}


void
QDjView::raiseErrorDialog(QMessageBox::Icon icon, QString caption)
{
  errorDialog->prepare(icon, makeCaption(caption));
  errorDialog->show();
  errorDialog->raise();
  errorDialog->activateWindow();
}


int
QDjView::execErrorDialog(QMessageBox::Icon icon, QString caption)
{
  errorDialog->prepare(icon, makeCaption(caption));
  return errorDialog->exec();
}

void  
QDjView::setPageLabelText(QString s)
{
  QLabel *m = pageLabel;
  m->setText(s);
  m->setMinimumWidth(qMax(m->minimumWidth(), m->sizeHint().width()));
}

void  
QDjView::setMouseLabelText(QString s)
{
  QLabel *m = mouseLabel;
  m->setText(s);
  m->setMinimumWidth(qMax(m->minimumWidth(), m->sizeHint().width()));
}



// ----------------------------------------
// QDJVIEW ARGUMENTS


bool 
QDjView::parseArgument(QString keyEqualValue)
{
  int n = keyEqualValue.indexOf("=");
  if (n < 0)
    return parseArgument(keyEqualValue, "yes");
  else
    return parseArgument(keyEqualValue.left(n),
                         keyEqualValue.mid(n+1));
}


bool 
QDjView::parseArgument(QString key, QString value)
{
  // TODO
  return false;
}


void 
QDjView::parseCgiArguments(QUrl url)
{
  // TODO
}




// -----------------------------------
// UTILITIES

int 
QDjView::pageNum(void)
{
  return documentPages.size();
}


QString 
QDjView::pageName(int pageno)
{
  if (pageno>=0 && pageno<documentPages.size())
    if ( documentPages[pageno].title )
      return QString::fromUtf8(documentPages[pageno].title);
  return QString::number(pageno + 1);
}


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


QString 
QDjView::makeCaption(QString caption)
{
  QString f = getShortFileName();
  if (! caption.isEmpty() && ! f.isEmpty())
    caption = caption + " - [" + f + "]";
  return caption;
}


bool 
QDjView::saveTextFile(QString text, QString filename)
{
  // obtain filename
  QString caption = makeCaption(tr("Save text", "dialog caption"));
  if (filename.isEmpty())
    {
      QString filters = "Text files (*.txt);;All files (*)";
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
      if (file.error() == QFile::OpenError && errno > 0)
        message = strerror(errno);
      QMessageBox::critical(this, caption,
                            tr("Cannot write file '%1'\n%2.")
                            .arg(QFileInfo(filename).fileName())
                            .arg(message) );
      file.remove();
      return false;
    }
  // save text in current locale encoding
  QTextStream(&file) << text;
  return true;
}


bool 
QDjView::saveImageFile(QImage image, QString filename)
{
  // obtain filename with suitable suffix
  QString caption = makeCaption(tr("Save image", "dialog caption"));
  if (filename.isEmpty())
    {
      QStringList patterns;
      foreach(QByteArray format, QImageWriter::supportedImageFormats())
        patterns << "*." + QString(format).toLower();
      QString filters = QString("All supported files (%1);;All files (*)");
      filters = filters.arg(patterns.join(" "));
      filename = QFileDialog::getSaveFileName(this, caption, filename, filters);
      if (filename.isEmpty())
        return false;
    }
  // suffix
  QString suffix = QFileInfo(filename).suffix();
  if (suffix.isEmpty())
    {
      QMessageBox::critical(this, caption,
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
        message = tr("Image format %1 is not supported.").arg(suffix.toUpper());
      else if (file.error() == QFile::OpenError && errno > 0)
        message = strerror(errno);
      QMessageBox::critical(this, caption,
                            tr("Cannot write file '%1'.\n%2.")
                            .arg(QFileInfo(filename).fileName())
                            .arg(message) );
      file.remove();
      return false;
    }
  return true;
}





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
QDjView::docinfo()
{
  if (document && documentPages.size()==0 &&
      ddjvu_document_decoding_status(*document)==DDJVU_JOB_OK)
    {
      // Obtain information about pages.
      int n = ddjvu_document_get_pagenum(*document);
#if WORKAROUND_FOR_DDJVUAPI_17
      ddjvu_document_type_t docType = ddjvu_document_get_type(*document);
      if (docType != DDJVU_DOCTYPE_BUNDLED &&  
          docType != DDJVU_DOCTYPE_INDIRECT )
        {
          // work around bug in ddjvuapi<=17 */
          for (int i=0; i<n; i++)
            {
              ddjvu_fileinfo_t info; info.type = 'P'; info.pageno = i;
              info.id = info.name = info.title = 0;
              documentPages << info;
            }
        }
      else
#endif
        {
          int m = ddjvu_document_get_filenum(*document);
          for (int i=0; i<m; i++)
            {
              ddjvu_fileinfo_t info;
              if (ddjvu_document_get_fileinfo(*document, i, &info) != DDJVU_JOB_OK)
                qWarning("Internal (docinfo): ddjvu_document_get_fileinfo failed.");
              if (info.title && info.name && !strcmp(info.title, info.name))
                info.title = 0;  // clear title if equal to name.
              if (info.type == 'P')
                documentPages << info;
            }
        }
      if (documentPages.size() != n)
        qWarning("Internal (docinfo): inconsistent number of pages.");

      // Fill page combo
      fillPageCombo(pageCombo);
      
      // Apply pending changes
      if (! pendingPageName.isNull())
        goToPage(pendingPageName);
      else if (pendingPageNo >= 0)
        goToPage(pendingPageNo);
      // ... TODO ...
      
      // Update actions
      updateActionsLater();
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
QDjView::pointerEnter(const Position &pos, miniexp_t maparea)
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
  else if (link.startsWith("#="))
    message = tr("Go: page %1.").arg(link.mid(2));
  else if (link.startsWith("#"))
    message = tr("Go: page %1.").arg(link.mid(1));
  else
    message = tr("Link: %1").arg(link);
  if (!target.isEmpty())
    message = message + " (in other window.)";
  
  
  statusBar->showMessage(message);
}


void 
QDjView::pointerLeave(const Position &pos, miniexp_t maparea)
{
  statusBar->clearMessage();
}


void 
QDjView::pointerClick(const Position &pos, miniexp_t maparea)
{
  // Obtain link information
  QString link = widget->linkUrl();
  if (link.isEmpty())
    return;
  QString target = widget->linkTarget();
  if (target=="_self" || target=="_page")
    target.clear();
  // Execute link
  if (link.startsWith("#"))
    {
      // Same document
      if (target.isEmpty())
        goToPage(link, pos.pageNo);
      else
        qWarning("TODO: same document, different window");
    }
  else if (viewerMode == STANDALONE)
    {
      qWarning("TODO: other document, standalone");
    }
  else
    {
      qWarning("TODO: other document, plugin");
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
  QAction *copyText = menu->addAction(tr("&Copy text (%1)").arg(s));
  QAction *saveText = menu->addAction(tr("&Save text as..."));
  copyText->setEnabled(l>0);
  saveText->setEnabled(l>0);
  menu->addSeparator();
  QAction *copyImage = menu->addAction(tr("Copy image (%1x%2 pixels)").arg(w).arg(h));
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


void 
QDjView::errorCondition(int pageno)
{
  QString message;
  if (pageno >= 0)
    message = tr("Cannot decode page %1.").arg(pageno);
  else
    message = tr("Cannot decode document.");
  addToErrorDialog(message);
  raiseErrorDialog(QMessageBox::Warning, tr("Decoding DjVu document..."));
}


void
QDjView::updateActionsLater()
{
  if (! needToUpdateActions)
    {
      needToUpdateActions = true;
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
       "<a href=http://djvulibre.djvuzone.org>http://djvulibre.djvuzone.org</a><br>"
       "Copyright \251 2006- L\351on Bottou."
       "</p>"
       "<p align=justify><small>"
       "This program is free software. "
       "You can redistribute or modify it under the terms of the "
       "GNU General Public License as published by the Free Software Foundation. "
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
  QString caption = tr("Open", "dialog caption");
  QString filters = "DjVu files (*.djvu *.djv)";
  QString filename = QFileDialog::getOpenFileName(this, caption, "", filters);
  if (! filename.isEmpty())
    open(filename);
}


void 
QDjView::performInformation(void)
{
  if (! documentPages.size())
    return;
  if (! infoDialog)
    infoDialog = new QDjViewInfoDialog(this);
  infoDialog->setWindowTitle(makeCaption(tr("DjView Information")));
  infoDialog->setPage(widget->page());
  infoDialog->refreshDocument();
  infoDialog->raise();
  infoDialog->show();
}


void 
QDjView::performRotation(void)
{
  QAction *action = qobject_cast<QAction*>(sender());
  widget->setRotation(action->data().toInt());
}


void 
QDjView::performZoom(void)
{
  QAction *action = qobject_cast<QAction*>(sender());
  widget->setZoom(action->data().toInt());
}


void 
QDjView::performSelect(bool checked)
{
  if (checked)
    widget->setModifiersForSelect(Qt::NoModifier);
  else
    widget->setModifiersForSelect(prefs->modifiersForSelect);
}


template<class T> 
static inline void
exch(T& a, T& b)
{
  T tmp = a; a = b; b = tmp;
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



/* -------------------------------------------------------------
   Local Variables:
   c++-font-lock-extra-types: ( "\\sw+_t" "[A-Z]\\sw*[a-z]\\sw*" )
   End:
   ------------------------------------------------------------- */
