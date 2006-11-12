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

#include <libdjvu/ddjvuapi.h>
#include <libdjvu/miniexp.h>
#include <locale.h>
#include <stdio.h>

#include "qdjvu.h"
#include "qdjview.h"
#include "qdjvuwidget.h"
#ifdef Q_WS_X11
# include "qdjviewplugin.h"
#endif

#include <QtGlobal>
#include <QApplication>
#include <QByteArray>
#include <QDebug>
#include <QDir>
#include <QLibraryInfo>
#include <QLocale>
#include <QRegExp>
#include <QString>
#include <QStringList>
#include <QTranslator>


#ifdef QT_NO_DEBUG
static bool verbose = false;
#else
static bool verbose = true;
#endif


static QtMsgHandler qtDefaultHandler;


static void 
qtMessageHandler(QtMsgType type, const char *msg)
{
  switch (type) 
    {
    case QtFatalMsg:
      fprintf(stderr,"djview fatal error: %s\n", msg);
      abort();
    case QtCriticalMsg:
      fprintf(stderr,"djview critical error: %s\n", msg);
      break;
    case QtWarningMsg:
      if (verbose)
        fprintf(stderr,"djview: %s\n", msg);
      break;
    default:
      if (verbose)
        fprintf(stderr,"%s\n", msg);
      break;
    }
}



static void
message(QString string, bool prefix=true)
{
  QByteArray m = string.toLocal8Bit();
  if (prefix)
    fprintf(stderr, "djview: ");
  fprintf(stderr, "%s\n", (const char*) m);
}



static void
message(QStringList sl)
{
  foreach (QString s, sl)
    message(s);
}



static void 
usage()
{
  QString msg = QApplication::tr
    ("Usage: djview [options] [filename-or-url]\n"
     "Common options include:\n"
     "-help\tPrints this message.\n"
     "-verbose\tPrints all warning messages.\n"
     "-display <xdpy>\tSelect the X11 display <xdpy>.\n"
     "-geometry <xgeom>\tSelect the initial window geometry.\n"
     "-font <xlfd>\tSelect the X11 name of the main font.\n"
     "-style <qtstyle>\tSelect the QT user interface style.\n"
     "-fullscreen, -fs\tStart djview in full screen mode.\n"
     "-page=<page>\tJump to page <page>.\n"
     "-zoom=<zoom>\tSet zoom factor.\n"
     "-continuous=<yn>\tSet continuous layout.\n"
     "-sidebyside=<yn>\tSet side-by-side layout.\n" );
  
  // align tabs
  QStringList opts = msg.split("\n");
  int tab = 0;
  foreach (QString s, opts)
    tab = qMax(tab, s.indexOf("\t"));
  foreach (QString s, opts)
    {
      int pos = s.indexOf("\t");
      if (pos >= 0)
        s = QString(" %1  %2").arg(s.left(pos), -tab).arg(s.mid(pos+1));
      message(s, false);
    }
  exit(10);
}


static void
addDirectory(QStringList &dirs, QString path)
{
  QDir dir = path;
  if (dir.exists())
    dirs << dir.canonicalPath();
}


void
setupApplication()
{
  QCoreApplication *app = QCoreApplication::instance();
  QTranslator *qtTrans = new QTranslator(app);
  QTranslator *djviewTrans = new QTranslator(app);
  
  // preferred languages
  QStringList langs; 
  QString varLanguage = ::getenv("LANGUAGE");
  if (varLanguage.size())
    langs += varLanguage.toLower().split(":", QString::SkipEmptyParts);
#ifdef LC_ALL
  QString varLcMessages = ::setlocale(LC_MESSAGES, 0);
  if (varLcMessages.size())
    langs += varLcMessages;
#endif
  QString qtLocale =  QLocale::system().name();
  if (qtLocale.size())
    langs += qtLocale;

  // potential directories
  QStringList dirs;
  QDir dir = app->applicationDirPath();
  QString dirPath = dir.canonicalPath();
  addDirectory(dirs, dirPath);
  addDirectory(dirs, dirPath + "/Resources/translations");
  addDirectory(dirs, dirPath + "/share/djvu/djview4/translations");
  addDirectory(dirs, dirPath + "/share/djview4/translations");
  addDirectory(dirs, dirPath + "/../Resources/translations");
  addDirectory(dirs, dirPath + "/../share/djvu/djview4/translations");
  addDirectory(dirs, dirPath + "/../share/djview4/translations");
#ifdef PREFIX_NAME
  QString prefix = PREFIX_NAME;
  addDirectory(dirs, prefix + "/share/djvu/djview4/translations");
  addDirectory(dirs, prefix + "/share/djview4/translations");
#endif
  addDirectory(dirs, "/usr/share/djvu/djview4/translations");
  addDirectory(dirs, "/usr/share/djview4/translations");
  addDirectory(dirs, QLibraryInfo::location(QLibraryInfo::TranslationsPath));
  
  // load translators
  foreach (QString lang, langs)
    {
      foreach (QString directory, dirs)
        {
          if (qtTrans->isEmpty())
            qtTrans->load("qt_" + lang, directory);
          if (djviewTrans->isEmpty())
            djviewTrans->load("djview_" + lang, directory);
        }
    }

  // install tranlators
  if (! qtTrans->isEmpty())
    app->installTranslator(qtTrans);
  if (! djviewTrans->isEmpty())
    app->installTranslator(djviewTrans);
}




int 
main(int argc, char *argv[])
{
  // Locale
#ifdef LC_ALL
  ::setlocale(LC_ALL, "");
  ::setlocale(LC_NUMERIC, "C");
#endif

  // Message verbosity
  qtDefaultHandler = qInstallMsgHandler(qtMessageHandler);
#if QT_VERSION < 0x40100
  verbose = true;
  qWarning("Using Qt < 4.1.0 with prejudice.");
#endif
#ifdef Q_OS_UNIX
  const char *s = ::getenv("DJVIEW_VERBOSE");
  if (s && strcmp(s,"0"))
    verbose = true;
#endif

  // Plugin mode
#ifdef Q_WS_X11
  for (int i=1; i<argc; i++)
    if (!strcmp(argv[i],"-netscape") || !strcmp(argv[i],"--netscape"))
      { // run as plugin
        verbose = true;
        QDjViewPlugin dispatcher(argv[0]);
        return dispatcher.exec();
      }
#endif

  // Start
  QDjVuContext djvuContext(argv[0]);
  QApplication app(argc, argv);
  setupApplication();
  QDjView *main = new QDjView(djvuContext);
  main->setAttribute(Qt::WA_DeleteOnClose);
  
  // Process command line
  while (argc > 1 && argv[1][0] == '-')
    {
      QString arg = QString::fromLocal8Bit(argv[1]).replace(QRegExp("^-+"),"");
      if (arg == "help")
        usage();
      else if (arg == "verbose")
        verbose = true;
      else if (arg == "quiet")
        verbose = false;
      else if (arg == "fix")
        message(QApplication::tr("Options 'fix' is deprecated."));
      else 
        message(main->parseArgument(arg));
      argc --;
      argv ++;
    }
  if (argc > 2)
    usage();
  
  // Open file
  if (argc > 1)
    {
      QString name = QString::fromLocal8Bit(argv[1]);
      bool okay = true;
      if (name.contains(QRegExp("^[a-zA-Z]{3,8}:/")))
        okay = main->open(QUrl(name));
      else
        okay = main->open(name);
      if (! okay)
        {
          message(QDjView::tr("cannot open '%1'.").arg(argv[1]));
          exit(10);
        }
    }

  // process events
  main->show();
  return app.exec();
}




/* -------------------------------------------------------------
   Local Variables:
   c++-font-lock-extra-types: ( "\\sw+_t" "[A-Z]\\sw*[a-z]\\sw*" )
   End:
   ------------------------------------------------------------- */
