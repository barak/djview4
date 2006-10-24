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


#include <QCoreApplication>
#include <QDebug>
#include <QMetaEnum>
#include <QMetaObject>
#include <QMutex>
#include <QMutexLocker>
#include <QPointer>
#include <QSettings>
#include <QStringList>
#include <QVariant>

#include "qdjviewprefs.h"
#include "qdjview.h"



/*! \class QDjViewPrefs
   \brief Holds the preferences for the \a QDjView instances. */

/*! \enum QDjViewPrefs::Option
   \brief Miscellanenous flags.
   This enumeration defines flags describing 
   (i) the visibility of the menubar, toolbar, etc.
   (ii) the page layout flags continuous and side by side.
   (iii) the interactive capabilities of the djvu widget. */

/*! \enum QDjViewPrefs::Tool
   \brief Toolbar flags.
   This enumeration defines flags describing
   the composition of the toolbar. */

/*! \class QDjViewPrefs::Saved
   \brief Mode dependent preferences.
   This structure holds the preferences that are 
   saved separately for each viewer mode.
   There are four copies of this structure,
   for the standalone viewer mode, for the full screen mode,
   for the full page plugin mode, and for the embedded plugin mode. */


/*! The default toolbar composition. */

const QDjViewPrefs::Tools 
QDjViewPrefs::defaultTools 
= TOOLBAR_TOP | TOOLBAR_BOTTOM
| TOOL_ZOOMCOMBO | TOOL_ZOOMBUTTONS
| TOOL_PAGECOMBO | TOOL_PREVNEXT | TOOL_FIRSTLAST 
| TOOL_OPEN | TOOL_SEARCH | TOOL_SELECT
| TOOL_PRINT | TOOL_LAYOUT
| TOOL_WHATSTHIS;


/*! The default set of miscellaneous flags. */

const QDjViewPrefs::Options
QDjViewPrefs::defaultOptions 
= SHOW_MENUBAR | SHOW_TOOLBAR | SHOW_SIDEBAR 
| SHOW_STATUSBAR | SHOW_SCROLLBARS | SHOW_FRAME
| HANDLE_MOUSE | HANDLE_KEYBOARD | HANDLE_LINKS 
| HANDLE_CONTEXTMENU;


static QPointer<QDjViewPrefs> preferences;

/*! Create the single preference object. */

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


QDjViewPrefs::Saved::Saved(void)
  : remember(true),
    options(defaultOptions),
    zoom(QDjVuWidget::ZOOM_FITWIDTH),
    sidebarTab(0)
{
}


QDjViewPrefs::QDjViewPrefs(void)
  : QObject(QCoreApplication::instance()),
    windowSize(640,400),
    tools(defaultTools),
    gamma(2.2),
    cacheSize(10*1024*1024),
    pixelCacheSize(256*1024),
    lensSize(300),
    lensPower(3),
    printerGamma(0.0),
    modifiersForLens(Qt::ControlModifier|Qt::ShiftModifier),
    modifiersForSelect(Qt::ControlModifier),
    modifiersForLinks(Qt::ShiftModifier)
{
  Options mss = (SHOW_MENUBAR|SHOW_STATUSBAR|SHOW_SIDEBAR);
  forFullScreen.options &= ~(mss|SHOW_SCROLLBARS|SHOW_TOOLBAR|SHOW_SIDEBAR);
  forFullPagePlugin.options &= ~(mss);
  forFullPagePlugin.remember = false;
  forEmbeddedPlugin.options &= ~(mss|SHOW_TOOLBAR);
  forEmbeddedPlugin.remember = false;
}


/*! Return a string with the djview version. */

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


/*! Return a string describing a set of modifier keys. */

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

/*! Return the modifiers described in string \a s. */

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


/*! Return a string describing a set of option flags. */

QString 
QDjViewPrefs::optionsToString(Options options)
{
  const QMetaObject *mo = metaObject();
  QMetaEnum me = mo->enumerator(mo->indexOfEnumerator("Option"));
  return me.valueToKeys(static_cast<int>(options));
}

/*! Return the option flags described by a string. */

QDjViewPrefs::Options 
QDjViewPrefs::stringToOptions(QString s)
{
  const QMetaObject *mo = metaObject();
  QMetaEnum me = mo->enumerator(mo->indexOfEnumerator("Option"));
  int options = me.keysToValue(s.toLatin1());
  return QFlag(options);
}


/*! Return a string describing a set of toolbar flags. */

QString 
QDjViewPrefs::toolsToString(Tools tools)
{
  const QMetaObject *mo = metaObject();
  QMetaEnum me = mo->enumerator(mo->indexOfEnumerator("Tool"));
  return me.valueToKeys(tools);
}


/*! Return the toolbar flags described by a string. */

QDjViewPrefs::Tools   
QDjViewPrefs::stringToTools(QString s)
{
  const QMetaObject *mo = metaObject();
  QMetaEnum me = mo->enumerator(mo->indexOfEnumerator("Tool"));
  int tools = me.keysToValue(s.toLatin1());
  return QFlag(tools);
}


void 
QDjViewPrefs::loadGroup(QSettings &s, QString name, Saved &saved)
{
  s.beginGroup(name);
  if (s.contains("remember"))
    saved.remember = s.value("remember").toBool();
  if (s.contains("options"))
    saved.options = stringToOptions(s.value("options").toString());
  if (s.contains("zoom"))
    saved.zoom = s.value("zoom").toInt();
  if (s.contains("state"))
    saved.state = s.value("state").toByteArray();
  if (s.contains("sidebarTab"))
    saved.sidebarTab = s.value("sidebarTab").toInt();
  s.endGroup();
}


/*! Load saved preferences and set all member variables. */

void
QDjViewPrefs::load(void)
{
  QSettings s(DJVIEW_ORG, DJVIEW_APP);

  loadGroup(s, "forEmbeddedPlugin", forEmbeddedPlugin);
  loadGroup(s, "forFullPagePlugin", forFullPagePlugin);
  loadGroup(s, "forStandalone", forStandalone);
  loadGroup(s, "forFullScreen", forFullScreen);
  
  if (s.contains("windowSize"))
    windowSize = s.value("windowSize").toSize();
  if (s.contains("tools"))
    tools = stringToTools(s.value("tools").toString());
  if (s.contains("gamma"))
    gamma = s.value("gamma").toDouble();
  if (s.contains("cacheSize"))
    cacheSize = s.value("cacheSize").toLongLong();
  if (s.contains("pixelCacheSize"))
    pixelCacheSize = s.value("pixelCacheSize").toInt();
  if (s.contains("lensSize"))
    lensSize = s.value("lensSize").toInt();
  if (s.contains("lensPower"))
    lensPower = s.value("lensPower").toInt();
  if (s.contains("printerGamma"))
    printerGamma = s.value("printerGamma").toDouble();
  if (s.contains("modifiersForLens"))
    modifiersForLens 
      = stringToModifiers(s.value("modifiersForLens").toString());
  if (s.contains("modifiersForSelect"))
    modifiersForSelect 
      = stringToModifiers(s.value("modifiersForSelect").toString());
  if (s.contains("modifiersForLinks"))
    modifiersForLinks 
      = stringToModifiers(s.value("modifiersForLinks").toString());
  if (s.contains("browserProgram"))
    browserProgram = s.value("browserProgram").toString();
}


void 
QDjViewPrefs::saveGroup(QSettings &s, QString name, Saved &saved)
{
  s.beginGroup(name);
  s.setValue("remember", saved.remember);
  s.setValue("options", optionsToString(saved.options));
  s.setValue("zoom", saved.zoom);
  s.setValue("state", saved.state);
  s.setValue("sidebarTab", saved.sidebarTab);
  s.endGroup();
}


/*! Save all member variables for later use. */

void
QDjViewPrefs::save(void)
{
  QSettings s(DJVIEW_ORG, DJVIEW_APP);
  
  s.setValue("version", versionString());

  saveGroup(s, "forEmbeddedPlugin", forEmbeddedPlugin);
  saveGroup(s, "forFullPagePlugin", forFullPagePlugin);
  saveGroup(s, "forStandalone", forStandalone);
  saveGroup(s, "forFullScreen", forFullScreen);

  s.setValue("windowSize", windowSize);
  s.setValue("tools", toolsToString(tools));
  s.setValue("gamma", gamma);
  s.setValue("cacheSize", cacheSize);
  s.setValue("pixelCacheSize", pixelCacheSize);
  s.setValue("lensSize", lensSize);
  s.setValue("lensPower", lensPower);
  s.setValue("printerGamma", printerGamma);
  s.setValue("modifiersForLens", modifiersToString(modifiersForLens));
  s.setValue("modifiersForSelect", modifiersToString(modifiersForSelect));
  s.setValue("modifiersForLinks", modifiersToString(modifiersForLinks));
  s.setValue("browserProgram", browserProgram);
}




/* -------------------------------------------------------------
   Local Variables:
   c++-font-lock-extra-types: ( "\\sw+_t" "[A-Z]\\sw*[a-z]\\sw*" )
   End:
   ------------------------------------------------------------- */
