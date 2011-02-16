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

/****************************************************************************
**
** Copyright (C) 2003-2008 Trolltech ASA. All rights reserved.
**
** This file is part of a Qt Solutions component.
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
** http://www.trolltech.com/products/qt/opensource.html
**
** If you are unsure which license is appropriate for your use, please
** review the following information:
** http://www.trolltech.com/products/qt/licensing.html or contact the
** Trolltech sales department at sales@trolltech.com.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include <QtGlobal>
#include <QVariant>

#include "qtbrowserplugin.h"
#include "qtbrowserplugin_p.h"
#include "npdjvu.h"

#ifndef WINAPI
# ifdef Q_WS_WIN
#  define WINAPI __stdcall
# else
#  define WINAPI
# endif
#endif

#ifdef Q_WS_X11
#  ifdef Bool
#    undef Bool
#  endif
#endif

static NPNetscapeFuncs *qNetscapeFuncs = 0;
static bool mozilla_has_npruntime = true;


// NPN functions, forwarding to function pointers provided by browser
void 
NPN_Version(int* plugin_major, int* plugin_minor, int* netscape_major, int* netscape_minor)
{
    Q_ASSERT(qNetscapeFuncs);
    *plugin_major   = NP_VERSION_MAJOR;
    *plugin_minor   = NP_VERSION_MINOR;
    *netscape_major = qNetscapeFuncs->version  >> 8;  // Major version is in high byte
    *netscape_minor = qNetscapeFuncs->version & 0xFF; // Minor version is in low byte
}

#define NPN_Prolog(x) \
    Q_ASSERT(qNetscapeFuncs); \
    Q_ASSERT(qNetscapeFuncs->x); \

#define NPN_PrologRuntime(x, statement) \
    Q_ASSERT(qNetscapeFuncs); \
    if (! mozilla_has_npruntime) statement; \
    Q_ASSERT(qNetscapeFuncs->x);


const char *
NPN_UserAgent(NPP instance)
{
    NPN_Prolog(uagent);
    return FIND_FUNCTION_POINTER(NPN_UserAgentFP, 
                                 qNetscapeFuncs->uagent)(instance);
}

void 
NPN_Status(NPP instance, const char* message)
{
    NPN_Prolog(status);
    FIND_FUNCTION_POINTER(NPN_StatusFP, 
                          qNetscapeFuncs->status)(instance, message);
}

NPError 
NPN_GetURL(NPP instance, const char* url, const char* window)
{
    NPN_Prolog(geturl);
    return FIND_FUNCTION_POINTER(NPN_GetURLFP, 
                                 qNetscapeFuncs->geturl)(instance, url, window);
}

NPError 
NPN_GetURLNotify(NPP instance, const char* url, const char* window, void* notifyData)
{
    if ((qNetscapeFuncs->version & 0xFF) < NPVERS_HAS_NOTIFICATION)
        return NPERR_INCOMPATIBLE_VERSION_ERROR;

    NPN_Prolog(geturlnotify);
    return FIND_FUNCTION_POINTER(NPN_GetURLNotifyFP, 
                                 qNetscapeFuncs->geturlnotify)(instance, url, window, notifyData);
}

NPError 
NPN_PostURLNotify(NPP instance, const char* url, const char* window, uint32 len, const char* buf, NPBool file, void* notifyData)
{
    if ((qNetscapeFuncs->version & 0xFF) < NPVERS_HAS_NOTIFICATION)
        return NPERR_INCOMPATIBLE_VERSION_ERROR;

    NPN_Prolog(posturlnotify);
    return FIND_FUNCTION_POINTER(NPN_PostURLNotifyFP, 
                                 qNetscapeFuncs->posturlnotify)(instance, url, window, len, buf, file, notifyData);
}

void* 
NPN_MemAlloc(uint32 size)
{
    NPN_Prolog(memalloc);
    return FIND_FUNCTION_POINTER(NPN_MemAllocFP, 
                                 qNetscapeFuncs->memalloc)(size);
}

void 
NPN_MemFree(void* ptr)
{
    NPN_Prolog(memfree);
    FIND_FUNCTION_POINTER(NPN_MemFreeFP, 
                          qNetscapeFuncs->memfree)(ptr);
}

uint32 
NPN_MemFlush(uint32 size)
{
    NPN_Prolog(memflush);
    return FIND_FUNCTION_POINTER(NPN_MemFlushFP, 
                                 qNetscapeFuncs->memflush)(size);
}

NPError	
NPN_GetValue(NPP instance, NPNVariable variable, void *ret_value)
{
    NPN_Prolog(getvalue);
    return FIND_FUNCTION_POINTER(NPN_GetValueFP, 
                                 qNetscapeFuncs->getvalue)(instance, variable, ret_value);
}

NPError 
NPN_SetValue(NPP instance, NPPVariable variable, void *ret_value)
{
    NPN_Prolog(setvalue);
    return FIND_FUNCTION_POINTER(NPN_SetValueFP, 
                                 qNetscapeFuncs->setvalue)(instance, variable, ret_value);
}

NPIdentifier 
NPN_GetStringIdentifier(const char* name)
{
  NPN_PrologRuntime(getstringidentifier, return 0);
    return FIND_FUNCTION_POINTER(NPN_GetStringIdentifierFP, 
                                 qNetscapeFuncs->getstringidentifier)(name);
}

void 
NPN_GetStringIdentifiers(const char** names, int32 nameCount, NPIdentifier* identifiers)
{
  NPN_PrologRuntime(getstringidentifiers, return);
    FIND_FUNCTION_POINTER(NPN_GetStringIdentifiersFP, 
                          qNetscapeFuncs->getstringidentifiers)(names, nameCount, identifiers);
}

NPIdentifier 
NPN_GetIntIdentifier(int32 intid)
{
    NPN_PrologRuntime(getintidentifier, return 0);
    return FIND_FUNCTION_POINTER(NPN_GetIntIdentifierFP, 
                                 qNetscapeFuncs->getintidentifier)(intid);
}

bool 
NPN_IdentifierIsString(NPIdentifier identifier)
{
    NPN_PrologRuntime(identifierisstring, return false);
    return FIND_FUNCTION_POINTER(NPN_IdentifierIsStringFP, 
                                 qNetscapeFuncs->identifierisstring)(identifier);
}

char* 
NPN_UTF8FromIdentifier(NPIdentifier identifier)
{
    NPN_PrologRuntime(utf8fromidentifier, return 0);
    return FIND_FUNCTION_POINTER(NPN_UTF8FromIdentifierFP, 
                                 qNetscapeFuncs->utf8fromidentifier)(identifier);
}

int32 
NPN_IntFromIdentifier(NPIdentifier identifier)
{
    NPN_PrologRuntime(intfromidentifier, return 0);
    return FIND_FUNCTION_POINTER(NPN_IntFromIdentifierFP, 
                                 qNetscapeFuncs->intfromidentifier)(identifier);
}

NPObject* 
NPN_CreateObject(NPP npp, NPClass *aClass)
{
    NPN_PrologRuntime(createobject, return 0);
    return FIND_FUNCTION_POINTER(NPN_CreateObjectFP, 
                                 qNetscapeFuncs->createobject)(npp, aClass);
}

NPObject* 
NPN_RetainObject(NPObject *obj)
{
    NPN_PrologRuntime(retainobject, return obj);
    return FIND_FUNCTION_POINTER(NPN_RetainObjectFP, 
                                 qNetscapeFuncs->retainobject)(obj);
}

void 
NPN_ReleaseObject(NPObject *obj)
{
    NPN_PrologRuntime(releaseobject, return);
    FIND_FUNCTION_POINTER(NPN_ReleaseObjectFP, 
                          qNetscapeFuncs->releaseobject)(obj);
}

bool 
NPN_Invoke(NPP npp, NPObject* obj, NPIdentifier methodName, 
           const NPVariant *args, int32 argCount, NPVariant *result)
{
    NPN_PrologRuntime(invoke, return false);
    return FIND_FUNCTION_POINTER(NPN_InvokeFP, 
                                 qNetscapeFuncs->invoke)(npp, obj, methodName, 
                                                         args, argCount, result);
}

bool 
NPN_InvokeDefault(NPP npp, NPObject* obj, const NPVariant *args, int32 argCount, NPVariant *result)
{
  NPN_PrologRuntime(invokedefault, return false);
    return FIND_FUNCTION_POINTER(NPN_InvokeDefaultFP, 
                                 qNetscapeFuncs->invokedefault)(npp, obj, args, argCount, result);
}

bool 
NPN_Evaluate(NPP npp, NPObject *obj, NPString *script, NPVariant *result)
{
    NPN_PrologRuntime(evaluate, return false);
    return FIND_FUNCTION_POINTER(NPN_EvaluateFP, 
                                 qNetscapeFuncs->evaluate)(npp, obj, script, result);
}

bool 
NPN_GetProperty(NPP npp, NPObject *obj, NPIdentifier propertyName, NPVariant *result)
{
    NPN_PrologRuntime(getproperty, return false);
    return FIND_FUNCTION_POINTER(NPN_GetPropertyFP, 
                                 qNetscapeFuncs->getproperty)(npp, obj, propertyName, result);
}

bool 
NPN_SetProperty(NPP npp, NPObject *obj, NPIdentifier propertyName, const NPVariant *value)
{
    NPN_PrologRuntime(setproperty, return false);
    return FIND_FUNCTION_POINTER(NPN_SetPropertyFP, 
                                 qNetscapeFuncs->setproperty)(npp, obj, propertyName, value);
}

bool 
NPN_RemoveProperty(NPP npp, NPObject *obj, NPIdentifier propertyName)
{
    NPN_PrologRuntime(removeproperty, return false);
    return FIND_FUNCTION_POINTER(NPN_RemovePropertyFP, 
                                 qNetscapeFuncs->removeproperty)(npp, obj, propertyName);
}

bool 
NPN_HasProperty(NPP npp, NPObject *obj, NPIdentifier propertyName)
{
    NPN_PrologRuntime(hasproperty, return false);
    return FIND_FUNCTION_POINTER(NPN_HasPropertyFP, 
                                 qNetscapeFuncs->hasproperty)(npp, obj, propertyName);
}

bool 
NPN_HasMethod(NPP npp, NPObject *obj, NPIdentifier methodName)
{
    NPN_PrologRuntime(hasmethod, return false);
    return FIND_FUNCTION_POINTER(NPN_HasMethodFP, 
                                 qNetscapeFuncs->hasmethod)(npp, obj, methodName);
}

void 
NPN_ReleaseVariantValue(NPVariant *variant)
{
    NPN_PrologRuntime(releasevariantvalue, return);
    FIND_FUNCTION_POINTER(NPN_ReleaseVariantValueFP, 
                          qNetscapeFuncs->releasevariantvalue)(variant);
}

void 
NPN_SetException(NPObject *obj, const char *message)
{
    qDebug("NPN_SetException: %s", message);
    NPN_PrologRuntime(setexception, return);
    FIND_FUNCTION_POINTER(NPN_SetExceptionFP, 
                          qNetscapeFuncs->setexception)(obj, message);
}

// Type conversions
NPString 
NPString::fromQString(const QString &qstr)
{
    NPString npstring;
    const QByteArray qutf8 = qstr.toUtf8();
    npstring.utf8length = qutf8.length();
    npstring.utf8characters = (char*)NPN_MemAlloc(npstring.utf8length);
    memcpy((char*)npstring.utf8characters, qutf8.constData(), npstring.utf8length);
    return npstring;
}

NPString::operator QString() const
{
    return QString::fromUtf8(utf8characters, utf8length);
}

NPVariant 
NPVariant::fromQVariant(QtNPInstance*, const QVariant &qvariant)
{
    NPVariant npvar;
    npvar.type = Null;
    QVariant qvar(qvariant);
    switch(qvariant.type()) {
    case QVariant::Bool:
        npvar.value.boolValue = qvar.toBool();
        npvar.type = Boolean;
        break;
    case QVariant::Int:
        npvar.value.intValue = qvar.toInt();
        npvar.type = Int32;
        break;
    case QVariant::Double:
        npvar.value.doubleValue = qvar.toDouble();
        npvar.type = Double;
        break;
    default: // including QVariant::String
        if (!qvar.convert(QVariant::String))
            break;
        npvar.type = String;
        npvar.value.stringValue = NPString::fromQString(qvar.toString());
        break;
    }
    return npvar;
}

NPVariant::operator QVariant() const
{
    switch(type) {
    case Void:
    case Null:
        return QVariant();
    case Boolean:
        return value.boolValue;
    case Int32:
        return value.intValue;
    case Double:
        return value.doubleValue;
    case String:
        return QString(value.stringValue);
    default:
        break;
    }
    return QVariant();
}

static void 
NPClass_Invalidate(NPObject *npobj)
{
  if (npobj)
    delete npobj->_class;
    npobj->_class = 0;
}

static bool 
NPClass_HasMethod(NPObject*, NPIdentifier)
{
  return false;
}

static bool 
NPClass_Invoke(NPObject*, NPIdentifier, const NPVariant*, uint32, NPVariant*)
{
  return false;
}

static bool 
NPClass_InvokeDefault(NPObject*, const NPVariant*, uint32, NPVariant*)
{
  return false;
}

static bool 
NPClass_HasProperty(NPObject*, NPIdentifier)
{
  return false;
}

static bool 
NPClass_GetProperty(NPObject*, NPIdentifier, NPVariant*)
{
  return false;
}

static bool 
NPClass_SetProperty(NPObject*, NPIdentifier, const NPVariant*)
{
  return false;
}

static bool 
NPClass_RemoveProperty(NPObject *npobj, NPIdentifier name)
{
  NPVariant var;
  return NPClass_SetProperty(npobj, name, &var);
}

NPClass::NPClass(QtNPInstance *This)
{
  structVersion = NP_CLASS_STRUCT_VERSION;
  allocate = 0;
  deallocate = 0;
  invalidate = NPClass_Invalidate;
  hasMethod = NPClass_HasMethod;
  invoke = NPClass_Invoke;
  invokeDefault = NPClass_InvokeDefault;
  hasProperty = NPClass_HasProperty;
  getProperty = NPClass_GetProperty;
  setProperty = NPClass_SetProperty;
  removeProperty = NPClass_RemoveProperty;
  qtnp = This;
  delete_qtnp = false;
}

NPClass::~NPClass()
{
  if (delete_qtnp)
    delete qtnp;
}



// Fills in functiontable used by browser to call entry points in plugin.
extern "C" NPError WINAPI 
NP_GetEntryPoints(NPPluginFuncs* pFuncs)
{
    if(!pFuncs)
        return NPERR_INVALID_FUNCTABLE_ERROR;
    if(!pFuncs->size)
        pFuncs->size = sizeof(NPPluginFuncs);
    else if (pFuncs->size < sizeof(NPPluginFuncs))
        return NPERR_INVALID_FUNCTABLE_ERROR;

    pFuncs->version       = (NP_VERSION_MAJOR << 8) | NP_VERSION_MINOR;
    pFuncs->newp          = MAKE_FUNCTION_POINTER(NPP_New);
    pFuncs->destroy       = MAKE_FUNCTION_POINTER(NPP_Destroy);
    pFuncs->setwindow     = MAKE_FUNCTION_POINTER(NPP_SetWindow);
    pFuncs->newstream     = MAKE_FUNCTION_POINTER(NPP_NewStream);
    pFuncs->destroystream = MAKE_FUNCTION_POINTER(NPP_DestroyStream);
    pFuncs->asfile        = MAKE_FUNCTION_POINTER(NPP_StreamAsFile);
    pFuncs->writeready    = MAKE_FUNCTION_POINTER(NPP_WriteReady);
    pFuncs->write         = MAKE_FUNCTION_POINTER(NPP_Write);
    pFuncs->print         = MAKE_FUNCTION_POINTER(NPP_Print);
    pFuncs->event         = MAKE_FUNCTION_POINTER(NPP_HandleEvent);
    pFuncs->urlnotify     = MAKE_FUNCTION_POINTER(NPP_URLNotify);
    pFuncs->javaClass     = 0;
    pFuncs->getvalue      = MAKE_FUNCTION_POINTER(NPP_GetValue);
    pFuncs->setvalue      = MAKE_FUNCTION_POINTER(NPP_SetValue);
    return NPERR_NO_ERROR;
}

#ifndef Q_WS_X11
extern "C" NPError WINAPI 
NP_Initialize(NPNetscapeFuncs* pFuncs)
{
    if(!pFuncs)
        return NPERR_INVALID_FUNCTABLE_ERROR;

    qNetscapeFuncs = pFuncs;
    int navMajorVers = qNetscapeFuncs->version >> 8;
    int navMinorVers = qNetscapeFuncs->version & 0xff;

    // if the plugin's major version is lower than the Navigator's,
    // then they are incompatible, and should return an error
    if(navMajorVers > NP_VERSION_MAJOR)
        return NPERR_INCOMPATIBLE_VERSION_ERROR;

    // checks if browser supports npruntime
    mozilla_has_npruntime = true;    
    if (navMajorVers == 0 && navMinorVers < 14)
      mozilla_has_npruntime = false;
    if (qNetscapeFuncs->size < (long)&(((NPNetscapeFuncs*)0)->setexception))
      mozilla_has_npruntime = false;
    return NPERR_NO_ERROR;
}
#else
extern "C" NPError WINAPI 
NP_Initialize(NPNetscapeFuncs* nFuncs, NPPluginFuncs* pFuncs)
{
    if(!nFuncs)
        return NPERR_INVALID_FUNCTABLE_ERROR;

    qNetscapeFuncs = nFuncs;
    int navMajorVers = qNetscapeFuncs->version >> 8;
    int navMinorVers = qNetscapeFuncs->version & 0xff;

    // if the plugin's major version is lower than the Navigator's,
    // then they are incompatible, and should return an error
    if(navMajorVers > NP_VERSION_MAJOR)
        return NPERR_INCOMPATIBLE_VERSION_ERROR;

    // checks if browser supports npruntime
    mozilla_has_npruntime = true;    
    if (navMajorVers == 0 && navMinorVers < 14)
      mozilla_has_npruntime = false;
    if (qNetscapeFuncs->size < (long)&(((NPNetscapeFuncs*)0)->setexception))
      mozilla_has_npruntime = false;
    
    // check if the Browser supports the XEmbed protocol
    int supportsXEmbed = 0;
    NPError err = NPN_GetValue(0, NPNVSupportsXEmbedBool, (void *)&supportsXEmbed);
    if (err != NPERR_NO_ERROR ||!supportsXEmbed)
        return NPERR_INCOMPATIBLE_VERSION_ERROR;

    return NP_GetEntryPoints(pFuncs);
}
#endif

extern "C" NPError WINAPI 
NP_Shutdown()
{
    NPP_Shutdown();
    qtns_shutdown();
    qNetscapeFuncs = 0;
    return NPERR_NO_ERROR;
}
