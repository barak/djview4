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

#include "qdjviewprefs.h"


QDjViewPrefs::ModeSpecific::ModeSpecific()
  : flags    ( SHOW_ALL|HANDLE_ALL|ACTION_ALL ),
    zoom     ( QDjVuWidget::ZOOM_FITWIDTH ),
    dispMode ( QDjVuWidget::DISPLAY_COLOR ),
    hAlign   ( QDjVuWidget::ALIGN_CENTER ),
    vAlign   ( QDjVuWidget::ALIGN_CENTER )
{
}


QDjViewPrefs::QDjViewPrefs()
{
  // Sane defaults
  forStandalone.flags &= ~(SHOW_SIDEBAR);
  forFullPagePlugin.flags &= ~(SHOW_MENUBAR|SHOW_STATUSBAR|SHOW_SIDEBAR);  
  forEmbeddedPlugin.flags &= ~(SHOW_MENUBAR|SHOW_STATUSBAR|SHOW_SIDEBAR);  
  forEmbeddedPlugin.flags &= ~(SHOW_TOOLBAR);
  

}





/* -------------------------------------------------------------
   Local Variables:
   c++-font-lock-extra-types: ( "\\sw+_t" "[A-Z]\\sw*[a-z]\\sw*" )
   End:
   ------------------------------------------------------------- */
