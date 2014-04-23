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

#ifndef DJVIEW_H
#define DJVIEW_H

#if AUTOCONF
# include "config.h"
#endif

#include <Qt>
#include <QObject>
#include <QApplication>
#include <QEvent>
#include <QPointer>
#include <QString>
#include <QStringList>

#include "qdjvu.h"

class QDjView;
class QTranslator;

class QDjViewApplication : public QApplication
{
  Q_OBJECT
  QDjVuContext context;
  QPointer<QDjView> lastWindow;
  QStringList translationLangs;
  QStringList translationDirs;
 public:
  QDjViewApplication(int &argc, char **argv);
  QDjVuContext *djvuContext() { return &context; }
  QDjView *newWindow();
  bool loadTranslators(QStringList langs, QTranslator *dt, QTranslator *qt);
protected:
  bool event(QEvent *ev);
protected slots:
  void saveSessionState(QSessionManager &sm);
private:
  QStringList getTranslationLangs();
  QStringList getTranslationDirs();
};


#endif


/* -------------------------------------------------------------
   Local Variables:
   c++-font-lock-extra-types: ( "\\sw+_t" "[A-Z]\\sw*[a-z]\\sw*" )
   End:
   ------------------------------------------------------------- */
