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

#include <QPointer>
#include <QCoreApplication>
#include <QMutex>
#include <QMutexLocker>

#include "qdjviewprefs.h"


static QPointer<QDjViewPrefs> preferences;

QDjViewPrefs *
QDjViewPrefs::create(void)
{
  QMutex mutex;
  QMutexLocker locker(&mutex);
  if (! preferences)
    {
      preferences = new QDjViewPrefs();
      preferences->load();
    }
  return preferences;
}


QDjViewPrefs::Appearance::Appearance()
  : options(QDjViewPrefs::DEFAULT_OPTIONS),
    zoom(QDjVuWidget::ZOOM_FITWIDTH),
    dispMode(QDjVuWidget::DISPLAY_COLOR),
    hAlign(QDjVuWidget::ALIGN_CENTER),
    vAlign(QDjVuWidget::ALIGN_CENTER)
{
}

QDjViewPrefs::QDjViewPrefs(void)
  : QObject(QCoreApplication::instance()),
    tools(DEFAULT_TOOLS),
    gamma(2.2),
    printerGamma(0.0)
{
  forFullScreen.options &= ~(SHOW_MENUBAR|SHOW_STATUSBAR);
  forFullScreen.options &= ~(SHOW_TOOLBAR|SHOW_SIDEBAR);
  forFullPagePlugin.options |= LAYOUT_PAGESETTINGS;
  forFullPagePlugin.options &= ~(SHOW_MENUBAR|SHOW_STATUSBAR);
  forEmbeddedPlugin.options |= LAYOUT_PAGESETTINGS;
  forEmbeddedPlugin.options &= ~(SHOW_MENUBAR|SHOW_STATUSBAR);
  forEmbeddedPlugin.options &= ~(SHOW_TOOLBAR|SHOW_SIDEBAR);
}



void
QDjViewPrefs::load(void)
{
}

void
QDjViewPrefs::save(void)
{
}




/* -------------------------------------------------------------
   Local Variables:
   c++-font-lock-extra-types: ( "\\sw+_t" "[A-Z]\\sw*[a-z]\\sw*" )
   End:
   ------------------------------------------------------------- */
