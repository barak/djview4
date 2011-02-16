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

#include "qtnpapi.h"

extern "C" NPError NPP_New(NPMIMEType pluginType, NPP instance,
                           uint16 mode, int16 argc, char* argn[],
                           char* argv[], NPSavedData* saved);

extern "C" NPError NPP_Destroy(NPP instance, NPSavedData** save);

extern "C" NPError NPP_SetWindow(NPP instance, NPWindow* window);

extern "C" NPError NPP_NewStream(NPP instance, NPMIMEType type,
                                 NPStream* stream, NPBool seekable,
                                 uint16* stype);

extern "C" NPError NPP_DestroyStream(NPP instance, NPStream* stream,
                                     NPReason reason);

extern "C" int32 NPP_WriteReady(NPP instance, NPStream* stream);

extern "C" int32 NPP_Write(NPP instance, NPStream* stream, int32 offset,
                           int32 len, void* buffer);

extern "C" void NPP_StreamAsFile(NPP instance, NPStream* stream,
                                 const char* fname);

extern "C" void NPP_Print(NPP instance, NPPrint* platformPrint);

extern "C" int16 NPP_HandleEvent(NPP instance, NPEvent* event);

extern "C" void NPP_URLNotify(NPP instance, const char* url,
                              NPReason reason, void* notifyData);

extern "C" NPError NPP_GetValue(NPP instance, 
                                NPPVariable variable, void *value);

extern "C" NPError NPP_SetValue(NPP instance, 
                                NPPVariable variable, void *value);

extern "C" void NPP_Shutdown();


#ifdef Q_WS_X11

extern "C" const char *NP_GetMIMEDescription(void);

extern "C" NPError NP_GetValue(void*, NPPVariable aVariable, void *aValue);

#endif
