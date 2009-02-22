//C-  -*- C++ -*-
//C- -------------------------------------------------------------------
//C- DjView4
//C- Copyright (c) 2009-  Leon Bottou
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

// $Id$

#include "config.h"

// Qt includes
#include <QtGlobal>
#include <QObject>
#include <QVariant>
#include <QWidget>

// Djview includes
#include "djview.h"
#include "qdjview.h"

// Npdjvu includes
#include "npdjvu.h"
#include "qtbrowserplugin.h"
#include "qtbrowserplugin_p.h"


static QApplication *theApp = 0;
static bool ownApp = false;


NPError 
NPP_New(NPMIMEType pluginType, NPP instance,
        uint16 mode, int16 argc, char* argn[],
        char* argv[], NPSavedData* saved)
{
  return NPERR_GENERIC_ERROR;
}


NPError 
NPP_Destroy(NPP instance, NPSavedData** save)
{
  return NPERR_GENERIC_ERROR;
}


NPError 
NPP_SetWindow(NPP instance, NPWindow* window)
{
  return NPERR_GENERIC_ERROR;
}


NPError 
NPP_NewStream(NPP instance, NPMIMEType type,
              NPStream* stream, NPBool seekable,
              uint16* stype)
{
  return NPERR_GENERIC_ERROR;
}


NPError 
NPP_DestroyStream(NPP instance, NPStream* stream,
                  NPReason reason)
{
  return NPERR_GENERIC_ERROR;
}


int32 
NPP_WriteReady(NPP instance, NPStream* stream)
{
  return 0x7fffffff;
}

int32 
NPP_Write(NPP instance, NPStream* stream, int32 offset,
          int32 len, void* buffer)
{
  return 0;
}


void 
NPP_StreamAsFile(NPP instance, NPStream* stream,
                 const char* fname)
{
  return;
}


void 
NPP_Print(NPP instance, NPPrint* platformPrint)
{
}


int16 
NPP_HandleEvent(NPP instance, NPEvent* event)
{
    if (!instance || !instance->pdata)
	return NPERR_INVALID_INSTANCE_ERROR;
    QtNPInstance* This = (QtNPInstance*) instance->pdata;
    return qtns_event(This, event) ? 1 : 0;
}


void 
NPP_URLNotify(NPP instance, const char* url,
              NPReason reason, void* notifyData)
{
}


NPError 
NPP_GetValue(NPP instance, 
             NPPVariable variable, void *value)
{
  NPError err = NPERR_GENERIC_ERROR;
  switch (variable)
    {
    case NPPVpluginNameString:
      *((const char **)value) = "DjView-" DJVIEW_VERSION_STR " (npdjvu)";
      err = NPERR_NO_ERROR;
      break;
    case NPPVpluginDescriptionString:
      *((const char **)value) =
        "This is the <a href=\"http://djvu.sourceforge.net\">"
        "DjView-" DJVIEW_VERSION_STR " (npdjvu)</a> version of "
        "the DjVu plugin.<br>"
        "See <a href=\"http://djvu.sourceforge.net\">DjVuLibre</a>.";
      err = NPERR_NO_ERROR;
      break;
    case NPPVpluginNeedsXEmbed:
      *((NPBool*)value) = TRUE;
      err = NPERR_NO_ERROR;
      break;
    case NPPVpluginScriptableNPObject:
      *((NPObject**)value) = 0;
      break;
    default:
      break;
    }
  return err;
}


NPError 
NPP_SetValue(NPP instance, 
             NPPVariable variable, void *value)
{
  return NPERR_GENERIC_ERROR;
}


#ifdef Q_WS_X11

const char*
NP_GetMIMEDescription(void)
{
  return
    "image/x-djvu:djvu,djv:DjVu File;"
    "image/x.djvu::DjVu File;"
    "image/djvu::DjVu File;"
    "image/x-dejavu::DjVu File (obsolete);"
    "image/x-iw44::DjVu File (obsolete);"
    "image/vnd.djvu::DjVu File;" ;
}

NPError
NP_GetValue(void*, NPPVariable aVariable, void *aValue)
{
  return NPP_GetValue(0, aVariable, aValue);
}

#endif

/* -------------------------------------------------------------
   Local Variables:
   c++-font-lock-extra-types: ( "\\sw+_t" "[A-Z]\\sw*[a-z]\\sw*" "NPP")
   End:
   ------------------------------------------------------------- */
