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

#include <qdjvu.h>
#include <qdjvuhttp.h>
#include <qdjvuwidget.h>

#include <Qt/QApplication>
#include <Qt/QPushButton>
#include <Qt/QFileInfo>

int 
main(int argc, char *argv[])
{
  QApplication app(argc, argv);  

  QDjVuContext djvuCtx(argv[0]);
  QDjVuWidget hello;
  QDjVuHttpDocument djvuDoc;
  QString arg = (argc>1) ? argv[1] : "files/test.djvu";
  if (arg.startsWith("file:/") || 
      arg.startsWith("http:/") )
    djvuDoc.setUrl(&djvuCtx, QUrl(arg));
  else
    djvuDoc.setFileName(&djvuCtx, arg);
  hello.setDocument(&djvuDoc);
  hello.show();
  return app.exec();
}
