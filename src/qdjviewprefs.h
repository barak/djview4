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

#include <QObject>
#include <QByteArray>
#include <QFlags>

#include "qdjvuwidget.h"


class QDjViewPrefs : public QObject
{
  Q_OBJECT

public:

  enum Option {
    SHOW_MENUBAR        = 0x0001,
    SHOW_TOOLBAR        = 0x0002,
    SHOW_SIDEBAR        = 0x0004,
    SHOW_STATUSBAR      = 0x0010,
    SHOW_SCROLLBARS     = 0x0020,
    SHOW_FRAME          = 0x0040,
    LAYOUT_CONTINUOUS   = 0x0100,
    LAYOUT_SIDEBYSIDE   = 0x0200,
    LAYOUT_PAGESETTINGS = 0x0400,
    HANDLE_KEYBOARD     = 0x1000,
    HANDLE_LINKS        = 0x2000,
    HANDLE_CONTEXTMENU  = 0x4000,
    DEFAULT_OPTIONS = 0x7077,
  };

  Q_DECLARE_FLAGS(Options, Option)

  enum Tool {
    TOOL_MODECOMBO    = 0x00001,
    TOOL_MODEBUTTONS  = 0x00002,
    TOOL_ZOOMCOMBO    = 0x00004,
    TOOL_ZOOMBUTTONS  = 0x00008,
    TOOL_PAGECOMBO    = 0x00010,
    TOOL_PREVNEXT     = 0x00020,
    TOOL_FIRSTLAST    = 0x00040,
    TOOL_BACKFORW     = 0x00080,
    TOOL_ROTATE       = 0x00100,
    TOOL_SEARCH       = 0x00200,
    TOOL_SAVE         = 0x00400,
    TOOL_PRINT        = 0x00800,
    TOOL_OPEN         = 0x01000,
    TOOL_NEW          = 0x02000,
    TOOL_LAYOUT       = 0x04000,
    DEFAULT_TOOLS = 0x5e7c
  };

  Q_DECLARE_FLAGS(Tools, Tool)

  struct Appearance {
    Appearance();
    Options                  options;
    int                      zoom;
  };

  Appearance forStandalone;
  Appearance forFullScreen;
  Appearance forEmbeddedPlugin;
  Appearance forFullPagePlugin;
  Tools      tools;
  QByteArray toolState;
  double     gamma;
  double     printerGamma;
  long       cacheSize;
  long       pixelCacheSize;
  int        lensSize;
  int        lensPower;

  static QDjViewPrefs *create();
  void load();
  void save();

private:
  QDjViewPrefs(void);
};


Q_DECLARE_OPERATORS_FOR_FLAGS(QDjViewPrefs::Options)
Q_DECLARE_OPERATORS_FOR_FLAGS(QDjViewPrefs::Tools)


#endif

/* -------------------------------------------------------------
   Local Variables:
   c++-font-lock-extra-types: ( "\\sw+_t" "[A-Z]\\sw*[a-z]\\sw*" )
   End:
   ------------------------------------------------------------- */
