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


#include "qdjvuwidget.h"

#include "QFlags"


struct QDjViewPrefs
{
  QDjViewPrefs();

  enum {
    SHOW_MENUBAR        = 0x01,
    SHOW_TOOLBAR        = 0x02,
    SHOW_SIDEBAR        = 0x04,
    SHOW_STATUSBAR      = 0x08,
    SHOW_SCROLLBARS     = 0x10,
    SHOW_FRAME          = 0x20,
    SHOW_ALL   = 0x3f,
    LAYOUT_CONTINUOUS   = 0x100,
    LAYOUT_SIDEBYSIDE   = 0x200,
    LAYOUT_PAGESETTINGS = 0x400,
    LAYOUT_ALL = 0x700,
    HANDLE_KEYBOARD     = 0x1000,
    HANDLE_LINKS        = 0x2000,
    HANDLE_CONTEXTMENU  = 0x4000,
    HANDLE_ALL = 0x7000,
    ACTION_MODECOMBO    = 0x00010000,
    ACTION_MODEBUTTONS  = 0x00020000,
    ACTION_ZOOMCOMBO    = 0x00040000,
    ACTION_ZOOMBUTTONS  = 0x00080000,
    ACTION_PAGECOMBO    = 0x00100000,
    ACTION_PREVNEXT     = 0x00200000,
    ACTION_FIRSTLAST    = 0x00400000,
    ACTION_BACKFORW     = 0x00800000,
    ACTION_ROTATE       = 0x01000000,
    ACTION_SEARCH       = 0x02000000,
    ACTION_SAVE         = 0x04000000,
    ACTION_PRINT        = 0x08000000,
    ACTION_ALL = 0x0fff0000,
  };

  struct ModeSpecific {
    ModeSpecific();
    int flags;
    int zoom;
    QDjVuWidget::DisplayMode dispMode;
    QDjVuWidget::Align       hAlign;
    QDjVuWidget::Align       vAlign;
  };

  ModeSpecific forStandalone;
  ModeSpecific forEmbeddedPlugin;
  ModeSpecific forFullPagePlugin;
  
};





#endif

/* -------------------------------------------------------------
   Local Variables:
   c++-font-lock-extra-types: ( "\\sw+_t" "[A-Z]\\sw*[a-z]\\sw*" )
   End:
   ------------------------------------------------------------- */
