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
#include <QClipboard>
#include <QTimer>
#include <QEvent>
#include <QCloseEvent>
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
// COMBO BOXES

void
QDjView::createCombos(void)
{
  // - mode combo box
  modeCombo = new QComboBox(toolBar);
  modeCombo->addItem(tr("Color","modeCombo"), 
                     QDjVuWidget::DISPLAY_COLOR );
  modeCombo->addItem(tr("Stencil","modeCombo"), 
                     QDjVuWidget::DISPLAY_STENCIL );
  modeCombo->addItem(tr("Foreground","modeCombo"), 
                     QDjVuWidget::DISPLAY_FG );
  modeCombo->addItem(tr("Background","modeCombo"), 
                     QDjVuWidget::DISPLAY_BG );
  connect(modeCombo, SIGNAL(activated(int)),
          this, SLOT(modeComboActivated(int)) );
  connect(modeCombo, SIGNAL(activated(int)),
          this, SLOT(updateActionsLater()) );

  // - zoom combo box
  zoomCombo = new QComboBox(toolBar);

  // - page combo box
  pageCombo = new QComboBox(toolBar);
  connect(pageCombo, SIGNAL(activated(int)),
          this, SLOT(goToPage(int)));
  
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

namespace {

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
    if (action->text().isNull())
      action->setText(string);
    else if (action->statusTip().isNull())
      action->setStatusTip(string);
    else if (action->toolTip().isNull())
      action->setToolTip(string);
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
    << tr("Close this window.")
    << Trigger(this, SLOT(close()));
  actionQuit = makeAction(tr("&Quit", "File|Quit"))
    << QKeySequence(tr("Ctrl+Q", "File|Quit"))
    << QIcon(":/images/icon_quit.png")
    << tr("Close all windows and quit the application.")
    << Trigger(QCoreApplication::instance(), SLOT(quit()));
  actionSave = makeAction(tr("&Save as...", "File|Save"))
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
    << tr("Find text in the DjVu document.");
  actionSelect = makeAction(tr("&Select"), false)
    << QIcon(":/images/icon_select.png")
    << tr("Select rectangle in document (same as middle mouse button).");
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
  actionPageInfo = makeAction(tr("Page &Information..."))
    << tr("Show DjVu encoding information for the current page.");
  actionDocInfo = makeAction(tr("&Document Information..."))
    << tr("Show DjVu encoding information for the document.");
  actionAbout = makeAction(tr("&About DjView..."))
    << QIcon(":/images/icon_djvu.png")
    << tr("Show information about this program.");
  actionDisplayColor = makeAction(tr("&Color", "Display|Color"), false)
    << tr("Display document in full colors.")
    << Trigger(widget, SLOT(displayModeColor()))
    << Trigger(this, SLOT(updateActionsLater()))
    << *modeActionGroup;
  actionDisplayBW = makeAction(tr("&Mask", "Display|BW"), false)
    << tr("Only display the document black&white stencil.")
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
    << tr("Show s&ide bar")
    << QKeySequence("F9")
    << QIcon(":/images/icon_sidebar.png")
    << tr("Show/hide the side bar.")
    << Trigger(this, SLOT(updateActionsLater()));
  actionViewToolBar = toolBar->toggleViewAction()
    << tr("Show &tool bar")
    << tr("Show/hide the standard tool bar.")
    << Trigger(this, SLOT(updateActionsLater()));
  actionViewStatusBar = makeAction(tr("Show &status bar"), true)
    << tr("Show/hide the status bar.")
    << Trigger(statusBar,SLOT(setVisible(bool)))
    << Trigger(this, SLOT(updateActionsLater()));
  actionViewFullScreen = makeAction(tr("&Full Screen","View|FullScreen"), false)
    << QKeySequence("F11")
    << QIcon(":/images/icon_fullscreen.png")
    << tr("Toggle full screen mode.");
  actionLayoutContinuous = makeAction(tr("&Continuous","Layout"), false)
    << QIcon(":/images/icon_continuous.png")
    << QKeySequence("F2")
    << tr("Toggle continuous layout mode.")
    << Trigger(widget, SLOT(setContinuous(bool)))
    << Trigger(this, SLOT(updateActionsLater()));
  actionLayoutSideBySide = makeAction(tr("&Side by side","Layout"), false)
    << QIcon(":/images/icon_sidebyside.png")
    << QKeySequence("F3")
    << tr("Toggle side-by-side layout mode.")
    << Trigger(widget, SLOT(setSideBySide(bool)))
    << Trigger(this, SLOT(updateActionsLater()));
  actionLayoutPageSettings = makeAction(tr("Obey &annotations"), false)
    << tr("Obey/ignore layout instructions from the page data.")
    << Trigger(widget, SLOT(setPageSettings(bool)));

  // Enumerate all actions
  QAction *a;
  QObject *o;
  allActions.clear();
  foreach(o, children())
    if ((a = qobject_cast<QAction*>(o)))
      allActions << a;
}


void
QDjView::updateActions()
{
  // Enable all actions
  foreach(QAction *action, allActions)
    action->setEnabled(true);
  
  // Some actions are only available in standalone mode
  actionNew->setVisible(viewerMode == STANDALONE);
  actionOpen->setVisible(viewerMode == STANDALONE);
  actionClose->setVisible(viewerMode == STANDALONE);
  actionQuit->setVisible(viewerMode == STANDALONE);
  actionViewFullScreen->setVisible(viewerMode == STANDALONE);
  
  
  // - zoom combo and actions

  // - mode combo and actions
  QDjVuWidget::DisplayMode mode = widget->displayMode();
  modeCombo->setCurrentIndex(modeCombo->findData(QVariant(mode)));
  actionDisplayColor->setChecked(mode == QDjVuWidget::DISPLAY_COLOR);
  actionDisplayBW->setChecked(mode == QDjVuWidget::DISPLAY_STENCIL);
  actionDisplayBackground->setChecked(mode == QDjVuWidget::DISPLAY_BG);
  actionDisplayForeground->setChecked(mode == QDjVuWidget::DISPLAY_FG);
  
  // - page combo and actions
  int pagenum = documentPages.size();
  int pageno = widget->page();
  pageCombo->setCurrentIndex(pageno);
  pageCombo->setEnabled(pagenum > 0);
  actionNavFirst->setEnabled(pagenum>0);
  actionNavPrev->setEnabled(pagenum>0 && pageno>0);
  actionNavNext->setEnabled(pagenum>0 && pageno<pagenum-1);
  actionNavLast->setEnabled(pagenum>0);

  // - misc actions
  actionLayoutContinuous->setChecked(widget->continuous());  
  actionLayoutSideBySide->setChecked(widget->sideBySide());
  actionLayoutPageSettings->setChecked(widget->pageSettings());


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
            action != actionAbout )
          action->setEnabled(false);
    }
  
  // Update option flags
  if (! needToApplyOptions)
    {
#define setFlag(flag, test) \
  options = (options & ~(QDjViewPrefs::flag)) \
          | ((test) ? (QDjViewPrefs::flag) \
                    : (QDjViewPrefs::Option)0)
      // Track options changeable by user interaction
      setFlag(SHOW_TOOLBAR,        !toolBar->isHidden());
      setFlag(SHOW_SIDEBAR,        !sideBar->isHidden());
      setFlag(SHOW_STATUSBAR,      !statusBar->isHidden());
      setFlag(LAYOUT_CONTINUOUS,   widget->continuous());
      setFlag(LAYOUT_SIDEBYSIDE,   widget->sideBySide());
      setFlag(LAYOUT_PAGESETTINGS, widget->pageSettings());
#undef setFlag
    }

  // Finished
  needToUpdateActions = false;
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
  settingsMenu->addAction(actionViewSideBar);
  settingsMenu->addAction(actionViewToolBar);
  settingsMenu->addAction(actionViewStatusBar);
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
// APPLY OPTIONS/TOOLS/PREFERENCES


void
QDjView::createToolBar(void)
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
  // Done
  toolBar->setVisible(!wasHidden);
  toolBarOptions = tools;
}


void
QDjView::applyOptions(void)
{
  

  
  // Recreate toolbar when changed
  if (toolBarOptions != tools)
    createToolBar();

  // Done
  needToApplyOptions = false;
}


void
QDjView::applyPreferences(void)
{
}



// ----------------------------------------
// CONSTRUCTOR



QDjView::QDjView(QDjVuContext &context, ViewerMode mode, QWidget *parent)
  : QMainWindow(parent),
    viewerMode(mode),
    djvuContext(context),
    document(0),
    needToUpdateActions(false),
    needToApplyOptions(false),
    pendingPageNo(-1)
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
  
  // Create dialogs
  errorDialog = new QDjViewDialogError(this);
  
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
  font.setPointSize(font.pointSize() * 3 / 4);
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

  // - toolbar  
  toolBar = new QToolBar(this);
  toolBar->setObjectName("toolbar");
  addToolBar(toolBar);
  toolBarOptions = 0;

  // - sidebar  
  sideBar = new QDockWidget(this);  // for now
  sideBar->setObjectName("sidebar");
  addDockWidget(Qt::LeftDockWidgetArea, sideBar);
  
  // Setup main window
  setWindowTitle(tr("DjView"));
  setWindowIcon(QIcon(":/images/icon32_djvu.png"));
  if (QApplication::windowIcon().isNull())
    QApplication::setWindowIcon(windowIcon());

  // Setup everything else
  createCombos();
  createActions();
  createMenus();
  applyOptions();
  applyPreferences();
  updateActions();
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
  QDjVuDocument *doc = document;
  if (doc)
    disconnect(doc, 0, this, 0);
  widget->setDocument(0);
  documentPages.clear();
  documentFileName.clear();
  documentUrl.clear();
  document = 0;
  if (doc)
    emit documentClosed();
}


void 
QDjView::open(QDjVuDocument *doc)
{
  closeDocument();
  document = doc;
  connect(doc,SIGNAL(destroyed(void)), this, SLOT(closeDocument(void)));
  connect(doc,SIGNAL(docinfo(void)), this, SLOT(docinfo(void)));
  disconnect(document, 0, errorDialog, 0);
  widget->setDocument(document);
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
      raiseErrorDialog(QMessageBox::Critical,
                       tr("Open DjVu file ..."),
                       tr("Cannot open file '%1'.").arg(filename) );
      return false;
    }
  open(doc);
  setWindowTitle(tr("Djview - %1").arg(filename));
  documentFileName = filename;
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
      raiseErrorDialog(QMessageBox::Critical,
                       tr("Open DjVu document ..."),
                       tr("Cannot open URL '%1'.").arg(url.toString()) );
      return false;
    }
  open(doc);
  setWindowTitle(tr("Djview - %1").arg(url.toString()));
  documentUrl = url;
  parseCgiArguments(url);
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
          name.contains(QRegExp("^#[-+=]?\\d+$")))
        {
          if (name[1]=='+')
            pageno = qMin(from + name.mid(2).toInt(), pagenum-1);
          else if (name[1]=='-')
            pageno = qMax(from - name.mid(2).toInt(), 0);
          else if (name[1]=='=')
            pageno = qBound(1, name.mid(2).toInt(), pagenum) - 1;
          else
            pageno = qBound(1, name.mid(1).toInt(), pagenum) - 1;
        }
      else if (name.startsWith("#="))
        name = name.mid(2);
      // Search exact name starting from current page
      QByteArray utf8Name= name.toUtf8();
      if (pageno < 0 || pageno >= pagenum)
        for (int i=from; i<pagenum; i++)
          if (! strcmp(utf8Name, documentPages[i].title))
            { pageno = i; break; }
      if (pageno < 0 || pageno >= pagenum)
        for (int i=0; i<from; i++)
          if (! strcmp(utf8Name, documentPages[i].title))
            { pageno = i; break; }
      // Otherwise let ddjvuapi do the search
      if (pageno < 0 || pageno >= pagenum)
        pageno = ddjvu_document_search_pageno(*document, utf8Name);
      // Done
      if (pageno >= 0 && pageno < pagenum)
        widget->setPage(pageno);
    }
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
    {
      ddjvu_fileinfo_t &info = documentPages[pageno];
      if (strcmp(info.name, info.title))
        return QString::fromUtf8(info.title);
    }
  return QString::number(pageno + 1);
}




// -----------------------------------
// OVERRIDES


void
QDjView::closeEvent(QCloseEvent *event)
{
  generalPrefs->toolState = saveState();
  generalPrefs->save();
  closeDocument();
  // Accept close event
  QMainWindow::closeEvent(event);
}


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
  if (document && documentPages.size()==0 &&
      ddjvu_document_decoding_status(*document)==DDJVU_JOB_OK)
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
      n = documentPages.size();
      if (n != ddjvu_document_get_pagenum(*document))
        qWarning("Internal (docinfo): inconsistent number of pages.");
      
      // Fill page combo
      pageCombo->clear();
      for (int j=0; j<n; j++)
        pageCombo->addItem(pageName(j));
      
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
  
  
  statusBar->showMessage(link+" "+message);
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
      if (target.isNull())
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
    { /* TODO */ }
  else if (action == copyImage)
    QApplication::clipboard()->setImage(widget->getImageForRect(rect));
  else if (action == saveImage)
    { /* TODO */ }
  
  // Cancel select mode.
  updateActionsLater();
  if (actionSelect->isChecked())
    {
      actionSelect->setChecked(false);
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
  raiseErrorDialog(QMessageBox::Warning,
                   tr("Decoding DjVu document ..."),
                   message);
}


void
QDjView::applyOptionsLater()
{
  if (! needToApplyOptions)
    {
      needToApplyOptions = true;
      QTimer::singleShot(0, this, SLOT(applyOptions()));
    }
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
  int mode = modeCombo->itemData(index).toUInt();
  widget->setDisplayMode((QDjVuWidget::DisplayMode)mode);
}


/* -------------------------------------------------------------
   Local Variables:
   c++-font-lock-extra-types: ( "\\sw+_t" "[A-Z]\\sw*[a-z]\\sw*" )
   End:
   ------------------------------------------------------------- */
