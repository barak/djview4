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
#include "qdjvu.h"

// Npdjvu includes
#include "npdjvu.h"
#include "qtbrowserplugin.h"
#include "qtbrowserplugin_p.h"


/* ------------------------------------------------------------- */


static QDjViewApplication *theApp = 0;


class QtNPForwarder: public QObject 
{
  Q_OBJECT
public:
  QtNPForwarder(QtNPInstance *instance, QObject *parent);
public slots:
  void showStatus(QString message);
  void getUrl(QUrl url, QString target);
  void onChange();
private:
  QtNPInstance *instance;
};


struct QtNPStream {
  QUrl          url;
  QtNPInstance *instance;
  int           streamid;
  bool          started;
  bool          checked;
  bool          closed;
  QtNPStream(int streamid, QUrl url, QtNPInstance *instance);
  ~QtNPStream();
};


class QtNPDocument : public QDjVuDocument
{
  Q_OBJECT
public: 
  QtNPDocument(QtNPInstance *instance);
  virtual void newstream(int streamid, QString name, QUrl url);
private:
  QtNPInstance *instance;
};



/* ------------------------------------------------------------- */


QtNPForwarder::QtNPForwarder(QtNPInstance *instance, QObject *parent)
  : QObject(parent), instance(instance)
{
}

void 
QtNPForwarder::showStatus(QString message)
{
  message.replace(QRegExp("\\s"), " ");
  QByteArray utf8 = message.toUtf8();
  NPN_Status(instance->npp, utf8.constData());
}

void 
QtNPForwarder::getUrl(QUrl url, QString target)
{
  if (instance)
    {
      QByteArray bUrl = url.toEncoded();
      QByteArray bTarget = target.toUtf8();
      NPN_GetURL(instance->npp, bUrl.constData(), 
             bTarget.isEmpty() ? 0 : bTarget.constData());
    }
}

void 
QtNPForwarder::onChange()
{
  if (instance && !instance->onchange.type == NPVariant::String)
    {
      NPVariant res;
      NPString *code = &instance->onchange.value.stringValue;
      NPN_Evaluate(instance->npp, instance->npobject, code, &res);
      NPN_ReleaseVariantValue(&res);
    }
}



/* ------------------------------------------------------------- */


QtNPStream::QtNPStream(int streamid, QUrl url, QtNPInstance *instance)
  : url(url), instance(instance), streamid(streamid), 
    started(false), checked(false), closed(false)
{
  instance->streams.insert(this);
}


QtNPStream::~QtNPStream()
{
  if (instance && instance->streams.contains(this))
    if (instance->document && started && !closed)
      ddjvu_stream_close(*instance->document, streamid, true);
  closed = true;
  started = checked = false;
  instance->streams.remove(this);
}



/* ------------------------------------------------------------- */


static void 
openInstance(QtNPInstance *instance)
{
}


static void 
closeInstance(QtNPInstance *instance)
{
  
}



/* ------------------------------------------------------------- */


QtNPDocument::QtNPDocument(QtNPInstance *instance)
  : QDjVuDocument(true), instance(instance)
{
  QUrl docurl = QDjView::removeDjVuCgiArguments(instance->url);
  setUrl(theApp->djvuContext(), docurl);
}

void
QtNPDocument::newstream(int streamid, QString, QUrl url)
{
  if (streamid > 0)
    {
      new QtNPStream(streamid, url, instance);
      QString message = tr("Requesting %1.").arg(url.toString());
      instance->forwarder->showStatus(message);
      instance->forwarder->getUrl(url, QString());
    }
}




/* ------------------------------------------------------------- */


NPError 
NPP_New(NPMIMEType pluginType, NPP npp,
        uint16 mode, int16 argc, char* argn[],
        char* argv[], NPSavedData* saved)
{
  return NPERR_GENERIC_ERROR;
}


NPError 
NPP_Destroy(NPP npp, NPSavedData** save)
{
  return NPERR_GENERIC_ERROR;
}


NPError 
NPP_SetWindow(NPP npp, NPWindow* window)
{
  return NPERR_GENERIC_ERROR;
}


NPError 
NPP_NewStream(NPP npp, NPMIMEType mime, NPStream* stream, 
              NPBool seekable, uint16 *stype)
{
  Q_UNUSED(mime)
  Q_UNUSED(seekable)
  Q_UNUSED(stype)
  QtNPInstance *instance = (npp) ? (QtNPInstance*)(npp->pdata) : 0;
  QUrl url = QUrl::fromEncoded(stream->url);
  if (instance)
    {
      if (! url.isValid() )
        {
          qWarning("npdjvu: invalid url '%s'\n", stream->url);
          return NPERR_GENERIC_ERROR;
        }
      QtNPStream *npstream = 0;
      if (! instance->url.isValid())
        {
          // first stream
          instance->url = url;
          url = QDjView::removeDjVuCgiArguments(url);
          npstream = new QtNPStream(0, url, instance);
          npstream->checked = true;
        }
      else
        {
          // more streams
          foreach(QtNPStream *s, instance->streams)
            if (! npstream && !s->started)
              if (s->url == url)
                npstream = s;
        }
      // mark started stream
      if (npstream)
        {
          stream->pdata = npstream;
          npstream->started = true;
          if (npstream->streamid == 0)
            openInstance(instance);
        }
    }
  return NPERR_NO_ERROR;
}


NPError 
NPP_DestroyStream(NPP npp, NPStream* stream,
                  NPReason reason)
{
  QtNPInstance *instance = (npp) ? (QtNPInstance*)(npp->pdata) : 0;
  QtNPStream *npstream = (stream) ? (QtNPStream*)(stream->pdata) : 0;
  bool okay = (reason == NPRES_DONE);
  if (instance && npstream && instance->streams.contains(npstream))
    {
      if (instance->document && !npstream->closed)
        ddjvu_stream_close(*instance->document, npstream->streamid, !okay);
      npstream->closed = true;
      if (instance->forwarder)
        instance->forwarder->showStatus(QString());
      delete npstream;
    }
  return NPERR_NO_ERROR;
}


int32 
NPP_WriteReady(NPP npp, NPStream* stream)
{
  Q_UNUSED(npp)
  Q_UNUSED(stream)
  return 0x7fffffff;
}

int32 
NPP_Write(NPP npp, NPStream* stream, int32 offset,
          int32 len, void *buffer)
{
  Q_UNUSED(offset)
  QtNPInstance *instance = (npp) ? (QtNPInstance*)(npp->pdata) : 0;
  QtNPStream *npstream = (stream) ? (QtNPStream*)(stream->pdata) : 0;
  const char *cdata = (const char*)buffer;
  if (instance && npstream && instance->streams.contains(npstream))
    {
      if (!npstream->checked && len > 0)
        {
          // -- if the stream is not a djvu file, 
          //    this is probably a html error message.
          //    But we need at least 8 bytes to check
          npstream->checked = true;
          if (instance->forwarder && len >= 8)
            {
              if (strncmp(cdata, "FORM", 4) && strncmp(cdata+4, "FORM", 4))
                {
                  instance->forwarder->getUrl(npstream->url, QString("_self")); 
                  return len;
                }
            }
        }
      // pass data
      QtNPDocument *document = instance->document;
      if (document)
        ddjvu_stream_write(*document, npstream->streamid, cdata, len);
    }
  return len;
}


void 
NPP_StreamAsFile(NPP npp, NPStream* stream, const char* fname)
{
  Q_UNUSED(npp)
  Q_UNUSED(stream)
  Q_UNUSED(fname)
}


void 
NPP_Print(NPP npp, NPPrint* platformPrint)
{
  QtNPInstance *instance = (npp) ? (QtNPInstance*)(npp->pdata) : 0;
  if (instance && instance->djview)
    {
      instance->djview->print();
      if (platformPrint && platformPrint->mode==NP_FULL)
        platformPrint->print.fullPrint.pluginPrinted=TRUE;
    }
}


int16 
NPP_HandleEvent(NPP npp, NPEvent* event)
{
  QtNPInstance *instance = (npp) ? (QtNPInstance*)(npp->pdata) : 0;
  if (instance)
    return qtns_event(instance, event) ? 1 : 0;
  return NPERR_INVALID_INSTANCE_ERROR;
}


void 
NPP_URLNotify(NPP npp, const char *url, NPReason reason, void *data)
{
  Q_UNUSED(npp)
  Q_UNUSED(url)
  Q_UNUSED(reason)
  Q_UNUSED(data)
}


NPError 
NPP_GetValue(NPP npp, 
             NPPVariable variable, void *value)
{
  Q_UNUSED(npp)
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
NPP_SetValue(NPP npp, 
             NPPVariable variable, void *value)
{
  Q_UNUSED(npp)
  Q_UNUSED(variable)
  Q_UNUSED(value)
  return NPERR_GENERIC_ERROR;
}


/* ------------------------------------------------------------- */

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


/* ------------------------------------------------------------- */


#include "npdjvu.moc"


/* -------------------------------------------------------------
   Local Variables:
   c++-font-lock-extra-types: ( "\\sw+_t" "[A-Z]\\sw*[a-z]\\sw*" "NPP" "QtNP\\sw*")
   End:
   ------------------------------------------------------------- */
