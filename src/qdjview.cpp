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
#include <QMainWindow>
#include <QMenuBar>
#include <QStatusBar>
#include <QToolBar>
#include <QAction>
#include <QMenu>
#include <QString>
#include <QList>
#include <QMessageBox>
#include <QScrollBar>
#include <QPalette>

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
public slots:
  void error(QString, QString, int);
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
  ui.iconLabel->setPixmap(QMessageBox::standardIcon(QMessageBox::Warning));
  QPalette palette = ui.textEdit->palette();
  palette.setColor(QPalette::Base, palette.color(QPalette::Window));
  ui.textEdit->setPalette(palette);
}

void 
QDjViewDialogError::error(QString message, QString, int)
{
  const int maxSize = 6;
  messages.prepend("<li>" + message + "</li>");
  if (messages.size() > maxSize)
    messages.removeLast();
  QString html;
  for (int i=0; i<messages.size(); i++)
    html = messages[i] + html;
  html = "<html><ul>" + html + "</ul></html>";
  ui.textEdit->setHtml(html);
  QScrollBar *scrollBar = ui.textEdit->verticalScrollBar();
  scrollBar->setValue(scrollBar->maximum());
}

void 
QDjViewDialogError::okay()
{
  messages.clear();
  accept();
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
  widget = new QDjVuWidget(this);
  errorDialog = new QDjViewDialogError(this);


  connect(widget, SIGNAL(errorCondition(int)),
          this, SLOT(raiseErrorDialog()) );

  setCentralWidget(widget);
  setWindowTitle("DjView");
}



void 
QDjView::closeDocument()
{
  widget->setDocument(0);
  document = 0;
}

void 
QDjView::open(QDjVuDocument *document, bool own)
{
  closeDocument();
  document = document;
  widget->setDocument(document, own);
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
      raiseErrorDialog(QString(tr("Cannot open '%1'").arg(filename)));
      return false;
    }
  setWindowTitle(QString("Djview - %1").arg(filename));
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
      raiseErrorDialog(QString(tr("Cannot open '%1'").arg(url.toString())));
      return false;
    }
  setWindowTitle(QString("Djview - %1").arg(url.toString()));
  open(doc, true);
  return true;
}

void
QDjView::raiseErrorDialog(QString message)
{
  if (! message.isEmpty())
    errorDialog->error(message,__FILE__,__LINE__);
  errorDialog->show();
  errorDialog->raise();
}

int
QDjView::execErrorDialog(QString message)
{
  if (! message.isEmpty())
    errorDialog->error(message,__FILE__,__LINE__);
  return errorDialog->exec();
}


// ----------------------------------------
// MOC

#include "moc_qdjview.inc"



/* -------------------------------------------------------------
   Local Variables:
   c++-font-lock-extra-types: ( "\\sw+_t" "[A-Z]\\sw*[a-z]\\sw*" )
   End:
   ------------------------------------------------------------- */
