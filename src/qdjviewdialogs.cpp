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

#include <QApplication>
#include <QDialog>
#include <QFont>
#include <QHeaderView>
#include <QList>
#include <QMessageBox>
#include <QObject>
#include <QRegExp>
#include <QScrollBar>
#include <QString>
#include <QTableWidget>
#include <QTableWidgetItem>

#include <libdjvu/miniexp.h>
#include <libdjvu/ddjvuapi.h>

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




// ----------- QDJVIEWINFODIALOG

#include "ui_qdjviewinfodialog.h"

struct QDjViewInfoDialog::Private {
  Ui::QDjViewInfoDialog ui;
  QDjView *djview;
  QDjVuDocument *document;
  QDjVuPage *page;
  QList<ddjvu_fileinfo_t> files;
  int fileno;
  int pageno;
  bool done;
};


QDjViewInfoDialog::~QDjViewInfoDialog()
{
  delete d->page;
  delete d;
}


QDjViewInfoDialog::QDjViewInfoDialog(QDjView *parent)
  : QDialog(parent), d(new Private)
{
  d->djview = parent;
  d->document = 0;
  d->page = 0;
  d->fileno = 0;
  d->pageno = -1;
  d->done = false;
  
  d->ui.setupUi(this);
  
  QFont font = d->ui.fileText->font();
  font.setFixedPitch(true);
  font.setFamily("monospace");
  font.setPointSize((font.pointSize() * 5 + 5) / 6);
  d->ui.fileText->setFont(font);
  d->ui.fileText->viewport()->setBackgroundRole(QPalette::Background);
  
  QStringList labels;
  d->ui.docTable->setColumnCount(5);
  labels << tr("") << tr("Name") << tr("Size") << tr("Type") << tr("Title");
  d->ui.docTable->setHorizontalHeaderLabels(labels);
  d->ui.docTable->horizontalHeader()->setStretchLastSection(true);
  d->ui.docTable->verticalHeader()->hide();

  d->ui.docLabel->setMinimumHeight(d->ui.fileLabel->height());

  connect(d->djview, SIGNAL(documentClosed()), 
          this, SLOT(documentClosed()));
  connect(d->ui.okButton, SIGNAL(clicked()), 
          this, SLOT(accept()) );
  connect(d->ui.nextFileButton, SIGNAL(clicked()), 
          this, SLOT(nextFile()) );
  connect(d->ui.prevFileButton, SIGNAL(clicked()), 
          this, SLOT(prevFile()) );
  connect(d->ui.docTable, SIGNAL(itemSelectionChanged()), 
          this, SLOT(selectFile()) );

  refreshDocument();
  refreshFile();
}

void 
QDjViewInfoDialog::refreshDocument()
{
  // synchronize document
  if (! d->document)
    {
      QDjVuDocument *doc;
      doc = d->djview->getDjVuWidget()->document();
      if (! doc)
        return;
      d->document = doc;
      connect(doc, SIGNAL(docinfo()),
              this, SLOT(refreshDocument()) );
      connect(doc, SIGNAL(pageinfo()),
              this, SLOT(refreshFile()) );
    }
  // synchronize file list
  if (! d->files.size())
    {
      QDjVuDocument *doc = d->document;
      if (ddjvu_document_decoding_status(*doc) != DDJVU_JOB_OK)
        return;
#if DDJVUAPI_VERSION >= 18
      int filenum = ddjvu_document_get_filenum(*doc);
      for (int i=0; i<filenum; i++)
        {
          ddjvu_fileinfo_t info;
          ddjvu_document_get_fileinfo(*doc, i, &info);
          d->files << info;
        }
#else
      int filenum;
      ddjvu_document_type_t docType = ddjvu_document_get_type(*doc);
      if (docType == DDJVU_DOCTYPE_BUNDLED ||
          docType == DDJVU_DOCTYPE_INDIRECT )
        filenum = ddjvu_document_get_filenum(*doc);
      else
        filenum = ddjvu_document_get_pagenum(*doc);
      for (int i=0; i<filenum; i++)
        {
          ddjvu_fileinfo_t info;
          info.type = 'P';
          info.pageno = i;
          info.size = -1;
          info.id = info.title = info.name = 0;
          if (docType == DDJVU_DOCTYPE_BUNDLED ||
              docType == DDJVU_DOCTYPE_INDIRECT )
            ddjvu_document_get_fileinfo(*doc, i, &info);
          d->files << info;
        }
#endif
      fillDocLabel();
      fillDocTable();
      if (d->pageno >= 0)
        setPage(d->pageno);
      else if (d->fileno >= 0)
        setFile(d->fileno);
      d->done = false;
      refreshFile();
    }
}

void 
QDjViewInfoDialog::refreshFile()
{
  if (d->done)
    return;
  
  d->done = true;
  if (d->fileno < 0 || d->fileno >= d->files.size())
    return;
  
  QDjVuDocument *doc = d->document;
  ddjvu_fileinfo_t &info = d->files[d->fileno];
  QString tab = tr("File #%1").arg(d->fileno+1);
  QString msg, dump;
  
  // prepare message
  if (info.type == 'P')
    {
      if (info.title && info.name && strcmp(info.title, info.name))
        msg = tr("Page #%1 named ' %2 '.")
          .arg(info.pageno + 1)
          .arg(QString::fromUtf8(info.title));
      else
        msg = tr("Page #%1.")
          .arg(info.pageno + 1);
    }
  else if (info.type == 'T')
    msg = tr("Thumbnails.");
  else if (info.type == 'S')
    msg = tr("Shared annotations.");
  else
    msg = tr("Shared data.");
  
  // file dump
  dump = tr("Waiting for data...");
#if DDJVUAPI_VERSION >= 18
  char *s = ddjvu_document_get_filedump(*doc, d->fileno);
  if (! s)
    d->done = false;
  else
    {
      dump = QString::fromUtf8(s);
      free(s);
    }
#else
  if (info.type != 'P')
    dump = tr("File dumps not available with ddjvuapi<18");
  else
    {
      if (! d->page)
        {
          d->page = new QDjVuPage(doc, info.pageno, this);
          connect(d->page, SIGNAL(pageinfo()),
                  this, SLOT(refreshFile()) );
        }
      if (!ddjvu_page_decoding_done(*d->page))
        d->done = false;
      else
        {
          char *s = ddjvu_page_get_long_description(*d->page);
          if (s)
            {
              QStringList d = QString::fromUtf8(s).split("\n");
              QStringList f;
              foreach(QString z, d)
                if (z.contains(QRegExp("^\\s*[0-9]")))
                  f << z;
              dump = f.join("\n");
              free(s);
            }
        }
    }
#endif

  // fill ui
  d->ui.tabWidget->setTabText(1, tab);
  d->ui.fileText->setPlainText(dump);
  d->ui.fileLabel->setText(msg);
  d->ui.prevFileButton->setEnabled(d->fileno - 1 >= 0);
  d->ui.nextFileButton->setEnabled(d->fileno + 1 < d->files.size());
}

void 
QDjViewInfoDialog::setPage(int pageno)
{
  if (d->document && d->files.size())
    {
      int i;
      for (i=0; i<d->files.size(); i++)
        if (d->files[i].pageno == pageno && 
            d->files[i].type == 'P' )
          break;
      if (i < d->files.size())
        setFile(i);
    }
  else
    {
      d->fileno = -1;
      d->pageno = pageno;
    }
}

void 
QDjViewInfoDialog::setFile(int fileno)
{
  if (d->document && d->files.size())
    {
      delete d->page;
      d->page = 0;
      d->fileno = qBound(0, fileno, d->files.size()-1);
      d->pageno = d->files[fileno].pageno;
      d->done = false;
      d->ui.docTable->selectRow(fileno);
      refreshFile();
    }
  else
    {
      d->pageno = -1;
      d->fileno = fileno;
    }
}

void 
QDjViewInfoDialog::prevFile()
{
  if (d->document && d->files.size())
    if (d->fileno - 1 >= 0)
      setFile(d->fileno - 1);
}

void 
QDjViewInfoDialog::nextFile()
{
  if (d->document && d->files.size())
    if (d->fileno + 1 < d->files.size())
      setFile(d->fileno + 1);
}

void 
QDjViewInfoDialog::selectFile()
{
  if (d->document && d->files.size())
    {
      int row = d->ui.docTable->currentRow();
      if (row >= 0 && row < d->files.size())
        setFile(row);
    }
}

void 
QDjViewInfoDialog::documentClosed()
{
  hide();
  if (d->document)
    disconnect(d->document, 0, this, 0);
  if (d->page)
    delete d->page;
  d->files.clear();
  d->document = 0;
  d->page = 0;
}

void 
QDjViewInfoDialog::fillDocLabel()
{
  if (d->document && d->files.size())
    {
      QStringList msg;
      QDjVuDocument *doc = d->document;
      ddjvu_document_type_t docType = ddjvu_document_get_type(*doc);
      if (docType == DDJVU_DOCTYPE_SINGLEPAGE)
        msg << tr("DjVu single page");
      else 
        {
          if (docType == DDJVU_DOCTYPE_BUNDLED)
            msg << tr("DjVu bundled document");
          else if (docType == DDJVU_DOCTYPE_INDIRECT)
            msg << tr("DjVu indirect document");
          else if (docType == DDJVU_DOCTYPE_OLD_BUNDLED)
            msg << tr("Obsolete DjVu bundled document");
          else if (docType == DDJVU_DOCTYPE_OLD_INDEXED)
            msg << tr("Obsolete DjVu indexed document");
          
          int pagenum = ddjvu_document_get_pagenum(*doc);
#if DDJVUAPI_VERSION < 18
          int filenum = pagenum;
          if (docType == DDJVU_DOCTYPE_BUNDLED ||
              docType == DDJVU_DOCTYPE_INDIRECT )
            filenum = ddjvu_document_get_filenum(*doc);
#else
          int filenum = ddjvu_document_get_filenum(*doc);
#endif
          msg << tr("%1 files").arg(filenum);
          msg << tr("%1 pages").arg(pagenum);
        }
      d->ui.docLabel->setText(msg.join(", ") + ".");
    }
}

void 
QDjViewInfoDialog::fillDocTable()
{
}






/* -------------------------------------------------------------
   Local Variables:
   c++-font-lock-extra-types: ( "\\sw+_t" "[A-Z]\\sw*[a-z]\\sw*" )
   End:
   ------------------------------------------------------------- */


