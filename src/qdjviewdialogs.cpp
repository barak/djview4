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

#include <QDebug>

#include <QObject>
#include <QApplication>
#include <QFont>
#include <QHeaderView>
#include <QMessageBox>
#include <QScrollBar>
#include <QDialog>
#include <QString>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QList>

#include "qdjviewdialogs.h"
#include "qdjvuwidget.h"
#include "qdjview.h"




// ----------- QDJVIEWERRORDIALOG

#include "ui_qdjviewerrordialog.h"

struct QDjViewErrorDialog::Private {
  Ui::QDjViewErrorDialog ui;
  QList<QString> messages;
  void compose();
};

QDjViewErrorDialog::~QDjViewErrorDialog()
{
  delete d;
}

QDjViewErrorDialog::QDjViewErrorDialog(QWidget *parent)
  : QDialog(parent), 
    d(new Private)
{
  d->ui.setupUi(this);
  connect(d->ui.okButton, SIGNAL(clicked()), this, SLOT(okay()));
  d->ui.textEdit->viewport()->setBackgroundRole(QPalette::Background);
  setWindowTitle(tr("DjView Error"));
}


void
QDjViewErrorDialog::compose()
{
  QString html;
  for (int i=0; i<d->messages.size(); i++)
    html = "<li>" + d->messages[i] + "</li>" + html;
  d->ui.textEdit->setHtml("<html><ul>" + html + "</ul></html>");
  QScrollBar *scrollBar = d->ui.textEdit->verticalScrollBar();
  scrollBar->setValue(scrollBar->maximum());
}

void 
QDjViewErrorDialog::error(QString msg, QString, int)
{
  // Remove [1-nnnnn] prefix from djvulibre-3.5
  if (msg.startsWith("["))
    msg = msg.replace(QRegExp("^\\[\\d*-?\\d*\\]\\s*") , "");
  // Ignore empty and duplicate messages
  if (msg.isEmpty()) return;
  if (!d->messages.isEmpty() && msg == d->messages[0]) return;
  // Add message
  d->messages.prepend(msg);
  while (d->messages.size() >= 16)
    d->messages.removeLast();
  compose();
}

void 
QDjViewErrorDialog::prepare(QMessageBox::Icon icon, QString caption)
{
  if (icon != QMessageBox::NoIcon)
    d->ui.iconLabel->setPixmap(QMessageBox::standardIcon(icon));
  if (!caption.isEmpty())
    setWindowTitle(caption);
}

void 
QDjViewErrorDialog::okay()
{
  d->messages.clear();
  accept();
  hide();
}




// ----------- QDJVIEWERRORDIALOG

#include "ui_qdjviewinfodialog.h"

struct QDjViewInfoDialog::Private {
  Ui::QDjViewInfoDialog ui;
  QDjView *djview;
  QDjVuDocument *document;
  int pageno;
  bool done;


};


QDjViewInfoDialog::~QDjViewInfoDialog()
{
  delete d;
}


QDjViewInfoDialog::QDjViewInfoDialog(QDjView *parent)
  : QDialog(parent), d(new Private)
{
  d->djview = parent;
  d->document = 0;
  d->pageno = -1;
  d->done = false;
  d->ui.setupUi(this);

  QFont font = d->ui.textEdit->font();
  font.setFixedPitch(true);
  font.setFamily("monospace");
  font.setPointSize((font.pointSize() * 5 + 5) / 6);
  d->ui.textEdit->setFont(font);
  d->ui.textEdit->viewport()->setBackgroundRole(QPalette::Background);
  QStringList labels = QStringList() << tr("Name") << tr("Value");
  d->ui.tableWidget->setHorizontalHeaderLabels(labels);
  d->ui.tableWidget->horizontalHeader()->setStretchLastSection(true);

  connect(d->djview, SIGNAL(documentClosed()), this, SLOT(documentClosed()));
  connect(d->ui.okButton, SIGNAL(clicked()), this, SLOT(accept()));
  connect(d->ui.thisPageButton, SIGNAL(clicked()), this, SLOT(toCurrentPage()));
  connect(d->ui.documentButton, SIGNAL(clicked()), this, SLOT(toDocument()));
  connect(d->ui.pageCombo, SIGNAL(activated(int)), this, SLOT(toPage(int)));
}


void
QDjViewInfoDialog::toCurrentPage()
{
  if (! d->document)
    return;
  d->pageno = d->djview->getDjVuWidget()->page();
  d->ui.pageCombo->setCurrentIndex(d->ui.pageCombo->findData(d->pageno));
  d->done = false;
  refresh();
}


void
QDjViewInfoDialog::toDocument()
{
  if (! d->document)
    return;
  d->pageno = -1;
  d->ui.pageCombo->setCurrentIndex(0);
  d->done = false;
  refresh();
}


void
QDjViewInfoDialog::toPage(int index)
{
  if (! d->document)
    return;
  d->pageno = d->ui.pageCombo->itemData(index).toInt();
  d->done = false;
  refresh();
}


void
QDjViewInfoDialog::documentClosed()
{
  hide();
  if (! d->document)
    return;
  disconnect(d->document, 0, this, 0);
  d->document = 0;
}


QString 
QDjViewInfoDialog::pageEncodingMessage(int pageno)
{
  char *utf8 = 0;
  QString message;
#if DDJVUAPI_VERSION < 18
  QDjVuWidget *widget = d->djview->getDjVuWidget();
  QDjVuPage *page = widget->getDjVuPage(pageno);
  if (page)
    utf8 = ddjvu_page_get_long_description(*page);
  message = tr("DDJVUAPI_VERSION>=18 is required.\n"
               "Previous version can only give information\n"
               "for pages currently displayed or cached.");
#else
  utf8 = ddjvu_document_get_pagedump(*d->document, pageno);
  message = tr("Information is not available.");
#endif
  if (utf8)
    message = QString::fromUtf8(utf8);
  if (utf8)
      free(utf8);
  return message;
}
  

QString 
QDjViewInfoDialog::documentEncodingMessage()
{
  QDjVuDocument *document = d->document;
  QString message;
  // -- document name
  QString name = d->djview->getShortFileName();
  if (! name.isEmpty())
    message += tr("Document name:    %1\n")
      .arg(d->djview->getShortFileName());
  // -- document format
  ddjvu_document_type_t docType = ddjvu_document_get_type(*document);
  QString format;
  if (docType == DDJVU_DOCTYPE_SINGLEPAGE)
    format = tr("SINGLE PAGE");
  else if (docType == DDJVU_DOCTYPE_BUNDLED)
    format = tr("BUNDLED");
  else if (docType == DDJVU_DOCTYPE_INDIRECT)
    format = tr("INDIRECT");
  else if (docType == DDJVU_DOCTYPE_OLD_BUNDLED)
    format = tr("OLD BUNDLED (obsolete format)");
  else if (docType == DDJVU_DOCTYPE_OLD_BUNDLED)
    format = tr("OLD BUNDLED (obsolete format)");
  if (! format.isEmpty())
    message += tr("Document format:  %1\n")
      .arg(format);
  // -- document number of files and pages
  int size = 0;
  bool okay = true;
  int npages = d->djview->pageNum();
#if DDJVUAPI_VERSION < 18
  int nfiles = npages;
  if (docType == DDJVU_DOCTYPE_BUNDLED ||
      docType != DDJVU_DOCTYPE_INDIRECT )
    nfiles = ddjvu_document_get_filenum(*d->document);
  else
    okay = false;
#else
  int nfiles = ddjvu_document_get_filenum(*d->document);
#endif
  // -- add size of all files
  for (int i=0; i<nfiles && okay; i++)
    {
      ddjvu_fileinfo_t info;
      if (ddjvu_document_get_fileinfo(*document, i, &info) == DDJVU_JOB_OK 
          && info.size > 0)
        {
          size += info.size;
          if (info.size & 1)
            size += 1;
        }
      else
        okay = false;
    }
#if DDJVUAPI_VERSION >= 18
  // -- add size of header
  ddjvu_fileinfo_t info;
  if (ddjvu_document_get_fileinfo(*document, -1, &info) == DDJVU_JOB_OK)
    {
      qDebug() << info.size;
      size += info.size;
      if (info.size & 1)
        size += 1;
    }
#endif
  if (okay)
    message += tr("Document size:    %1\n").arg(size);
  message += tr("Number of files:  %1\n").arg(nfiles);
  message += tr("Number of pages:  %1\n").arg(npages);

  // -- finished
  return message;
}



void
QDjViewInfoDialog::refresh()
{
  // Synchronize document
  if (! d->document && d->djview->documentPages.size())
    {
      setWindowTitle(d->djview->makeCaption(tr("DjView Information")));
      d->document = d->djview->getDocument();
      connect(d->document, SIGNAL(pageinfo()), this, SLOT(refresh()));
      d->djview->fillPageCombo(d->ui.pageCombo, tr("page %1"));
      d->ui.pageCombo->insertItem(0, tr("document"), QVariant(-1));
      d->ui.pageCombo->setCurrentIndex(0);
      d->pageno = -1;
    }
  if (! d->document || d->done)
    return;

  // Synchronize displayed contents
  if (d->pageno < 0)
    {
      // ============ Document information
      d->done = true;
      d->ui.tabWidget->setTabText(0, tr("Document &Properties"));
      d->ui.tabWidget->setTabText(1, tr("Document &Encoding"));
      d->ui.textEdit->setPlainText(documentEncodingMessage());


    }
  else
    {
      // ============ Page information
      d->done = true;
      d->ui.tabWidget->setTabText(0, tr("Page &Properties"));
      d->ui.tabWidget->setTabText(1, tr("Page &Encoding"));
      d->ui.textEdit->setPlainText(pageEncodingMessage(d->pageno));
      
      
      
    }
}



/* -------------------------------------------------------------
   Local Variables:
   c++-font-lock-extra-types: ( "\\sw+_t" "[A-Z]\\sw*[a-z]\\sw*" )
   End:
   ------------------------------------------------------------- */


