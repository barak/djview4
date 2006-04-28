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
#include <QMenuBar>
#include <QStatusBar>
#include <QStackedLayout>
#include <QFrame>
#include <QLabel>
#include <QToolBar>
#include <QAction>
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

#include "qdjview.h"
#include "qdjviewprefs.h"
#include "qdjvu.h"
#include "qdjvuhttp.h"
#include "qdjvuwidget.h"


#include "ui_qdjviewdialogerror.h"



// ----------------------------------------
// QDJVIEWDIALOGERROR



class QDjViewDialogError : public QDialog
{
  Q_OBJECT
public:
  QDjViewDialogError(QWidget *parent);
  void prepare(QMessageBox::Icon icon, QString caption);
public slots:
  void error(QString message, QString filename, int lineno);
  void okay(void);
private:
  Ui::QDjViewDialogError ui;
  QList<QString> messages;
};

QDjViewDialogError::QDjViewDialogError(QWidget *parent)
  : QDialog(parent)
{
  ui.setupUi(this);
  connect(ui.okButton, SIGNAL(clicked()), this, SLOT(okay()));
  ui.textEdit->viewport()->setBackgroundRole(QPalette::Window);
  setWindowTitle(tr("DjView Error Message"));
}

void 
QDjViewDialogError::error(QString message, QString, int)
{
  // Remove [1-nnnnn] prefix from djvulibre-3.5
  if (message.startsWith("["))
    message = message.replace(QRegExp("^\\[\\d*-?\\d*\\]\\s*") , "");
  // Ignore empty and duplicate messages
  if (message.isEmpty()) 
    return;
  if (!messages.isEmpty() && message == messages[0]) 
    return;
  // Add message
  messages.prepend(message);
  if (messages.size() >= 16)
    messages.removeLast();
  // Show message
  QString html;
  for (int i=0; i<messages.size(); i++)
    html = "<li>" + messages[i] + "</li>" + html;
  html = "<html><ul>" + html + "</ul></html>";
  ui.textEdit->setHtml(html);
  QScrollBar *scrollBar = ui.textEdit->verticalScrollBar();
  scrollBar->setValue(scrollBar->maximum());
}

void 
QDjViewDialogError::prepare(QMessageBox::Icon icon, QString caption)
{
  if (icon != QMessageBox::NoIcon)
    ui.iconLabel->setPixmap(QMessageBox::standardIcon(icon));
  if (!caption.isEmpty())
    setWindowTitle(caption);
}

void 
QDjViewDialogError::okay()
{
  messages.clear();
  accept();
  hide();
}



// ----------------------------------------
// QDJVIEW ACTIONS




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
    << tr("Close this window.");
  actionQuit = makeAction(tr("&Quit", "File|Quit"))
    << QKeySequence(tr("Ctrl+Q", "File|Quit"))
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
    << QKeySequence(tr("Ctrl++", "Zoom|In"))
    << QIcon(":/images/icon_zoomin.png")
    << tr("Increase the magnification.");
  actionZoomOut = makeAction(tr("Zoom &Out"))
    << QKeySequence(tr("Ctrl+-", "Zoom|Out"))
    << QIcon(":/images/icon_zoomout.png")
    << tr("Decrease the magnification.");
  actionZoomFitWidth = makeAction(tr("Fit &Width"))
    << tr("Set magnification to fit page width.");
  actionZoomFitPage = makeAction(tr("Fit &Page"))
    << tr("Set magnification to fit page.");
  
  actionNavFirst = makeAction(tr("&First Page"))
    << QIcon(":images/icon_first.png")
    << tr("Jump to first document page.");
  actionNavNext = makeAction(tr("&Next Page"))
    << QIcon(":images/icon_next.png")
    << tr("Jump to next document page.");
  actionNavPrev = makeAction(tr("&Previous Page"))
    << QIcon(":images/icon_prev.png")
    << tr("Jump to previous document page.");
  actionNavLast = makeAction(tr("&Last Page"))
    << QIcon(":images/icon_last.png")
    << tr("Jump to last document page.");
  
  actionRotateLeft = makeAction(tr("Rotate &Left"))
    << QIcon(":images/icon_rotateleft.png")
    << tr("Rotate page image counter-clockwise.");
  actionRotateLeft = makeAction(tr("Rotate &Right"))
    << QIcon(":images/icon_rotateright.png")
    << tr("Rotate page image clockwise.");
  actionRotate0 = makeAction(tr("Rotate &0°"))
    << tr("Set natural page orientation.");
  actionRotate0 = makeAction(tr("Rotate &90°"))
    << tr("Turn page on its left side.");
  actionRotate0 = makeAction(tr("Rotate &180°"))
    << tr("Turn page upside-down.");
  actionRotate0 = makeAction(tr("Rotate &270°"))
    << tr("Turn page on its right side.");

  actionPageInfo = makeAction(tr("Page &Information"))
    << tr("Show DjVu encoding information for the current page.");
  actionDocInfo = makeAction(tr("&Document Information"))
    << tr("Show DjVu encoding information for the document.");
  actionAbout = makeAction(tr("&About DjView"))
    << tr("Show information about this program.");

  actionDisplayColor = makeAction(tr("&Color", "Display|Color"))
    << tr("Display document in full colors.");
  actionDisplayBW = makeAction(tr("&Mask", "Display|BW"))
    << tr("Only display the document black&white stencil.");
  actionDisplayForeground = makeAction(tr("&Foreground", "Display|Foreground"))
    << tr("Only display the foreground layer.");
  actionDisplayBackground = makeAction(tr("&Background", "Display|Background"))
    << tr("Only display the background layer.");

  actionPreferences = makeAction(tr("Prefere&nces")) 
    << tr("Show the preferences dialog.");
  
  actionViewToolbar = makeAction(tr("&Toolbar","View|Toolbar"), false)
    << tr("Show/hide the standard toolbar.");
  actionViewSearchbar = makeAction(tr("&Searchbar","View|Searchbar"), false)
    << tr("Show/hide the search toolbar.");
  actionViewStatusbar = makeAction(tr("S&tatusbar","View|Statusbar"), false)
    << tr("Show/hide the search toolbar.");
  actionViewSidebar = makeAction(tr("Side&bar","View|Sidebar"), false)
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

  // Temporary
  QMenu *menu = menuBar->addMenu("Layout");
  menu->addAction(actionLayoutContinuous);
  menu->addAction(actionLayoutSideBySide);
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
// QDJVIEW


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
  appearanceFlags = appearancePrefs->flags;
  
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
  statusBarLabel1 = new QLabel(statusBar);
  statusBarLabel1->setFont(font);
  statusBarLabel1->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
  statusBarLabel1->setFrameStyle(QFrame::Panel);
  statusBarLabel1->setFrameShadow(QFrame::Sunken);
  statusBarLabel1->setMinimumWidth(metric.width(" 88 (8888x8888@888dpi) ")); 
  statusBar->addPermanentWidget(statusBarLabel1);
  statusBarLabel2 = new QLabel(statusBar);
  statusBarLabel2->setFont(font);
  statusBarLabel2->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
  statusBarLabel2->setFrameStyle(QFrame::Panel);
  statusBarLabel2->setFrameShadow(QFrame::Sunken);
  statusBarLabel2->setMinimumWidth(metric.width(" x=888 y=888 "));
  statusBar->addPermanentWidget(statusBarLabel2);
  setStatusBar(statusBar);
  
  // create actions
  createActions();
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
          statusBarLabel1->clear();
          statusBarLabel2->clear();
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
  QLabel *p = statusBarLabel1;
  QLabel *m = statusBarLabel2;
  p->setText(tr(" %1 (%2x%3@%4dpi) ").arg(pageName(pos.pageNo)).
             arg(page.width).arg(page.height).arg(page.dpi) );
  p->setMinimumWidth(qMax(p->minimumWidth(), p->sizeHint().width()));
  m->clear();
  if (pos.inPage)
    m->setText(tr(" x=%1 y=%2 ").arg(pos.posPage.x()).arg(pos.posPage.y()));
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



// ----------------------------------------
// MOC

#include "moc_qdjview.inc"



/* -------------------------------------------------------------
   Local Variables:
   c++-font-lock-extra-types: ( "\\sw+_t" "[A-Z]\\sw*[a-z]\\sw*" )
   End:
   ------------------------------------------------------------- */
