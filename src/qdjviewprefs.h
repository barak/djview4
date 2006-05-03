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

#ifndef QDJVIEWPREFS_H
#define QDJVIEWPREFS_H

#include <Qt>
#include <QObject>
#include <QByteArray>
#include <QSettings>
#include <QFlags>

#include "qdjvuwidget.h"


#ifndef DJVIEW_ORG
# define DJVIEW_ORG "DjVuLibre"
#endif
#ifndef DJVIEW_APP
# define DJVIEW_APP "DjView"
#endif
#ifndef DJVIEW_VERSION
# define DJVIEW_VERSION 0x40000
#endif


class QDjViewPrefs : public QObject
{
  Q_OBJECT
  Q_ENUMS(Option Tool)

public:

  enum Option {
    SHOW_MENUBAR        = 0x000001,
    SHOW_TOOLBAR        = 0x000002,
    SHOW_SIDEBAR        = 0x000004,
    SHOW_STATUSBAR      = 0x000010,
    SHOW_SCROLLBARS     = 0x000020,
    SHOW_FRAME          = 0x000040,
    SHOW_MASK           = 0x000FFF, 
    LAYOUT_CONTINUOUS   = 0x001000,
    LAYOUT_SIDEBYSIDE   = 0x002000,
    LAYOUT_MASK         = 0x00F000, 
    HANDLE_MOUSE        = 0x100000,
    HANDLE_KEYBOARD     = 0x200000,
    HANDLE_LINKS        = 0x400000,
    HANDLE_CONTEXTMENU  = 0x800000,
  };

  enum Tool {
    TOOLBAR_TOP    = 0x80000000,
    TOOLBAR_BOTTOM = 0x40000000,
    TOOL_MODECOMBO    = 0x00001,
    TOOL_MODEBUTTONS  = 0x00002,
    TOOL_ZOOMCOMBO    = 0x00004,
    TOOL_ZOOMBUTTONS  = 0x00008,
    TOOL_PAGECOMBO    = 0x00010,
    TOOL_PREVNEXT     = 0x00020,
    TOOL_FIRSTLAST    = 0x00040,
    TOOL_BACKFORW     = 0x00080,
    TOOL_NEW          = 0x00100,
    TOOL_OPEN         = 0x00200,
    TOOL_SEARCH       = 0x00400,
    TOOL_SELECT       = 0x00800,
    TOOL_SAVE         = 0x01000,
    TOOL_PRINT        = 0x02000,
    TOOL_LAYOUT       = 0x04000,
    TOOL_ROTATE       = 0x08000,
    TOOL_WHATSTHIS    = 0x10000,
  };

  Q_DECLARE_FLAGS(Options, Option)
  Q_DECLARE_FLAGS(Tools, Tool)
  
  static const Options defaultOptions;
  static const Tools   defaultTools;
  static QString versionString();
  QString modifiersToString(Qt::KeyboardModifiers);
  Qt::KeyboardModifiers stringToModifiers(QString);
  QString optionsToString(Options);
  Options stringToOptions(QString);
  QString toolsToString(Tools);
  Tools   stringToTools(QString);


  struct Saved {
    Saved();
    bool       remember;
    Options    options;
    int        zoom;
    QByteArray state;
  };

  // Preferences remembered between program invocations.
  Saved      forEmbeddedPlugin;
  Saved      forFullPagePlugin;
  Saved      forStandalone;
  Saved      forFullScreen;
  QSize      windowSize;

  // Preferences set via the preference dialog.
  Tools      tools;
  double     gamma;
  qlonglong  cacheSize;
  int        pixelCacheSize;
  int        lensSize;
  int        lensPower;
  double     printerGamma;  
  Qt::KeyboardModifiers modifiersForLens;
  Qt::KeyboardModifiers modifiersForSelect;
  Qt::KeyboardModifiers modifiersForLinks;
  
  // Preferences set via the print dialog.
  // ...TODO... 

public:
  static QDjViewPrefs *create(void);
  void load(void);
  void save(void);

private:
  QDjViewPrefs(void);
  void loadGroup(QSettings &s, QString name, Saved &saved);
  void saveGroup(QSettings &s, QString name, Saved &saved);
};


Q_DECLARE_OPERATORS_FOR_FLAGS(QDjViewPrefs::Options)
Q_DECLARE_OPERATORS_FOR_FLAGS(QDjViewPrefs::Tools)


#endif

/* -------------------------------------------------------------
   Local Variables:
   c++-font-lock-extra-types: ( "\\sw+_t" "[A-Z]\\sw*[a-z]\\sw*" "qlonglong" )
   End:
   ------------------------------------------------------------- */
