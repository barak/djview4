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
#endif

#include <libdjvu/ddjvuapi.h>
#include <libdjvu/miniexp.h>
#include <locale.h>
#include <stdio.h>

#include "qdjvu.h"
#include "qdjview.h"
#include "qdjvuwidget.h"
#include "djview.h"
#ifdef Q_WS_X11
#  include "qdjviewplugin.h"
#endif

#include <QtGlobal>
#include <QApplication>
#include <QByteArray>
#include <QDebug>
#include <QDir>
#include <QFileOpenEvent>
#include <QLibraryInfo>
#include <QLocale>
#include <QRegExp>
#include <QSettings>
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
  QString msg = QDjViewApplication::tr
    ("Usage: djview [options] [filename-or-url]\n"
     "Common options include:\n"
     "-help~~~Prints this message.\n"
     "-verbose~~~Prints all warning messages.\n"
     "-display <xdpy>~~~Select the X11 display <xdpy>.\n"
     "-geometry <xgeom>~~~Select the initial window geometry.\n"
     "-font <xlfd>~~~Select the X11 name of the main font.\n"
     "-style <qtstyle>~~~Select the QT user interface style.\n"
     "-fullscreen, -fs~~~Start djview in full screen mode.\n"
     "-page=<page>~~~Jump to page <page>.\n"
     "-zoom=<zoom>~~~Set zoom factor.\n"
     "-continuous=<yn>~~~Set continuous layout.\n"
     "-sidebyside=<yn>~~~Set side-by-side layout.\n" );
  
  // align tabs
  QStringList opts = msg.split("\n");
  int tab = 0;
  foreach (QString s, opts)
    tab = qMax(tab, s.indexOf("~~~"));
  foreach (QString s, opts)
    {
      int pos = s.indexOf("~~~");
      if (pos >= 0)
        s = QString(" %1  %2").arg(s.left(pos), -tab).arg(s.mid(pos+3));
      message(s, false);
    }
  exit(10);
}


static void
addDirectory(QStringList &dirs, QString path)
{
  QString dirname = QDir::cleanPath(path);
  if (! dirs.contains(dirname))
    dirs << dirname;
}


QDjViewApplication::QDjViewApplication(int &argc, char **argv)
  : QApplication(argc, argv), context(argv[0])
{
  // Application data
  setOrganizationName(DJVIEW_ORG);
  setOrganizationDomain(DJVIEW_DOMAIN);
  setApplicationName(DJVIEW_APP);
  
  // Translators
  QTranslator *qtTrans = new QTranslator(this);
  QTranslator *djviewTrans = new QTranslator(this);
  // - determine preferred languages
  QStringList langs; 
  QString varLanguage = ::getenv("LANGUAGE");
  if (varLanguage.size())
    langs += varLanguage.toLower().split(":", QString::SkipEmptyParts);
#ifdef LC_MESSAGES
  QString varLcMessages = ::setlocale(LC_MESSAGES, 0);
  if (varLcMessages.size())
    langs += varLcMessages;
#else
# ifdef LC_ALL
  QString varLcMessages = ::setlocale(LC_ALL, 0);
  if (varLcMessages.size())
    langs += varLcMessages;
# endif
#endif
#ifdef Q_WS_MAC
  langs += QSettings(".", "globalPreferences")
    .value("AppleLanguages").toStringList();
#endif
  QString qtLocale =  QLocale::system().name();
  if (qtLocale.size())
    langs += qtLocale.toLower();

  // - determine potential directories
  QStringList dirs;
  QDir dir = applicationDirPath();
  QString dirPath = dir.canonicalPath();
  addDirectory(dirs, dirPath);
#ifdef DIR_DATADIR
  QString datadir = DIR_DATADIR;
  addDirectory(dirs, datadir + "/djvu/djview4");
  addDirectory(dirs, datadir + "/djview4");
#endif
#ifdef Q_WS_MAC
  addDirectory(dirs, dirPath + "/Resources/$LANG.lproj");
  addDirectory(dirs, dirPath + "/../Resources/$LANG.lproj");
  addDirectory(dirs, dirPath + "/../../Resources/$LANG.lproj");
#endif
  addDirectory(dirs, dirPath + "/share/djvu/djview4");
  addDirectory(dirs, dirPath + "/share/djview4");
  addDirectory(dirs, dirPath + "/../share/djvu/djview4");
  addDirectory(dirs, dirPath + "/../share/djview4");
  addDirectory(dirs, dirPath + "/../../share/djvu/djview4");
  addDirectory(dirs, dirPath + "/../../share/djview4");
  addDirectory(dirs, "/usr/share/djvu/djview4");
  addDirectory(dirs, "/usr/share/djview4");
  addDirectory(dirs, QLibraryInfo::location(QLibraryInfo::TranslationsPath));
  // - load translators
  bool qtTransValid = false;
  bool djviewTransValid = false;
  foreach (QString lang, langs)
    {
      foreach (QString dir, dirs)
        {
          dir = dir.replace(QRegExp("\\$LANG(?!\\w)"),lang);
          QDir qdir(dir);
          if (! qtTransValid && qdir.exists())
            qtTransValid = qtTrans->load("qt_" + lang, dir, "_.-");
          if (! djviewTransValid && qdir.exists())
            djviewTransValid= djviewTrans->load("djview_" + lang, dir, "_.-");
        }
      if (lang == "en") 
        break;
    }
  // - install tranlators
  if (qtTransValid)
    installTranslator(qtTrans);
  if (djviewTransValid)
    installTranslator(djviewTrans);
}


QDjView * 
QDjViewApplication::newWindow()
{
  if (lastWindow && !lastWindow->getDocument())
    return lastWindow;
  QDjView *main = new QDjView(context);
  main->setAttribute(Qt::WA_DeleteOnClose);
  lastWindow = main;
  return main;
}


bool 
QDjViewApplication::event(QEvent *ev)
{
  if (ev->type() == QEvent::FileOpen)
    {
      QString name = static_cast<QFileOpenEvent*>(ev)->file();
      QDjView *main = newWindow();
      if (main->open(name))
        {
          main->show();
        }
      else
        {
          message(tr("cannot open '%1'.").arg(name));
          delete main;
        }
      return true;
    }
  else if (ev->type() == QEvent::Close)
    {
      closeAllWindows();
    }
  return QApplication::event(ev);
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
#ifdef Q_OS_UNIX
  const char *s = ::getenv("DJVIEW_VERBOSE");
  if (s && strcmp(s,"0"))
    verbose = true;
#endif
  
  // Color specification 
  // (cause XRender errors under many versions of Qt/X11)
#ifndef Q_WS_X11
  QApplication::setColorSpec(QApplication::ManyColor);
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
  QDjViewApplication app(argc, argv);
  QDjView *main = app.newWindow();
  
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
        message(QApplication::tr("Option '-fix' is deprecated."));
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
