//C-  -*- C++ -*-
//C- -------------------------------------------------------------------
//C- DjView4
//C- Copyright (c) 2006-  Leon Bottou
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

#if AUTOCONF
# include "config.h"
#endif

#include <QAuthenticator>
#include <QCoreApplication>
#include <QList>
#include <QMap>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPointer>
#include <QSslError>

#include "qdjvunet.h"
#include "qdjviewprefs.h"
#include <libdjvu/ddjvuapi.h>


class QDjVuNetDocument::Private : public QObject
{
  Q_OBJECT

public:
  Private(QDjVuNetDocument *q);
  ~Private();

  QDjVuNetDocument *q;
  QMap<QNetworkReply*, int> reqid;
  QMap<QNetworkReply*, bool> reqok;
  QDjVuContext *ctx;
  QUrl url;
  bool cache;

public slots:
  void readyRead();
  void readyRead(QNetworkReply *reply);
  void finished(QNetworkReply *reply);
  void sslErrors(QNetworkReply *reply, const QList<QSslError>&);

};


QDjVuNetDocument::Private::Private(QDjVuNetDocument *q)
  : QObject(q), 
    q(q) 
{
}


QDjVuNetDocument::Private::~Private()
{
  QMap<QNetworkReply*,int>::iterator it;
  for(it = reqid.begin(); it != reqid.end(); ++it)
    {
      QNetworkReply *reply = it.key();
      int streamid = it.value();
      if (streamid >= 0)
        ddjvu_stream_close(*q, streamid, true);
      reply->abort();
      reply->deleteLater();
    }
  reqid.clear();
  reqok.clear();
}


void 
QDjVuNetDocument::Private::readyRead()
{
  QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
  if (reply) 
    readyRead(reply);
}


void 
QDjVuNetDocument::Private::readyRead(QNetworkReply *reply)
{
  QByteArray b = reply->readAll();
  int streamid = reqid.value(reply, -1);
  if (streamid >= 0 && b.size() > 0)
    {
      if (! reqok.value(reply, false))
        {
          int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
          QUrl location = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
          QByteArray type = reply->header(QNetworkRequest::ContentTypeHeader).toByteArray();
          // check redirection
          if (location.isValid())
            {
              reqid[reply] = -1;
              QUrl nurl = reply->url().resolved(location);
              if (streamid > 0 || status == 307)
                { 
                  q->newstream(streamid, QString(), nurl);
                  return;
                }
              // Permanent redirect on main stream changes the base url.
              ddjvu_stream_close(*q, streamid, false);
              q->setUrl(ctx, nurl, cache);
              if (q->isValid())
                return;
            }
          // check status code
          if (status != 200 && status != 203 && status != 0)
            {
              QString msg = tr("Received http status %1 while retrieving %2.",
                       "%1 is an http status code")
                .arg(status)
                .arg(reply->url().toString());
              emit q->error(msg, __FILE__, __LINE__);
              ddjvu_stream_close(*q, streamid, false);
              reqid[reply] = -1;
              return;
            }
          // check content type
          if (type.startsWith("text/"))
            {
              QString msg = tr("Received <%1> data while retrieving %2.", 
                               "%1 is a mime type")
                .arg(QString::fromLatin1(type))
                .arg(reply->url().toString());
              emit q->error(msg, __FILE__, __LINE__);
            }
          reqok[reply] = true;
        }
      // process data
      ddjvu_stream_write(*q, streamid, b.data(), b.size());
    }
}


void 
QDjVuNetDocument::Private::finished(QNetworkReply *reply)
{
  int streamid = reqid.value(reply, -1);
  if (streamid >= 0)
    {
      if (reply->bytesAvailable() > 0)
        readyRead(reply);
      if (reply->error() != QNetworkReply::NoError)
        {
          QString msg = tr("%1 while retrieving '%2'.")
            .arg(reply->errorString())
            .arg(url.toString());
          emit q->error(msg , __FILE__, __LINE__);
        }
      ddjvu_stream_close(*q, streamid, false);
    }
  reqid.remove(reply);
  reqok.remove(reply);
  reply->deleteLater();
}


void 
QDjVuNetDocument::Private::sslErrors(QNetworkReply *reply, 
                                     const QList<QSslError>& )
{
  /// TODO: keep a list of trusted hosts.
  reply->ignoreSslErrors();
}


/*! \class QDjVuNetDocument
  \brief Represents DjVu documents accessible via the network This class is
  derived from \a QDjVuDocument and reimplements method \a newstream to handle
  documents available throught network requests. */


/*! Construct a \a QDjVuNetDocument object that can perform
    up to \a nConnections simultaneous HTTP connections. 
    See \a QDjVuDocument::QDjVuDocument for the explanation
    of the other two arguments. */

QDjVuNetDocument::QDjVuNetDocument(bool autoDelete, QObject *parent)
  : QDjVuDocument(autoDelete, parent), 
    p(new QDjVuNetDocument::Private(this))
{
}

QDjVuNetDocument::~QDjVuNetDocument()
{
  delete p;
}


/*! \overload */

QDjVuNetDocument::QDjVuNetDocument(QObject *parent)
  : QDjVuDocument(parent), 
    p(new QDjVuNetDocument::Private(this))
{
  QNetworkAccessManager *mgr = manager();
  connect(mgr, SIGNAL(finished(QNetworkReply*)),
          p, SLOT(finished(QNetworkReply*)) );
  connect(mgr, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslErrors>&)),
          p, SLOT(sslErrors(QNetworkReply*, const QList<QSslErrors>&)) );
  
}

QNetworkAccessManager* 
QDjVuNetDocument::manager()
{
  static QPointer<QNetworkAccessManager> mgr;
  QObject *app = QCoreApplication::instance();
  if (! mgr)
    {
      mgr = new QNetworkAccessManager(app);
      // maybe set cache
      // maybe set ssl
    }
  return mgr;
}

/*! Sets the application proxy using the host, port, user, and password
    specified by url \a proxyUrl.  The proxy type depends on the url scheme.
    Recognized schemes are \a "http" and \a "ftp" for caching proxies, 
    \a "tp-socks5" and \a "tp-http" for transparent proxies.  */

void 
QDjVuNetDocument::setProxy(QUrl proxyUrl)
{
  QNetworkProxy proxy;
  QString scheme = proxyUrl.scheme();
  if (scheme == "http")
    proxy.setType(QNetworkProxy::HttpCachingProxy);
  if (scheme == "ftp")
    proxy.setType(QNetworkProxy::FtpCachingProxy);
  else if (scheme == "tp-socks5" || scheme == "socks5")
    proxy.setType(QNetworkProxy::Socks5Proxy);
  else if (scheme == "tp-http")
    proxy.setType(QNetworkProxy::HttpProxy);
  proxy.setHostName(proxyUrl.host());
  proxy.setPort(proxyUrl.port());
  proxy.setUser(proxyUrl.userName());
  proxy.setPassword(proxyUrl.password());
  QNetworkProxy::setApplicationProxy(proxy);
}


/*! Associates the \a QDjVuDocument object with
    with the \a QDjVuContext object \ctx in order
    to decode the DjVu data located at URL \a url.  */

bool 
QDjVuNetDocument::setUrl(QDjVuContext *ctx, QUrl url, bool cache)
{
  if (url.isValid())
    {
      if (url.scheme() == "file" && url.host().isEmpty())
        QDjVuDocument::setFileName(ctx, url.toLocalFile(), cache);
      else
        QDjVuDocument::setUrl(ctx, url, cache);
    }
  if (isValid())
    {
      p->url = url;
      p->ctx = ctx;
      p->cache = cache;
      return true;
    }
  return false;
}


/* Perform a request. */

void 
QDjVuNetDocument::newstream(int streamid, QString, QUrl url)
{
  QNetworkRequest request(url);
  QString agent = "Djview/" + QDjViewPrefs::versionString();
  request.setRawHeader("User-Agent", agent.toAscii());
  QNetworkReply *reply = manager()->get(request);
  connect(reply, SIGNAL(readyRead()), p, SLOT(readyRead()));
  emit info(tr("Requesting '%1'").arg(url.toString()));
  p->reqid[reply] = streamid;
  p->reqok[reply] = false;
}


// ----------------------------------------
// MOC

#include "qdjvunet.moc"



/* -------------------------------------------------------------
   Local Variables:
   c++-font-lock-extra-types: ( "\\sw+_t" "Q[A-Z]\\sw*[a-z]\\sw*" )
   End:
   ------------------------------------------------------------- */
