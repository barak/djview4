//C-  -*- C++ -*-
//C- -------------------------------------------------------------------
//C- DjView4
//C- Copyright (c) 2006-  Leon Bottou
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

#if AUTOCONF
# include "config.h"
#else
# define HAVE_STRING_H     1
# define HAVE_SYS_TYPES_H  1
# define HAVE_STRERROR     1
# ifndef WIN32
#  define HAVE_UNISTD_H     1
# endif
#endif

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#if HAVE_SYS_TYPES_H
# include <sys/types.h>
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
#include <algorithm>

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
#include <QTextDocument>
#include <QTimer>
#include <QVBoxLayout>
#include <QVector>
#if QT_VERSION >= 0x50E00
# define tr8 tr
#else
# define tr8 trUtf8 
#endif

#include <libdjvu/miniexp.h>
#include <libdjvu/ddjvuapi.h>

#include "qdjview.h"
#include "qdjviewprefs.h"
#include "qdjviewdialogs.h"
#include "qdjviewexporters.h"
#include "qdjvuwidget.h"
#include "qdjvu.h"




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
#if QT_VERSION >= 0x50000
  msg = msg.toHtmlEscaped();
#else
  msg = Qt::escape(msg);
#endif
  // Ignore empty and duplicate messages
  if (msg.isEmpty()) return;
  if (!d->messages.isEmpty() && msg == d->messages[0]) return;
  // Add message
  d->messages.prepend(msg);
  while (d->messages.size() >= 16)
    d->messages.removeLast();
  compose();
  // For verbose mode
  QByteArray msga = msg.toLocal8Bit();
  qWarning("%s", msga.constData());
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
// QDJVIEWAUTHDIALOG
// =======================================

#include "ui_qdjviewauthdialog.h"

struct QDjViewAuthDialog::Private
{
  Ui_QDjViewAuthDialog ui;
};

QDjViewAuthDialog::~QDjViewAuthDialog()
{
  delete d;
}

QDjViewAuthDialog::QDjViewAuthDialog(QWidget *parent)
  : QDialog(parent), 
    d(new Private)
{
  d->ui.setupUi(this);
  setWindowTitle(tr("Authentication required - DjView"));
}

QString 
QDjViewAuthDialog::user() const
{
  return d->ui.userLineEdit->text();
}

QString 
QDjViewAuthDialog::pass() const
{
  return d->ui.passLineEdit->text();
}

void 
QDjViewAuthDialog::setInfo(QString why)
{
#if QT_VERSION >= 0x50000
  QString ewhy = why.toHtmlEscaped();
#else
  QString ewhy = Qt::escape(why);
#endif
  QString txt = QString("<html>%1</html>").arg(ewhy);
  d->ui.whyLabel->setText(txt);
}

void 
QDjViewAuthDialog::setUser(QString user)
{
  d->ui.userLineEdit->setText(user);
}

void 
QDjViewAuthDialog::setPass(QString pass)
{
  d->ui.passLineEdit->setText(pass);
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
  font.setStyleHint(QFont::Monospace);
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
                      "to display detailed information in the <tt>File</tt> "
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
            msg = tr8("Page #%1 - \302\253 %2 \302\273") // << .. >>
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
          msg << ((filenum==1) ? tr("1 file") : tr("%n files", 0, filenum));
          msg << ((pagenum==1) ? tr("1 page") : tr("%n pages", 0, pagenum));
        }
      d->ui.docLabel->setText(msg.join(" - "));
    }
}

void 
QDjViewInfoDialog::fillDocTable()
{
  int filenum = d->files.size();
  QTableWidget *table = d->ui.docTable;
  table->setRowCount(filenum);
  table->setSortingEnabled(false);
  for (int i=0; i<filenum; i++)
    fillDocRow(i);
  table->resizeColumnsToContents();
  table->resizeRowsToContents();
  table->horizontalHeader()->setStretchLastSection(true);
}

void 
QDjViewInfoDialog::fillDocRow(int i)
{
  ddjvu_fileinfo_t &info = d->files[i];
  QTableWidget *table = d->ui.docTable;

  QTableWidgetItem *numItem = new QTableWidgetItem();
  numItem->setText(QString("%1 ").arg(i+1,5));
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
#if QT_VERSION >= 0x50000
  d->ui.docTable->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
#else
  d->ui.docTable->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
#endif
  d->ui.docTable->verticalHeader()->hide();
  d->ui.pageTable->setColumnCount(2);
  d->ui.pageTable->setHorizontalHeaderLabels(labels);
  d->ui.pageTable->horizontalHeader()->setHighlightSections(false);
  d->ui.pageTable->horizontalHeader()->setStretchLastSection(true);
#if QT_VERSION >= 0x50000
  d->ui.pageTable->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
#else
  d->ui.pageTable->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
#endif
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
  std::sort(keys.begin(), keys.end());
  int nkeys = keys.size();
  table->setRowCount(nkeys);
  table->setSortingEnabled(false);
  table->setWordWrap(false);
  for(int j = 0; j < nkeys; j++)
    {
      QTableWidgetItem *kitem = new QTableWidgetItem(keys[j]);
      QTableWidgetItem *vitem = new QTableWidgetItem(m[keys[j]]);
      kitem->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
      vitem->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
      table->setItem(j, 0, kitem);
      table->setItem(j, 1, vitem);
    }
  table->resizeColumnToContents(0);
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
// QDJVIEWSAVEDIALOG
// =======================================


#include "ui_qdjviewsavedialog.h"


struct QDjViewSaveDialog::Private
{
  QDjView *djview;
  QDjVuDocument *document;
  Ui::QDjViewSaveDialog ui;
  QDjViewExporter *exporter;
  bool stopping;
};


QDjViewSaveDialog::~QDjViewSaveDialog()
{
  delete d;
}


QDjViewSaveDialog::QDjViewSaveDialog(QDjView *djview)
  : QDialog(djview), d(0)
{
  d = new QDjViewSaveDialog::Private;
  
  d->djview = djview;
  d->document = 0;
  d->stopping = false;
  d->exporter = 0;

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
  connect(d->ui.fileNameEdit, SIGNAL(textChanged(QString)), 
          this, SLOT(refresh()));
  connect(djview, SIGNAL(documentClosed(QDjVuDocument*)),
          this, SLOT(clear()));
  connect(djview, SIGNAL(documentReady(QDjVuDocument*)),
          this, SLOT(refresh()));
  
  setWhatsThis(tr("<html><b>Saving.</b><br/> "
                  "You can save the whole document or a page range. "
                  "The bundled format creates a single file. "
                  "The indirect format creates multiple files "
                  "suitable for web serving.</html>"));
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
  QDjViewExporter *exporter = d->exporter;
  QString exporterName = "DJVU/BUNDLED";
  if (d->ui.formatCombo->currentIndex() > 0)
    exporterName = "DJVU/INDIRECT";
  if (exporter && 
      exporter->status() <= DDJVU_JOB_STARTED &&
      exporter->name() != exporterName )
    {
      exporter->saveProperties();
      delete exporter;
      exporter = d->exporter = 0;
    }
  if (! exporter)
    {
      exporter = QDjViewExporter::create(this, d->djview, exporterName);
      if (exporter)
        {
          exporter->loadProperties();
          connect(exporter, SIGNAL(progress(int)), this, SLOT(progress(int)));
          d->exporter = exporter;
        }
    }
  if (exporter)
    {
      nofmt = false;
      if (exporter->status() >= DDJVU_JOB_STARTED)
        nojob = false;
    }
  d->ui.destinationGroupBox->setEnabled(nojob && !nodoc);
  d->ui.saveGroupBox->setEnabled(nojob && !nodoc && !nofmt);
  d->ui.formatCombo->setEnabled(nojob && !nodoc && !nofmt);
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
  QDjViewExporter *exporter = d->exporter;
  if (exporter)
    {
      int lastPage = d->djview->pageNum()-1;
      int curPage = d->djview->getDjVuWidget()->page();
      int fromPage = d->ui.fromPageCombo->currentIndex();
      int toPage = d->ui.toPageCombo->currentIndex();
      if (d->ui.currentPageButton->isChecked())
        exporter->setFromTo(curPage, curPage);
      else if (d->ui.pageRangeButton->isChecked())
        exporter->setFromTo(fromPage, toPage);
      else
        exporter->setFromTo(0, lastPage);
      // ignition!
      exporter->save(fname);
    }
  refresh();
}


void 
QDjViewSaveDialog::progress(int percent)
{
  ddjvu_status_t jobstatus = DDJVU_JOB_NOTSTARTED;
  QDjViewExporter *exporter = d->exporter;
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
  QDjViewExporter *exporter = d->exporter;
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
  QStringList info = QDjViewExporter::info("DJVU/BUNDLED");
  QString filters = tr("All files", "save filter") + " (*)";
  QString suffix;
  if (info.size() > 3)
    filters = info[3] + ";;" + filters;
  if (info.size() > 1)
    suffix = info[1];
  fname = QFileDialog::getSaveFileName(this, 
                                       tr("Save - DjView", "dialog caption"),
                                       fname, filters, 0,
                                       QFileDialog::DontConfirmOverwrite);
  if (! fname.isEmpty())
    {
      QFileInfo finfo(fname);
      if (finfo.suffix().isEmpty() && !suffix.isEmpty())
        fname = QFileInfo(finfo.dir(), finfo.completeBaseName() 
                          + "." + suffix).filePath();
      d->ui.fileNameEdit->setText(fname);
    }
}


void 
QDjViewSaveDialog::done(int reason)
{
  setEnabled(false);
  QDjViewExporter *exporter = d->exporter;
  if (!d->stopping && exporter && exporter->status()==DDJVU_JOB_STARTED)
    {
      stop();
      return;
    }
  if (exporter)
    exporter->saveProperties();
  delete exporter;
  d->exporter = 0;
  QDialog::done(reason);
}


void 
QDjViewSaveDialog::closeEvent(QCloseEvent *event)
{
  QDjViewExporter *exporter = d->exporter;
  if (!d->stopping && exporter && exporter->status()==DDJVU_JOB_STARTED)
    {
      stop();
      event->ignore();
      return;
    }
  if (exporter)
    exporter->saveProperties();
  delete exporter;
  d->exporter = 0;
  event->accept();
  QDialog::closeEvent(event);
}





// =======================================
// QDJVIEWEXPORTDIALOG
// =======================================


#include "ui_qdjviewexportdialog.h"


struct QDjViewExportDialog::Private
{
  QDjView *djview;
  QDjVuDocument *document;
  Ui::QDjViewExportDialog ui;
  bool stopping;
  QDjViewExporter *exporter;
  QStringList exporterNames;
};


QDjViewExportDialog::~QDjViewExportDialog()
{
  delete d;
}


QDjViewExportDialog::QDjViewExportDialog(QDjView *djview)
  : QDialog(djview), d(0)
{
  d = new QDjViewExportDialog::Private;

  d->djview = djview;
  d->document = 0;
  d->stopping = false;
  d->exporter = 0;

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

  foreach (QString name, QDjViewExporter::names())
    if (!name.startsWith("DJVU") && !name.startsWith("PRN"))
      {
        QStringList info = QDjViewExporter::info(name);
        if (info.size() > 2)
          {
            d->exporterNames << name;
            d->ui.formatCombo->addItem(info[2], QVariant(name));
          }
      }
  d->ui.formatCombo->setCurrentIndex(0);
  refresh();
}



void 
QDjViewExportDialog::refresh()
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
  QDjViewExporter *exporter = d->exporter;
  int formatIndex = d->ui.formatCombo->currentIndex();
  QString formatName = d->ui.formatCombo->itemData(formatIndex).toString();
  if (exporter && 
      exporter->status() <= DDJVU_JOB_STARTED &&
      exporter->name() != formatName )
    {
      exporter->saveProperties();
      delete exporter;
      exporter = d->exporter = 0;
    }
  if (! exporter)
    {
      exporter = QDjViewExporter::create(this, d->djview, formatName);
      if (exporter)
        {
          exporter->loadProperties();
          connect(exporter, SIGNAL(progress(int)), this, SLOT(progress(int)));
          d->exporter = exporter;
          // update tabs
          while (d->ui.tabWidget->count() > 1)
            d->ui.tabWidget->removeTab(1);
          QWidget *w;
          for (int i=0; i<exporter->propertyPages(); i++)
            if ((w = exporter->propertyPage(i)))
              d->ui.tabWidget->addTab(w, w->objectName());
          // update fname
          QString fname = d->ui.fileNameEdit->text();
          QStringList info = QDjViewExporter::info(formatName);
          if (info.size() > 1)
            {
              QString suffix = info[1];
              QFileInfo finfo(fname);
              if (!finfo.suffix().isEmpty() && !suffix.isEmpty())
                fname = QFileInfo(finfo.dir(), finfo.completeBaseName() 
                                  + "." + suffix).filePath();
              d->ui.fileNameEdit->setText(fname);
            }
          // update exportgroupbox
          disconnect(d->ui.fromPageCombo, 0, d->ui.toPageCombo, 0);
          d->ui.documentButton->setEnabled(true);
          d->ui.toPageCombo->setEnabled(true);
          d->ui.toLabel->setEnabled(true);
          if (exporter->exportOnePageOnly())
            {
              connect(d->ui.fromPageCombo, SIGNAL(activated(int)),
                      d->ui.toPageCombo, SLOT(setCurrentIndex(int)));
              d->ui.toPageCombo->setEnabled(false);
              d->ui.toLabel->setEnabled(false);
              d->ui.documentButton->setEnabled(false);
              if (d->ui.documentButton->isChecked())
                d->ui.currentPageButton->setChecked(true);
              int page = d->ui.fromPageCombo->currentIndex();
              d->ui.toPageCombo->setCurrentIndex(page);
            }
        }
    }
  if (exporter)
    {
      nofmt = false;
      if (exporter->status() >= DDJVU_JOB_STARTED)
        nojob = false;
      if (exporter->propertyPages() > 0)
        noopt = false;
      QWidget *w = 0;
      for (int i=0; i<exporter->propertyPages(); i++)
        if ((w = exporter->propertyPage(i)))
          w->setEnabled(nojob);
    }
  d->ui.destinationGroupBox->setEnabled(nojob && !nodoc);
  d->ui.saveGroupBox->setEnabled(nojob && !nodoc && !nofmt);
  d->ui.resetButton->setEnabled(nojob && !nodoc && !noopt);
  d->ui.okButton->setEnabled(nojob && !nodoc && !notxt && !nofmt);
  d->ui.cancelButton->setEnabled(nojob);
  d->ui.stopButton->setEnabled(!nojob);
  d->ui.stackedWidget->setCurrentIndex(nojob ? 0 : 1);
}


void 
QDjViewExportDialog::clear()
{
  d->stopping = true;
  reject();
}


void 
QDjViewExportDialog::reset()
{
  QDjViewExporter *exporter = d->exporter;
  if (exporter) 
    exporter->resetProperties();
  refresh();
}


void 
QDjViewExportDialog::start()
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
  QDjViewExporter *exporter = d->exporter;
  if (exporter)
    {
      int lastPage = d->djview->pageNum()-1;
      int curPage = d->djview->getDjVuWidget()->page();
      int fromPage = d->ui.fromPageCombo->currentIndex();
      int toPage = d->ui.toPageCombo->currentIndex();
      if (d->ui.currentPageButton->isChecked())
        exporter->setFromTo(curPage, curPage);
      else if (d->ui.pageRangeButton->isChecked())
        exporter->setFromTo(fromPage, toPage);
      else
        exporter->setFromTo(0, lastPage);
      // ignition!
      exporter->save(fname);
    }
  refresh();
}


void 
QDjViewExportDialog::progress(int percent)
{
  QDjViewExporter *exporter = d->exporter;
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
QDjViewExportDialog::stop()
{
  QDjViewExporter *exporter = d->exporter;
  if (exporter && exporter->status() == DDJVU_JOB_STARTED)
    {
      exporter->stop();
      d->ui.stopButton->setEnabled(false);
      d->stopping = true;
    }
}


void 
QDjViewExportDialog::browse()
{
  QString fname = d->ui.fileNameEdit->text();
  QDjViewExporter *exporter = d->exporter;
  QString format = (exporter) ? exporter->name() : QString();
  QStringList info = QDjViewExporter::info(format);
  QString filters = tr("All files", "save filter") + " (*)";
  QString suffix;
  if (info.size() > 3)
    filters = info[3] + ";;" + filters;
  if (info.size() > 1)
    suffix = info[1];
  fname = QFileDialog::getSaveFileName(this, 
                                       tr("Export - DjView", "dialog caption"),
                                       fname, filters, 0,
                                       QFileDialog::DontConfirmOverwrite);
  if (! fname.isEmpty())
    {
      QFileInfo finfo(fname);
      if (finfo.suffix().isEmpty() && !suffix.isEmpty())
        fname = QFileInfo(finfo.dir(), finfo.completeBaseName() 
                          + "." + suffix).filePath();
      d->ui.fileNameEdit->setText(fname);
    }
}


void 
QDjViewExportDialog::done(int reason)
{
  setEnabled(false);
  QDjViewExporter *exporter = d->exporter;
  if (!d->stopping && exporter && exporter->status()==DDJVU_JOB_STARTED)
    {
      stop();
      return;
    }
  if (exporter)
    exporter->saveProperties();
  delete exporter;
  d->exporter = 0;
  QDialog::done(reason);
}


void 
QDjViewExportDialog::closeEvent(QCloseEvent *event)
{
  QDjViewExporter *exporter = d->exporter;
  if (!d->stopping && exporter && exporter->status()==DDJVU_JOB_STARTED)
    {
      stop();
      event->ignore();
      return;
    }
  if (exporter)
    exporter->saveProperties();
  delete exporter;
  d->exporter = 0;
  event->accept();
  QDialog::closeEvent(event);
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
  QDjViewExporter *exporter;
  QString groupname;
  QPrinter *printer;
  bool stopping;
};


QDjViewPrintDialog::~QDjViewPrintDialog()
{
  if (d && d->printer)
     delete d->printer;
  if (d)
    delete d;
}


QDjViewPrintDialog::QDjViewPrintDialog(QDjView *djview)
  : QDialog(djview), d(0)
{
  d = new QDjViewPrintDialog::Private;
  
  d->djview = djview;
  d->document = 0;
  d->stopping = false;
  d->exporter = 0;
  d->printer = new QPrinter(QPrinter::HighResolution);
  d->ui.setupUi(this);
  setAttribute(Qt::WA_GroupLeader, true);
  connect(d->ui.okButton, SIGNAL(clicked()), 
          this, SLOT(start()));
  connect(d->ui.stopButton, SIGNAL(clicked()), 
          this, SLOT(stop()));
  connect(d->ui.resetButton, SIGNAL(clicked()), 
          this, SLOT(reset()));
  connect(djview, SIGNAL(documentClosed(QDjVuDocument*)),
          this, SLOT(clear()));
  connect(djview, SIGNAL(documentReady(QDjVuDocument*)),
          this, SLOT(refresh()));
  connect(d->ui.printToFileCheckBox, SIGNAL(clicked()),
          this, SLOT(refresh()));
  connect(d->ui.fileNameEdit, SIGNAL(textChanged(QString)), 
          this, SLOT(refresh()));
  connect(d->ui.printerButton, SIGNAL(clicked()),
          this, SLOT(choose()));
  connect(d->ui.browseButton, SIGNAL(clicked()),
          this, SLOT(browse()));
  
  setWhatsThis(tr("<html><b>Printing.</b><br/> "
                  "You can print the whole document or a page range. "
                  "Use the <tt>Choose</tt> button to select a print "
                  "destination and specify printer options. Additional "
                  "dialog tabs might appear to specify conversion "
                  "options.</html>"));

  // Load preferences
  d->ui.printerLabel->setText(QString());
  d->ui.fileNameEdit->setText("print.ps");
  QDjViewPrefs *prefs = QDjViewPrefs::instance();
  QString printerName = prefs->printerName;
  QString printFile = prefs->printFile;
  if (! printFile.isEmpty())
    {
      d->ui.printToFileCheckBox->setChecked(true);
      d->ui.fileNameEdit->setText(printFile);
      printerName = QString();
    }
  else if (! printerName.isEmpty())
    {
      d->ui.printToFileCheckBox->setChecked(false);
      d->ui.printerLabel->setText(prefs->printerName);
      d->printer->setPrinterName(printerName);
    }
  else
    {
      printFile = "print.ps";
      d->ui.printToFileCheckBox->setChecked(true);
      d->ui.fileNameEdit->setText(printFile);
    }
  d->printer->setCollateCopies(prefs->printCollate);
  if (prefs->printReverse)
    d->printer->setPageOrder(QPrinter::LastPageFirst);
  else
    d->printer->setPageOrder(QPrinter::FirstPageFirst);
  
  // Create exporter
#ifdef Q_OS_WIN
  if (! d->exporter)
    d->exporter = QDjViewExporter::create(this, d->djview, "PRN");
#endif
  if (! d->exporter)
    d->exporter = QDjViewExporter::create(this, d->djview, "PS");
  if (! d->exporter)
    d->exporter = QDjViewExporter::create(this, d->djview, "PRN");
  if (d->exporter)
    {
      QDjViewExporter *exporter = d->exporter;
      exporter->loadProperties("Print-" + d->exporter->name());
      // setup exporter property pages
      QWidget *w;
      for (int i=0; i<exporter->propertyPages(); i++)
        if ((w = exporter->propertyPage(i)))
          d->ui.tabWidget->addTab(w, w->objectName());
      // setup exporter connections
      connect(exporter, SIGNAL(progress(int)), this, SLOT(progress(int)));
    }
  // Refresh Ui
  refresh();
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
  bool tofile = d->ui.printToFileCheckBox->isChecked();
  bool noexp = !d->exporter;
  bool nojob = true;
  bool notxt = true;
  if (tofile) 
    {
      notxt = d->ui.fileNameEdit->text().isEmpty();
    } 
  else if (d->printer)
    {
      QString pName = d->printer->printerName();
      notxt = pName.isEmpty();
      d->ui.printerLabel->setText(pName);
      if (notxt)
        d->ui.printerLabel->setText(tr("(invalid printer)"));            
    }
  if (d->exporter)
    {
      noexp = false;
      if (d->exporter->status() >= DDJVU_JOB_STARTED)
        nojob = false;
      QWidget *w = 0;
      for (int i=0; i<d->exporter->propertyPages(); i++)
        if ((w = d->exporter->propertyPage(i)))
          w->setEnabled(nojob);
    }
  d->ui.pageFile->setEnabled(tofile);
  d->ui.pagePrinter->setEnabled(!tofile);
  d->ui.destStackedWidget->setCurrentIndex(tofile ? 0 : 1);
  d->ui.destinationGroupBox->setEnabled(nojob && !noexp);
  d->ui.printGroupBox->setEnabled(nojob && !nodoc && !noexp);
  d->ui.resetButton->setEnabled(!noexp && nojob && !nodoc);
  d->ui.okButton->setEnabled(nojob && !nodoc && !notxt && !noexp);
  d->ui.cancelButton->setEnabled(nojob);
  d->ui.stopButton->setEnabled(!nojob);
  d->ui.stackedWidget->setCurrentIndex(nojob ? 0 : 1);
}


void 
QDjViewPrintDialog::browse()
{
  QString fname = d->ui.fileNameEdit->text();
  QDjViewExporter *exporter = d->exporter;
  QString format = (exporter) ? exporter->name() : QString();
  QStringList info = QDjViewExporter::info(format);
  QString filters = tr("All files", "save filter") + " (*)";
  QString suffix;
  if (info.size() > 3)
    filters = info[3] + ";;" + filters;
  if (info.size() > 1)
    suffix = info[1];
  fname = QFileDialog::getSaveFileName(this, 
                                       tr("Print To File - DjView", 
                                          "dialog caption"),
                                       fname, filters);
  if (! fname.isEmpty())
    {
      QFileInfo finfo(fname);
      if (finfo.suffix().isEmpty() && !suffix.isEmpty())
        fname = QFileInfo(finfo.dir(), finfo.completeBaseName() 
                          + "." + suffix).filePath();
      d->ui.fileNameEdit->setText(fname);
    }
}


void 
QDjViewPrintDialog::choose()
{
  if (d->exporter)
    {
      d->exporter->savePrintSetup(d->printer);
      QPrintDialog *dialog = new QPrintDialog(d->printer, this);
      // options
      QPrintDialog::PrintDialogOptions options = dialog->enabledOptions();
      options |= QPrintDialog::PrintCollateCopies;
      options &= ~QPrintDialog::PrintToFile;
      options &= ~QPrintDialog::PrintSelection;
      dialog->setEnabledOptions(options);
      // copy page spec
      int pagenum = d->djview->pageNum();
      int curpage = d->djview->getDjVuWidget()->page();
      int frompage = d->ui.fromPageCombo->currentIndex();
      int topage = d->ui.toPageCombo->currentIndex();
      dialog->setMinMax(1, pagenum);
      dialog->setFromTo(1, pagenum);
      dialog->setPrintRange(QPrintDialog::PageRange);
      if (d->ui.currentPageButton->isChecked())
        dialog->setFromTo(curpage+1, curpage+1);
      else if (d->ui.pageRangeButton->isChecked())
        dialog->setFromTo(frompage+1, topage+1);
      else
        dialog->setPrintRange(QPrintDialog::AllPages);
      // exec
      if (dialog->exec() == QDialog::Accepted)
        {
          d->exporter->loadPrintSetup(d->printer, dialog);
          // copy page spec back
          frompage = qBound(0, dialog->fromPage()-1, pagenum-1);
          topage = qBound(0, dialog->toPage()-1, pagenum-1);
          if (dialog->printRange() == QPrintDialog::AllPages)
            {
              d->ui.documentButton->setChecked(true);
              d->ui.fromPageCombo->setCurrentIndex(0);
              d->ui.toPageCombo->setCurrentIndex(pagenum-1);
            }
          else
            {
              d->ui.fromPageCombo->setCurrentIndex(frompage);
              d->ui.toPageCombo->setCurrentIndex(topage);
              if (frompage == curpage && topage == curpage)
                d->ui.currentPageButton->setChecked(true);
              else
                d->ui.pageRangeButton->setChecked(true);
            }
        }
      delete dialog;
    }
  refresh();
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
  QDjViewExporter *exporter = d->exporter;
  if (exporter) 
    exporter->resetProperties();
  refresh();
}


void 
QDjViewPrintDialog::start()
{
  // start conversion
  QDjViewExporter *exporter = d->exporter;
  if (exporter)
    {
      int lastPage = d->djview->pageNum()-1;
      int curPage = d->djview->getDjVuWidget()->page();
      int fromPage = d->ui.fromPageCombo->currentIndex();
      int toPage = d->ui.toPageCombo->currentIndex();
      if (d->ui.currentPageButton->isChecked())
        exporter->setFromTo(curPage, curPage);
      else if (d->ui.pageRangeButton->isChecked())
        exporter->setFromTo(fromPage, toPage);
      else
        exporter->setFromTo(0, lastPage);
      // ignition!
      if (d->ui.printToFileCheckBox->isChecked())
        {
          QString fname = d->ui.fileNameEdit->text();
          QFileInfo info(fname);
          if (info.exists() &&
              QMessageBox::question(this, 
                                    tr("Question - DjView", "dialog caption"),
                                    tr("A file with this name already exists.\n"
                                       "Do you want to replace it?"),
                                    tr("&Replace"),
                                    tr("&Cancel") ))
            return;
          // save preferences
          QDjViewPrefs *prefs = QDjViewPrefs::instance();
          prefs->printerName = QString();
          prefs->printFile = fname;
          // print to file
          exporter->save(fname);
        }
      else
        {
          QPrinter *printer = d->printer;
          // save preferences
          QDjViewPrefs *prefs = QDjViewPrefs::instance();
          prefs->printFile = QString();
          prefs->printerName = printer->printerName();
          // print
          exporter->print(d->printer);
        }
    }
  // refresh ui
  refresh();
}


void 
QDjViewPrintDialog::progress(int percent)
{
  ddjvu_status_t jobstatus = DDJVU_JOB_NOTSTARTED;
  QDjViewExporter *exporter = d->exporter;
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
QDjViewPrintDialog::stop()
{
  QDjViewExporter *exporter = d->exporter;
  if (exporter && exporter->status() == DDJVU_JOB_STARTED)
    {
      exporter->stop();
      d->ui.stopButton->setEnabled(false);
      d->stopping = true;
    }
}


void 
QDjViewPrintDialog::done(int reason)
{
  setEnabled(false);
  QDjViewExporter *exporter = d->exporter;
  if (!d->stopping && exporter && exporter->status()==DDJVU_JOB_STARTED )
    {
      stop();
      return;
    }
  if (exporter)
    exporter->saveProperties("Printer-" + exporter->name());
  delete exporter;
  d->exporter = 0;
  QDialog::done(reason);
}


void 
QDjViewPrintDialog::closeEvent(QCloseEvent *event)
{
  QDjViewExporter *exporter = d->exporter;
  if (!d->stopping && exporter && exporter->status() == DDJVU_JOB_STARTED)
    {
      stop();
      event->ignore();
      return;
    }
  if (exporter)
    exporter->saveProperties("Printer-" + exporter->name());
  delete exporter;
  d->exporter = 0;
  event->accept();
  QDialog::closeEvent(event);
}






/* -------------------------------------------------------------
   Local Variables:
   c++-font-lock-extra-types: ( "\\sw+_t" "[A-Z]\\sw*[a-z]\\sw*" )
   End:
   ------------------------------------------------------------- */


