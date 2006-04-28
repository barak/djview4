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



#include <QObject>
#include <QApplication>
#include <QMessageBox>
#include <QScrollBar>
#include <QDialog>
#include <QString>
#include <QList>

#include "qdjviewdialogs.h"


// ----------- QDJVIEWDIALOGERROR

#include "ui_qdjviewdialogerror.h"

struct QDjViewDialogError::Private {
  Ui::QDjViewDialogError ui;
  QList<QString> messages;
};

QDjViewDialogError::~QDjViewDialogError()
{
  delete d;
}

QDjViewDialogError::QDjViewDialogError(QWidget *parent)
  : QDialog(parent), 
    d(new Private)
{
  d->ui.setupUi(this);
  connect(d->ui.okButton, SIGNAL(clicked()), this, SLOT(okay()));
  d->ui.textEdit->viewport()->setBackgroundRole(QPalette::Window);
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
  if (!d->messages.isEmpty() && message == d->messages[0]) 
    return;
  // Add message
  d->messages.prepend(message);
  if (d->messages.size() >= 16)
    d->messages.removeLast();
  // Show message
  QString html;
  for (int i=0; i<d->messages.size(); i++)
    html = "<li>" + d->messages[i] + "</li>" + html;
  html = "<html><ul>" + html + "</ul></html>";
  d->ui.textEdit->setHtml(html);
  QScrollBar *scrollBar = d->ui.textEdit->verticalScrollBar();
  scrollBar->setValue(scrollBar->maximum());
}

void 
QDjViewDialogError::prepare(QMessageBox::Icon icon, QString caption)
{
  if (icon != QMessageBox::NoIcon)
    d->ui.iconLabel->setPixmap(QMessageBox::standardIcon(icon));
  if (!caption.isEmpty())
    setWindowTitle(caption);
}

void 
QDjViewDialogError::okay()
{
  d->messages.clear();
  accept();
  hide();
}


/* -------------------------------------------------------------
   Local Variables:
   c++-font-lock-extra-types: ( "\\sw+_t" "[A-Z]\\sw*[a-z]\\sw*" )
   End:
   ------------------------------------------------------------- */


