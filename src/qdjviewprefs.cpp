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


QDjViewPrefs::QDjViewPrefs(void)
  : QObject(QCoreApplication::instance()),
    forStandalone(QDjViewPrefs::DEFAULT_OPTIONS),
    forFullScreen(QDjViewPrefs::DEFAULT_OPTIONS),
    forEmbeddedPlugin(QDjViewPrefs::DEFAULT_OPTIONS),
    forFullPagePlugin(QDjViewPrefs::DEFAULT_OPTIONS),
    tools(DEFAULT_TOOLS),
    windowSize(640,400),
    modifiersForLens(Qt::ControlModifier|Qt::ShiftModifier),
    modifiersForSelect(Qt::ControlModifier),
    modifiersForLinks(Qt::ShiftModifier),
    zoom(QDjVuWidget::ZOOM_FITWIDTH),
    gamma(2.2),
    printerGamma(0.0),
    cacheSize(10*1024*1024),
    pixelCacheSize(256*1024),
    lensSize(300),
    lensPower(3)
{
  Options mss = (SHOW_MENUBAR|SHOW_STATUSBAR|SHOW_SIDEBAR);
  forFullScreen &= ~(mss|SHOW_SCROLLBARS|SHOW_TOOLBAR|SHOW_SIDEBAR);
  forEmbeddedPlugin &= ~(mss|SHOW_TOOLBAR);
  forFullPagePlugin &= ~(mss);
}


QString 
QDjViewPrefs::versionString()
{
  QString version = QString("%1.%2");
  version = version.arg((DJVIEW_VERSION>>16)&0xFF);
  version = version.arg((DJVIEW_VERSION>>8)&0xFF);
  if (DJVIEW_VERSION & 0xFF)
    version = version + QString(".%1").arg(DJVIEW_VERSION&0xFF);
  return version;
}


QString 
QDjViewPrefs::modifiersToString(Qt::KeyboardModifiers m)
{
  QStringList l;
  if (m & Qt::MetaModifier)
    l << "Meta";
  if (m & Qt::AltModifier)
    l << "Alt";
  if (m & Qt::ControlModifier)
    l << "Control";
  if (m & Qt::ShiftModifier)
    l << "Shift";
  return l.join("+");
}


Qt::KeyboardModifiers 
QDjViewPrefs::stringToModifiers(QString s)
{
  Qt::KeyboardModifiers m = Qt::NoModifier;
  QStringList l = s.split("+");
  foreach(s, l)
    {
      if (s.toLower() == "shift")
        m |= Qt::ShiftModifier;
      else if (s.toLower() == "control")
        m |= Qt::ControlModifier;
      else if (s.toLower() == "alt")
        m |= Qt::AltModifier;
      else if (s.toLower() == "meta")
        m |= Qt::MetaModifier;
    }
  return m;
}


void
QDjViewPrefs::load(void)
{
}


void
QDjViewPrefs::save(void)
{
}


void
QDjViewPrefs::saveWindow(void)
{
}




/* -------------------------------------------------------------
   Local Variables:
   c++-font-lock-extra-types: ( "\\sw+_t" "[A-Z]\\sw*[a-z]\\sw*" )
   End:
   ------------------------------------------------------------- */
