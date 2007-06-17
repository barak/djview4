//C-  -*- C++ -*-
//C- -------------------------------------------------------------------
//C- DjView4
//C- Copyright (c) 2006  Leon Bottou
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

#if AUTOCONF
# include "config.h"
#endif

#include "qdjvuhttp.h"
#include <libdjvu/ddjvuapi.h>

#include <QDebug>


/*! \class QDjVuHttpDocument
  \brief Represents DjVu documents accessible via HTTP.
  This class is derived from \a QDjVuDocument
  and reimplements method \a newstream to handle 
  documents available throught HTTP requests. */

void
QDjVuHttpDocument::init(void)
{
  for (int i=0; i<connections.size(); i++)
    {
      connections[i].http = new QHttp(this);
      connections[i].reqid = 0;
      connections[i].streamid = -1;
      QHttp *http = connections[i].http;
      connect(http, SIGNAL(responseHeaderReceived(const QHttpResponseHeader&)),
              this, SLOT(response(const QHttpResponseHeader&)) );
      connect(http, SIGNAL(readyRead(const QHttpResponseHeader&)), 
              this, SLOT(read(void)) );
      connect(http, SIGNAL(requestFinished(int,bool)), 
              this, SLOT(finished(int,bool)) );
    }
}


/*! Construct a \a QDjVuHttpDocument object that can perform
    up to \a nConnections simultaneous HTTP connections. 
    See \a QDjVuDocument::QDjVuDocument for the explanation
    of the other two arguments. */

QDjVuHttpDocument::QDjVuHttpDocument(int nConnections, bool autoDelete, 
                                     QObject *parent)
  : QDjVuDocument(autoDelete, parent), 
    connections(nConnections), 
    ctx(0)
{
  init();
}


/*! \overload */

QDjVuHttpDocument::QDjVuHttpDocument(bool autoDelete, QObject *parent)
  : QDjVuDocument(autoDelete, parent), 
    connections(2), 
    ctx(0)
{
  init();
}


/*! \overload */

QDjVuHttpDocument::QDjVuHttpDocument(QObject *parent)
  : QDjVuDocument(parent), 
    connections(2), 
    ctx(0)
{
  init();
}



/*! Destructor.
  Destroying the http document stops all streams. */

QDjVuHttpDocument::~QDjVuHttpDocument()
{
  for (int i=0; i<connections.size(); i++)
    if (connections[i].streamid >= 0)
      ddjvu_stream_close(*this, connections[i].streamid, true);
  foreach(Req req, requests)
    if (req.streamid >= 0)
      ddjvu_stream_close(*this, req.streamid, true);
}



/*! Specify an HTTP proxy. */

void 
QDjVuHttpDocument::setProxy(QString host, int port, QString us, QString pw)
{
  for (int i=0; i<connections.size(); i++)
    connections[i].http->setProxy(host,port,us,pw);
}


/*! Associates the \a QDjVuDocument object with
    with the \a QDjVuContext object \ctx in order
    to decode the DjVu data located at URL \a url.
    Only HTTP and FILE urls are recognized. */

bool 
QDjVuHttpDocument::setUrl(QDjVuContext *ctx, QUrl url, bool cache)
{
  bool okay = false;
  if (url.isValid())
    {
      okay = true;
      QString scheme = url.scheme().toLower();
      if (scheme == "http")
        QDjVuDocument::setUrl(ctx, url, cache);
#if QT_VERSION >= 0x40300
      else if (scheme == "https")
        QDjVuDocument::setUrl(ctx, url, cache);
#endif
      else if (scheme == "file" && url.host().isEmpty())
        QDjVuDocument::setFileName(ctx, url.toLocalFile(), cache);
      else
        emit error(tr("Unsupported url scheme '%1:'.").arg(scheme), 
                   __FILE__, __LINE__ );
      if (! isValid())
        okay = false;
    }
  if (okay)
    {
      this->url = url;
      this->ctx = ctx;
      this->cache = cache;
      return true;
    }
  qWarning("QDjVuHttpDocument::setUrl: unrecognized url");
  return false;
}


void 
QDjVuHttpDocument::newstream(int streamid, QString, QUrl url)
{
  Req req;
  req.streamid = streamid;
  req.url = url;
  // Note: this is a stack, not a queue, because
  // the last request is likely to block viewing
  // the pages that the user is trying to view now.
  requests.append(req);
  schedule();
}


void 
QDjVuHttpDocument::schedule(void)
{
  while (requests.size() > 0)
    {
      // search free connection
      int c;
      for (c=0; c<connections.size(); c++)
        if (connections[c].reqid == 0)
          break;
      if (c >= connections.size())
        return;
      // initiate http transaction
      Conn& conn = connections[c];
      Req req = requests.last();
      requests.removeLast();
      if (req.url.port() > 0)
        conn.http->setHost(req.url.host(), req.url.port());
      else
        conn.http->setHost(req.url.host());
      if (! req.url.userName().isEmpty())
        conn.http->setUser(req.url.userName(), req.url.password());
      conn.streamid = req.streamid;
      QString g = req.url.toEncoded(QUrl::RemoveAuthority|QUrl::RemoveScheme);
      conn.reqid = conn.http->get(g);
      QString m = tr("Requesting '%1'").arg(req.url.toString());
      emit info(m);
    }
}


void
QDjVuHttpDocument::response(const QHttpResponseHeader &resp)
{
  int status = resp.statusCode();
  for (int c=0; c<connections.size(); c++)
    if ( connections[c].http == sender() )
      {
        Conn& conn = connections[c];
        // HTTP redirect
        if (status == 301 || status == 302 || status == 303 || status == 307 )
          if (resp.hasKey("location"))
            {
              QUrl nurl = url.resolved(QUrl(resp.value("location")));
              // TODO: detect loops
              if (conn.streamid > 0 || status == 307)
                { 
                  newstream(conn.streamid, QString(), nurl);
                  conn.streamid = -1;
                  return;
                }
              // Permanent redirect on main stream changes the base url.
              ddjvu_stream_close(*this, conn.streamid, false);
              conn.streamid = -1;
              setUrl(ctx, nurl, cache);
              if (isValid())
                return;
            }
        // HTTP and Content-Type errors
        QString msg;
        QString type = resp.contentType();
        if (type.startsWith("text/"))
          msg = tr("Received %1 data while retrieving %2.",
                   "%1 is a mime type").arg(type);
        if (status != 200 && status != 203)
          msg = tr("Received http status %1 while retrieving %2.",
                   "%1 is an http status code").arg(status);
        if (! msg.isEmpty())
          {
            if (conn.streamid >= 0)
              ddjvu_stream_close(*this, conn.streamid, false);
            conn.streamid = -1;
            emit error(msg.arg(url.toString()), __FILE__, __LINE__);
          }
        return;
      }
}

void 
QDjVuHttpDocument::read(void)
{
  for (int c=0; c<connections.size(); c++)
    if ( connections[c].http == sender() )
      {
        Conn& conn = connections[c];
        QByteArray b = conn.http->readAll();
        if (conn.streamid >= 0 && b.size() > 0)
          ddjvu_stream_write(*this, conn.streamid, b.data(), b.size());
      }
}

void 
QDjVuHttpDocument::finished(int id, bool err)
{
  read();
  for (int c=0; c<connections.size(); c++)
    if (connections[c].reqid == id)
      {
        Conn& conn = connections[c];
        if (err)
          {
            QString msg = tr("%1 while retrieving '%2'.")
              .arg(conn.http->errorString())
              .arg(url.toString());
            emit error(msg , __FILE__, __LINE__);
          }
        if (conn.streamid >= 0)
          ddjvu_stream_close(*this, conn.streamid, false);
        conn.reqid = 0;
        conn.streamid = -1;
      }
  schedule();
}




/* -------------------------------------------------------------
   Local Variables:
   c++-font-lock-extra-types: ( "\\sw+_t" "Q[A-Z]\\sw*[a-z]\\sw*" )
   End:
   ------------------------------------------------------------- */
