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
#  define HAVE_SYS_WAIT_H  1
#  define HAVE_WAITPID     1
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
# include <tiff.h>
# include <tiffio.h>
# include <tiffconf.h>
# include "tiff2pdf.h"
#endif
#ifdef WIN32
# include <io.h>
#endif

#include <QApplication>
#include <QCheckBox>
#include <QDebug>
#include <QDir>
#include <QDialog>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QImageWriter>
#include <QList>
#include <QMap>
#include <QMessageBox>
#include <QObject>
#include <QPointer>
#include <QPrinter>
#include <QPrintDialog>
#include <QPrintEngine>
#include <QRegExp>
#include <QSettings>
#include <QSpinBox>
#include <QString>
#include <QTemporaryFile>
#include <QTimer>

#include <libdjvu/miniexp.h>
#include <libdjvu/ddjvuapi.h>

#include "qdjview.h"
#include "qdjviewprefs.h"
#include "qdjviewdialogs.h"
#include "qdjviewexporters.h"
#include "qdjvuwidget.h"
#include "qdjvu.h"

#ifdef WIN32
# define wdup(fh) _dup(fh)
#else
# define wdup(fh) (fh)
#endif

// ----------------------------------------
// FACTORY


typedef QDjViewExporter* (*exporter_creator_t)(QDialog*,QDjView*,QString);

static QStringList exporterNames;
static QMap<QString,QStringList> exporterInfo;
static QMap<QString,exporter_creator_t> exporterCreators;


static void
addExporterData(QString name, QString suffix, 
                QString lname, QString filter,
                exporter_creator_t creator)
{
  QStringList info;
  info << name << suffix << lname << filter;
  exporterNames << name;
  exporterInfo[name] = info;
  exporterCreators[name] = creator;
}


static void createExporterData();


QDjViewExporter *
QDjViewExporter::create(QDialog *dialog, QDjView *djview, QString name)
{
  if (exporterNames.isEmpty())
    createExporterData();
  if (exporterCreators.contains(name))
    return (*exporterCreators[name])(dialog, djview, name);
  return 0;
}

QStringList 
QDjViewExporter::names()
{
  if (exporterNames.isEmpty())
    createExporterData();
  return exporterNames;
}

QStringList 
QDjViewExporter::info(QString name)
{
  if (exporterNames.isEmpty())
    createExporterData();
  if (exporterInfo.contains(name))
    return exporterInfo[name];
  return QStringList();
}





// ----------------------------------------
// QDJVIEWEXPORTER



QDjViewExporter::~QDjViewExporter()
{ 
}


bool 
QDjViewExporter::exportOnePageOnly()
{ 
  return false; 
}


void 
QDjViewExporter::resetProperties() 
{ 
  // reset all properties to default values
}


void 
QDjViewExporter::loadProperties(QString) 
{ 
  // fill properties from settings keyed by string
}


void 
QDjViewExporter::saveProperties(QString) 
{ 
  // save properties into settings keyed by string
}


bool
QDjViewExporter::loadPrintSetup(QPrinter*, QPrintDialog*) 
{ 
  // fill properties using printer and print dialog data
  return false;
}


bool
QDjViewExporter::savePrintSetup(QPrinter*) 
{ 
  // save properties into printer settings
  return false;
}


int 
QDjViewExporter::propertyPages() 
{ 
  return 0; 
}


QWidget* 
QDjViewExporter::propertyPage(int) 
{ 
  return 0; 
}


void 
QDjViewExporter::setFromTo(int fp, int tp) 
{
  fromPage = fp;
  toPage = tp;
}


void 
QDjViewExporter::setErrorCaption(QString caption)
{
  errorCaption = caption;
}


ddjvu_status_t 
QDjViewExporter::status() 
{ 
  return DDJVU_JOB_NOTSTARTED; 
}


QString 
QDjViewExporter::name()
{
  return exporterName;
}


bool
QDjViewExporter::print(QPrinter *) 
{ 
  qWarning("QDjViewExporter does not support printing.");
  return false;
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
      errorDialog->setWindowModality(Qt::WindowModal);
    }
  errorDialog->error(message, filename, lineno);
  errorDialog->show();
}


QDjViewExporter::QDjViewExporter(QDialog *parent, QDjView *djview, QString name)
  : parent(parent), djview(djview), 
    errorDialog(0),
    exporterName(name),
    fromPage(0), 
    toPage(-1)
{
}






// ----------------------------------------
// QDJVIEWDJVUEXPORTER




class QDjViewDjVuExporter : public QDjViewExporter
{
  Q_OBJECT
public:
  static QDjViewExporter *create(QDialog*, QDjView*, QString);
  static void setup();
public:
  ~QDjViewDjVuExporter();
  QDjViewDjVuExporter(QDialog *parent, QDjView *djview, 
                      QString name, bool indirect);
  virtual bool save(QString fileName);
  virtual void stop();
  virtual ddjvu_status_t status();
protected:
  QFile file;
  QDjVuJob *job;
  FILE *output;
  bool indirect;
  bool failed;
};


QDjViewExporter*
QDjViewDjVuExporter::create(QDialog *parent, QDjView *djview, QString name)
{
  if (name == "DJVU/BUNDLED")
    return new QDjViewDjVuExporter(parent, djview, name, false);
  if (name == "DJVU/INDIRECT")
    return new QDjViewDjVuExporter(parent, djview, name, true);
  return 0;
}


void
QDjViewDjVuExporter::setup()
{
  addExporterData("DJVU/BUNDLED", "djvu",
                  tr("DjVu Bundled Document"),
                  tr("DjVu Files (*.djvu *.djv)"),
                  QDjViewDjVuExporter::create);
  addExporterData("DJVU/INDIRECT", "djvu",
                  tr("DjVu Indirect Document"),
                  tr("DjVu Files (*.djvu *.djv)"),
                  QDjViewDjVuExporter::create);
}


QDjViewDjVuExporter::QDjViewDjVuExporter(QDialog *parent, QDjView *djview, 
                                         QString name, bool indirect)
  : QDjViewExporter(parent, djview, name),
    job(0), 
    output(0),
    indirect(indirect), 
    failed(false)
{
}


QDjViewDjVuExporter::~QDjViewDjVuExporter()
{
  QIODevice::OpenMode mode = file.openMode();
  ddjvu_status_t st = status();
  if (st == DDJVU_JOB_STARTED)
	if (job)
      ddjvu_job_stop(*job);
  if (output)
    ::fclose(output);
  output = 0;
  if (file.openMode())
    file.close();
  if (st != DDJVU_JOB_OK)
      if (mode & (QIODevice::WriteOnly|QIODevice::Append))
        file.remove();
  if (djview)
    ddjvu_cache_clear(djview->getDjVuContext());
}


bool 
QDjViewDjVuExporter::save(QString fname)
{
  QDjVuDocument *document = djview->getDocument();
  int pagenum = djview->pageNum();
  if (output || document==0 || pagenum <= 0 || fname.isEmpty())
    return false;
  QFileInfo info(fname);
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
    return false;
  toPage = qBound(0, toPage, pagenum-1);
  fromPage = qBound(0, fromPage, pagenum-1);
  QByteArray pagespec;
  if (fromPage == toPage && pagenum > 1)
    pagespec.append(QString("--pages=%1").arg(fromPage+1));
  else if (fromPage != 0 || toPage != pagenum - 1)
    pagespec.append(QString("--pages=%1-%2").arg(fromPage+1).arg(toPage+1));
  QByteArray namespec;
  if (indirect)
    namespec = "--indirect=" + fname.toUtf8();

  int argc = 0;
  const char *argv[2];
  if (! namespec.isEmpty())
    argv[argc++] = namespec.data();
  if (! pagespec.isEmpty())
    argv[argc++] = pagespec.data();
  
  file.setFileName(fname);
  file.remove();
  if (file.open(QIODevice::WriteOnly))
    output = ::fdopen(wdup(file.handle()), "wb");
  if (! output)
    {
      failed = true;
      QString message = tr("Unknown error.");
      if (! file.errorString().isEmpty())
        message = tr("System error: %1.").arg(file.errorString());
      error(message, __FILE__, __LINE__);
      return false;
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
      return false;
    }
  job = new QDjVuJob(pjob, this);
  connect(job, SIGNAL(error(QString,QString,int)),
          this, SLOT(error(QString,QString,int)) );
  connect(job, SIGNAL(progress(int)),
          this, SIGNAL(progress(int)) );
  emit progress(0);
  return true;
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





// ----------------------------------------
// QDJVIEWPSEXPORTER


#include "ui_qdjviewexportps1.h"
#include "ui_qdjviewexportps2.h"
#include "ui_qdjviewexportps3.h"


class QDjViewPSExporter : public QDjViewExporter
{
  Q_OBJECT
public:
  static QDjViewExporter *create(QDialog*, QDjView*, QString);
  static void setup();
public:
  ~QDjViewPSExporter();
  QDjViewPSExporter(QDialog *parent, QDjView *djview, 
                    QString name, bool eps);
  virtual bool exportOnePageOnly();
  virtual void resetProperties();
  virtual void loadProperties(QString group);
  virtual void saveProperties(QString group);
  virtual bool loadPrintSetup(QPrinter *printer, QPrintDialog *dialog);
  virtual bool savePrintSetup(QPrinter *printer);
  virtual int propertyPages();
  virtual QWidget* propertyPage(int num);
  virtual bool save(QString fileName);
  virtual bool print(QPrinter *printer);
  virtual ddjvu_status_t status();
  virtual void stop();
public slots:
  void refresh();
protected:
  bool start();
  void openFile();
  void closeFile();
protected:
  QFile file;
  QDjVuJob *job;
  FILE *output;
  int outputfd;
  int copies;
  bool collate;
  bool lastfirst;
  bool duplex;
  bool failed;
  bool encapsulated;
  QPrinter *printer;
  QPointer<QWidget> page1;
  QPointer<QWidget> page2;
  QPointer<QWidget> page3;
  Ui::QDjViewExportPS1 ui1;
  Ui::QDjViewExportPS2 ui2;
  Ui::QDjViewExportPS3 ui3;
};


QDjViewExporter*
QDjViewPSExporter::create(QDialog *parent, QDjView *djview, QString name)
{
  if (name == "PS")
    return new QDjViewPSExporter(parent, djview, name, false);
  if (name == "EPS")
    return new QDjViewPSExporter(parent, djview, name, true);
  return 0;
}


void
QDjViewPSExporter::setup()
{
  addExporterData("PS", "ps",
                  tr("PostScript"),
                  tr("PostScript Files (*.ps *.eps)"),
                  QDjViewPSExporter::create);
  addExporterData("EPS", "eps",
                  tr("Encapsulated PostScript"),
                  tr("PostScript Files (*.ps *.eps)"),
                  QDjViewPSExporter::create);
}


QDjViewPSExporter::~QDjViewPSExporter()
{
  // cleanup output
  QIODevice::OpenMode mode = file.openMode();
  ddjvu_status_t st = status();
  if (st == DDJVU_JOB_STARTED)
      if (job)
        ddjvu_job_stop(*job);
  closeFile();
  if (st != DDJVU_JOB_OK)
	if (mode & (QIODevice::WriteOnly|QIODevice::Append))
        file.remove();
  // delete property pages
  delete page1;
  delete page2;
  delete page3;
}


QDjViewPSExporter::QDjViewPSExporter(QDialog *parent, QDjView *djview, 
                                     QString name, bool eps)
  : QDjViewExporter(parent, djview, name),
    job(0), 
    output(0),
    outputfd(-1),
    copies(1),
    collate(true),
    lastfirst(false),
    duplex(false),
    failed(false),
    encapsulated(eps),
    printer(0)
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
  resetProperties();
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
                         "Option <tt>Scale to fit</tt> accommodates "
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


bool
QDjViewPSExporter::exportOnePageOnly()
{
  return encapsulated;
}


void 
QDjViewPSExporter::resetProperties()
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


void
QDjViewPSExporter::loadProperties(QString group)
{
  // load settings (ui1)
  QSettings s;
  if (group.isEmpty()) 
    group = "Export-" + name();
  s.beginGroup(group);
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
}


void
QDjViewPSExporter::saveProperties(QString group)
{
  // save properties
  QSettings s;
  if (group.isEmpty()) 
    group = "Export-" + name();
  s.beginGroup(group);
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
}


bool
QDjViewPSExporter::loadPrintSetup(QPrinter *printer, QPrintDialog *dialog)
{
  bool grayscale = (printer->colorMode() == QPrinter::GrayScale);
  bool landscape = (printer->orientation() == QPrinter::Landscape);
  ui1.grayScaleButton->setChecked(grayscale);  
  ui1.colorButton->setChecked(!grayscale);  
  ui2.landscapeButton->setChecked(landscape);
  ui2.portraitButton->setChecked(!landscape);
  // BEGIN HACK //
  // Directly steal settings from print dialog.
  copies = 1;
  collate = true;
  lastfirst = false;
#if QT_VERSION >= 0x50000
  QSpinBox* lcop = dialog->findChild<QSpinBox*>("copies");
  QCheckBox* lcol = dialog->findChild<QCheckBox*>("collate");
  QCheckBox* lplf = dialog->findChild<QCheckBox*>("reverse");
#elif QT_VERSION >= 0x40400
  QSpinBox* lcop = qFindChild<QSpinBox*>(dialog, "copies");
  QCheckBox* lcol = qFindChild<QCheckBox*>(dialog, "collate");
  QCheckBox* lplf = qFindChild<QCheckBox*>(dialog, "reverse");
#else
  QSpinBox* lcop = qFindChild<QSpinBox*>(dialog, "sbNumCopies");
  QCheckBox* lcol = qFindChild<QCheckBox*>(dialog, "chbCollate");
  QCheckBox* lplf = qFindChild<QCheckBox*>(dialog, "chbPrintLastFirst");
#endif
  if (lcop)
    copies = qMax(1, lcop->value());
  if (lcol)
    collate = lcol->isChecked();
  if (lplf)
    lastfirst = lplf->isChecked();
  // END HACK //
  return true;
}


bool
QDjViewPSExporter::savePrintSetup(QPrinter *printer)
{
  bool g = ui1.grayScaleButton->isChecked();
  bool l = ui2.landscapeButton->isChecked();
  printer->setColorMode(g ? QPrinter::GrayScale : QPrinter::Color);
  printer->setOrientation(l ? QPrinter::Landscape : QPrinter::Portrait);
  return true;
}


int 
QDjViewPSExporter::propertyPages()
{
  return (encapsulated) ? 1 : 3;
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
QDjViewPSExporter::openFile()
{
  if (printer)
    {
      file.close();
      if (! printer->outputFileName().isEmpty())
        file.setFileName(printer->outputFileName());
    }
  if (! file.fileName().isEmpty())
    {
      file.remove();
      if (file.open(QIODevice::WriteOnly))
        output = ::fdopen(wdup(file.handle()), "wb");
    }
  else if (printer)
    {
      QString printerName = printer->printerName();
      // disable sigpipe
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
      // Arguments for lp/lpr
      QVector<const char*> lpargs;
      QVector<const char*> lprargs;
      lpargs << "lp";
      lprargs << "lpr";
      QByteArray pname = printerName.toLocal8Bit();
      if (! pname.isEmpty())
        {
          lpargs << "-d";
          lpargs << pname.data();
          lprargs << "-P";
          lprargs << pname.data();
        }
      // Arguments for cups.
      bool cups = false;
      QList<QByteArray> cargs;
      QPrintEngine *engine = printer->printEngine();
# define PPK_CupsOptions QPrintEngine::PrintEnginePropertyKey(0xfe00)
# define PPK_CupsStringPageSize QPrintEngine::PrintEnginePropertyKey(0xfe03)
      QVariant cPageSize = engine->property(PPK_CupsStringPageSize);
      QVariant cOptions = engine->property(PPK_CupsOptions);
      if (! cPageSize.toString().isEmpty())
        {
          cups = true;
          cargs << "-o" << "media=" + cPageSize.toString().toLocal8Bit();
        }
      QStringList options = cOptions.toStringList();
      if (! options.isEmpty())
        {
          cups = true;
          QStringList::const_iterator it = options.constBegin();
          while (it != options.constEnd())
            {
              cargs << "-o";
              cargs << (*it).toLocal8Bit() + "=" + (*(it+1)).toLocal8Bit();
              it += 2;
            }
        }
      QByteArray bCopies = QByteArray::number(copies);
      if (cups)
        {
          if (collate)
            cargs << "-o" << "Collate=True";
          if (lastfirst)
            cargs << "-o" << "outputorder=reverse";
          lastfirst = false;
          if (duplex)
            cargs << "-o" << "sides=two-sided-long-edge";
          // add cups option for number of copies
          if (copies > 1)
            {
              lprargs << "-#" << bCopies.data();
              lpargs << "-n" << bCopies.data();
            }
          // add remaining cups options
          for(int i=0; i<cargs.size(); i++)
            {
              lprargs << cargs.at(i).data();
              lpargs << cargs.at(i).data();
            }
          // reset copies since cups takes care of it
          copies = 1;
        }
      // open pipe for lp/lpr.
      lpargs << 0;
      lprargs << 0;
#ifndef WIN32
      int fds[2];
      if (pipe(fds) == 0)
        {
          int pid = fork();
          if (pid == 0)
            {
              // rehash file descriptors
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
              int status = ::dup(fds[0]);
              for (int i=1; i<nfd; i++)
                ::close(i);
              if (status >= 0 && fork() == 0)
                {
                  // try lp and lpr
                  (void)execvp("lpr", (char**)lprargs.data());
                  (void)execv("/bin/lpr", (char**)lprargs.data());
                  (void)execv("/usr/bin/lpr", (char**)lprargs.data());
                  (void)execvp("lp", (char**)lpargs.data());
                  (void)execv("/bin/lp", (char**)lpargs.data());
                  (void)execv("/usr/bin/lp", (char**)lpargs.data());
                }
              // exit without running destructors
              (void)execlp("true", "true", (char *)0);
              (void)execl("/bin/true", "true", (char *)0);
              (void)execl("/usr/bin/true", "true", (char *)0);
              ::_exit(0);
              ::exit(0);
            }
          ::close(fds[0]);
          outputfd = fds[1];
          output = fdopen(outputfd, "wb");
          if (pid >= 0)
            {
# if HAVE_WAITPID
              ::waitpid(pid, 0, 0);
# else
              ::wait(0);
# endif
            }
          else
            closeFile();
        }
#endif
    }
}


void 
QDjViewPSExporter::closeFile()
{
  if (output)
    ::fclose(output);
  if (outputfd >= 0)
    ::close(outputfd);
  if (file.openMode())
    file.close();
  output = 0;
  outputfd = -1;
  printer = 0;
}


bool
QDjViewPSExporter::save(QString fname)
{
  if (output) 
    return false;
  printer = 0;
  file.close();
  file.setFileName(fname);
  return start();
}


bool
QDjViewPSExporter::print(QPrinter *qprinter)
{
  if (output) 
    return false;
  printer = qprinter;
  file.close();
  file.setFileName(QString());
  QDjViewPrefs *prefs = QDjViewPrefs::instance();
  prefs->printReverse = lastfirst;
  prefs->printCollate = collate;
  return start();
}


bool 
QDjViewPSExporter::start()
{
  QDjViewPrefs *prefs = QDjViewPrefs::instance();
  QDjVuDocument *document = djview->getDocument();
  int pagenum = djview->pageNum();
  if (document==0 || pagenum <= 0)
    return false;
  int tpg = qBound(0, toPage, pagenum-1);
  int fpg = qBound(0, fromPage, pagenum-1);
  openFile();
  if (! output)
    return false;

  // prepare arguments
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

  // convert arguments
  int argc = args.count();
  QVector<const char*> argv(argc);
  for (int i=0; i<argc; i++)
    argv[i] = args[i].constData();

  // start print job
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
      return false;
    }
  job = new QDjVuJob(pjob, this);
  connect(job, SIGNAL(error(QString,QString,int)),
          this, SLOT(error(QString,QString,int)) );
  connect(job, SIGNAL(progress(int)),
          this, SIGNAL(progress(int)) );
  emit progress(0);
  refresh();
  return true;
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


// ----------------------------------------
// QDJVIEWPAGEEXPORTER


class QDjViewPageExporter : public QDjViewExporter
{
  Q_OBJECT
public:
  ~QDjViewPageExporter();
  QDjViewPageExporter(QDialog *parent, QDjView *djview, QString name);
  virtual ddjvu_status_t status() { return curStatus; }
public slots:
  virtual bool start();
  virtual void stop();
  virtual void error(QString message, QString filename, int lineno);
protected slots:
  void checkPage();
protected:
  virtual void openFile() {}
  virtual void closeFile() {}
  virtual void doFinal() {}
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


QDjViewPageExporter::QDjViewPageExporter(QDialog *parent, QDjView *djview,
                                         QString name)
  : QDjViewExporter(parent, djview, name),
    curStatus(DDJVU_JOB_NOTSTARTED), 
    curPage(0), curProgress(0)
{
}


bool 
QDjViewPageExporter::start()
{
  if (curStatus != DDJVU_JOB_STARTED)
    {
      curStatus = DDJVU_JOB_STARTED;
      openFile();
      checkPage();
    }
  if (curStatus <= DDJVU_JOB_OK)
    return true;
  return false;
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
        {
          curStatus = DDJVU_JOB_OK;
          doFinal();
        }
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





// ----------------------------------------
// QDJVIEWTIFFEXPORTER


#include "ui_qdjviewexporttiff.h"


class QDjViewTiffExporter : public QDjViewPageExporter
{
  Q_OBJECT
public:
  static QDjViewExporter *create(QDialog*, QDjView*, QString);
  static void setup();
public:
  ~QDjViewTiffExporter();
  QDjViewTiffExporter(QDialog *parent, QDjView *djview, QString name);
  virtual void resetProperties();
  virtual void loadProperties(QString group);
  virtual void saveProperties(QString group);
  virtual int propertyPages();
  virtual QWidget* propertyPage(int num);
  virtual bool save(QString fileName);
protected:
  virtual void closeFile();
  virtual void doPage();
protected:
  void checkTiffSupport();
  Ui::QDjViewExportTiff ui;
  QPointer<QWidget> page;
  QFile file;
#if HAVE_TIFF
  TIFF *tiff;
#else
  FILE *tiff;
#endif
};


QDjViewExporter*
QDjViewTiffExporter::create(QDialog *parent, QDjView *djview, QString name)
{
  if (name == "TIFF")
    return new QDjViewTiffExporter(parent, djview, name);
  return 0;
}


void
QDjViewTiffExporter::setup()
{
  addExporterData("TIFF", "tiff",
                  tr("TIFF Document"),
                  tr("TIFF Files (*.tiff *.tif)"),
                  QDjViewTiffExporter::create);
}


QDjViewTiffExporter::~QDjViewTiffExporter()
{
  closeFile();
  delete page;
}


QDjViewTiffExporter::QDjViewTiffExporter(QDialog *parent, QDjView *djview,
                                         QString name)
  : QDjViewPageExporter(parent, djview, name), 
    tiff(0)
{
  page = new QWidget();
  ui.setupUi(page);
  page->setObjectName(tr("TIFF Options", "tab caption"));
  resetProperties();
  page->setWhatsThis(tr("<html><b>TIFF options.</b><br>"
                        "The resolution box specifies an upper "
                        "limit for the resolution of the TIFF images. "
                        "Forcing bitonal G4 compression "
                        "encodes all pages in black and white "
                        "using the CCITT Group 4 compression. "
                        "Allowing JPEG compression uses lossy JPEG "
                        "for all non bitonal or subsampled images. "
                        "Otherwise, allowing deflate compression "
                        "produces more compact (but less portable) files "
                        "than the default packbits compression."
                        "</html>") );
}


void 
QDjViewTiffExporter::checkTiffSupport()
{
#ifdef CCITT_SUPPORT
  ui.bitonalCheckBox->setEnabled(true);
#else
  ui.bitonalCheckBox->setEnabled(false);
#endif
#ifdef JPEG_SUPPORT
  ui.jpegCheckBox->setEnabled(true);
  ui.jpegSpinBox->setEnabled(true);
#else
  ui.jpegCheckBox->setEnabled(false);
  ui.jpegSpinBox->setEnabled(false);
#endif
#ifdef ZIP_SUPPORT
  ui.deflateCheckBox->setEnabled(true);
#else
  ui.deflateCheckBox->setEnabled(false);
#endif
}


void     
QDjViewTiffExporter::resetProperties()
{
  ui.dpiSpinBox->setValue(600);
  ui.bitonalCheckBox->setChecked(false);
  ui.jpegCheckBox->setChecked(true);
  ui.jpegSpinBox->setValue(75);
  ui.deflateCheckBox->setChecked(true);
  checkTiffSupport();
}


void
QDjViewTiffExporter::loadProperties(QString group)
{
  QSettings s;
  if (group.isEmpty()) 
    group = "Export-" + name();
  s.beginGroup(group);
  ui.dpiSpinBox->setValue(s.value("dpi", 600).toInt());
  ui.bitonalCheckBox->setChecked(s.value("bitonal", false).toBool());
  ui.jpegCheckBox->setChecked(s.value("jpeg", true).toBool());
  ui.jpegSpinBox->setValue(s.value("jpegQuality", 75).toInt());
  ui.deflateCheckBox->setChecked(s.value("deflate", true).toBool());
  checkTiffSupport();
}


void
QDjViewTiffExporter::saveProperties(QString group)
{
  QSettings s;
  if (group.isEmpty()) 
    group = "Export-" + name();
  s.beginGroup(group);
  s.setValue("dpi", ui.dpiSpinBox->value());
  s.setValue("bitonal", ui.bitonalCheckBox->isChecked());
  s.setValue("jpeg", ui.jpegCheckBox->isChecked());
  s.setValue("jpegQuality", ui.jpegSpinBox->value());
  s.setValue("deflate", ui.deflateCheckBox->isChecked());
}


int
QDjViewTiffExporter::propertyPages()
{
  return 1;
}


QWidget* 
QDjViewTiffExporter::propertyPage(int num)
{
  if (num == 0)
    return page;
  return 0;
}


#if HAVE_TIFF

static QDjViewTiffExporter *tiffExporter;

static void 
tiffHandler(const char *, const char *fmt, va_list ap)
{
  QString message;
#if QT_VERSION >= 0x50500
  message.vasprintf(fmt, ap);
#else
  message.vsprintf(fmt, ap);
#endif
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
  tiff = 0;
  QIODevice::OpenMode mode = file.openMode();
  file.close();
  if (curStatus > DDJVU_JOB_OK)
    if (mode & (QIODevice::WriteOnly|QIODevice::Append))
      file.remove();
}


bool 
QDjViewTiffExporter::save(QString fname)
{
  if (file.openMode())
    return false;
  file.setFileName(fname);
  return start();
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
  else if (file.open(QIODevice::ReadWrite))
    tiff = TIFFFdOpen(wdup(file.handle()), file.fileName().toLocal8Bit().data(),"w");
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
#ifdef CCITT_SUPPORT
      if (style == DDJVU_FORMAT_MSBTOLSB 
          && TIFFFindCODEC(COMPRESSION_CCITT_T6))
        compression = COMPRESSION_CCITT_T6;
#endif
#ifdef JPEG_SUPPORT
      else if (ui.jpegCheckBox->isChecked()
               && style != DDJVU_FORMAT_MSBTOLSB 
               && TIFFFindCODEC(COMPRESSION_JPEG))
        compression = COMPRESSION_JPEG;
#endif
#ifdef ZIP_SUPPORT
      else if (ui.deflateCheckBox->isChecked()
               && style != DDJVU_FORMAT_MSBTOLSB 
               && TIFFFindCODEC(COMPRESSION_DEFLATE))
        compression = COMPRESSION_DEFLATE;
#endif
#ifdef PACKBITS_SUPPORT
      else if (TIFFFindCODEC(COMPRESSION_PACKBITS))
        compression = COMPRESSION_PACKBITS;
#endif
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
#ifdef CCITT_SUPPORT
      if (compression != COMPRESSION_CCITT_T6)
#endif
#ifdef JPEG_SUPPORT
        if (compression != COMPRESSION_JPEG)
#endif
#ifdef ZIP_SUPPORT
          if (compression != COMPRESSION_DEFLATE)
#endif
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
#ifdef JPEG_SUPPORT
        if (compression == COMPRESSION_JPEG)
          TIFFSetField(tiff, TIFFTAG_JPEGQUALITY, quality);
#endif
        if (style == DDJVU_FORMAT_GREY8) {
          TIFFSetField(tiff, TIFFTAG_SAMPLESPERPIXEL, (uint16)1);
          TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
        } else {
          TIFFSetField(tiff, TIFFTAG_SAMPLESPERPIXEL, (uint16)3);
          TIFFSetField(tiff, TIFFTAG_COMPRESSION, compression);
#ifdef JPEG_SUPPORT
          if (compression == COMPRESSION_JPEG) {
            TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_YCBCR);
            TIFFSetField(tiff, TIFFTAG_JPEGCOLORMODE, JPEGCOLORMODE_RGB);
          } else 
#endif
            TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
        }
      }
      // render and save
      char white = (char)0xff;
      int rowsize = rect.w * 3;
      if (style == DDJVU_FORMAT_MSBTOLSB) {
        white = 0x00;
        rowsize = (rect.w + 7) / 8;
      } else if (style == DDJVU_FORMAT_GREY8)
        rowsize = rect.w;
      if (! (image = (char*)malloc(rowsize * rect.h)))
        message = tr("Out of memory.");
      else if (rowsize != TIFFScanlineSize(tiff))
        message = tr("Internal error.");
      else 
        {
          char *s = image;
          ddjvu_rect_t *r = &rect;
          if (! ddjvu_page_render(*page, mode, r, r, fmt, rowsize, image))
            memset(image, white, rowsize * rect.h);
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







// ----------------------------------------
// QDJVIEWPDFEXPORTER


class QDjViewPdfExporter : public QDjViewTiffExporter
{
  Q_OBJECT
public:
  static QDjViewExporter *create(QDialog*, QDjView*, QString);
  static void setup();
public:
  QDjViewPdfExporter(QDialog *parent, QDjView *djview, QString name);
  virtual bool save(QString fileName);
protected:
  virtual void doFinal();
protected:
  QTemporaryFile tempFile;
  QFile pdfFile;
};


QDjViewExporter*
QDjViewPdfExporter::create(QDialog *parent, QDjView *djview, QString name)
{
  if (name == "PDF")
    return new QDjViewPdfExporter(parent, djview, name);
  return 0;
}


void
QDjViewPdfExporter::setup()
{
  addExporterData("PDF", "pdf",
                  tr("PDF Document"),
                  tr("PDF Files (*.pdf)"),
                  QDjViewPdfExporter::create);
}


QDjViewPdfExporter::QDjViewPdfExporter(QDialog *parent, QDjView *djview,
                                         QString name)
  : QDjViewTiffExporter(parent, djview, name)
{
  page->setObjectName(tr("PDF Options", "tab caption"));
  page->setWhatsThis(tr("<html><b>PDF options.</b><br>"
                        "These options control the characteristics of "
                        "the images embedded in the exported PDF files. "
                        "The resolution box limits their maximal resolution. "
                        "Forcing bitonal G4 compression "
                        "encodes all pages in black and white "
                        "using the CCITT Group 4 compression. "
                        "Allowing JPEG compression uses lossy JPEG "
                        "for all non bitonal or subsampled images. "
                        "Otherwise, allowing deflate compression "
                        "produces more compact files. "
                        "</html>") );
}


void 
QDjViewPdfExporter::doFinal()
{
  QString message;
  // close tiff
#if HAVE_TIFF2PDF
  tiffExporter = this;
  TIFFSetErrorHandler(tiffHandler);
  TIFFSetWarningHandler(0);
  if (tiff)
    TIFFClose(tiff);
  tiff = 0;
  // open files
  TIFF *input = 0;
  FILE *output = 0;
  QByteArray inameArray = file.fileName().toLocal8Bit();
  QByteArray onameArray = pdfFile.fileName().toLocal8Bit();
  file.close();
  if (file.open(QIODevice::ReadOnly))
    input = TIFFFdOpen(wdup(file.handle()), inameArray.data(), "r");
  if (pdfFile.open(QIODevice::WriteOnly))
    output = ::fdopen(wdup(pdfFile.handle()),"wb");
  if (input && output)
    {
      const char *argv[3];
      argv[0] = "tiff2pdf";
      argv[1] = "-o";
      argv[2] = onameArray.data();
      if (tiff2pdf(input, output, 3, argv) != EXIT_SUCCESS)
        message = tr("Error while creating pdf file.");
    }
  else if (! output)
    {
      message = tr("Unable to create output file.");
      if (! pdfFile.errorString().isEmpty())
        message = tr("System error: %1.").arg(pdfFile.errorString());
    }
  else
    message = tr("Unable to reopen temporary file.");
  // close all
  if (input)
    TIFFClose(input);
  if (output)
    fclose(output);
  if (file.openMode())
    file.close();
  if (pdfFile.openMode())
    pdfFile.close();
  if (tempFile.openMode())
    tempFile.close();
  if (tempFile.exists())
    tempFile.remove();
#else
  message = tr("PDF export was not compiled.");
#endif
  if (!message.isEmpty())
    {
      curStatus = DDJVU_JOB_FAILED;
      error(message, __FILE__, __LINE__);
    }
}


bool 
QDjViewPdfExporter::save(QString fname)
{
  if (file.openMode() || tempFile.openMode() || pdfFile.openMode())
    return false;
  pdfFile.setFileName(fname);
  tempFile.setFileTemplate(fname);
  if (tempFile.open())
    {
      file.setFileName(tempFile.fileName());
      tempFile.close();
      return start();
    }
  // prepare error message
  QString message = tr("Unable to create temporary file.");
  if (! tempFile.errorString().isEmpty())
    message = tr("System error: %1.").arg(tempFile.errorString());
  error(message, __FILE__, __LINE__);
  return false;
}


// ----------------------------------------
// QDJVIEWIMGEXPORTER



class QDjViewImgExporter : public QDjViewPageExporter
{
  Q_OBJECT
public:
  static QDjViewExporter *create(QDialog*, QDjView*, QString);
  static void setup();
public:
  QDjViewImgExporter(QDialog *parent, QDjView *djview, 
                     QString name, QByteArray format);
  virtual bool exportOnePageOnly();
  virtual bool save(QString fname);
protected:
  virtual void doPage();
  QString fileName;
  QByteArray format;
};


QDjViewExporter*
QDjViewImgExporter::create(QDialog *parent, QDjView *djview, QString name)
{
  QByteArray format = name.toLocal8Bit();
  QList<QByteArray> formats = QImageWriter::supportedImageFormats();
  
  if (formats.contains(format))
    return new QDjViewImgExporter(parent, djview, name, format);
  return 0;
}


void
QDjViewImgExporter::setup()
{
  QByteArray format;
  QList<QByteArray> formats = QImageWriter::supportedImageFormats();
  foreach(format, formats)
    {
      QString name = QString::fromLocal8Bit(format);
      QString uname = name.toUpper();
      QString suffix = name.toLower();
      addExporterData(name, suffix,
                      tr("%1 Image", "JPG Image").arg(uname),
                      tr("%1 Files (*.%2)", "JPG Files").arg(uname).arg(suffix),
                      QDjViewImgExporter::create);
    }
}


QDjViewImgExporter::QDjViewImgExporter(QDialog *parent, QDjView *djview, 
                                       QString name, QByteArray format)
  : QDjViewPageExporter(parent, djview, name), 
    format(format)
{
}


bool 
QDjViewImgExporter::exportOnePageOnly()
{
  return true;
}


bool 
QDjViewImgExporter::save(QString fname)
{
  fileName = fname;
  return start();
}


void
QDjViewImgExporter::doPage()
{
  QDjVuPage *page = curPage;
  // determine rectangle
  ddjvu_rect_t rect;
  int imgdpi = ddjvu_page_get_resolution(*page);
  int dpi = imgdpi; // (TODO: add property page with resolution!)
  rect.x = rect.y = 0;
  rect.w = ( ddjvu_page_get_width(*page) * dpi + imgdpi/2 ) / imgdpi;
  rect.h = ( ddjvu_page_get_height(*page) * dpi + imgdpi/2 ) / imgdpi;
  // prepare format
  ddjvu_format_t *fmt = 0;
#if DDJVUAPI_VERSION < 18
  unsigned int masks[3] = { 0xff0000, 0xff00, 0xff };
  fmt = ddjvu_format_create(DDJVU_FORMAT_RGBMASK32, 3, masks);
#else
  unsigned int masks[4] = { 0xff0000, 0xff00, 0xff, 0xff000000 };
  fmt = ddjvu_format_create(DDJVU_FORMAT_RGBMASK32, 4, masks);
#endif
  ddjvu_format_set_row_order(fmt, true);
  ddjvu_format_set_gamma(fmt, 2.2);
  // determine mode (TODO: add property page with mode!)
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
  QImage img(rect.w, rect.h, QImage::Format_RGB32);
  if (! ddjvu_page_render(*page, mode, &rect, &rect, fmt,
                          img.bytesPerLine(), (char*)img.bits() ))
    message = tr("Cannot render page.");
  else
    {
      QFile file(fileName);
      QImageWriter writer(&file, format);
      if (! writer.write(img))
        {
          message = file.errorString();
          file.remove();
          if (writer.error() == QImageWriter::UnsupportedFormatError)
            message = tr("Image format %1 not supported.")
              .arg(QString::fromLocal8Bit(format).toUpper());
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




// ----------------------------------------
// QDJVIEWPRNEXPORTER



#include "ui_qdjviewexportprn.h"


class QDjViewPrnExporter : public QDjViewPageExporter
{
  Q_OBJECT
public:
  static QDjViewExporter *create(QDialog*, QDjView*, QString);
  static void setup();
public:
  ~QDjViewPrnExporter();
  QDjViewPrnExporter(QDialog *parent, QDjView *djview, QString name);
  virtual void resetProperties();
  virtual void loadProperties(QString group);
  virtual void saveProperties(QString group);
  virtual bool loadPrintSetup(QPrinter *printer, QPrintDialog *dialog);
  virtual bool savePrintSetup(QPrinter *printer);
  virtual int propertyPages();
  virtual QWidget* propertyPage(int num);
  virtual bool save(QString fileName);
  virtual bool print(QPrinter *printer);
protected:
  virtual void closeFile();
  virtual void doPage();
protected:
  Ui::QDjViewExportPrn ui;
  QPointer<QWidget> page;
  QPrinter *printer;
  QPainter *painter;
  QString   fileName;
};


QDjViewExporter*
QDjViewPrnExporter::create(QDialog *parent, QDjView *djview, QString name)
{
  if (name == "PRN")
    return new QDjViewPrnExporter(parent, djview, name);
  return 0;
}


void
QDjViewPrnExporter::setup()
{
  addExporterData("PRN", "prn",
                  tr("Printer data"),
                  tr("PRN Files (*.prn)"),
                  QDjViewPrnExporter::create);
}


QDjViewPrnExporter::~QDjViewPrnExporter()
{
  closeFile();
  delete page;
}


QDjViewPrnExporter::QDjViewPrnExporter(QDialog *parent, QDjView *djview,
                                         QString name)
  : QDjViewPageExporter(parent, djview, name),
    printer(0),
    painter(0)
{
  page = new QWidget();
  ui.setupUi(page);
  page->setObjectName(tr("Printing Options", "tab caption"));
  resetProperties();
  page->setWhatsThis(tr("<html><b>Printing options.</b><br>"
                        "Option <tt>Color</tt> enables color printing. "
                        "Document pages can be decorated with a frame. "
                        "Option <tt>Scale to fit</tt> accommodates "
                        "whatever paper size your printer uses. "
                        "Zoom factor <tt>100%</tt> reproduces the initial "
                        "document size. Orientation <tt>Automatic</tt> "
                        "chooses portrait or landscape on a page per "
                        "page basis.</html>") );
}


void     
QDjViewPrnExporter::resetProperties()
{
  ui.colorButton->setChecked(true);
  ui.frameCheckBox->setChecked(false);
  ui.cropMarksCheckBox->setChecked(false);
  ui.autoOrientCheckBox->setChecked(true);
  ui.scaleToFitButton->setChecked(true);
  ui.zoomSpinBox->setValue(100);
}


void
QDjViewPrnExporter::loadProperties(QString group)
{
  QSettings s;
  if (group.isEmpty()) 
    group = "Export-" + name();
  s.beginGroup(group);
  bool color = s.value("color", true).toBool();
  ui.colorButton->setChecked(color);
  ui.grayScaleButton->setChecked(!color);
  ui.frameCheckBox->setChecked(s.value("frame", false).toBool());
  ui.cropMarksCheckBox->setChecked(false);
  ui.autoOrientCheckBox->setChecked(s.value("autoOrient", true).toBool());
  bool landscape = s.value("landscape",false).toBool();
  ui.portraitButton->setChecked(!landscape);
  ui.landscapeButton->setChecked(landscape);
  int zoom = s.value("zoom",0).toInt();
  ui.scaleToFitButton->setChecked(zoom == 0);
  ui.zoomButton->setChecked(zoom!=0);
  ui.zoomSpinBox->setValue(zoom ? qBound(25,zoom,2400) : 100);
}


void
QDjViewPrnExporter::saveProperties(QString group)
{
  QSettings s;
  if (group.isEmpty()) 
    group = "Export-" + name();
  s.beginGroup(group);
  s.setValue("color", ui.colorButton->isChecked());
  s.setValue("frame", ui.frameCheckBox->isChecked());
  s.setValue("autoOrient", ui.autoOrientCheckBox->isChecked());
  s.setValue("landscape", ui.landscapeButton->isChecked());
  int zoom = ui.zoomSpinBox->value();
  s.setValue("zoom", QVariant(ui.zoomButton->isChecked() ? zoom : 0));
}


bool
QDjViewPrnExporter::loadPrintSetup(QPrinter *printer, QPrintDialog *)
{
  bool grayscale = (printer->colorMode() == QPrinter::GrayScale);
  bool landscape = (printer->orientation() == QPrinter::Landscape);
  ui.grayScaleButton->setChecked(grayscale);  
  ui.colorButton->setChecked(!grayscale);  
  ui.landscapeButton->setChecked(landscape);
  ui.portraitButton->setChecked(!landscape);
  return true;
}


bool
QDjViewPrnExporter::savePrintSetup(QPrinter *printer)
{
  bool grayscale = ui.grayScaleButton->isChecked();
  bool landscape = ui.landscapeButton->isChecked();
  printer->setColorMode(grayscale ? QPrinter::GrayScale : QPrinter::Color);
  printer->setOrientation(landscape ? QPrinter::Landscape : QPrinter::Portrait);
  return true;
}


int
QDjViewPrnExporter::propertyPages()
{
  return 1;
}


QWidget* 
QDjViewPrnExporter::propertyPage(int num)
{
  if (num == 0)
    return page;
  return 0;
}


bool
QDjViewPrnExporter::save(QString fileName)
{
  closeFile();
  this->fileName = fileName;
  printer = new QPrinter(QPrinter::HighResolution);
  printer->setOutputFileName(fileName);
#if defined(Q_OS_UNIX) && !defined(Q_OS_DARWIN) 
  printer->setOutputFormat(QPrinter::PdfFormat);
#endif
  return start();
}


bool 
QDjViewPrnExporter::print(QPrinter *printer)
{
  closeFile();
  this->printer = printer;
  return start();
}


void 
QDjViewPrnExporter::closeFile()
{
  if (painter)
    delete painter;
  painter = 0;
  if (printer && !fileName.isEmpty())
    delete printer;
  printer = 0;
  if (curStatus > DDJVU_JOB_OK && !fileName.isEmpty())
    ::remove(QFile::encodeName(fileName).data());
  fileName = QString();
}


void 
QDjViewPrnExporter::doPage()
{
  QDjVuPage *page = curPage;
  // determine zoom factor and rectangles
  ddjvu_rect_t rect;
  rect.x = rect.y = 0;
  rect.w = ddjvu_page_get_width(*page);
  rect.h = ddjvu_page_get_height(*page);
  QRect pageRect = printer->pageRect();
  bool landscape = false;
  int prndpi = printer->resolution();
  int imgdpi = ddjvu_page_get_resolution(*page);
  if (ui.autoOrientCheckBox->isChecked())
    landscape = (rect.w > rect.h) ^ (pageRect.width() > pageRect.height());
  else if (ui.landscapeButton->isChecked())
    landscape = true;
  int printW = (landscape) ? pageRect.height() : pageRect.width();
  int printH = (landscape) ? pageRect.width() : pageRect.height();
  int targetW = (rect.w * prndpi + imgdpi/2 ) / imgdpi;
  int targetH = (rect.h * prndpi + imgdpi/2 ) / imgdpi;
  int zoom = 100;
  if (ui.zoomButton->isChecked())
    zoom = qBound(25, ui.zoomSpinBox->value(), 2400);
  else if (ui.scaleToFitButton->isChecked())
    zoom = qMin( printW * 100 / targetW, printH * 100 / targetH);
  targetW = targetW * zoom / 100;
  targetH = targetH * zoom / 100;
  int dpi = qMin(prndpi * zoom / 100, imgdpi);
  rect.w = ( rect.w * dpi + imgdpi/2 ) / imgdpi;
  rect.h = ( rect.h * dpi + imgdpi/2 ) / imgdpi;
  // create image
  QImage::Format imageFormat = QImage::Format_RGB32;
  if (ddjvu_page_get_type(*page) == DDJVU_PAGETYPE_BITONAL && dpi >= imgdpi)
    imageFormat = QImage::Format_Mono;
  else if (ui.grayScaleButton->isChecked())
    imageFormat = QImage::Format_Indexed8;
  QImage img(rect.w, rect.h, imageFormat);
  // render
  bool rendered = false;
  ddjvu_format_t *fmt = 0;
  ddjvu_render_mode_t mode = DDJVU_RENDER_COLOR;
  QDjViewPrefs *prefs = QDjViewPrefs::instance();
  if (imageFormat == QImage::Format_RGB32)
    {
#if DDJVUAPI_VERSION < 18
      unsigned int masks[3] = { 0xff0000, 0xff00, 0xff };
      fmt = ddjvu_format_create(DDJVU_FORMAT_RGBMASK32, 3, masks);
#else
      unsigned int masks[4] = { 0xff0000, 0xff00, 0xff, 0xff000000 };
      fmt = ddjvu_format_create(DDJVU_FORMAT_RGBMASK32, 4, masks);
#endif
      if (prefs->printerGamma > 0)
        ddjvu_format_set_gamma(fmt, prefs->printerGamma);
      ddjvu_format_set_row_order(fmt, true);
      if (ddjvu_page_render(*page, mode, &rect, &rect, fmt,
                              img.bytesPerLine(), (char*)img.bits() ))
        rendered = true;
    }
  else if (imageFormat == QImage::Format_Indexed8)
    {
#if QT_VERSION >= 0x40600
      img.setColorCount(256);
#else
      img.setNumColors(256);
#endif
      for (int i=0; i<256; i++)
        img.setColor(i, qRgb(i,i,i));
      fmt = ddjvu_format_create(DDJVU_FORMAT_GREY8, 0, 0);
      if (prefs->printerGamma > 0)
        ddjvu_format_set_gamma(fmt, prefs->printerGamma);
      ddjvu_format_set_row_order(fmt, true);
      if (ddjvu_page_render(*page, mode, &rect, &rect, fmt,
                              img.bytesPerLine(), (char*)img.bits() ))
        rendered = true;
    }
  else if (imageFormat == QImage::Format_Mono)
    {
      fmt = ddjvu_format_create(DDJVU_FORMAT_MSBTOLSB, 0, 0);
      ddjvu_format_set_row_order(fmt, true);
      if (ddjvu_page_render(*page, mode, &rect, &rect, fmt,
                              img.bytesPerLine(), (char*)img.bits() ))
        rendered = true;
      if (rendered)
        img.invertPixels();
    }
  // prepare painter
  if (! painter)
    painter = new QPainter(printer);
  else
    printer->newPage();
  // prepare settings
  painter->save();
  if (landscape)
    {
      painter->translate(QPoint(pageRect.width(),0));
      painter->rotate(90);
    }
  painter->translate(qMax(0, (printW-targetW)/2), qMax(0, (printH-targetH)/2));

  // print page
  QRect sourceRect(0,0,rect.w,rect.h);
  QRect targetRect(0,0,targetW,targetH);
  if (rendered)
    painter->drawImage(targetRect, img, sourceRect);
  QPen pen(Qt::black, 0, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin);
  painter->setPen(pen);
  painter->setBrush(Qt::NoBrush);
  if (ui.frameCheckBox->isChecked())
    painter->drawRect(targetRect);
  // finish
  painter->restore();
  if (! rendered)
    {
      QString pageName = djview->pageName(curPage->pageNo());
      error(tr("Cannot render page %1.").arg(pageName), __FILE__, __LINE__);
    }
  if (fmt)
    ddjvu_format_release(fmt);
  fmt = 0;
}






// ----------------------------------------
// CREATEEXPORTERDATA


static void 
createExporterData()
{
  QDjViewDjVuExporter::setup();
#if HAVE_TIFF2PDF
  QDjViewPdfExporter::setup();
#endif
#if HAVE_TIFF
  QDjViewTiffExporter::setup();
#endif
  QDjViewPSExporter::setup();
  QDjViewImgExporter::setup();
  QDjViewPrnExporter::setup();
}



// ----------------------------------------
// MOC

#include "qdjviewexporters.moc"





/* -------------------------------------------------------------
   Local Variables:
   c++-font-lock-extra-types: ( "\\sw+_t" "[A-Z]\\sw*[a-z]\\sw*" )
   End:
   ------------------------------------------------------------- */
