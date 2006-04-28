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

#include "qdjvu.h"
#include "qdjview.h"

#include <QApplication>
#include <QRegExp>

int 
main(int argc, char *argv[])
{
  QApplication app(argc, argv);  
  QDjVuContext djvuContext(argv[0]);
  QDjView *main = new QDjView(djvuContext);
  bool okay = true;
  if (argc > 1)
    {
      QString arg = argv[1];
      if (arg.contains(QRegExp("^[a-zA-Z]{3,8}:/")))
        okay = main->open(QUrl(arg));
      else
        okay = main->open(arg);
    }
  main->resize(640,400);
  main->show();
  if (! okay)
    {
      main->execErrorDialog(QMessageBox::Critical);
      return 10;
    }
  return app.exec();
}
