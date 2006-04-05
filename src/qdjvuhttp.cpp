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

#include "qdjvuhttp.h"
#include <libdjvu/ddjvuapi.h>

#include <Qt/QDebug>



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
              this, SLOT(read(const QHttpResponseHeader&)) );
      connect(http, SIGNAL(requestFinished(int,bool)), 
              this, SLOT(finished(int,bool)) );
    }
}


/*! Construct a \a QDjVuHttpDocument object that can perform
    up to \a nconnections simultaneous HTTP connections. */

QDjVuHttpDocument::QDjVuHttpDocument(int nconnections, QObject *p)
  : QDjVuDocument(p), connections(nconnections), ctx(0)
{
  init();
}


/*! Convenience constructor that allows 4 simultaneous HTTP connections. */

QDjVuHttpDocument::QDjVuHttpDocument(QObject *p)
  : QDjVuDocument(p), connections(4), ctx(0)
{
  init();
}

/*! Convenience constructor that calls \a setUrl 
  to specify the target url. */

QDjVuHttpDocument::QDjVuHttpDocument(QDjVuContext *ctx, QUrl url, QObject *p)
  : QDjVuDocument(p), connections(4), ctx(0)
{
  init();
  setUrl(ctx, url, true);
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
      if (url.scheme()=="http" && ! url.host().isEmpty())
        QDjVuDocument::setUrl(ctx, url, cache);
      else if (url.scheme()=="file" && url.host().isEmpty())
        QDjVuDocument::setFileName(ctx, url.toLocalFile(), cache);
      else
        okay = false;
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
  qWarning("QDjVuHttoDocument::setUrl: unrecognized url");
  return false;
}


void 
QDjVuHttpDocument::newstream(int streamid, QString name, QUrl url)
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
      QString m = QString("Requesting %1").arg(req.url.toString());
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
          if (resp.hasKey("Location"))
            {
              QUrl nurl = url.resolved(QUrl(resp.value("Location")));
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
        // HTTP errors
        if (status != 200 && status != 203)
          {
            if (conn.streamid >= 0)
              ddjvu_stream_close(*this, conn.streamid, false);
            conn.streamid = -1;
            QString msg = QString("Http status %1").arg(status);
            emit error(msg, __FILE__, __LINE__);
          }
        return;
      }
}

void 
QDjVuHttpDocument::read(const QHttpResponseHeader& resp)
{
  for (int c=0; c<connections.size(); c++)
    if ( connections[c].http == sender() )
      {
        Conn& conn = connections[c];
        QByteArray b = conn.http->readAll();
        if (conn.streamid >= 0)
          ddjvu_stream_write(*this, conn.streamid, b.data(), b.size());
      }
}

void 
QDjVuHttpDocument::finished(int id, bool err)
{
  for (int c=0; c<connections.size(); c++)
    if (connections[c].reqid == id)
      {
        Conn& conn = connections[c];
        if (err)
          emit error(conn.http->errorString() , __FILE__, __LINE__);
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
