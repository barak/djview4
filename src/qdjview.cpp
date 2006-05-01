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
#include <QWhatsThis>
#include <QFileInfo>
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
#include <QLineEdit>
#include <QRegExpValidator>
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
  modeCombo->addItem(tr("Color","modeCombo"), QDjVuWidget::DISPLAY_COLOR);
  modeCombo->addItem(tr("Stencil","modeCombo"), QDjVuWidget::DISPLAY_STENCIL);
  modeCombo->addItem(tr("Foreground","modeCombo"), QDjVuWidget::DISPLAY_FG);
  modeCombo->addItem(tr("Background","modeCombo"), QDjVuWidget::DISPLAY_BG );
  connect(modeCombo, SIGNAL(activated(int)),
          this, SLOT(modeComboActivated(int)) );
  connect(modeCombo, SIGNAL(activated(int)),
          this, SLOT(updateActionsLater()) );

  // - zoom combo box
  zoomCombo = new QComboBox(toolBar);
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
  connect(zoomCombo, SIGNAL(activated(int)),
          this, SLOT(zoomComboActivated(int)) );
  zoomCombo->setEditable(true);
  zoomCombo->setInsertPolicy(QComboBox::NoInsert);
  connect(zoomCombo->lineEdit(), SIGNAL(editingFinished()),
          this, SLOT(zoomComboEdited()) );

  // - page combo box
  pageCombo = new QComboBox(toolBar);
  pageCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  pageCombo->setMinimumWidth(80);
  connect(pageCombo, SIGNAL(activated(int)),
          this, SLOT(pageComboActivated(int)));
  pageCombo->setEditable(true);
  pageCombo->setInsertPolicy(QComboBox::NoInsert);
  connect(pageCombo->lineEdit(), SIGNAL(editingFinished()),
          this, SLOT(pageComboEdited()) );
  
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
  if (action->text().isNull())
    action->setText(string);
  else if (action->statusTip().isNull())
    action->setStatusTip(string);
  else if (action->whatsThis().isNull())
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
    << tr("Magnify r0%")
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

  actionPageInfo = makeAction(tr("Page &Information..."))
    << tr("Show DjVu encoding information for the current page.");

  actionDocInfo = makeAction(tr("Document In&formation..."))
    << QKeySequence("Ctrl+I")
    << tr("Show DjVu encoding information for the document.");

  actionWhatsThis = QWhatsThis::createAction(this);

  actionAbout = makeAction(tr("&About DjView..."))
    << QIcon(":/images/icon_djvu.png")
    << tr("Show information about this program.");

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
    << tr("Show s&ide bar")
    << QKeySequence("F9")
    << QIcon(":/images/icon_sidebar.png")
    << tr("Show/hide the side bar.")
    << Trigger(this, SLOT(updateActionsLater()));

  actionViewToolBar = toolBar->toggleViewAction()
    << tr("Show &tool bar")
    << QKeySequence("F10")
    << tr("Show/hide the standard tool bar.")
    << Trigger(this, SLOT(updateActionsLater()));

  actionViewStatusBar = makeAction(tr("Show &status bar"), true)
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
QDjView::updateActions()
{
  // Rebuild toolbar if necessary
  if (tools != toolsCached)
    createToolBar();

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
  pageCombo->setCurrentIndex(pageno);
  if (pageno >= 0 && pagenum > 0)
    pageCombo->setEditText(pageName(pageno));
  pageCombo->setEnabled(pagenum > 0);
  actionNavFirst->setEnabled(pagenum>0);
  actionNavPrev->setEnabled(pagenum>0 && pageno>0);
  actionNavNext->setEnabled(pagenum>0 && pageno<pagenum-1);
  actionNavLast->setEnabled(pagenum>0);

  // - misc actions
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
  QMenu *infoMenu = contextMenu->addMenu("Infor&mation");
  infoMenu->addAction(actionPageInfo);
  infoMenu->addAction(actionDocInfo);
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



// ----------------------------------------
// APPLY PREFERENCES


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
  if (contextMenu && enable && oldContextMenu != contextMenu)
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
QDjView::applyPreferences(void)
{
  // Window state
  if (prefs->windowState.size() > 0)
    restoreState(prefs->windowState);
  if (prefs->windowSize.isValid())
    resize(prefs->windowSize);

  // Cache size
  djvuContext.setCacheSize(prefs->cacheSize);

  // Toolbar visibility
  menuBar->setVisible(options & QDjViewPrefs::SHOW_MENUBAR);
  toolBar->setVisible(options & QDjViewPrefs::SHOW_TOOLBAR);
  sideBar->setVisible(options & QDjViewPrefs::SHOW_SIDEBAR);
  statusBar->setVisible(options & QDjViewPrefs::SHOW_STATUSBAR);

  // Other QDjVuWidget preferences
  widget->setZoom(prefs->zoom);
  widget->setModifiersForLens(prefs->modifiersForLens);
  widget->setModifiersForSelect(prefs->modifiersForSelect);
  widget->setModifiersForLinks(prefs->modifiersForLinks);
  widget->setGamma(prefs->gamma);
  widget->setPixelCacheSize(prefs->pixelCacheSize);
  widget->setLensSize(prefs->lensSize);
  widget->setLensPower(prefs->lensPower);
  widget->setDisplayFrame(options & QDjViewPrefs::SHOW_FRAME);
  widget->setContinuous(options & QDjViewPrefs::LAYOUT_CONTINUOUS);
  widget->setSideBySide(options & QDjViewPrefs::LAYOUT_SIDEBYSIDE);
  widget->enableKeyboard(options & QDjViewPrefs::HANDLE_KEYBOARD);
  widget->enableMouse(options & QDjViewPrefs::HANDLE_MOUSE);
  widget->enableHyperlink(options & QDjViewPrefs::HANDLE_LINKS);
  enableScrollBars(options & QDjViewPrefs::SHOW_SCROLLBARS);
  enableContextMenu(options & QDjViewPrefs::HANDLE_CONTEXTMENU);

  // Make everything default actions
  widget->makeDefaults();
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
  ms = QDjViewPrefs::modifiersToString(prefs->modifiersForSelect);
  ml = QDjViewPrefs::modifiersToString(prefs->modifiersForLens);

  Help(tr("<html><b>Selecting a rectangle...</b><br/> "
          "Once a rectanglular area is selected, a popup menu "
          "lets you copy the corresponding text or image. "
          "Instead of using this tool, you can also hold %1 "
          "and use the left mouse button."
          "</html>").arg(ms))
            >> actionSelect;
  
  Help(tr("<html><b>Zooming...</b><br/> "
          "Choose a zoom level for viewing the document. "
          "Zoom level <tt>100%</tt> displays the document suitably for a 100 dpi screen. "
          "Zoom levels <tt>Fit Page</tt> and <tt>Fit Width</tt> ensure that "
          "the full page or the page width fit in the window. "
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
          "Jump to a specific page of the document. </html>"))
            >> actionNavFirst >> actionNavPrev >> actionNavNext >> actionNavLast
            >> pageCombo;
  
  Help(tr("<html><b>Continuous layout.</b><br/> "
          "Display all the document pages arranged vertically "
          "inside the scrollable document viewing area.</html>"))
            >> actionLayoutContinuous;
  
  Help(tr("<html><b>Side by side layout.</b><br/> "
          "Display pairs of pages side by side "
          "inside the scrollable document viewing area.</html>"))
            >> actionLayoutSideBySide;
  
  Help(tr("<html><b>Page information.</b><br/> "
          "Display the page name followed by the page size in pixels "
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
          "    to zoom and rotate the document.</li>"
          "<li>Left mouse button for panning and selecting links.</li>"
          "<li>%1+Left mouse button for selecting text or images.</li>"
          "<li>%2 for popping the magnification lens.</li>"
          "</ul></html>").arg(ms).arg(ml))
            >> widget;
  
  Help(tr("<html><b>Document viewing area.</b><br/> "
          "This is the main display area for the DjVu document. "
          "But you must first open a DjVu document to see anything."
          "</html>"))
            >> splash;
    
  // TODO...
}



// ----------------------------------------
// CONSTRUCTOR



QDjView::QDjView(QDjVuContext &context, ViewerMode mode, QWidget *parent)
  : QMainWindow(parent),
    viewerMode(mode),
    djvuContext(context),
    document(0),
    pendingPageNo(-1),
    needToUpdateActions(false)
{
  // obtain preferences
  prefs = QDjViewPrefs::create();
  tools = prefs->tools;
  options = prefs->forStandalone;
  fsOptions = prefs->forFullScreen;
  if (viewerMode == EMBEDDED_PLUGIN)
    options = prefs->forEmbeddedPlugin;
  else if (viewerMode == FULLPAGE_PLUGIN)
    options = prefs->forFullPagePlugin;
  toolsCached = 0;
  
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
  enableContextMenu(true);

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
  toolBar->setAllowedAreas(Qt::TopToolBarArea|Qt::BottomToolBarArea);
  addToolBar(toolBar);

  // - sidebar  
  sideBar = new QDockWidget(this);  // for now
  sideBar->setObjectName("sidebar");
  sideBar->setAllowedAreas(Qt::LeftDockWidgetArea|Qt::RightDockWidgetArea);
  addDockWidget(Qt::LeftDockWidgetArea, sideBar);
  
  // Setup main window
  setWindowTitle(tr("DjView"));
  setWindowIcon(QIcon(":/images/djvu.png"));
  if (QApplication::windowIcon().isNull())
    QApplication::setWindowIcon(windowIcon());

  // Setup everything else
  createCombos();
  createActions();
  createMenus();
  createWhatsThis();
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
  setWindowTitle(tr("Djview - %1[*]").arg(filename));
  documentFileName = filename;
  documentUrl = QUrl::fromLocalFile(QFileInfo(filename).absoluteFilePath());
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
  setWindowTitle(tr("Djview - %1[*]").arg(url.toString()));
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
          if (! strcmp(utf8Name, documentPages[i].title))
            { pageno = i; break; }
      if (pageno < 0 || pageno >= pagenum)
        for (int i=0; i<from; i++)
          if (! strcmp(utf8Name, documentPages[i].title))
            { pageno = i; break; }
      // Otherwise try a number in range [1..pagenum]
      if (pageno < 0 || pageno >= pagenum)
        pageno = name.toInt() - 1;
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
  prefs->windowState = saveState();
  if (isWindow() && !isHidden() && !isFullScreen() &&
      !isMaximized() && !isMinimized())
    prefs->windowSize = size();
  prefs->saveWindow();
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
  raiseErrorDialog(QMessageBox::Warning,
                   tr("Decoding DjVu document ..."),
                   message);
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


void
QDjView::zoomComboActivated(int index)
{
  int zoom = zoomCombo->itemData(index).toUInt();
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
  goToPage(index);
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


void 
QDjView::performViewFullScreen(bool checked)
{
  if (viewerMode != STANDALONE)
    return;
  
  if (checked)
    {
      // Save toolbar visibility
      options &= ~QDjViewPrefs::SHOW_MENUBAR;
      options &= ~QDjViewPrefs::SHOW_TOOLBAR;
      options &= ~QDjViewPrefs::SHOW_STATUSBAR;
      options &= ~QDjViewPrefs::SHOW_SIDEBAR;
      options &= ~QDjViewPrefs::SHOW_SCROLLBARS;
      options &= ~QDjViewPrefs::SHOW_FRAME;
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
      // Apply fullscreen options
      menuBar->setVisible(fsOptions & QDjViewPrefs::SHOW_MENUBAR);
      toolBar->setVisible(fsOptions & QDjViewPrefs::SHOW_TOOLBAR);
      sideBar->setVisible(fsOptions & QDjViewPrefs::SHOW_SIDEBAR);
      statusBar->setVisible(fsOptions & QDjViewPrefs::SHOW_STATUSBAR);
      enableScrollBars(fsOptions & QDjViewPrefs::SHOW_SCROLLBARS);
      widget->setDisplayFrame(fsOptions & QDjViewPrefs::SHOW_FRAME);
      // Make sure full screen action remains accessible (F11)
      if (! actions().contains(actionViewFullScreen))
        addAction(actionViewFullScreen);
      // Run fullscreen
      Qt::WindowStates state = windowState();
      fsSavedState = state;
      state &= ~Qt::WindowMinimized;
      state &= ~Qt::WindowMaximized;
      state |= Qt::WindowFullScreen;
      setWindowState(state);
    }
  else
    {
      fsOptions &= ~QDjViewPrefs::SHOW_MENUBAR;
      fsOptions &= ~QDjViewPrefs::SHOW_TOOLBAR;
      fsOptions &= ~QDjViewPrefs::SHOW_STATUSBAR;
      fsOptions &= ~QDjViewPrefs::SHOW_SIDEBAR;
      fsOptions &= ~QDjViewPrefs::SHOW_SCROLLBARS;
      fsOptions &= ~QDjViewPrefs::SHOW_FRAME;
      if (! menuBar->isHidden())
        fsOptions |= QDjViewPrefs::SHOW_MENUBAR;
      if (! toolBar->isHidden())
        fsOptions |= QDjViewPrefs::SHOW_TOOLBAR;
      if (! sideBar->isHidden())
        fsOptions |= QDjViewPrefs::SHOW_SIDEBAR;
      if (! statusBar->isHidden())
        fsOptions |= QDjViewPrefs::SHOW_STATUSBAR;
      if (widget->verticalScrollBarPolicy() != Qt::ScrollBarAlwaysOff)
        fsOptions |= QDjViewPrefs::SHOW_SCROLLBARS;
      if (widget->displayFrame())
        fsOptions |= QDjViewPrefs::SHOW_FRAME;
      // Restore normal options
      menuBar->setVisible(options & QDjViewPrefs::SHOW_MENUBAR);
      toolBar->setVisible(options & QDjViewPrefs::SHOW_TOOLBAR);
      sideBar->setVisible(options & QDjViewPrefs::SHOW_SIDEBAR);
      statusBar->setVisible(options & QDjViewPrefs::SHOW_STATUSBAR);
      enableScrollBars(options & QDjViewPrefs::SHOW_SCROLLBARS);
      widget->setDisplayFrame(options & QDjViewPrefs::SHOW_FRAME);
      // Demote full screen action to normal status
      if (actions().contains(actionViewFullScreen))
        removeAction(actionViewFullScreen);
      // Restore window state
      Qt::WindowStates state = windowState();
      state &= ~Qt::WindowFullScreen;
      state &= ~Qt::WindowMinimized;
      state &= ~Qt::WindowMaximized;
      state |= (fsSavedState & Qt::WindowMinimized);
      state |= (fsSavedState & Qt::WindowMaximized);
      setWindowState(state);
    }
}




/* -------------------------------------------------------------
   Local Variables:
   c++-font-lock-extra-types: ( "\\sw+_t" "[A-Z]\\sw*[a-z]\\sw*" )
   End:
   ------------------------------------------------------------- */
