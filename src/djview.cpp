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

#include <QtGlobal>
#include <QApplication>
#include <QRegExp>


static void 
usage()
{
  fprintf(stderr,"usage: djview [options] [filename-or-http-url]\n");
  exit(10);
}


static QtMsgHandler qtDefaultHandler;

void 
qtMessageHandler(QtMsgType type, const char *msg)
{
  switch (type) 
    {
    case QtFatalMsg:
      fprintf(stderr,"Fatal error: %s\n", msg);
      abort();
    case QtCriticalMsg:
      fprintf(stderr,"Critical error: %s\n", msg);
    case QtDebugMsg:
    case QtWarningMsg:
      break;
    }
}

int 
main(int argc, char *argv[])
{
  // qtDefaultHandler = qInstallMsgHandler(qtMessageHandler);
  QApplication app(argc, argv);  
  QDjVuContext djvuContext(argv[0]);
  QDjView *main = new QDjView(djvuContext);
  main->setAttribute(Qt::WA_DeleteOnClose);
  
  // Process command line
  while (argc > 1 && argv[1][0] == '-')
    {
      QString arg = QString::fromLocal8Bit(argv[1]).replace(QRegExp("-+"),"");
      if (arg == "help")
        usage();
      else if (arg == "verbose")
        qInstallMsgHandler(qtDefaultHandler);
      else if (! main->parseArgument(arg))
        fprintf(stderr,"djview: unrecognized option '%s'\n", argv[1]);
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
          fprintf(stderr,"djview: cannot open '%s'.\n", argv[1]);
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
