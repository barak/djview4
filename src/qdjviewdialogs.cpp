//C-  -*- C++ -*-
//C- -------------------------------------------------------------------
//C- DjView4
//C- Copyright (c) 2006  Leon Bottou
//C-
//C- This software is subject to, and may be distributed under, the
//C- GNU General Public License, either version 2 of the license,
//C- or (at your option) any later version. The license should have
//C- accompanied the software or you may obtain a copy of the license
//C- from the Free Software Foundation at http://www.fsf.org .
//C-
//C- This program is distributed in the hope that it will be useful,
//C- but WITHOUT ANY WARRANTY; without even the implied warranty of
//C- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//C- GNU General Public License for more details.
//C-  ------------------------------------------------------------------

// $Id$

#if AUTOCONF
# include "config.h"
#else
# define HAVE_STRING_H     1
# define HAVE_SYS_TYPES_H  1
# define HAVE_SYS_WAIT_H   1
# define HAVE_UNISTD_H     1
# define HAVE_STRERROR     1
# define HAVE_WAITPID      1
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#if HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#if HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif
#if HAVE_STRING_H
# include <string.h>
#endif
#if HAVE_UNISTD_H
# include <unistd.h>
#endif
#if HAVE_TIFF
# include <tiffio.h>
#endif

#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QDialog>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QImageWriter>
#include <QItemDelegate>
#include <QKeySequence>
#include <QList>
#include <QMap>
#include <QMessageBox>
#include <QObject>
#include <QPainter>
#include <QPrinter>
#include <QPrintDialog>
#include <QPrintEngine>
#include <QPushButton>
#include <QRegExp>
#include <QScrollBar>
#include <QSettings>
#include <QShortcut>
#include <QString>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTabWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QVector>
#include <QtAlgorithms>

#include <libdjvu/miniexp.h>
#include <libdjvu/ddjvuapi.h>

#include "qdjview.h"
#include "qdjviewprefs.h"
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
  if (caption.isEmpty())
    caption = tr("Error - DjView", "dialog caption");
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
  QList<ddjvu_fileinfo_t> files;
  int fileno;
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
  d->fileno = 0;
  d->pageno = -1;
  d->done = false;
  d->ui.setupUi(this);

  QDjViewPrefs *prefs = QDjViewPrefs::instance();
  int index = prefs->infoDialogTab;
  if (index >= 0 && index < d->ui.tabWidget->count())
    d->ui.tabWidget->setCurrentIndex(index);
  
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
                      "to display detailled information in the <tt>File</tt> "
                      "tab. Double click a component file to show "
                      "the corresponding page in the main window. "
                      "</html>"));
  QWidget *wf = d->ui.tabFile;
  wf->setWhatsThis(tr("<html><b>File and page information</b><br>"
                      "This panel shows the structure of the DjVu data "
                      "corresponding to the component file or the page "
                      "selected in the <tt>Document</tt> tab. "
                      "The arrow buttons jump to the previous or next "
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
  d->files.clear();
  d->document = 0;
  QDjViewPrefs *prefs = QDjViewPrefs::instance();
  prefs->infoDialogTab = d->ui.tabWidget->currentIndex();
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
  
  // get preferences
  QDjViewPrefs *prefs = QDjViewPrefs::instance();
  int index = prefs->metaDialogTab;
  if (index >= 0 && index < d->ui.tabWidget->count())
    d->ui.tabWidget->setCurrentIndex(index);

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
  if (d->document)
    disconnect(d->document, 0, this, 0);
  d->document = 0;
  d->pageno = 0;
  d->docAnno = miniexp_dummy;
  d->pageAnno = miniexp_dummy;
  d->ui.pageCombo->clear();
  d->ui.pageCombo->setEnabled(false);
  d->ui.docTable->setRowCount(0);
  d->ui.pageTable->setRowCount(0);
  d->ui.jumpButton->setEnabled(false);
  QDjViewPrefs *prefs = QDjViewPrefs::instance();
  prefs->infoDialogTab = d->ui.tabWidget->currentIndex();
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
// QDJVIEWEXPORTER
// =======================================



class QDjViewExporter : public QObject
{
  Q_OBJECT
public:
  QDjViewExporter(QDialog *parent, QDjView *djview);
  virtual bool canExportOnePage()         { return true; }
  virtual bool canExportPageRange()       { return true; }
  virtual bool canCollate()               { return true; }
  virtual void setFileName(QString s)     { fileName = s; }
  virtual void setPrinter(QPrinter*, int) { }
  virtual void iniPrinter(QPrinter*)      { }
  virtual void resetOptions()             { }
  virtual void setPageRange(int f, int t) { fromPage = f; toPage = t; }
  virtual void setErrorCaption(QString s) { errorCaption = s; }
  virtual int      propertyPages()        { return 0; }
  virtual QWidget* propertyPage(int)      { return 0; }
  virtual ddjvu_status_t status();
public slots:
  virtual void run() = 0;
  virtual void stop();
  virtual void error(QString, QString, int);
signals:
  void progress(int i);
protected:
  QDialog            *parent;
  QDjView            *djview;
  QDjViewErrorDialog *errorDialog;
  QString             errorCaption;
  QString             fileName;
  int                 fromPage;
  int                 toPage;
};


QDjViewExporter::QDjViewExporter(QDialog *parent, QDjView *djview)
  : QObject(parent),
    parent(parent),
    djview(djview),
    errorDialog(0),
    fromPage(0),
    toPage(-1)
{
}


ddjvu_status_t 
QDjViewExporter::status()
{
  return DDJVU_JOB_NOTSTARTED;
}


void
QDjViewExporter::stop()
{
}


void 
QDjViewExporter::error(QString message, QString filename, int lineno)
{
  if (! errorDialog)
    {
      errorDialog = new QDjViewErrorDialog(parent);
      errorDialog->prepare(QMessageBox::Critical, errorCaption);
      connect(errorDialog, SIGNAL(closing()), parent, SLOT(reject()));
#if QT_VERSION >= 0x040100
      errorDialog->setWindowModality(Qt::WindowModal);
#else
      errorDialog->setModal(true);
#endif
    }
  errorDialog->error(message, filename, lineno);
  errorDialog->show();
}



// -------------------
// QDJVIEWDJVUEXPORTER



class QDjViewDjVuExporter : public QDjViewExporter
{
  Q_OBJECT
public:
  ~QDjViewDjVuExporter();
  QDjViewDjVuExporter(QDialog *parent, QDjView *djview, bool indirect);
  virtual void run();
  virtual void stop();
  virtual ddjvu_status_t status();
protected:
  QDjVuJob *job;
  FILE *output;
  bool indirect;
  bool failed;
};


QDjViewDjVuExporter::QDjViewDjVuExporter(QDialog *parent, QDjView *djview, 
                                         bool indirect)
  : QDjViewExporter(parent, djview),
    job(0), 
    output(0),
    indirect(indirect), 
    failed(false)
{
}


QDjViewDjVuExporter::~QDjViewDjVuExporter()
{
  if (output)
    ::fclose(output);
  output = 0;
  if (djview)
    ddjvu_cache_clear(djview->getDjVuContext());
  switch(status())
    {
    case DDJVU_JOB_STARTED:
      if (job)
        ddjvu_job_stop(*job);
    case DDJVU_JOB_FAILED:
    case DDJVU_JOB_STOPPED:
      if (! fileName.isEmpty())
        ::remove(QFile::encodeName(fileName).data());
    default:
      break;
    }
}
 

void 
QDjViewDjVuExporter::run()
{
  QDjVuDocument *document = djview->getDocument();
  int pagenum = djview->pageNum();
  if (document==0 || pagenum <= 0 || fileName.isEmpty())
    return;
#if QT_VERSION >= 0x40100
  QFileInfo info(fileName);
  QDir::Filters filters = QDir::AllEntries|QDir::NoDotAndDotDot;
  if (indirect && !info.dir().entryList(filters).isEmpty() &&
      QMessageBox::question(parent,
                            tr("Question - DjView", "dialog caption"),
                            tr("<html> This file belongs to a non empty "
                               "directory. Saving an indirect document "
                               "creates many files in this directory. "
                               "Do you want to continue and risk "
                               "overwriting files in this directory?"
                               "</html>"),
                            tr("Con&tinue"),
                            tr("&Cancel") ) )
    return;
#endif
  toPage = qBound(0, toPage, pagenum-1);
  fromPage = qBound(0, fromPage, pagenum-1);
  QByteArray pagespec;
  if (fromPage == toPage && pagenum > 1)
    pagespec.append(QString("--pages=%1").arg(fromPage+1));
  else if (fromPage != 0 || toPage != pagenum - 1)
    pagespec.append(QString("--pages=%1-%2").arg(fromPage+1).arg(toPage+1));
  QByteArray namespec;
  if (indirect)
    namespec = "--indirect=" + QFile::encodeName(fileName);

  int argc = 0;
  const char *argv[2];
  if (! namespec.isEmpty())
    argv[argc++] = namespec.data();
  if (! pagespec.isEmpty())
    argv[argc++] = pagespec.data();
  
  QByteArray fname = QFile::encodeName(fileName);
  ::remove(fname.data());
  output = ::fopen(fname.data(), "w");
  if (! output)
    {
      failed = true;
      QString message = tr("Unknown error.");
#ifdef HAVE_STRERROR
      if (errno)
        message = tr("System error: %1.").arg(strerror(errno));
#endif
      error(message, __FILE__, __LINE__);
      return;
    }
  ddjvu_job_t *pjob;
  pjob = ddjvu_document_save(*document, output, argc, argv);
  if (! pjob)
    {
      failed = true;
      // error messages went to the document
      connect(document, SIGNAL(error(QString,QString,int)),
              this, SLOT(error(QString,QString,int)) );
      qApp->sendEvent(&djview->getDjVuContext(), new QEvent(QEvent::User));
      disconnect(document, 0, this, 0);
      // main error message
      QString message = tr("Save job creation failed!");
      error(message, __FILE__, __LINE__);
      return;
    }
  job = new QDjVuJob(pjob, this);
  connect(job, SIGNAL(error(QString,QString,int)),
          this, SLOT(error(QString,QString,int)) );
  connect(job, SIGNAL(progress(int)),
          this, SIGNAL(progress(int)) );
  emit progress(0);
}


void 
QDjViewDjVuExporter::stop()
{
  if (job && status() == DDJVU_JOB_STARTED)
    ddjvu_job_stop(*job);
}


ddjvu_status_t 
QDjViewDjVuExporter::status()
{
  if (job)
    return ddjvu_job_status(*job);
  else if (failed)
    return DDJVU_JOB_FAILED;
  else
    return DDJVU_JOB_NOTSTARTED;
}




// -----------------
// QDJVIEWPSEXPORTER


#include "ui_qdjviewexportps1.h"
#include "ui_qdjviewexportps2.h"
#include "ui_qdjviewexportps3.h"


class QDjViewPSExporter : public QDjViewExporter
{
  Q_OBJECT
public:
  ~QDjViewPSExporter();
  QDjViewPSExporter(QDialog *parent, QDjView *djview, 
                    QString group, bool eps=false);
  virtual void run();
  virtual void stop();
  virtual ddjvu_status_t status();
  virtual void resetOptions();
  virtual void setFileName(QString s);
  virtual void setPrinter(QPrinter *p, int copies);
  virtual void iniPrinter(QPrinter *p);
  virtual QWidget* propertyPage(int);
  virtual bool canExportOnePage()    { return true; }
  virtual bool canExportPageRange()  { return !encapsulated; }
  virtual bool canCollate()          { return !cupsOptions.isEmpty(); }
  virtual int propertyPages()        { return encapsulated ? 1 : 3; }
public slots:
  void refresh();
protected:
  void openFile();
  void closeFile();
protected:
  QDjVuJob *job;
  FILE *output;
  bool failed;
  bool encapsulated;
  QString group;
  QString printerName;
  int copies;
  QList<QByteArray> cupsOptions;
  QPointer<QWidget> page1;
  QPointer<QWidget> page2;
  QPointer<QWidget> page3;
  Ui::QDjViewExportPS1 ui1;
  Ui::QDjViewExportPS2 ui2;
  Ui::QDjViewExportPS3 ui3;
};


QDjViewPSExporter::~QDjViewPSExporter()
{
  // save properties
  QSettings s(DJVIEW_ORG, DJVIEW_APP);
  s.beginGroup("Export-" + group);
  s.setValue("color", ui1.colorButton->isChecked());
  s.setValue("frame", ui1.frameCheckBox->isChecked());
  s.setValue("cropMarks", ui1.cropMarksCheckBox->isChecked());
  s.setValue("psLevel", ui1.levelSpinBox->value());
  s.setValue("autoOrient", ui2.autoOrientCheckBox->isChecked());
  s.setValue("landscape", ui2.landscapeButton->isChecked());
  int zoom = ui2.zoomSpinBox->value();
  s.setValue("zoom", QVariant(ui2.zoomButton->isChecked() ? zoom : 0));
  s.setValue("booklet", ui3.bookletCheckBox->isChecked());
  s.setValue("bookletSheets", ui3.sheetsSpinBox->value());
  s.setValue("bookletRectoVerso", ui3.rectoVersoCombo->currentIndex());
  s.setValue("bookletShift", ui3.rectoVersoShiftSpinBox->value());
  s.setValue("bookletCenter", ui3.centerMarginSpinBox->value());
  s.setValue("bookletCenterAdd", ui3.centerIncreaseSpinBox->value());
  // delete property pages
  delete page1;
  delete page2;
  delete page3;
  // cleanup output
  closeFile();
  switch(status())
    {
    case DDJVU_JOB_STARTED:
      if (job)
        ddjvu_job_stop(*job);
    case DDJVU_JOB_FAILED:
    case DDJVU_JOB_STOPPED:
      if (! fileName.isEmpty())
        ::remove(QFile::encodeName(fileName).data());
    default:
      break;
    }
}


void 
QDjViewPSExporter::resetOptions()
{
  ui1.colorButton->setChecked(true);
  ui1.frameCheckBox->setChecked(false);
  ui1.cropMarksCheckBox->setChecked(false);
  ui1.levelSpinBox->setValue(3);
  ui2.autoOrientCheckBox->setChecked(true);
  ui2.scaleToFitButton->setChecked(true);
  ui2.zoomSpinBox->setValue(100);
  ui3.bookletCheckBox->setChecked(false);
  ui3.sheetsSpinBox->setValue(0);
  ui3.rectoVersoCombo->setCurrentIndex(0);
  ui3.rectoVersoShiftSpinBox->setValue(0);
  ui3.centerMarginSpinBox->setValue(18);
  ui3.centerIncreaseSpinBox->setValue(40);
  refresh();
}


QDjViewPSExporter::QDjViewPSExporter(QDialog *parent, QDjView *djview, 
                                     QString group, bool eps)
  : QDjViewExporter(parent, djview),
    job(0), 
    output(0),
    failed(false),
    encapsulated(eps),
    group(group),
    copies(1)
{
  // create pages
  page1 = new QWidget();
  page2 = new QWidget();
  page3 = new QWidget();
  ui1.setupUi(page1);
  ui2.setupUi(page2);
  ui3.setupUi(page3);
  page1->setObjectName(tr("PostScript","tab caption"));
  page2->setObjectName(tr("Position","tab caption"));
  page3->setObjectName(tr("Booklet","tab caption"));

  // load settings (ui1)
  QSettings s(DJVIEW_ORG, DJVIEW_APP);
  s.beginGroup("Export-" + group);
  bool color = s.value("color", true).toBool();
  ui1.colorButton->setChecked(color);
  ui1.grayScaleButton->setChecked(!color);
  ui1.frameCheckBox->setChecked(s.value("frame", false).toBool());
  ui1.cropMarksCheckBox->setChecked(s.value("cropMarks", false).toBool());
  int level = s.value("psLevel", 3).toInt();
  ui1.levelSpinBox->setValue(qBound(1,level,3));

  // load settings (ui2)  
  ui2.autoOrientCheckBox->setChecked(s.value("autoOrient", true).toBool());
  bool landscape = s.value("landscape",false).toBool();
  ui2.portraitButton->setChecked(!landscape);
  ui2.landscapeButton->setChecked(landscape);
  int zoom = s.value("zoom",0).toInt();
  ui2.scaleToFitButton->setChecked(zoom == 0);
  ui2.zoomButton->setChecked(zoom!=0);
  ui2.zoomSpinBox->setValue(zoom ? qBound(25,zoom,2400) : 100);

  // load settings (ui3)  
  ui3.bookletCheckBox->setChecked(s.value("booklet", false).toBool());
  ui3.sheetsSpinBox->setValue(s.value("bookletSheets", 0).toInt());
  int rectoVerso = qBound(0, s.value("bookletRectoVerso",0).toInt(), 2);
  ui3.rectoVersoCombo->setCurrentIndex(rectoVerso);
  int rectoVersoShift = qBound(-72, s.value("bookletShift",0).toInt(), 72);
  ui3.rectoVersoShiftSpinBox->setValue(rectoVersoShift);
  int centerMargin = qBound(0, s.value("bookletCenter", 18).toInt(), 144);
  ui3.centerMarginSpinBox->setValue(centerMargin);
  int centerIncrease = qBound(0, s.value("bookletCenterAdd", 40).toInt(), 200);
  ui3.centerIncreaseSpinBox->setValue(centerIncrease);

  // connect stuff
  connect(ui2.autoOrientCheckBox, SIGNAL(clicked()), 
          this, SLOT(refresh()) );
  connect(ui3.bookletCheckBox, SIGNAL(clicked()), 
          this, SLOT(refresh()) );
  connect(ui2.zoomSpinBox, SIGNAL(valueChanged(int)), 
          ui2.zoomButton, SLOT(click()) );

  // whatsthis
  page1->setWhatsThis(tr("<html><b>PostScript options.</b><br>"
                         "Option <tt>Color</tt> enables color printing. "
                         "Document pages can be decorated with frame "
                         "and crop marks. "
                         "PostScript language level 1 is only useful "
                         "with very old printers. Level 2 works with most "
                         "printers. Level 3 print color document faster "
                         "on recent printers.</html>") );

  page2->setWhatsThis(tr("<html><b>Position and scaling.</b><br>"
                         "Option <tt>Scale to fit</tt> accomodates "
                         "whatever paper size your printer uses. "
                         "Zoom factor <tt>100%</tt> reproduces the initial "
                         "document size. Orientation <tt>Automatic</tt> "
                         "chooses portrait or landscape on a page per "
                         "page basis.</html>") );

  page3->setWhatsThis(tr("<html><b>Producing booklets.</b><br>"
                         "The booklet mode prints the selected "
                         "pages as sheets suitable for folding one or several "
                         "booklets. Several booklets might be produced when "
                         "a maximum number of sheets per booklet is "
                         "specified. You can either use a duplex printer or "
                         "print rectos and versos separately.<p> "
                         "Shifting rectos and versos is useful "
                         "with poorly aligned duplex printers. "
                         "The center margins determine how much "
                         "space is left between the pages to fold the "
                         "sheets. This space slowly increases from the "
                         "inner sheet to the outer sheet."
                         "</html>") );
  
  // adjust ui
  refresh();
}


void 
QDjViewPSExporter::refresh()
{
  if (encapsulated)
    {
      ui1.cropMarksCheckBox->setEnabled(false);
      ui1.cropMarksCheckBox->setChecked(false);
    }
  bool autoOrient = ui2.autoOrientCheckBox->isChecked();
  ui2.portraitButton->setEnabled(!autoOrient);
  ui2.landscapeButton->setEnabled(!autoOrient);
  bool booklet = ui3.bookletCheckBox->isChecked();
  ui3.bookletGroupBox->setEnabled(booklet);
  bool nojob = (status() < DDJVU_JOB_STARTED);
  page1->setEnabled(nojob);
  page2->setEnabled(nojob && !encapsulated);
  page3->setEnabled(nojob && !encapsulated);
}


void 
QDjViewPSExporter::setFileName(QString s)
{
  printerName = QString::null;
  fileName = s;
  refresh();
}


void 
QDjViewPSExporter::setPrinter(QPrinter *p, int ncopies)
{
  if (! p->outputFileName().isEmpty())
    {
      fileName = p->outputFileName();
      printerName = QString::null;
    }
  else
    {
      fileName = QString::null;
      printerName = p->printerName();
    }
#if QT_VERSION >= 0x40100  // bug?
  QPrinter::ColorMode colorMode = p->colorMode();
  ui1.colorButton->setChecked(colorMode == QPrinter::Color);
  ui1.grayScaleButton->setChecked(colorMode == QPrinter::GrayScale);
#endif
  QPrinter::Orientation orient = p->orientation();
  ui2.portraitButton->setChecked(orient == QPrinter::Portrait);
  ui2.landscapeButton->setChecked(orient == QPrinter::Landscape);
  copies = qMax(ncopies, p->numCopies());
  refresh();
  
#if QT_VERSION >= 0x40200
# define PPK_CupsOptions QPrintEngine::PrintEnginePropertyKey(0xfe00)
# define PPK_CupsStringPageSize QPrintEngine::PrintEnginePropertyKey(0xfe03)
  cupsOptions.clear();
  QPrintEngine *engine = p->printEngine();
  QVariant cPageSize = engine->property(PPK_CupsStringPageSize);
  if (! cPageSize.toString().isEmpty())
    {
      cupsOptions << "media=" + cPageSize.toString().toLocal8Bit();
      if (p->collateCopies())
        cupsOptions << "Collate=True";
    }
  QVariant cOptions = engine->property(PPK_CupsOptions);
  QStringList options = cOptions.toStringList();
  if (! options.isEmpty())
    {
      QStringList::const_iterator it = options.constBegin();
      while (it != options.constEnd())
        {
          cupsOptions << (*it).toLocal8Bit() + "=" + (*(it+1)).toLocal8Bit();
          it += 2;
        }
    }
#endif
}


void 
QDjViewPSExporter::iniPrinter(QPrinter *p)
{
  if (ui1.grayScaleButton->isChecked())
    p->setColorMode(QPrinter::GrayScale);
  else
    p->setColorMode(QPrinter::Color);
  if (ui2.landscapeButton->isChecked())
    p->setOrientation(QPrinter::Landscape);
  else
    p->setOrientation(QPrinter::Portrait);
}


void 
QDjViewPSExporter::closeFile()
{
  if (output)
    ::fclose(output);
  output = 0;
  // Special handling
#if Q_OS_WIN
  // TODO: windows postscript passthru
  //       with {Open,Write,Close}Printer
#endif
}


void 
QDjViewPSExporter::openFile()
{
  closeFile();

  if (fileName.isEmpty())
    {
#ifdef Q_WS_WIN
      // TODO: tempfile?
#endif
#ifdef Q_WS_MAC
      // TODO: dunno?
#endif 
      // Disable SIGPIPE
#ifdef SIGPIPE
# ifndef HAVE_SIGACTION
#  ifdef SA_RESTART
#   define HAVE_SIGACTION 1
#  endif
# endif
# if HAVE_SIGACTION
      sigset_t mask;
      struct sigaction act;
      sigemptyset(&mask);
      sigaddset(&mask, SIGPIPE);
      sigprocmask(SIG_BLOCK, &mask, 0);
      sigaction(SIGPIPE, 0, &act);
      act.sa_handler = SIG_IGN;
      sigaction(SIGPIPE, &act, 0);
# else
      signal(SIGPIPE, SIG_IGN);
# endif
#endif

      // Prepare lp/lpr arguments
      QByteArray pname = printerName.toLocal8Bit();
      QByteArray ncopies = QByteArray::number(copies);
      QVector<const char*> lpargs;
      QVector<const char*> lprargs;
      lpargs << "lp";
      lprargs << "lpr";
      if (! printerName.isEmpty())
        {
          lpargs << "-d";
          lpargs << pname.data();
          lprargs << "-P";
          lprargs << pname.data();
        }
      if (! cupsOptions.isEmpty())
        {
          if (copies > 1)
            {
              lprargs << "-#";
              lprargs << ncopies.data();
            }
          for(int i=0; i<cupsOptions.size(); i++)
            {
              lprargs << "-o";
              lprargs << cupsOptions.at(i).data();
            }
        }
      lpargs << 0;
      lprargs << 0;
      
      // Open pipe for lp/lpr.
      int fds[2];
      if (pipe(fds) == 0)
        {
          int pid = fork();
          if (pid == 0)
            {
              // Rehash file descriptors
#if defined(_SC_OPEN_MAX)
              int nfd = (int)sysconf(_SC_OPEN_MAX);
#elif defined(_POSIX_OPEN_MAX)
              int nfd = (int)_POSIX_OPEN_MAX;
#elif defined(OPEN_MAX)
              int nfd = (int)OPEN_MAX;
#else
              int nfd = 256;
#endif
              ::close(0);
              ::dup(fds[0]);
              for (int i=1; i<nfd; i++)
                ::close(i);
              if (fork() == 0)
                {
                  // Try lp and lpr
                  (void)execvp("lpr", (char**)lprargs.data());
                  (void)execv("/bin/lpr", (char**)lprargs.data());
                  (void)execv("/usr/bin/lpr", (char**)lprargs.data());
                  (void)execvp("lp", (char**)lpargs.data());
                  (void)execv("/bin/lp", (char**)lpargs.data());
                  (void)execv("/usr/bin/lp", (char**)lpargs.data());
                }
              // Exit without running destructors
              (void)execlp("true", "true", (char *)0);
              (void)execl("/bin/true", "true", (char *)0);
              (void)execl("/usr/bin/true", "true", (char *)0);
              ::_exit(0);
              ::exit(0);
            }
          ::close(fds[0]);
          output = fdopen(fds[1], "w");
          if (pid >= 0)
            {
#if HAVE_WAITPID
              ::waitpid(pid, 0, 0);
#else
              ::wait(0);
#endif
            }
          else
            closeFile();
        }
    }
  else if (! fileName.isEmpty())
    {
      // Open descriptor to file
      QByteArray fname = QFile::encodeName(fileName);
      ::remove(fname.data());
      output = ::fopen(fname.data(), "w");
    }
  if (! output)
    {
      failed = true;
      QString message = tr("Unknown error.");
#if HAVE_STRERROR
      if (errno)
        message = tr("System error: %1.").arg(strerror(errno));
#endif
      error(message, __FILE__, __LINE__);
    }
}


void 
QDjViewPSExporter::run()
{
  QDjViewPrefs *prefs = QDjViewPrefs::instance();
  QDjVuDocument *document = djview->getDocument();
  int pagenum = djview->pageNum();
  if (document==0 || pagenum <= 0)
    return;
  int tpg = qBound(0, toPage, pagenum-1);
  int fpg = qBound(0, fromPage, pagenum-1);
  openFile();
  if (! output)
    return;

  // Prepare arguments
  QList<QByteArray> args;
  if (copies > 1)
    args << QString("--copies=%1").arg(copies).toLocal8Bit();
  if (fromPage == toPage && pagenum > 1)
    args << QString("--pages=%1").arg(fpg+1).toLocal8Bit();
  else if (fromPage != 0 || toPage != pagenum - 1)
    args << QString("--pages=%1-%2").arg(fpg+1).arg(tpg+1).toLocal8Bit();
  if (! ui2.autoOrientCheckBox->isChecked())
    {
      if (ui2.portraitButton->isChecked())
        args << QByteArray("--orientation=portrait");
      else if (ui2.landscapeButton->isChecked())
        args << QByteArray("--orientation=landscape");
    }
  if (ui2.zoomButton->isChecked())
    args << QString("--zoom=%1").arg(ui2.zoomSpinBox->value()).toLocal8Bit();
  if (ui1.frameCheckBox->isChecked())
    args << QByteArray("--frame=yes");
  if (ui1.cropMarksCheckBox->isChecked())
    args << QByteArray("--cropmarks=yes");
  if (ui1.levelSpinBox)
    args << QString("--level=%1").arg(ui1.levelSpinBox->value()).toLocal8Bit();
  if (ui1.grayScaleButton->isChecked())
    args << QByteArray("--color=no");
  if (prefs && prefs->printerGamma != 0.0)
    {
      double gamma = qBound(0.3, prefs->printerGamma, 5.0);
      args << QByteArray("--colormatch=no");
      args << QString("--gamma=%1").arg(gamma).toLocal8Bit();
    }
  if (ui3.bookletCheckBox->isChecked())
    {
      int bMode = ui3.rectoVersoCombo->currentIndex();
      if (bMode == 1)
        args << QByteArray("--booklet=recto");
      else if (bMode == 2)
        args << QByteArray("--booklet=verso");
      else
        args << QByteArray("--booklet=yes");
      int bMax = ui3.sheetsSpinBox->value();
      if (bMax > 0)
        args << QString("--bookletmax=%1").arg(bMax).toLocal8Bit();
      int bAlign = ui3.rectoVersoShiftSpinBox->value();
      args << QString("--bookletalign=%1").arg(bAlign).toLocal8Bit();
      int bCenter = ui3.centerMarginSpinBox->value();
      int bIncr = ui3.centerIncreaseSpinBox->value() * 10;
      args << QString("--bookletfold=%1+%2")
        .arg(bCenter).arg(bIncr).toLocal8Bit();
    }
  if (encapsulated)
    args << QByteArray("--format=eps");
  QDjVuWidget::DisplayMode displayMode;
  displayMode = djview->getDjVuWidget()->displayMode();
  if (displayMode == QDjVuWidget::DISPLAY_STENCIL)
    args << QByteArray("--mode=black");
  else if (displayMode == QDjVuWidget::DISPLAY_BG)
    args << QByteArray("--mode=background");
  else if (displayMode == QDjVuWidget::DISPLAY_FG)
    args << QByteArray("--mode=foreground");

  // Convert arguments
  int argc = args.count();
  QVector<const char*> argv(argc);
  for (int i=0; i<argc; i++)
    argv[i] = args[i].constData();

  // Start print job
  ddjvu_job_t *pjob;
  pjob = ddjvu_document_print(*document, output, argc, argv.data());
  if (! pjob)
    {
      failed = true;
      // error messages went to the document
      connect(document, SIGNAL(error(QString,QString,int)),
              this, SLOT(error(QString,QString,int)) );
      qApp->sendEvent(&djview->getDjVuContext(), new QEvent(QEvent::User));
      disconnect(document, 0, this, 0);
      // main error message
      QString message = tr("Save job creation failed!");
      error(message, __FILE__, __LINE__);
      return;
    }
  job = new QDjVuJob(pjob, this);
  connect(job, SIGNAL(error(QString,QString,int)),
          this, SLOT(error(QString,QString,int)) );
  connect(job, SIGNAL(progress(int)),
          this, SIGNAL(progress(int)) );
  emit progress(0);
  refresh();
}


void 
QDjViewPSExporter::stop()
{
  if (job && status() == DDJVU_JOB_STARTED)
    ddjvu_job_stop(*job);
}


ddjvu_status_t 
QDjViewPSExporter::status()
{
  if (job)
    return ddjvu_job_status(*job);
  else if (failed)
    return DDJVU_JOB_FAILED;
  else
    return DDJVU_JOB_NOTSTARTED;
}


QWidget* 
QDjViewPSExporter::propertyPage(int n)
{
  if (n == 0)
    return page1;
  else if (n == 1)
    return page2;
  else if (n == 2)
    return page3;
  return 0;
}



// --------------------
// QDJVIEWPAGEEXPORTER


class QDjViewPageExporter : public QDjViewExporter
{
  Q_OBJECT
public:
  ~QDjViewPageExporter();
  QDjViewPageExporter(QDialog *parent, QDjView *djview);
  virtual ddjvu_status_t status() { return curStatus; }
public slots:
  virtual void run();
  virtual void stop();
  virtual void error(QString message, QString filename, int lineno);
protected slots:
  void checkPage();
protected:
  virtual void openFile() {}
  virtual void closeFile() {}
  virtual void doPage() = 0;
  ddjvu_status_t curStatus;
  QDjVuPage *curPage;
  int curProgress;
};


QDjViewPageExporter::~QDjViewPageExporter()
{
  curStatus = DDJVU_JOB_NOTSTARTED;
  closeFile();
}


QDjViewPageExporter::QDjViewPageExporter(QDialog *parent, QDjView *djview)
  : QDjViewExporter(parent, djview),
    curStatus(DDJVU_JOB_NOTSTARTED), 
    curPage(0), curProgress(0)
{
}


void 
QDjViewPageExporter::run()
{
  if (curStatus != DDJVU_JOB_STARTED)
    {
      curStatus = DDJVU_JOB_STARTED;
      openFile();
      checkPage();
    }
}


void 
QDjViewPageExporter::stop()
{
  if (curStatus == DDJVU_JOB_STARTED)
    curStatus = DDJVU_JOB_STOPPED;
  if (curPage)
    if (ddjvu_page_decoding_status(*curPage) == DDJVU_JOB_STARTED)
      ddjvu_job_stop(ddjvu_page_job(*curPage));
  emit progress(curProgress);
}


void 
QDjViewPageExporter::error(QString message, QString filename, int lineno)
{
  if (curStatus == DDJVU_JOB_STARTED)
    curStatus = DDJVU_JOB_FAILED;
  QDjViewExporter::error(message, filename, lineno);
}


void 
QDjViewPageExporter::checkPage()
{
  QDjVuDocument *document = djview->getDocument();
  int pagenum = djview->pageNum();
  if (document==0 || pagenum <= 0)
    return;
  ddjvu_status_t pageStatus = DDJVU_JOB_NOTSTARTED;
  if (curPage)
    pageStatus = ddjvu_page_decoding_status(*curPage);
  if (pageStatus > DDJVU_JOB_OK)
    {
      curStatus = pageStatus;
      emit progress(curProgress);
      disconnect(curPage, 0, this, 0);
      delete curPage;
      curPage = 0;
      closeFile();
      return;
    }
  int tpg = qBound(0, toPage, pagenum-1);
  int fpg = qBound(0, fromPage, pagenum-1);
  int nextPage = fpg;
  if (pageStatus == DDJVU_JOB_OK)
    {
      nextPage = curPage->pageNo() + 1;
      doPage();
      curProgress = 100 * (nextPage - fpg) / (1 + tpg - fpg);
      disconnect(curPage, 0, this, 0);
      delete curPage;
      curPage = 0;
      if (curStatus < DDJVU_JOB_OK && nextPage > tpg)
        curStatus = DDJVU_JOB_OK;
      emit progress(curProgress);
      if (curStatus >= DDJVU_JOB_OK)
        closeFile();
    }
  if (curStatus == DDJVU_JOB_STARTED && !curPage)
    {
      curPage = new QDjVuPage(document, nextPage, this);
      connect(curPage, SIGNAL(pageinfo()), 
              this, SLOT(checkPage()));
      connect(curPage, SIGNAL(error(QString,QString,int)),
              this, SLOT(error(QString,QString,int)) );
      if (ddjvu_page_decoding_done(*curPage))
        QTimer::singleShot(0, this, SLOT(checkPage()));
    }
}



// -------------------
// QDJVIEWTIFFEXPORTER


#include "ui_qdjviewexporttiff.h"


class QDjViewTiffExporter : public QDjViewPageExporter
{
  Q_OBJECT
public:
  ~QDjViewTiffExporter();
  QDjViewTiffExporter(QDialog *parent, QDjView *djview, QString group);
  virtual void resetOptions();
  virtual int propertyPages() { return 1; }
  virtual QWidget *propertyPage(int) { return page; }
protected:
  virtual void closeFile();
  virtual void doPage();
protected:
  Ui::QDjViewExportTiff ui;
  QPointer<QWidget> page;
  QString group;
#if HAVE_TIFF
  TIFF *tiff;
#else
  FILE *tiff;
#endif
};


QDjViewTiffExporter::~QDjViewTiffExporter()
{
  // save settings
  QSettings s(DJVIEW_ORG, DJVIEW_APP);
  s.beginGroup("Export-" + group);
  s.setValue("dpi", ui.dpiSpinBox->value());
  s.setValue("bitonal", ui.bitonalCheckBox->isChecked());
  s.setValue("jpeg", ui.jpegCheckBox->isChecked());
  s.setValue("jpegQuality", ui.jpegSpinBox->value());
  // delete page
  delete page;
  // close
  closeFile();
}


void 
QDjViewTiffExporter::resetOptions()
{
  ui.dpiSpinBox->setValue(600);
  ui.bitonalCheckBox->setChecked(false);
  ui.jpegCheckBox->setChecked(true);
  ui.jpegSpinBox->setValue(75);
}


QDjViewTiffExporter::QDjViewTiffExporter(QDialog *parent, QDjView *djview,
                                         QString group )
  : QDjViewPageExporter(parent, djview), 
    group(group),
    tiff(0)
{
  // create setup page
  page = new QWidget();
  ui.setupUi(page);
  page->setObjectName(tr("TIFF Options", "tab caption"));
  // read settings
  QSettings s(DJVIEW_ORG, DJVIEW_APP);
  s.beginGroup("Export-" + group);
  ui.dpiSpinBox->setValue(s.value("dpi", 600).toInt());
  ui.bitonalCheckBox->setChecked(s.value("bitonal", false).toBool());
  ui.jpegCheckBox->setChecked(s.value("jpeg", true).toBool());
  ui.jpegSpinBox->setValue(s.value("jpegQuality", 75).toInt());
  // whatsthis
  page->setWhatsThis(tr("<html><b>TIFF options.</b><br>"
                        "The resolution box specifies an upper "
                        "limit for the resolution of the TIFF images. "
                        "Forcing bitonal G4 compression "
                        "encodes all pages in black and white "
                        "using the CCITT Group 4 compression. "
                        "Allowing JPEG compression uses lossy JPEG "
                        "for all non bitonal or subsampled images. "
                        "</html>") );
}


#if HAVE_TIFF

static QDjViewTiffExporter *tiffExporter;

static void 
tiffHandler(const char *, const char *fmt, va_list ap)
{
  QString message;
  message.vsprintf(fmt, ap);
  tiffExporter->error(message, __FILE__, __LINE__);
}

#endif


void 
QDjViewTiffExporter::closeFile()
{
#if HAVE_TIFF
  tiffExporter = this;
  TIFFSetErrorHandler(tiffHandler);
  TIFFSetWarningHandler(0);
  if (tiff)
    TIFFClose(tiff);
#endif
  if (curStatus > DDJVU_JOB_OK && !fileName.isEmpty())
    ::remove(QFile::encodeName(fileName).data());
  tiff = 0;
}


void 
QDjViewTiffExporter::doPage()
{
  ddjvu_format_t *fmt = 0;
  char *image = 0;
  QString message;
#if HAVE_TIFF
  tiffExporter = this;
  TIFFSetErrorHandler(tiffHandler);
  TIFFSetWarningHandler(0);
  QDjVuPage *page = curPage;
  // open file or write directory
  if (tiff)
    TIFFWriteDirectory(tiff);
  else
    tiff = TIFFOpen(QFile::encodeName(fileName).data(), "w");
  if (!tiff)
    message = tr("Cannot open output file.");
  else
    {
      // determine rectangle
      ddjvu_rect_t rect;
      int imgdpi = ddjvu_page_get_resolution(*page);
      int dpi = qMin(imgdpi, ui.dpiSpinBox->value());
      rect.x = rect.y = 0;
      rect.w = ( ddjvu_page_get_width(*page) * dpi + imgdpi/2 ) / imgdpi;
      rect.h = ( ddjvu_page_get_height(*page) * dpi + imgdpi/2 ) / imgdpi;
      // determine mode
      ddjvu_render_mode_t mode = DDJVU_RENDER_COLOR;
      QDjVuWidget::DisplayMode displayMode;
      displayMode = djview->getDjVuWidget()->displayMode();
      if (ui.bitonalCheckBox->isChecked())
        mode = DDJVU_RENDER_BLACK;
      else if (displayMode == QDjVuWidget::DISPLAY_STENCIL)
        mode = DDJVU_RENDER_BLACK;
      else if (displayMode == QDjVuWidget::DISPLAY_BG)
        mode = DDJVU_RENDER_BACKGROUND;
      else if (displayMode == QDjVuWidget::DISPLAY_FG)
        mode = DDJVU_RENDER_FOREGROUND;
      // determine format and compression
      ddjvu_page_type_t type = ddjvu_page_get_type(*page);
      ddjvu_format_style_t style = DDJVU_FORMAT_RGB24;
      int compression = COMPRESSION_NONE;
      if (ui.bitonalCheckBox->isChecked())
        style = DDJVU_FORMAT_MSBTOLSB;
      else if (mode==DDJVU_RENDER_BLACK 
               || type==DDJVU_PAGETYPE_BITONAL)
        style = (dpi == imgdpi) 
          ? DDJVU_FORMAT_MSBTOLSB 
          : DDJVU_FORMAT_GREY8;
      if (style == DDJVU_FORMAT_MSBTOLSB 
          && TIFFFindCODEC(COMPRESSION_CCITT_T6))
        compression = COMPRESSION_CCITT_T6;
      else if (ui.jpegCheckBox->isChecked()
               && style != DDJVU_FORMAT_MSBTOLSB 
               && TIFFFindCODEC(COMPRESSION_JPEG))
        compression = COMPRESSION_JPEG;
      else if (TIFFFindCODEC(COMPRESSION_PACKBITS))
        compression = COMPRESSION_PACKBITS;
      fmt = ddjvu_format_create(style, 0, 0);
      ddjvu_format_set_row_order(fmt, 1);
      ddjvu_format_set_gamma(fmt, 2.2);
      // set parameters
      int quality = ui.jpegSpinBox->value();
      TIFFSetField(tiff, TIFFTAG_IMAGEWIDTH, (uint32)rect.w);
      TIFFSetField(tiff, TIFFTAG_IMAGELENGTH, (uint32)rect.h);
      TIFFSetField(tiff, TIFFTAG_XRESOLUTION, (float)(dpi));
      TIFFSetField(tiff, TIFFTAG_YRESOLUTION, (float)(dpi));
      TIFFSetField(tiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
      TIFFSetField(tiff, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
      TIFFSetField(tiff, TIFFTAG_ROWSPERSTRIP, (uint32)64);
      if (style == DDJVU_FORMAT_MSBTOLSB) {
        TIFFSetField(tiff, TIFFTAG_BITSPERSAMPLE, (uint16)1);
        TIFFSetField(tiff, TIFFTAG_SAMPLESPERPIXEL, (uint16)1);
        TIFFSetField(tiff, TIFFTAG_FILLORDER, FILLORDER_MSB2LSB);
        TIFFSetField(tiff, TIFFTAG_COMPRESSION, compression);
        TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISWHITE);
      } else {
        TIFFSetField(tiff, TIFFTAG_BITSPERSAMPLE, (uint16)8);
        TIFFSetField(tiff, TIFFTAG_COMPRESSION, compression);
        if (compression == COMPRESSION_JPEG)
          TIFFSetField(tiff, TIFFTAG_JPEGQUALITY, quality);
        if (style == DDJVU_FORMAT_GREY8) {
          TIFFSetField(tiff, TIFFTAG_SAMPLESPERPIXEL, (uint16)1);
          TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
        } else {
          TIFFSetField(tiff, TIFFTAG_SAMPLESPERPIXEL, (uint16)3);
          TIFFSetField(tiff, TIFFTAG_COMPRESSION, compression);
          if (compression == COMPRESSION_JPEG) {
            TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_YCBCR);
            TIFFSetField(tiff, TIFFTAG_JPEGCOLORMODE, JPEGCOLORMODE_RGB);
          } else 
            TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
        }
      }
      // render and save
      int rowsize = rect.w * 3;
      if (style == DDJVU_FORMAT_MSBTOLSB)
        rowsize = (rect.w + 7) / 8;
      else if (style == DDJVU_FORMAT_GREY8)
        rowsize = rect.w;
      if (! (image = (char*)malloc(rowsize * rect.h)))
        message = tr("Out of memory.");
      else if (rowsize != TIFFScanlineSize(tiff))
        message = tr("Internal error.");
      else
        {
          char *s = image;
          for (int i=0; i<(int)rect.h; i++, s+=rowsize)
            TIFFWriteScanline(tiff, s, i, 0);
        }
    }
#else
  message = tr("TIFF export has not been compiled.");
#endif
  if (! message.isEmpty())
    error(message, __FILE__, __LINE__);
  if (fmt)
    ddjvu_format_release(fmt);
  if (image)
    free(image);
}



// -------------------
// QDJVIEWIMGEXPORTER



class QDjViewImgExporter : public QDjViewPageExporter
{
  Q_OBJECT
public:
  QDjViewImgExporter(QDialog *parent, QDjView *djview);
  virtual bool canExportPageRange()  { return false; }
  static QString filters();
protected:
  virtual void doPage();
  //Ui::QDjViewExportImg ui;
  //QPointer<QWidget> page;
  //QString group;
};


QDjViewImgExporter::QDjViewImgExporter(QDialog *parent, QDjView *djview)
  : QDjViewPageExporter(parent, djview)
{
}


void
QDjViewImgExporter::doPage()
{
  QDjVuPage *page = curPage;
  // determine rectangle
  ddjvu_rect_t rect;
  int imgdpi = ddjvu_page_get_resolution(*page);
  int dpi = imgdpi; // no max resolution yet
  rect.x = rect.y = 0;
  rect.w = ( ddjvu_page_get_width(*page) * dpi + imgdpi/2 ) / imgdpi;
  rect.h = ( ddjvu_page_get_height(*page) * dpi + imgdpi/2 ) / imgdpi;
  // prepare format
  ddjvu_format_t *fmt = 0;
  unsigned int masks[4] = { 0xff0000, 0xff00, 0xff, 0xff000000 };
  fmt = ddjvu_format_create(DDJVU_FORMAT_RGBMASK32, 3, masks);
  ddjvu_format_set_row_order(fmt, true);
  ddjvu_format_set_gamma(fmt, 2.2);
  // determine mode
  ddjvu_render_mode_t mode = DDJVU_RENDER_COLOR;
  QDjVuWidget::DisplayMode displayMode;
  displayMode = djview->getDjVuWidget()->displayMode();
  if (displayMode == QDjVuWidget::DISPLAY_STENCIL)
    mode = DDJVU_RENDER_BLACK;
  else if (displayMode == QDjVuWidget::DISPLAY_BG)
    mode = DDJVU_RENDER_BACKGROUND;
  else if (displayMode == QDjVuWidget::DISPLAY_FG)
    mode = DDJVU_RENDER_FOREGROUND;
  // compute image
  QString message;
  QString suffix = QFileInfo(fileName).suffix();
  QImage img(rect.w, rect.h, QImage::Format_RGB32);
  if (! ddjvu_page_render(*page, mode, &rect, &rect, fmt,
                          img.bytesPerLine(), (char*)img.bits() ))
    message = tr("Cannot render page.");
  else if (suffix.isEmpty())
    message = tr("File name has no file format suffix.");
  else
    {
      QFile file(fileName);
      QImageWriter writer(&file, suffix.toLatin1());
      if (! writer.write(img))
        {
          message = file.errorString();
          file.remove();
          if (writer.error() == QImageWriter::UnsupportedFormatError)
            message = tr("Image format %1 not supported.")
              .arg(suffix.toUpper());
#if HAVE_STRERROR
          if (file.error() == QFile::OpenError && errno > 0)
            message = strerror(errno);
#endif
        }
    }
  if (! message.isEmpty())
    error(message, __FILE__, __LINE__);
  if (fmt)
    ddjvu_format_release(fmt);
}


QString
QDjViewImgExporter::filters()
{
  QStringList patterns;
  foreach(QByteArray format, QImageWriter::supportedImageFormats())
    patterns << "*." + QString(format).toLower();
  return tr("Image files (%1)").arg(patterns.join(" "));
}



// --------------------
// QDJVIEWPRNEXPORTER








// =======================================
// QDJVIEWSAVEDIALOG
// =======================================


#include "ui_qdjviewsavedialog.h"


struct QDjViewSaveDialog::Private
{
  QDjView *djview;
  QDjVuDocument *document;
  Ui::QDjViewSaveDialog ui;
  bool stopping;

  int exporterIndex;
  QList<QDjViewExporter*> exporters;
  QStringList exporterFilters;
  QStringList exporterExtensions;
};




QDjViewSaveDialog::QDjViewSaveDialog(QDjView *djview)
  : QDialog(djview), d(0)
{
  d = new QDjViewSaveDialog::Private;

  d->djview = djview;
  d->document = 0;
  d->stopping = false;
  d->exporterIndex = 0;

  d->ui.setupUi(this);
  setAttribute(Qt::WA_GroupLeader, true);

  connect(d->ui.okButton, SIGNAL(clicked()), 
          this, SLOT(start()));
  connect(d->ui.stopButton, SIGNAL(clicked()), 
          this, SLOT(stop()));
  connect(d->ui.browseButton, SIGNAL(clicked()), 
          this, SLOT(browse()));
  connect(d->ui.formatCombo, SIGNAL(activated(int)), 
          this, SLOT(refresh()));
  connect(d->ui.resetButton, SIGNAL(clicked()), 
          this, SLOT(reset()));
  connect(d->ui.fileNameEdit, SIGNAL(textChanged(QString)), 
          this, SLOT(refresh()));
  connect(djview, SIGNAL(documentClosed(QDjVuDocument*)),
          this, SLOT(clear()));
  connect(djview, SIGNAL(documentReady(QDjVuDocument*)),
          this, SLOT(refresh()));

  setWhatsThis(tr("<html><b>Saving.</b><br/> "
                  "You can save the whole document or a page range "
                  "under a variety of formats. Selecting certain "
                  "formats creates additional dialog pages for "
                  "specifying format options.</html>"));

  addExporter(tr("DjVu Bundled Document"),
              tr("DjVu Files (*.djvu *.djv)"), ".djvu",
              new QDjViewDjVuExporter(this, d->djview, false));
  addExporter(tr("DjVu Indirect Document"),
              tr("DjVu Files (*.djvu *.djv)"), ".djvu",
              new QDjViewDjVuExporter(this, d->djview, true));
#if HAVE_TIFF
  addExporter(tr("TIFF"),
              tr("TIFF Files (*.tif *.tiff)"), ".tif",
              new QDjViewTiffExporter(this, d->djview, "TIFF"));
#endif
  addExporter(tr("PostScript"),
              tr("PostScript Files (*.ps)"), ".ps",
              new QDjViewPSExporter(this, d->djview, "PS", false));
  addExporter(tr("Encapsulated PostScript"),
              tr("Encapsulated PostScript Files (*.eps)"), ".eps",
              new QDjViewPSExporter(this, d->djview, "EPS", true));
  addExporter(tr("Other image formats"),
              QDjViewImgExporter::filters(), ".jpg",
              new QDjViewImgExporter(this, d->djview));
  refresh();
}



void 
QDjViewSaveDialog::refresh()
{
  bool nodoc = !d->document;
  if (nodoc && d->djview->pageNum() > 0)
    {
      nodoc = false;
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
      d->ui.toPageCombo->setCurrentIndex(d->djview->pageNum()-1);
    }
  bool notxt = d->ui.fileNameEdit->text().isEmpty();
  bool nojob = true;
  bool nofmt = true;
  bool noopt = true;
  bool nopag = true;
  int oldIndex = d->exporterIndex;
  d->exporterIndex = d->ui.formatCombo->currentIndex();
  QDjViewExporter *exporter = currentExporter();
  if (exporter && d->exporterIndex != oldIndex)
    {
      // update tabs
      while (d->ui.tabWidget->count() > 1)
        d->ui.tabWidget->removeTab(1);
      QWidget *w;
      for (int i=0; i<exporter->propertyPages(); i++)
        if ((w = exporter->propertyPage(i)))
          d->ui.tabWidget->addTab(w, w->objectName());
      // update filename
      QString fname = d->ui.fileNameEdit->text();
      QString ext = d->exporterExtensions[d->exporterIndex]; 
      QFileInfo finfo(fname);
      if (!finfo.completeSuffix().isEmpty() && !ext.isEmpty())
        fname = QFileInfo(finfo.dir(), finfo.baseName() + ext).filePath();
      d->ui.fileNameEdit->setText(fname);
    }
  if (exporter)
    {
      nofmt = false;
      if (exporter->status()>=DDJVU_JOB_STARTED)
        nojob = false;
      if (exporter->propertyPages() > 0)
        noopt = false;
      disconnect(d->ui.fromPageCombo, 0, d->ui.toPageCombo, 0);
      d->ui.documentButton->setEnabled(true);
      d->ui.toPageCombo->setEnabled(true);
      d->ui.toLabel->setEnabled(true);
      if (exporter->canExportPageRange() || exporter->canExportOnePage())
        {
          nopag = false;
          if (! exporter->canExportPageRange())
            {
              connect(d->ui.fromPageCombo, SIGNAL(activated(int)),
                      d->ui.toPageCombo, SLOT(setCurrentIndex(int)));
              d->ui.toPageCombo->setEnabled(false);
              d->ui.toLabel->setEnabled(false);
              d->ui.documentButton->setEnabled(false);
              if (d->ui.documentButton->isChecked())
                d->ui.currentPageButton->setChecked(true);
              if (! nodoc)
                {
                  int page = d->ui.fromPageCombo->currentIndex();
                  d->ui.toPageCombo->setCurrentIndex(page);
                }
            }
        }
    }
  d->ui.destinationGroupBox->setEnabled(nojob && !nodoc);
  d->ui.saveGroupBox->setEnabled(nojob && !nodoc && !nofmt && !nopag);
  d->ui.resetButton->setEnabled(nojob && !nodoc && !noopt);
  d->ui.okButton->setEnabled(nojob && !nodoc && !notxt && !nofmt);
  d->ui.cancelButton->setEnabled(nojob);
  d->ui.stopButton->setEnabled(!nojob);
  d->ui.stackedWidget->setCurrentIndex(nojob ? 0 : 1);
}


void 
QDjViewSaveDialog::clear()
{
  d->stopping = true;
  reject();
}


void 
QDjViewSaveDialog::reset()
{
  QDjViewExporter *exporter = currentExporter();
  if (exporter) 
    exporter->resetOptions();
  refresh();
}


void 
QDjViewSaveDialog::start()
{
  QString fname = d->ui.fileNameEdit->text();
  QFileInfo info(fname);
  if (info.exists())
    {
      QString docname = d->djview->getDocumentFileName();
      if (info == QFileInfo(docname) && ! docname.isEmpty())
        {
          QMessageBox::critical(this, 
                                tr("Error - DjView", "dialog caption"),
                                tr("Overwriting the current file "
                                   "is not allowed!" ) );
          return;
        }
      if ( QMessageBox::question(this, 
                                 tr("Question - DjView", "dialog caption"),
                                 tr("A file with this name already exists.\n"
                                    "Do you want to replace it?"),
                                 tr("&Replace"),
                                 tr("&Cancel") ))
        return;
    }
  
  QDjViewExporter *exporter = currentExporter();
  if (exporter)
    {
      int lastPage = d->djview->pageNum()-1;
      int curPage = d->djview->getDjVuWidget()->page();
      int fromPage = d->ui.fromPageCombo->currentIndex();
      int toPage = d->ui.toPageCombo->currentIndex();
      exporter->setFileName(fname);
      if (d->ui.currentPageButton->isChecked())
        exporter->setPageRange(curPage, curPage);
      else if (d->ui.pageRangeButton->isChecked())
        exporter->setPageRange(fromPage, toPage);
      else
        exporter->setPageRange(0, lastPage);
      // ignition!
      exporter->run();
    }
  refresh();
}


void 
QDjViewSaveDialog::progress(int percent)
{
  QDjViewExporter *exporter = currentExporter();
  ddjvu_status_t jobstatus = DDJVU_JOB_NOTSTARTED;
  if (exporter)
    jobstatus = exporter->status();
  d->ui.progressBar->setValue(percent);
  switch(jobstatus)
    {
    case DDJVU_JOB_OK:
      QTimer::singleShot(0, this, SLOT(accept()));
      break;
    case DDJVU_JOB_FAILED:
      exporter->error(tr("This operation has failed."),
                      __FILE__, __LINE__);
      break;
    case DDJVU_JOB_STOPPED:
      exporter->error(tr("This operation has been interrupted."),
                      __FILE__, __LINE__);
      break;
    default:
      break;
    }
}


void 
QDjViewSaveDialog::stop()
{
  QDjViewExporter *exporter = currentExporter();
  if (exporter && exporter->status() == DDJVU_JOB_STARTED)
    {
      exporter->stop();
      d->ui.stopButton->setEnabled(false);
      d->stopping = true;
    }
}


void 
QDjViewSaveDialog::browse()
{
  QString fname = d->ui.fileNameEdit->text();
  QDjViewExporter *exporter = currentExporter();
  QString filters;
  if (exporter)
    filters += d->exporterFilters[d->exporterIndex] + ";;";
  filters += tr("All files", "save filter") + " (*)";
  fname = QFileDialog::getSaveFileName(this, 
                                       tr("Save - DjView", "dialog caption"),
                                       fname, filters, 0,
                                       QFileDialog::DontConfirmOverwrite);
  if (! fname.isEmpty())
    {
      QFileInfo finfo(fname);
      QString ext = d->exporterExtensions[d->exporterIndex]; 
      if (finfo.completeSuffix().isEmpty() && !ext.isEmpty())
        fname = QFileInfo(finfo.dir(), finfo.baseName() + ext).filePath();
      d->ui.fileNameEdit->setText(fname);
    }
}


void 
QDjViewSaveDialog::done(int reason)
{
  setEnabled(false);
  QDjViewExporter *exporter = currentExporter();
  if (!d->stopping && exporter && exporter->status()==DDJVU_JOB_STARTED)
    {
      stop();
      return;
    }
  foreach(exporter, d->exporters)
    delete exporter;
  d->exporters.clear();
  QDialog::done(reason);
}


void 
QDjViewSaveDialog::closeEvent(QCloseEvent *event)
{
  QDjViewExporter *exporter = currentExporter();
  if (!d->stopping && exporter && exporter->status()==DDJVU_JOB_STARTED)
    {
      stop();
      event->ignore();
      return;
    }
  foreach(exporter, d->exporters)
    delete exporter;
  d->exporters.clear();
  event->accept();
  QDialog::closeEvent(event);
}


QDjViewExporter *
QDjViewSaveDialog::currentExporter()
{
  if (d->exporterIndex >= 0)
    if (d->exporterIndex < d->exporters.count())
      return d->exporters.at(d->exporterIndex);
  return 0;
}


int 
QDjViewSaveDialog::addExporter(QString name, QString filter, QString extension,
                               QDjViewExporter *exporter)
{
  int index = d->exporters.count();
  d->ui.formatCombo->addItem(name, QVariant(index));
  d->ui.formatCombo->setCurrentIndex(0);
  d->exporterExtensions += extension;
  d->exporterFilters += filter;
  d->exporters += exporter;
  connect(exporter, SIGNAL(progress(int)), this, SLOT(progress(int)));
  exporter->setObjectName(name);
  exporter->setErrorCaption(tr("Save Error - DjView", "dialog caption"));
  return index;
}






// =======================================
// QDJVIEWPRINTDIALOG
// =======================================


#include "ui_qdjviewprintdialog.h"

struct QDjViewPrintDialog::Private
{
  QDjView *djview;
  QDjVuDocument *document;
  Ui::QDjViewPrintDialog ui;
  QDjViewExporter* exporter;
  QPrinter* printer;
  QPrintDialog *printDialog;
  bool stopping;
};


QDjViewPrintDialog::QDjViewPrintDialog(QDjView *djview)
  : QDialog(djview), d(0)
{
  d = new QDjViewPrintDialog::Private;
  
  d->djview = djview;
  d->document = 0;
  d->stopping = false;
  d->exporter = 0;
  d->printer = new QPrinter();
  d->printDialog = new QPrintDialog(d->printer, this);
  d->printDialog->addEnabledOption(QPrintDialog::PrintToFile);
  d->printDialog->addEnabledOption(QPrintDialog::PrintPageRange);
  d->printDialog->addEnabledOption(QPrintDialog::PrintCollateCopies);

  d->ui.setupUi(this);
  setAttribute(Qt::WA_GroupLeader, true);
  connect(d->ui.okButton, SIGNAL(clicked()), 
          this, SLOT(start()));
  connect(d->ui.stopButton, SIGNAL(clicked()), 
          this, SLOT(stop()));
  connect(d->ui.chooseButton, SIGNAL(clicked()), 
          this, SLOT(choose()));
  connect(d->ui.resetButton, SIGNAL(clicked()), 
          this, SLOT(reset()));
  connect(djview, SIGNAL(documentClosed(QDjVuDocument*)),
          this, SLOT(clear()));
  connect(djview, SIGNAL(documentReady(QDjVuDocument*)),
          this, SLOT(refresh()));

  setWhatsThis(tr("<html><b>Printing.</b><br/> "
                  "You can print the whole document or a page range. "
                  "Use the <tt>Choose</tt> button to select a print "
                  "destination and specify printer options. "
                  "Additional dialog tabs might appear "
                  "to specify conversion options.</html>"));

  QDjViewPrefs *prefs = QDjViewPrefs::instance();
  if (! prefs->printFile.isEmpty())
    d->printer->setOutputFileName(prefs->printFile);
  else if (!prefs->printerName.isEmpty())
    d->printer->setPrinterName(prefs->printerName);
  d->ui.reverseCheckBox->setChecked(prefs->printReverse);
  d->ui.collateCheckBox->setChecked(prefs->printCollate);
  d->ui.numCopiesSpinBox->setValue(1);

  setCurrentExporter();
  refresh();
}



QDjViewPrintDialog::~QDjViewPrintDialog()
{
  // save settings
  QDjViewPrefs *prefs = QDjViewPrefs::instance();
  prefs->printFile = d->printer->outputFileName();
  prefs->printerName = d->printer->printerName();
  prefs->printReverse = d->ui.reverseCheckBox->isChecked();
  prefs->printCollate = d->ui.collateCheckBox->isChecked();
  // delete
  delete d->exporter;
  delete d->printer;
  delete d->printDialog;
  delete d;
}


bool 
QDjViewPrintDialog::isPrinterValid()
{
  if (! d->printer->outputFileName().isEmpty())
    return true;
  if (! d->printer->printerName().isEmpty())
    return true;
  return false;
}


bool 
QDjViewPrintDialog::isPrinterPostScript()
{
  if (! d->printer->outputFileName().isEmpty())
    return true;
#ifdef Q_WS_WIN
  // TODO: use EnumPrinter to get a PRINTER_INFO_2 structure.
  //       check that the driver is the postscript driver.
  return false;
#else
  return true;
#endif
}


void 
QDjViewPrintDialog::setCurrentExporter()
{
  // destroy previous exporter
  if (d->exporter)
    {
      while (d->ui.tabWidget->count() > 1)
        d->ui.tabWidget->removeTab(1);
      delete d->exporter;
      d->exporter = 0;
    }
  // select exporter for printer
  if (isPrinterValid())
    {
      QPrinter *p = d->printer;
      QString group;
      if (p->outputFileName().isEmpty())
        group = p->printerName();
      QString gps = (group.isEmpty() ? "PS" : group);
#if HAS_PRNEXPORTER
      QString gprn = (group.isEmpty() ? "PRN" : group);
      if (! isPrinterPostScript())
        d->exporter = new QDjViewPrnExporter(this, d->djview, gprn);
      else
#endif
        d->exporter = new QDjViewPSExporter(this, d->djview, gps);
      d->exporter->setPrinter(d->printer, d->ui.numCopiesSpinBox->value());
      connect(d->exporter, SIGNAL(progress(int)), this, SLOT(progress(int)));
    }
  // update ui pages
  if (d->exporter)
    {
      QWidget *w;
      for (int i=0; i<d->exporter->propertyPages(); i++)
        if ((w = d->exporter->propertyPage(i)))
          d->ui.tabWidget->addTab(w, w->objectName());
    }
  // set destination label
  QString msg;
  if (d->exporter)
    {
      QString fileName = d->printer->outputFileName();
      QString printerName = d->printer->printerName();
      if (! fileName.isEmpty())
        msg = tr("File: %1").arg(fileName);
      else if (! printerName.isEmpty())
        msg = tr("Printer: %1").arg(printerName);
    }
  if (msg.isEmpty())
    msg = tr("No destination");
  QFontMetrics metric(d->ui.destinationLabel->font());
  int width = qMax(358, d->ui.destinationLabel->width()) - 8;
  msg = QItemDelegate::elidedText(metric, width, Qt::ElideMiddle, msg);
  d->ui.destinationLabel->setText(msg);
}


void 
QDjViewPrintDialog::choose()
{
  // update printdialog
  if (d->exporter)
    d->exporter->iniPrinter(d->printer);
  d->printer->setNumCopies(d->ui.numCopiesSpinBox->value());
  d->printer->setCollateCopies(d->ui.collateCheckBox->isChecked());
  if (d->ui.reverseCheckBox->isChecked())
    d->printer->setPageOrder(QPrinter::LastPageFirst);
  else
    d->printer->setPageOrder(QPrinter::FirstPageFirst);
  int npages = d->djview->pageNum();
  int fPage = d->ui.fromPageCombo->currentIndex();
  int lPage = d->ui.toPageCombo->currentIndex();
  int cPage = d->djview->getDjVuWidget()->page();
  d->printDialog->setMinMax(1, npages);
  if (d->ui.documentButton->isChecked())
    d->printDialog->setPrintRange(QPrintDialog::AllPages);
  else
    d->printDialog->setPrintRange(QPrintDialog::PageRange);
  if (d->ui.currentPageButton->isChecked())
    d->printDialog->setFromTo(cPage+1, cPage+1);
  else
    d->printDialog->setFromTo(qMin(fPage,lPage)+1, qMax(fPage,lPage)+1);
  // exec print dialog
  if (d->printDialog->exec() == QDialog::Accepted)
    {
      // update ui
      setCurrentExporter();
      if (d->printer->numCopies() > 1)
        d->ui.numCopiesSpinBox->setValue(d->printer->numCopies());
      d->ui.collateCheckBox->setChecked(d->printer->collateCopies());
      QPrinter::PageOrder order = d->printer->pageOrder();
      d->ui.reverseCheckBox->setChecked(order == QPrinter::LastPageFirst);
      QPrintDialog::PrintRange range = d->printDialog->printRange();
      if (range == QPrintDialog::AllPages)
        d->ui.documentButton->setChecked(true);
      else
        {
          fPage = qBound(1, d->printDialog->fromPage(), npages) - 1;
          lPage = qBound(1, d->printDialog->toPage(), npages) -1;
          d->ui.fromPageCombo->setCurrentIndex(fPage);
          d->ui.toPageCombo->setCurrentIndex(lPage);
          if (fPage == lPage && fPage == cPage)
            d->ui.currentPageButton->setChecked(true);
          else
            d->ui.pageRangeButton->setChecked(true);
        }  
      refresh();
    }
}


void 
QDjViewPrintDialog::refresh()
{
  bool nodoc = !d->document;
  if (nodoc && d->djview->pageNum() > 0)
    {
      nodoc = false;
      d->document = d->djview->getDocument();
      d->djview->fillPageCombo(d->ui.fromPageCombo);
      d->ui.fromPageCombo->setCurrentIndex(0);
      d->djview->fillPageCombo(d->ui.toPageCombo);
      d->ui.toPageCombo->setCurrentIndex(d->djview->pageNum()-1);
    }
  bool nojob = true;
  bool noexp = true;
  bool noopt = true;
  bool nopag = true;
  if (d->exporter)
    {
      noexp = false;
      if (d->exporter->status()>=DDJVU_JOB_STARTED)
        nojob = false;
      if (d->exporter->propertyPages() > 0)
        noopt = false;
      disconnect(d->ui.fromPageCombo, 0, d->ui.toPageCombo, 0);
      d->ui.documentButton->setEnabled(true);
      d->ui.toPageCombo->setEnabled(true);
      d->ui.toLabel->setEnabled(true);
      d->ui.collateCheckBox->setEnabled(d->exporter->canCollate());
      if (! d->exporter->canCollate())
        d->ui.collateCheckBox->setChecked(true);
      if (d->exporter->canExportPageRange() || 
          d->exporter->canExportOnePage())
        {
          nopag = false;
          if (! d->exporter->canExportPageRange())
            {
              connect(d->ui.fromPageCombo, SIGNAL(activated(int)),
                      d->ui.toPageCombo, SLOT(setCurrentIndex(int)));
              d->ui.toPageCombo->setEnabled(false);
              d->ui.toLabel->setEnabled(false);
              d->ui.documentButton->setEnabled(false);
              if (d->ui.documentButton->isChecked())
                d->ui.currentPageButton->setChecked(true);
              if (! nodoc)
                {
                  int page = d->ui.fromPageCombo->currentIndex();
                  d->ui.toPageCombo->setCurrentIndex(page);
                }
            }
        }
    }
  d->ui.destinationGroupBox->setEnabled(nojob && !nodoc);
  d->ui.printGroupBox->setEnabled(nojob && !nodoc && !noexp && !nopag);
  d->ui.copiesGroupBox->setEnabled(nojob && !nodoc && !noexp && !nopag);
  d->ui.resetButton->setEnabled(nojob && !nodoc);
  d->ui.okButton->setEnabled(nojob && !nodoc && !noexp);
  d->ui.cancelButton->setEnabled(nojob);
  d->ui.stopButton->setEnabled(!nojob);
  d->ui.stackedWidget->setCurrentIndex(nojob ? 0 : 1);
}


void 
QDjViewPrintDialog::clear()
{
  d->stopping = true;
  reject();
}


void 
QDjViewPrintDialog::reset()
{
  if (d->exporter) 
    d->exporter->resetOptions();
  d->ui.numCopiesSpinBox->setValue(1);
  d->ui.collateCheckBox->setChecked(true);
  d->ui.reverseCheckBox->setChecked(false);
  refresh();
}


void 
QDjViewPrintDialog::start()
{
  // start conversion
  if (d->exporter)
    {
      int npages = d->djview->pageNum();
      int fPage = d->ui.fromPageCombo->currentIndex();
      int lPage = d->ui.toPageCombo->currentIndex();
      int cPage = d->djview->getDjVuWidget()->page();
      if (d->ui.currentPageButton->isChecked())
        fPage =lPage = cPage;
      if (d->ui.documentButton->isChecked())
        { fPage = 0; lPage = npages-1; }
      if (d->ui.reverseCheckBox->isChecked() ^ (fPage > lPage))
        qSwap(fPage, lPage);
      d->printer->setNumCopies(d->ui.numCopiesSpinBox->value());
      d->printer->setCollateCopies(d->ui.collateCheckBox->isChecked());
      d->exporter->setPrinter(d->printer, d->ui.numCopiesSpinBox->value());
      d->exporter->setPageRange(fPage, lPage);
      d->exporter->run();
    }
  // refresh ui
  refresh();
}


void 
QDjViewPrintDialog::progress(int percent)
{
  ddjvu_status_t jobstatus = DDJVU_JOB_NOTSTARTED;
  if (d->exporter)
    jobstatus = d->exporter->status();
  d->ui.progressBar->setValue(percent);
  switch(jobstatus)
    {
    case DDJVU_JOB_OK:
      QTimer::singleShot(0, this, SLOT(accept()));
      break;
    case DDJVU_JOB_FAILED:
      d->exporter->error(tr("This operation has failed."),
                         __FILE__, __LINE__);
      break;
    case DDJVU_JOB_STOPPED:
      d->exporter->error(tr("This operation has been interrupted."),
                         __FILE__, __LINE__);
      break;
    default:
      break;
    }
}


void 
QDjViewPrintDialog::stop()
{
  if (d->exporter && d->exporter->status() == DDJVU_JOB_STARTED)
    {
      d->exporter->stop();
      d->ui.stopButton->setEnabled(false);
      d->stopping = true;
    }
}


void 
QDjViewPrintDialog::done(int reason)
{
  setEnabled(false);
  if (!d->stopping && d->exporter && 
      d->exporter->status()==DDJVU_JOB_STARTED )
    {
      stop();
      return;
    }
  delete d->exporter;
  d->exporter = 0;
  QDialog::done(reason);
}


void 
QDjViewPrintDialog::closeEvent(QCloseEvent *event)
{
  if (!d->stopping && d->exporter && 
      d->exporter->status() == DDJVU_JOB_STARTED)
    {
      stop();
      event->ignore();
      return;
    }
  delete d->exporter;
  d->exporter = 0;
  event->accept();
  QDialog::closeEvent(event);
}







// ----------------------------------------
// MOC

#include "qdjviewdialogs.moc"




/* -------------------------------------------------------------
   Local Variables:
   c++-font-lock-extra-types: ( "\\sw+_t" "[A-Z]\\sw*[a-z]\\sw*" )
   End:
   ------------------------------------------------------------- */


