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

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QDebug>
#include <QDir>
#include <QDialog>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QHeaderView>
#include <QKeySequence>
#include <QList>
#include <QMap>
#include <QMessageBox>
#include <QObject>
#include <QPainter>
#include <QPrinter>
#include <QRegExp>
#include <QScrollBar>
#include <QShortcut>
#include <QString>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QtAlgorithms>

#include <libdjvu/miniexp.h>
#include <libdjvu/ddjvuapi.h>

#include "qdjview.h"
#include "qdjviewdialogs.h"
#include "qdjvuwidget.h"
#include "qdjvu.h"


#if DDJVUAPI_VERSION < 18
# error "DDJVUAPI_VERSION>=18 is required !"
#endif


// =======================================
// QDJVIEWERRORDIALOG
// =======================================



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
  connect(d->ui.okButton, SIGNAL(clicked()), this, SLOT(accept()));
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
QDjViewErrorDialog::done(int result)
{
  emit closing();
  d->messages.clear();
  QDialog::done(result);
}




// =======================================
// QDJVIEWINFODIALOG
// =======================================


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
  d->ui.docTable->setColumnCount(6);
  labels << tr("File #") << tr("File Name") 
         << tr("File Size") << tr("File Type") 
         << tr("Page #") << tr("Page Title");
  d->ui.docTable->setHorizontalHeaderLabels(labels);
  d->ui.docTable->verticalHeader()->hide();
  d->ui.docTable->horizontalHeader()->setHighlightSections(false);
  d->ui.docTable->horizontalHeader()->setStretchLastSection(true);

  connect(d->djview, SIGNAL(documentClosed(QDjVuDocument*)), 
          this, SLOT(clear()));
  connect(d->djview, SIGNAL(documentReady(QDjVuDocument*)), 
          this, SLOT(refresh()));
  connect(d->djview->getDjVuWidget(), SIGNAL(pageChanged(int)),
          this, SLOT(setPage(int)));
  connect(d->ui.okButton, SIGNAL(clicked()), 
          this, SLOT(accept()) );
  connect(d->ui.nextButton, SIGNAL(clicked()), 
          this, SLOT(nextFile()) );
  connect(d->ui.jumpButton, SIGNAL(clicked()), 
          this, SLOT(jumpToSelectedPage()) );
  connect(d->ui.fileCombo, SIGNAL(activated(int)), 
          this, SLOT(setFile(int)) );
  connect(d->ui.prevButton, SIGNAL(clicked()), 
          this, SLOT(prevFile()) );
  connect(d->ui.docTable, SIGNAL(currentCellChanged(int,int,int,int)), 
          this, SLOT(setFile(int)) );
  connect(d->ui.docTable, SIGNAL(itemDoubleClicked(QTableWidgetItem*)),
          this, SLOT(jumpToSelectedPage()) );
  
  d->ui.fileCombo->setEnabled(false);
  d->ui.jumpButton->setEnabled(false);
  d->ui.prevButton->setEnabled(false);
  d->ui.nextButton->setEnabled(false);

  // what's this
  QWidget *wd = d->ui.tabDocument;
  wd->setWhatsThis(tr("<html><b>Document information</b><br>"
                      "This panel shows information about the document and "
                      "its component files. Select a component file "
                      "to display detailled information in the 'File' "
                      "tab. Double click a component file to show "
                      "the corresponding page in the main window."
                      "</html>"));
  QWidget *wf = d->ui.tabFile;
  wf->setWhatsThis(tr("<html><b>File and page information</b><br>"
                      "This panel shows the structure of the DjVu data "
                      "corresponding to the component file or the page "
                      "selected in the 'Document' tab. The arrow buttons "
                      "let you navigate to the previous or next "
                      "component file.</html>"));
}

void 
QDjViewInfoDialog::refresh()
{
  // document known
  if (! d->document)
    {
      QDjVuDocument *doc;
      doc = d->djview->getDjVuWidget()->document();
      if (! doc)
        return;
      d->document = doc;
      connect(doc, SIGNAL(pageinfo()),
              this, SLOT(refresh()) );
    }
  // document ready
  if (! d->files.size())
    {
      QDjVuDocument *doc = d->document;
      if (ddjvu_document_decoding_status(*doc) != DDJVU_JOB_OK)
        return;
      int filenum = ddjvu_document_get_filenum(*doc);
      for (int i=0; i<filenum; i++)
        {
          ddjvu_fileinfo_t info;
          ddjvu_document_get_fileinfo(*doc, i, &info);
          d->files << info;
        }
      fillFileCombo();
      fillDocLabel();
      fillDocTable();
      if (d->pageno >= 0)
        setPage(d->pageno);
      else if (d->fileno >= 0)
        setFile(d->fileno);
      d->ui.fileCombo->setEnabled(true);
      d->done = false;
    }
  
  // file ready
  if (! d->done)
    {
      QDjVuDocument *doc = d->document;
      ddjvu_fileinfo_t &info = d->files[d->fileno];
      d->done = true;

      // file dump
      QString dump = tr("Waiting for data...");
      char *s = ddjvu_document_get_filedump(*doc, d->fileno);
      if (! s)
        d->done = false;
      else
        {
          dump = QString::fromUtf8(s);
          free(s);
        }
      d->ui.fileText->setPlainText(dump);
      d->ui.prevButton->setEnabled(d->fileno - 1 >= 0);
      d->ui.nextButton->setEnabled(d->fileno + 1 < d->files.size());
      d->ui.jumpButton->setEnabled(info.type == 'P');
      d->ui.fileCombo->setCurrentIndex(d->fileno);
      QTableWidget *table = d->ui.docTable;
      int rows = table->rowCount();
      int cols = table->columnCount();
      QTableWidgetSelectionRange all(0,0,rows-1,cols-1);
      table->setRangeSelected(all, false);
      table->selectRow(d->fileno);
    }
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
      if (fileno >= 0 && 
          fileno < d->files.size() &&
          fileno != d->fileno )
        {
          delete d->page;
          d->page = 0;
          d->fileno = fileno;
          d->pageno = d->files[fileno].pageno;
          d->done = false;
          refresh();
        }
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
QDjViewInfoDialog::jumpToSelectedPage()
{
  if (d->document && d->files.size())
    {
      ddjvu_fileinfo_t &info = d->files[d->fileno];
      if (info.type == 'P')
        d->djview->goToPage(info.pageno);
    }
}

void 
QDjViewInfoDialog::clear()
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
QDjViewInfoDialog::fillFileCombo()
{
  QComboBox *fileCombo = d->ui.fileCombo;
  fileCombo->clear();
  for (int fileno=0; fileno<d->files.size(); fileno++)
    {
      ddjvu_fileinfo_t &info = d->files[fileno];
      QString msg;
      if (info.type == 'P')
        {
          if (info.title && info.name && strcmp(info.title, info.name))
            msg = tr("Page #%1 - \253 %2 \273") // << .. >>
              .arg(info.pageno + 1).arg(QString::fromUtf8(info.title));
          else
            msg = tr("Page #%1").arg(info.pageno + 1);
        }
      else if (info.type == 'T')
        msg = tr("Thumbnails");
      else if (info.type == 'S')
        msg = tr("Shared annotations");
      else
        msg = tr("Shared data");
      fileCombo->addItem(tr("File #%1 - ").arg(fileno+1) + msg);
    }
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
        msg << tr("Single DjVu page");
      else 
        {
          if (docType == DDJVU_DOCTYPE_BUNDLED)
            msg << tr("Bundled DjVu document");
          else if (docType == DDJVU_DOCTYPE_INDIRECT)
            msg << tr("Indirect DjVu document");
          else if (docType == DDJVU_DOCTYPE_OLD_BUNDLED)
            msg << tr("Obsolete bundled DjVu document");
          else if (docType == DDJVU_DOCTYPE_OLD_INDEXED)
            msg << tr("Obsolete indexed DjVu document");
          
          int pagenum = ddjvu_document_get_pagenum(*doc);
          int filenum = ddjvu_document_get_filenum(*doc);
          msg << tr("%1 files").arg(filenum);
          msg << tr("%1 pages").arg(pagenum);
        }
      d->ui.docLabel->setText(msg.join(" - "));
    }
}

static void
setTableWhatsThis(QTableWidget *table, QString text)
{
  for (int i=0; i<table->rowCount(); i++)
    for (int j=0; j<table->columnCount(); j++)
      table->item(i,j)->setWhatsThis(text);
}


void 
QDjViewInfoDialog::fillDocTable()
{
  int filenum = d->files.size();
  QTableWidget *table = d->ui.docTable;
  table->setRowCount(filenum);
  for (int i=0; i<filenum; i++)
    fillDocRow(i);
#if QT_VERSION >= 0x040100
  table->resizeColumnsToContents();
  table->resizeRowsToContents();
#endif
  table->horizontalHeader()->setStretchLastSection(true);
  setTableWhatsThis(table, d->ui.tabDocument->whatsThis());
}

void 
QDjViewInfoDialog::fillDocRow(int i)
{
  ddjvu_fileinfo_t &info = d->files[i];
  QTableWidget *table = d->ui.docTable;

  QTableWidgetItem *numItem = new QTableWidgetItem();
  numItem->setText(QString(" %1 ").arg(i+1));
  numItem->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
  numItem->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
  table->setItem(i, 0, numItem);

  QTableWidgetItem *nameItem = new QTableWidgetItem();
  QString name = (info.name) ? QString::fromUtf8(info.name) : tr("n/a");
  nameItem->setText(QString(" %1 ").arg(name));
  nameItem->setTextAlignment(Qt::AlignLeft|Qt::AlignVCenter);
  nameItem->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
  table->setItem(i, 1, nameItem);

  QTableWidgetItem *sizeItem = new QTableWidgetItem();
  sizeItem->setText((info.size>0) ? QString(" %1 ").arg(info.size) : tr("n/a"));
  sizeItem->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
  sizeItem->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
  table->setItem(i, 2, sizeItem);

  QTableWidgetItem *typeItem = new QTableWidgetItem();
  if (info.type == 'P')
    typeItem->setText(tr(" Page "));
  else if (info.type == 'T')
    typeItem->setText(tr(" Thumbnails "));
  else
    typeItem->setText(tr(" Shared "));
  typeItem->setTextAlignment(Qt::AlignLeft|Qt::AlignVCenter);
  typeItem->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
  table->setItem(i, 3, typeItem);
  
  QTableWidgetItem *pnumItem = new QTableWidgetItem();
  if (info.type == 'P')
    pnumItem->setText(QString(" %1 ").arg(info.pageno+1));
  pnumItem->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
  pnumItem->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
  table->setItem(i, 4, pnumItem);
  
  QTableWidgetItem *titleItem = new QTableWidgetItem();
  if (info.type == 'P')
    {
      QString title = d->djview->pageName(info.pageno, true);
      titleItem->setText(QString(" %1 ").arg(title));
    }
  titleItem->setTextAlignment(Qt::AlignLeft|Qt::AlignVCenter);
  titleItem->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
  table->setItem(i, 5, titleItem);
}





// =======================================
// QDJVIEWMETADIALOG
// =======================================


#include "ui_qdjviewmetadialog.h"

struct QDjViewMetaDialog::Private {
  Ui::QDjViewMetaDialog ui;
  QDjView *djview;
  QDjVuDocument *document;
  int pageno;
  minivar_t docAnno;
  minivar_t pageAnno;
};


QDjViewMetaDialog::~QDjViewMetaDialog()
{
  delete d;
}


QDjViewMetaDialog::QDjViewMetaDialog(QDjView *parent)
  : QDialog(parent), d(new Private)
{
  d->djview = parent;
  d->document = 0;
  d->pageno = 0;
  d->docAnno = miniexp_dummy;
  d->pageAnno = miniexp_dummy;
  d->ui.setupUi(this);
  // make ctrl+c work
  new QShortcut(QKeySequence(tr("Ctrl+C","copy")), this, SLOT(copy()));
  // tweaks
  QStringList labels;
  labels << tr(" Key ") << tr(" Value ");
  d->ui.docTable->setColumnCount(2);
  d->ui.docTable->setHorizontalHeaderLabels(labels);
  d->ui.docTable->horizontalHeader()->setHighlightSections(false);
  d->ui.docTable->horizontalHeader()->setStretchLastSection(true);
  d->ui.docTable->verticalHeader()->hide();
  d->ui.pageTable->setColumnCount(2);
  d->ui.pageTable->setHorizontalHeaderLabels(labels);
  d->ui.pageTable->horizontalHeader()->setHighlightSections(false);
  d->ui.pageTable->horizontalHeader()->setStretchLastSection(true);
  d->ui.pageTable->verticalHeader()->hide();
  d->ui.pageCombo->setEnabled(false);
  d->ui.jumpButton->setEnabled(false);
  d->ui.prevButton->setEnabled(false);
  d->ui.nextButton->setEnabled(false);
  // connections
  connect(d->djview, SIGNAL(documentClosed(QDjVuDocument*)),
          this, SLOT(clear()) );
  connect(d->djview, SIGNAL(documentReady(QDjVuDocument*)), 
          this, SLOT(refresh()));
  connect(d->djview->getDjVuWidget(), SIGNAL(pageChanged(int)),
          this, SLOT(setPage(int)) );
  connect(d->ui.okButton, SIGNAL(clicked()),
          this, SLOT(accept()) );
  connect(d->ui.jumpButton, SIGNAL(clicked()),
          this, SLOT(jumpToSelectedPage()) );
  connect(d->ui.pageCombo, SIGNAL(activated(int)),
          this, SLOT(setPage(int)) );
  connect(d->ui.prevButton, SIGNAL(clicked()),
          this, SLOT(prevPage()) );
  connect(d->ui.nextButton, SIGNAL(clicked()),
          this, SLOT(nextPage()) );
  connect(d->ui.pageCombo, SIGNAL(activated(int)),
          this, SLOT(setPage(int)) );
  // what's this
  QWidget *wd = d->ui.docTab;
  wd->setWhatsThis(tr("<html><b>Document metadata</b><br>"
                      "This panel displays metadata pertaining "
                      "to the document, such as author, title, "
                      "references, etc. "
                      "This information can be saved into the document "
                      "with program <tt>djvused</tt>: use the commands "
                      "<tt>create-shared-ant</tt> and <tt>set-meta</tt>."
                      "</html>"));
  QWidget *wp = d->ui.pageTab;
  wp->setWhatsThis(tr("<html><b>Page metadata</b><br>"
                      "This panel displays metadata pertaining "
                      "to a specific page. "
                      "Page specific metadata override document metadata. "
                      "This information can be saved into the document "
                      "with program <tt>djvused</tt>: use command "
                      "<tt>select</tt> to select the page and command "
                      "<tt>set-meta</tt> to specify the metadata entries."
                      "</html>"));
}

static QMap<QString,QString>
metadataFromAnnotations(miniexp_t p)
{
  QMap<QString,QString> m;
  miniexp_t s_metadata = miniexp_symbol("metadata");
  while (miniexp_consp(p))
    {
      if (miniexp_caar(p) == s_metadata)
        {
          miniexp_t q = miniexp_cdar(p);
          while (miniexp_consp(q))
            {
              miniexp_t a = miniexp_car(q);
              q = miniexp_cdr(q);
              if (miniexp_consp(a) && 
                  miniexp_symbolp(miniexp_car(a)) &&
                  miniexp_stringp(miniexp_cadr(a)) )
                {
                  QString k;
                  k = QString::fromUtf8(miniexp_to_name(miniexp_car(a)));
                  m[k] = QString::fromUtf8(miniexp_to_str(miniexp_cadr(a)));
                }
            }
        }
      p = miniexp_cdr(p);
    }
  return m;
}

static void
metadataSubtract(QMap<QString,QString> &from, QMap<QString,QString> m)
{
  QMap<QString,QString>::const_iterator i = m.constBegin();
  for( ; i != m.constEnd(); i++)
    if (from.contains(i.key()) && from[i.key()] == i.value())
      from.remove(i.key());
}

static void
metadataFill(QTableWidget *table, QMap<QString,QString> m)
{
  QList<QString> keys;
  QMap<QString,QString>::const_iterator i = m.constBegin();
  for( ; i != m.constEnd(); i++)
    keys << i.key();
  qSort(keys.begin(), keys.end());
  int nkeys = keys.size();
  table->setRowCount(nkeys);
  for(int j = 0; j < nkeys; j++)
    {
      QTableWidgetItem *kitem = new QTableWidgetItem(keys[j]);
      QTableWidgetItem *vitem = new QTableWidgetItem(m[keys[j]]);
      kitem->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
      vitem->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
      table->setItem(j, 0, kitem);
      table->setItem(j, 1, vitem);
    }
#if QT_VERSION >= 0x040100
  table->resizeColumnsToContents();
  table->resizeRowsToContents();
#endif
  table->horizontalHeader()->setStretchLastSection(true);
}

void 
QDjViewMetaDialog::refresh()
{
  // new document?
  if (! d->document)
    {
      QDjVuDocument *doc;
      doc = d->djview->getDjVuWidget()->document();
      if (! doc)
        return;
      d->document = doc;
      connect(doc, SIGNAL(pageinfo()),
              this, SLOT(refresh()) );
    }
  // document ready
  if (! d->ui.pageCombo->count())
    {
      if (! d->djview->pageNum())
        return;
      d->djview->fillPageCombo(d->ui.pageCombo);
      d->ui.pageCombo->setEnabled(true);
      d->ui.jumpButton->setEnabled(true);
    }
  // document annotations known
  if (d->docAnno == miniexp_dummy)
    {
      d->docAnno = d->document->getDocumentAnnotations();
      if (d->docAnno != miniexp_dummy)
        {
          QMap<QString,QString> docMeta = metadataFromAnnotations(d->docAnno);
          metadataFill(d->ui.docTable, docMeta);
          setTableWhatsThis(d->ui.docTable, d->ui.docTab->whatsThis());
        }
    }
  // page annotations
  if (d->ui.pageCombo->count() > 0)
    {
      d->pageno = qBound(0, d->pageno, d->djview->pageNum()-1);
      d->ui.prevButton->setEnabled(d->pageno-1 >= 0);
      d->ui.nextButton->setEnabled(d->pageno+1 < d->djview->pageNum());
      d->ui.pageCombo->setCurrentIndex(d->pageno);
      if (d->document && d->pageAnno == miniexp_dummy)
        {
          d->pageAnno = d->document->getPageAnnotations(d->pageno);
          if (d->pageAnno != miniexp_dummy)
            {
              QMap<QString,QString> docMeta 
                = metadataFromAnnotations(d->docAnno);
              QMap<QString,QString> pageMeta 
                = metadataFromAnnotations(d->pageAnno);
              metadataSubtract(pageMeta, docMeta);
              metadataFill(d->ui.pageTable, pageMeta);
              setTableWhatsThis(d->ui.pageTable, d->ui.pageTab->whatsThis());
            }
        }
    }
}

void 
QDjViewMetaDialog::prevPage()
{
  setPage(qMax(d->pageno - 1, 0));
}

void 
QDjViewMetaDialog::nextPage()
{
  setPage(qMin(d->pageno + 1, d->djview->pageNum() - 1));
}

void 
QDjViewMetaDialog::setPage(int pageno)
{
  if (d->document && pageno != d->pageno)
    {
      d->pageno = pageno;
      d->pageAnno = miniexp_dummy;
      d->ui.pageTable->setRowCount(0);
      refresh();
    }
}

void 
QDjViewMetaDialog::jumpToSelectedPage()
{
  if (d->document &&
      d->pageno >= 0 && d->pageno < d->djview->pageNum() )
    d->djview->goToPage(d->pageno);
}

void 
QDjViewMetaDialog::clear()
{
  hide();
  d->document = 0;
  d->pageno = 0;
  d->docAnno = miniexp_dummy;
  d->pageAnno = miniexp_dummy;
  d->ui.pageCombo->clear();
  d->ui.pageCombo->setEnabled(false);
  d->ui.docTable->setRowCount(0);
  d->ui.pageTable->setRowCount(0);
  d->ui.jumpButton->setEnabled(false);
}

void 
QDjViewMetaDialog::copy()
{
  QTableWidget *table = d->ui.pageTable;
  if (d->ui.tabWidget->currentWidget() == d->ui.docTab)
    table = d->ui.docTable;
  QList<QTableWidgetItem*> selected = table->selectedItems();
  if (selected.size() == 1)
    QApplication::clipboard()->setText(selected[0]->text());
}




// =======================================
// QDJVIEWSAVEDIALOG
// =======================================



#include "ui_qdjviewsavedialog.h"

struct QDjViewSaveDialog::Private {
  Ui::QDjViewSaveDialog ui;
  QDjView *djview;
  QDjVuDocument *document;
  QDjVuJob *job;
  QDjViewErrorDialog *errdialog;
  FILE *output;
};


QDjViewSaveDialog::~QDjViewSaveDialog()
{
  if (d && d->output)
    fclose(d->output);
  delete d;
}


QDjViewSaveDialog::QDjViewSaveDialog(QDjView *parent)
  : QDialog(parent), d(new Private)
{
  d->djview = parent;
  d->document = 0;
  d->job = 0;
  d->errdialog = 0;
  d->output = 0;
  d->ui.setupUi(this);
  connect(d->djview, SIGNAL(documentClosed(QDjVuDocument*)),
          this, SLOT(reject()) );
  connect(d->djview, SIGNAL(documentReady(QDjVuDocument*)),
          this, SLOT(refresh()) );
  connect(d->ui.browseButton, SIGNAL(clicked() ),
          this, SLOT(browse()) );
  connect(d->ui.cancelButton, SIGNAL(clicked() ),
          this, SLOT(reject()) );
  connect(d->ui.saveButton, SIGNAL(clicked() ),
          this, SLOT(save()) );
  connect(d->ui.stopButton, SIGNAL(clicked() ),
          this, SLOT(stop()) );
  connect(d->ui.fromPageCombo, SIGNAL(activated(int)),
          d->ui.pageRangeButton, SLOT(click()) );
  connect(d->ui.toPageCombo, SIGNAL(activated(int)),
          d->ui.pageRangeButton, SLOT(click()) );
  connect(d->ui.fromPageCombo, SIGNAL(activated(int)),
          this, SLOT(refresh()) );
  connect(d->ui.toPageCombo, SIGNAL(activated(int)),
          this, SLOT(refresh()) );
  connect(d->ui.fileNameEdit, SIGNAL(textChanged(QString)),
          this, SLOT(refresh()));
  setWhatsThis(tr("<html><b>Saving DjVu data.</b><br/> "
                  "You can save the whole document or a page range. "
                  "The bundled format conveniently saves all the pages "
                  "into a single file. The indirect format spreads them "
                  "into several files for efficient web serving. "
                  "<br><i>Saving in indirect format does not work yet!</i>"
                  "</html>"));
  refresh();
}


void 
QDjViewSaveDialog::refresh()
{
  int npages = d->djview->pageNum();
  if (!d->document && npages>0)
    {
      d->document = d->djview->getDocument();
      QString fname = d->djview->getDocumentFileName();
      if (fname.isEmpty())
        fname = d->djview->getShortFileName();
      else 
        fname = QFileInfo(fname).absoluteFilePath();
      d->ui.fileNameEdit->setText(fname);
      d->djview->fillPageCombo(d->ui.fromPageCombo);
      d->ui.fromPageCombo->setCurrentIndex(0);
      d->djview->fillPageCombo(d->ui.toPageCombo);
      d->ui.toPageCombo->setCurrentIndex(npages-1);
      d->ui.destinationGroupBox->setEnabled(true);
      d->ui.formatGroupBox->setEnabled(true);
      d->ui.saveGroupBox->setEnabled(true);
      d->ui.saveButton->setEnabled(true);
    }
  bool nodoc = !d->document;
  bool nojob = !d->job;
  bool notxt = d->ui.fileNameEdit->text().isEmpty();
  d->ui.destinationGroupBox->setEnabled(nojob && !nodoc);
#if DDJVUAPI_VERSION <= 18
  d->ui.formatGroupBox->setEnabled(false);
#else
  d->ui.formatGroupBox->setEnabled(nojob && !nodoc && false);
#endif
  d->ui.saveGroupBox->setEnabled(nojob && !nodoc);
  d->ui.saveButton->setEnabled(nojob && !nodoc && !notxt);
  d->ui.cancelButton->setEnabled(nojob);
  d->ui.stopButton->setEnabled(!nojob);
  d->ui.stackedWidget->setCurrentIndex(nojob ? 0 : 1);
}


void 
QDjViewSaveDialog::browse()
{
  QString caption = d->djview->makeCaption(tr("Save", "dialog caption"));
  QString fname = d->ui.fileNameEdit->text();
  QString filters = "DjVu files (*.djvu *.djv);;All files (*)";
  fname = QFileDialog::getSaveFileName(this, caption, fname, filters, 0,
                                       QFileDialog::DontConfirmOverwrite);
  if (fname.section("/",-1).lastIndexOf(".") < 0)
    fname += ".djvu";
  d->ui.fileNameEdit->setText(fname);
}


void 
QDjViewSaveDialog::save()
{
  // checks
  QString fname = d->ui.fileNameEdit->text();
  QFileInfo info(fname);
  QDir dir = info.dir();
  if (!d->document)
    return;
  if (info.exists() &&
      QMessageBox::question(this, tr("Overwrite file?"),
                            tr("A file with this name already exists.\n"
                               "Do you want to overwrite it?"),
                            tr("&Overwrite"),
                            tr("&Cancel") ) )
    return;
#if QT_VERSION >= 0x40100
  if (d->ui.indirectButton->isChecked() &&
      !dir.entryList(QDir::AllEntries|QDir::NoDotAndDotDot).isEmpty() &&
      QMessageBox::question(this, tr("Directory is not empty"),
                            tr("<html> This file belongs to a non empty "
                               "directory. Saving an indirect document "
                               "creates many files in this directory. "
                               "Do you want to continue and risk overwriting "
                               "the contents of this directory? </html>"),
                            tr("Con&tinue"),
                            tr("&Cancel") ) )
    return;
#endif
  
  QByteArray pagespec;
  int curpage = d->djview->getDjVuWidget()->page();
  int frompage = d->ui.fromPageCombo->currentIndex();
  int topage = d->ui.toPageCombo->currentIndex();
  if (frompage > topage)
    qSwap(frompage, topage);
  if (d->ui.currentPageButton->isChecked())
    pagespec = QString("--pages=%1")
      .arg(curpage+1).toLocal8Bit();
  else if (d->ui.pageRangeButton->isChecked())
    pagespec = QString("--pages=%1-%2")
      .arg(frompage+1).arg(topage+1).toLocal8Bit();
  
  int argc = 0;
  const char *argv[2];
  if (d->ui.indirectButton->isChecked())
    argv[argc++] = "--indirect";
  if (! pagespec.isEmpty())
    argv[argc++] = pagespec.data();
  
  QByteArray sname = QFile::encodeName(fname);
  ::remove(sname.data());
  d->output = ::fopen(sname.data(), "w");
  if (! d->output)
    {
      QString message = tr("System error: %1").arg(strerror(errno));
      error(message, __FILE__, __LINE__);
      return;
    }
  ddjvu_job_t *job;
  job = ddjvu_document_save(*d->document, d->output, argc, argv);
  if (! job)
    {
      QString message = tr("Save job creation failed!");
      error(message, __FILE__, __LINE__);
      return;
    }
  d->job = new QDjVuJob(job, this);
  connect(d->job, SIGNAL(error(QString,QString,int)),
          this, SLOT(error(QString,QString,int)) );
  connect(d->job, SIGNAL(progress(int)),
          this, SLOT(progress(int)) );
  
  d->ui.progressBar->setValue(0);
  refresh();
}


void 
QDjViewSaveDialog::progress(int percent)
{
  d->ui.progressBar->setValue(percent);
  if (d->job)
    {
      ddjvu_status_t status = ddjvu_job_status(*d->job);
      switch(status)
        {
        case DDJVU_JOB_OK:
          accept();
          break;
        case DDJVU_JOB_FAILED:
          error(tr("This job has failed."), __FILE__, __LINE__);
          break;
        case DDJVU_JOB_STOPPED:
          error(tr("This job has been interrupted."), __FILE__, __LINE__);
          break;
        default:
          break;
        }
    }
}

void 
QDjViewSaveDialog::stop()
{
  if (d->job)
    {
      ddjvu_job_stop(*d->job);
      d->ui.stopButton->setEnabled(false);
    }
}


void 
QDjViewSaveDialog::error(QString message, QString filename, int lineno)
{
  if (! d->errdialog)
    {
      d->errdialog = new QDjViewErrorDialog(this);
      QString caption = d->djview->makeCaption(tr("Saving DjVu file..."));
      d->errdialog->prepare(QMessageBox::Critical, caption);
      connect(d->errdialog, SIGNAL(closing()), this, SLOT(reject()));
#if QT_VERSION >= 0x040100
      d->errdialog->setWindowModality(Qt::WindowModal);
#else
      d->errdialog->setModal(true);
#endif
    }
  d->errdialog->error(message, filename, lineno);
  d->errdialog->show();
}


void 
QDjViewSaveDialog::done(int result)
{
  if (d->job)
    ddjvu_job_stop(*d->job);
  if (d->output && result == QDialog::Rejected)
    QFile(d->ui.fileNameEdit->text()).remove();
  if (d->output)
    fclose(d->output);
  d->output = 0;
  QDialog::done(result);
}




// =======================================
// QDJVIEWPRINTDIALOG
// =======================================


#include "ui_qdjviewprintdialog.h"

struct QDjViewPrintDialog::Private {
  Ui::QDjViewPrintDialog ui;
  QDjView *djview;
  QPrinter *printer;
  QDjVuDocument *document;
  QDjVuJob *job;
  QDjVuPage *page;
  QDjViewErrorDialog *errdialog;
  FILE *output;
  bool stop;
};


QDjViewPrintDialog::~QDjViewPrintDialog()
{
  delete d->printer;
  delete d;
}

QDjViewPrintDialog::QDjViewPrintDialog(QDjView *djview)
  : d(new Private)
{
  d->djview = djview;
  d->printer = new QPrinter(QPrinter::HighResolution);
  d->document = 0;
  d->job = 0;
  d->page = 0;
  d->output = 0;
  d->stop = false;
  d->ui.setupUi(this);

  connect(d->djview, SIGNAL(documentClosed(QDjVuDocument*)),
          this, SLOT(reject()) );
  connect(d->djview, SIGNAL(documentReady(QDjVuDocument*)),
          this, SLOT(refresh()) );
  connect(d->ui.fromPageCombo, SIGNAL(activated(int)),
          d->ui.pageRangeButton, SLOT(click()) );
  connect(d->ui.toPageCombo, SIGNAL(activated(int)),
          d->ui.pageRangeButton, SLOT(click()) );
  connect(d->ui.fromPageCombo, SIGNAL(activated(int)),
          this, SLOT(refresh()) );
  connect(d->ui.toPageCombo, SIGNAL(activated(int)),
          this, SLOT(refresh()) );
  connect(d->ui.cancelButton, SIGNAL(clicked() ),
          this, SLOT(reject()) );
  connect(d->ui.printButton, SIGNAL(clicked() ),
          this, SLOT(print()) );

  refresh();
}


void 
QDjViewPrintDialog::refresh()
{
  int npages = d->djview->pageNum();
  if (!d->document && npages>0)
    {
      d->document = d->djview->getDocument();
    }
  
#if QT_VERSION < 0x40100
  d->ui.pdfButton.setEnabled(false);
#endif
  
}


void 
QDjViewPrintDialog::progress(int percent)
{
  d->ui.progressBar->setValue(percent);
  if (d->job)
    {
      ddjvu_status_t status = ddjvu_job_status(*d->job);
      switch(status)
        {
        case DDJVU_JOB_OK:
          accept();
          break;
        case DDJVU_JOB_FAILED:
          error(tr("This job has failed."), __FILE__, __LINE__);
          break;
        case DDJVU_JOB_STOPPED:
          error(tr("This job has been interrupted."), __FILE__, __LINE__);
          break;
        default:
          break;
        }
    }
  if (d->stop)
    reject();
}


void 
QDjViewPrintDialog::print()
{
  // start printing in whatever format
}


void 
QDjViewPrintDialog::printDjVu()
{
  // print one more page in native format
}


void 
QDjViewPrintDialog::printNative()
{
  if (d->stop)
    reject();
  // print one more page in native format
}


void 
QDjViewPrintDialog::stop()
{
  if (d->job)
    ddjvu_job_stop(*d->job);
  d->stop = true;
  d->ui.stopButton->setEnabled(false);
}


void 
QDjViewPrintDialog::error(QString message, QString filename, int lineno)
{
  if (! d->errdialog)
    {
      d->errdialog = new QDjViewErrorDialog(this);
      QString caption = d->djview->makeCaption(tr("Printing DjVu file..."));
      d->errdialog->prepare(QMessageBox::Critical, caption);
      connect(d->errdialog, SIGNAL(closing()), this, SLOT(reject()));
#if QT_VERSION >= 0x040100
      d->errdialog->setWindowModality(Qt::WindowModal);
#else
      d->errdialog->setModal(true);
#endif
    }
  d->errdialog->error(message, filename, lineno);
  d->errdialog->show();
}


void 
QDjViewPrintDialog::done(int result)
{
  if (d->job) 
    ddjvu_job_stop(*d->job);
  if (d->output && result == QDialog::Rejected)
    { /* remove printed file */ }
  if (d->output)
    fclose(d->output);
  d->output = 0;
  QDialog::done(result);
}



/* -------------------------------------------------------------
   Local Variables:
   c++-font-lock-extra-types: ( "\\sw+_t" "[A-Z]\\sw*[a-z]\\sw*" )
   End:
   ------------------------------------------------------------- */


