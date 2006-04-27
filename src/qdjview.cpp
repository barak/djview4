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

#include <QObject>
#include <QCoreApplication>
#include <QApplication>
#include <QMainWindow>
#include <QMenuBar>
#include <QStatusBar>
#include <QStackedWidget>
#include <QLabel>
#include <QToolBar>
#include <QAction>
#include <QIcon>
#include <QMenu>
#include <QString>
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
  QPalette palette = ui.textEdit->palette();
  palette.setColor(QPalette::Base, palette.color(QPalette::Window));
  ui.textEdit->setPalette(palette);
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




// ----------------------------------------
// QDJVIEW


QDjView::~QDjView()
{
}

QDjView::QDjView(QDjVuContext &context, ViewerMode mode, QWidget *parent)
  : QMainWindow(parent),
    djvuContext(context),
    viewerMode(mode),
    document(0)
{
  central = new QStackedWidget(this);
  widget = new QDjVuWidget(central);
  splash = new QLabel(central);
  splash->setPixmap(QPixmap(":/images/splash.png"));
  central->addWidget(widget);
  central->addWidget(splash);
  central->setCurrentWidget(splash);
  setCentralWidget(central);

  errorDialog = new QDjViewDialogError(this);
  
  connect(widget, SIGNAL(errorCondition(int)),
          this, SLOT(errorCondition(int)));
  
  setWindowTitle(tr("DjView"));
  setWindowIcon(QIcon(":/images/icon32_djvu.png"));
  if (QApplication::windowIcon().isNull())
    QApplication::setWindowIcon(windowIcon());
}

QDjVuWidget *
QDjView::djvuWidget()
{
  return widget;
}

void 
QDjView::closeDocument()
{
  central->setCurrentWidget(splash);
  widget->setDocument(0);
  document = 0;
}

void 
QDjView::open(QDjVuDocument *document, bool own)
{
  closeDocument();
  document = document;
  widget->setDocument(document, own);
  central->setCurrentWidget(widget);
}

bool
QDjView::open(QString filename)
{
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
  return true;
}

bool
QDjView::open(QUrl url)
{
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
  return true;
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
