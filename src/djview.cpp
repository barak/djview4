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
#endif

#include <libdjvu/ddjvuapi.h>
#include <libdjvu/miniexp.h>
#include <locale.h>
#include <stdio.h>

#include "qdjvu.h"
#include "qdjview.h"
#include "qdjvuwidget.h"
#include "djview.h"
#ifndef NPDJVU
# include "qdjviewplugin.h"
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
#include <QSessionManager>
#include <QString>
#include <QStringList>
#include <QTranslator>

#if defined(Q_OS_WIN32)
# include <mbctype.h>
#endif


#ifdef QT_NO_DEBUG
static bool verbose = false;
#else
static bool verbose = true;
#endif
static bool appReady = false;

static void 
qt4MessageHandler(QtMsgType type, const char *msg)
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
      if (verbose || !appReady)
        fprintf(stderr,"djview: %s\n", msg);
      break;
    default:
      if (verbose || !appReady)
        fprintf(stderr,"%s\n", msg);
      break;
    }
}

#if QT_VERSION >= 0x50000
static void 
qt5MessageHandler(QtMsgType type, const QMessageLogContext&, const QString &msg)
{
  QByteArray bmsg = msg.toLocal8Bit();
  qt4MessageHandler(type, bmsg.data());
}
#endif


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

QDjViewApplication::QDjViewApplication(int &argc, char **argv)
  : QApplication(argc, argv), 
    context(argv[0])
{
  // Message handler
#if QT_VERSION >= 0x50000
  qInstallMessageHandler(qt5MessageHandler);
#else
  qInstallMsgHandler(qt4MessageHandler);
#endif
  
  // Locale should not mess with printf
  // We do this again because old libdjvulibre
  // did not correctly set LC_NUMERIC.
#ifdef LC_ALL
  ::setlocale(LC_ALL, "");
# ifdef LC_NUMERIC
  ::setlocale(LC_NUMERIC, "C");
# endif
#endif

  // Parse arguments using OEM code page under windows.
#if defined(Q_OS_WIN32)
  _setmbcp(_MB_CP_OEM);
#endif
  // Mac/Cocoa bug workaround
#if defined(Q_OS_DARWIN) && defined(QT_MAC_USE_COCOA) && QT_VERSION<0x40503
  extern void qt_mac_set_native_menubar(bool);
  qt_mac_set_native_menubar(false);
#endif
  
  // Enable highdpi pixmaps
#if QT_VERSION >= 0x50200
  setAttribute(Qt::AA_UseHighDpiPixmaps, true);
#endif
  
  // Wire session management signals
  connect(this, SIGNAL(saveStateRequest(QSessionManager&)),
          this, SLOT(saveSessionState(QSessionManager&)) );
  
  // Install translators
  QStringList langs = getTranslationLangs();
  QTranslator *djviewTrans = new QTranslator(this);
  QTranslator *qtTrans = new QTranslator(this);
  if (loadTranslators(langs, djviewTrans, qtTrans))
    {
      installTranslator(qtTrans);
      installTranslator(djviewTrans);
    }
  else
    {
      delete qtTrans;
      delete djviewTrans;
    }
  // Application is ready
  appReady = true;
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


static void
addDirectory(QStringList &dirs, QString path)
{
  QString dirname = QDir::cleanPath(path);
  if (! dirs.contains(dirname))
    dirs << dirname;
}


QStringList 
QDjViewApplication::getTranslationDirs()
{
  if (translationDirs.isEmpty())
    {
      QStringList dirs;
      QDir dir = applicationDirPath();
      QString dirPath = dir.canonicalPath();
      addDirectory(dirs, dirPath);
#ifdef DIR_DATADIR
      QString datadir = DIR_DATADIR;
      addDirectory(dirs, datadir + "/djvu/djview4");
      addDirectory(dirs, datadir + "/djview4");
#endif
#ifdef Q_OS_DARWIN
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
      translationDirs = dirs;
    }
  return translationDirs;
}


static void
addLang(QStringList &langs, QString s)
{
  if (s.size() > 0)
    {
      s = s.replace(QChar('-'), QChar('_'));
      if (! langs.contains(s))
        langs << s;
#ifdef Q_OS_DARWIN
      if (s == "zh_Hans" && ! langs.contains("zh_CN"))
        langs << "zh_CN";
      if (s == "zh_Hant" && ! langs.contains("zh_TW"))
        langs << "zh_TW";
#endif
    }
}


QStringList 
QDjViewApplication::getTranslationLangs()
{
  if (translationLangs.isEmpty())
    {
      QStringList langs; 
      addLang(langs, QSettings().value("language").toString());
      QString varLanguage = QString::fromLocal8Bit(::getenv("LANGUAGE"));
      foreach(QString lang, varLanguage.split(":"))
        if (! lang.isEmpty())
          addLang(langs, lang);
#ifdef LC_MESSAGES
      addLang(langs, QString::fromLocal8Bit(::setlocale(LC_MESSAGES, 0)));
#endif
      addLang(langs, QString::fromLocal8Bit(::getenv("LANG")));
#ifdef Q_OS_DARWIN
      QSettings g(".", "globalPreferences");
      foreach (QString lang, g.value("AppleLanguages").toStringList())
        addLang(langs, lang);
#endif
      addLang(langs, QLocale::system().name());
      translationLangs = langs;
    }
  return translationLangs;
}


static bool loadOneTranslator(QTranslator *trans, 
                              QString name, QString lang, QStringList dirs)
{
  QString llang = lang.toLower();
  foreach (QString dir, dirs)
    {
      dir = dir.replace(QRegExp("\\$LANG(?!\\w)"), lang);
      QDir qdir(dir);
      if (qdir.exists())
        {
          if (trans->load(name + "_" + lang, dir, "_.-"))
            return true;
          if (lang != llang && trans->load(name + "_" + llang, dir, "_.-"))
            return true;
        }
    }
  return false;
}

bool
QDjViewApplication::loadTranslators(QStringList langs, 
                                    QTranslator *dTrans, QTranslator *qTrans)
{
  QStringList dirs = getTranslationDirs();
  foreach (QString lang, langs)
    {
      bool okay = true;
      if (okay && dTrans && !loadOneTranslator(dTrans, "djview", lang, dirs))
        okay = false;
      if (okay && qTrans && !loadOneTranslator(qTrans, "qt", lang, dirs))
        okay = false;
      if (okay || lang.startsWith("en_") || lang == "en")
        return okay;
    }
  return false;
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

void 
QDjViewApplication::saveSessionState(QSessionManager &sm)
{
  int n = 0;
  QSettings s;
  foreach(QWidget *w, topLevelWidgets())
    {
      QDjView *djview = qobject_cast<QDjView*>(w);
      if (djview && !djview->objectName().isEmpty() &&
          djview->getViewerMode() == QDjView::STANDALONE )
        {
          if (++n == 1)
            {
              QStringList discard = QStringList(applicationFilePath());
              discard << QLatin1String("-discard") << sessionId();
              sm.setDiscardCommand(discard);
              QStringList restart = QStringList(applicationFilePath());
              restart << QLatin1String("-session");
              restart << sessionId() + "_" + sessionKey();
              sm.setRestartCommand(restart);
              QString id = QLatin1String("session_") + sessionId();
              s.sync();
              s.remove(id);
              s.beginGroup(id);
            }
          s.beginGroup(QString::number(n));
          djview->saveSession(&s);
          s.endGroup();
        }
    }
  if (n > 0)
    {
      s.setValue("windows", n);
      s.endGroup();
      s.sync();
    }
}


#ifndef NPDJVU

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


int 
main(int argc, char *argv[])
{
  // Application data
  QApplication::setOrganizationName(DJVIEW_ORG);
  QApplication::setOrganizationDomain(DJVIEW_DOMAIN);
  QApplication::setApplicationName(DJVIEW_APP);
  
  // Message handler
#if QT_VERSION >= 0x50000
  qInstallMessageHandler(qt5MessageHandler);
#else
  qInstallMsgHandler(qt4MessageHandler);
#endif

  // Set verbose flag as early as possible
#ifdef Q_OS_UNIX
  const char *s = ::getenv("DJVIEW_VERBOSE");
  if (s && strcmp(s,"0"))
    verbose = true;
#endif
  for (int i=1; i<argc; i++)
    {
      char *s = argv[i];
      while (s && *s=='-') 
        s++;
      if (! strcmp(s, "verbose"))
        verbose = true;
    }
  
  // Color specification 
  // (cause XRender errors under many versions of Qt4/X11)
#ifndef Q_WS_X11
# if QT_VERSION < 0x50000
  QApplication::setColorSpec(QApplication::ManyColor);
# endif
#endif
  
  // Plugin mode
#if WITH_DJVIEWPLUGIN
  if (argc==2 && !strcmp(argv[1],"-netscape"))
    {
      verbose = true;
      QDjViewPlugin dispatcher(argv[0]);
      return dispatcher.exec();
    }
#endif
  
  // Discard session
  if (argc==3 && !strcmp(argv[1],"-discard"))
    {
      QSettings s;
      s.remove(QLatin1String("session_")+QLatin1String(argv[2]));
      s.sync();
      return 0;
    }
  
  // Create application
  QDjViewApplication app(argc, argv);

  // Restore session
  if (app.isSessionRestored())
    {
      QSettings s;
      s.beginGroup(QLatin1String("session_") + app.sessionId());
      int windows = s.value("windows").toInt();
      if (windows > 0)
        {
          for (int n=1; n<=windows; n++)
            {
              QDjView *w = new QDjView(*app.djvuContext());
              w->setAttribute(Qt::WA_DeleteOnClose);
              s.beginGroup(QString::number(n));
              w->restoreSession(&s);
              s.endGroup();
              w->show();
            }
          return app.exec();
        }
    }
  
  // Process command line (from QCoreApplication for win unicode)
  QStringList args;
  QDjView *main = app.newWindow();
  QStringList qargv = QCoreApplication::arguments();
  int qi = 1;
  while (qi < qargv.size() && qargv.at(qi)[0] == '-')
    {
      QString arg = qargv.at(qi);
      arg.replace(QRegExp("^-+"),"");
      QString key = arg.section(QChar('='),0,1);
      if (arg == "help")
        usage();
      else if (arg == "verbose")
        verbose = true;
      else if (arg == "quiet")
        verbose = false;
      else if (arg == "fix")
        message(QApplication::tr("Option '-fix' is deprecated."));
      else 
        args += arg;
      qi += 1;
    }
  if (qi < qargv.size() - 1)
    usage();

  // Open file
  if (qi == qargv.size() - 1)
    {
      QString name = qargv.at(qi);
      bool okay = true;
      if (name.contains(QRegExp("^[a-zA-Z]{3,8}:/")))
        okay = main->open(QUrl(name));
      else
        okay = main->open(name);
      if (! okay)
        {
          message(QDjView::tr("cannot open '%1'.").arg(name));
          exit(10);
        }
    }
  
  // Process options and go
  foreach(QString arg, args)
    message(main->parseArgument(arg));
  main->show();
  return app.exec();
}

#endif


/* -------------------------------------------------------------
   Local Variables:
   c++-font-lock-extra-types: ( "\\sw+_t" "[A-Z]\\sw*[a-z]\\sw*" )
   End:
   ------------------------------------------------------------- */
