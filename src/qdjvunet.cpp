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

#include "qdjvunet.h"
#include "qdjviewprefs.h"
#include <libdjvu/ddjvuapi.h>

#include <QCoreApplication>
#include <QDebug>
#include <QList>
#include <QMap>
#include <QSet>
#include <QPointer>

#if QT_VERSION >= 0x40400

#include <QAuthenticator>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslError>
#include <QSslSocket>

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
  void finished();
  void error(QNetworkReply::NetworkError code);
  void sslErrors(const QList<QSslError>&);
  void authenticationRequired (QNetworkReply *reply, QAuthenticator *auth);
  void proxyAuthenticationRequired (const QNetworkProxy &proxy, QAuthenticator *auth);

private:
  bool doHeaders(QNetworkReply *reply, int streamid);
  bool doAuth(QString why, QAuthenticator *auth);
};

QDjVuNetDocument::Private::Private(QDjVuNetDocument *q)
  : QObject(q), 
    q(q) 
{
  QNetworkAccessManager *mgr = manager();
  connect(mgr, SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)),
          this, SLOT(authenticationRequired(QNetworkReply*,QAuthenticator*)) );
  connect(mgr, SIGNAL(proxyAuthenticationRequired(const QNetworkProxy&,QAuthenticator*)),
          this, SLOT(proxyAuthenticationRequired(const QNetworkProxy&,QAuthenticator*)) );
}

QDjVuNetDocument::Private::~Private()
{
  while (! reqid.isEmpty())
    {
      QMap<QNetworkReply*,int>::iterator it = reqid.begin();
      QNetworkReply *reply = it.key();
      int streamid = it.value();
      if (streamid >= 0)
        ddjvu_stream_close(*q, streamid, true);
      reply->abort();
      reply->deleteLater();
      reqid.remove(reply);
    }
  reqid.clear();
  reqok.clear();
}

bool
QDjVuNetDocument::Private::doHeaders(QNetworkReply *reply, int streamid)
{
  if (streamid >= 0 && !reqok.value(reply, false))
    {
      int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
      QUrl location = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
      QByteArray type = reply->header(QNetworkRequest::ContentTypeHeader).toByteArray();
      // check redirection
      if (location.isValid())
        {
          reqid[reply] = -1;
          QUrl nurl = reply->url().resolved(location);
          if (streamid > 0 || status == 307) {
            q->newstream(streamid, QString(), nurl);
          } else {
            // Permanent redirect on main stream changes the base url.
            ddjvu_document_set_user_data(*q, 0);
            ddjvu_stream_close(*q, streamid, true);
            q->setUrl(ctx, nurl, cache);
          }
          return false;
        }
      // check status code
      if (status != 200 && status != 203 && status != 0)
        {
          reqid[reply] = -1;
          ddjvu_stream_close(*q, streamid, false);
          QString msg = tr("Received http status %1 while retrieving %2.",
                           "%1 is an http status code")
            .arg(status)
            .arg(reply->url().toString());
          emit q->error(msg, __FILE__, __LINE__);
          return false;
        }
      // broadcast content type
      bool okay = !type.startsWith("text/");
      if (! type.isEmpty())
        emit q->gotContentType(type, okay);
      if (! okay) 
        {
          reqid[reply] = -1;
          ddjvu_stream_close(*q, streamid, false);
          QString msg = tr("Received <%1> data while retrieving %2.", 
                           "%1 is a mime type")
            .arg(QString::fromLatin1(type))
            .arg(reply->url().toString());
          emit q->error(msg, __FILE__, __LINE__);
          return false;
        }
      reqok[reply] = true;
      return true;
    }
  return false;
}

void 
QDjVuNetDocument::Private::readyRead()
{
  QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
  if (reply) 
    {
      int streamid = reqid.value(reply, -1);
      bool okay = reqok.value(reply, false);
      if (streamid >= 0 && !okay)
        okay = doHeaders(reply, streamid);
      if (streamid >= 0 && okay)
        {
          QByteArray b = reply->readAll();
          if (b.size() > 0)
            ddjvu_stream_write(*q, streamid, b.data(), b.size());
        }
    }
}

void 
QDjVuNetDocument::Private::error(QNetworkReply::NetworkError code)
{
  QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
  if (reply) 
    {
      int streamid = reqid.value(reply, -1);
      if (streamid >= 0 && code != QNetworkReply::NoError)
        {
          reqid[reply] = -1;
          ddjvu_stream_close(*q, streamid, false);
          QString msg = tr("%1 while retrieving '%2'.")
            .arg(reply->errorString())
            .arg(reply->url().toString());
          emit q->error(msg , __FILE__, __LINE__);
        }
    }
}

void 
QDjVuNetDocument::Private::finished()
{
  QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
  if (reply)
    {
      int streamid = reqid.value(reply, -1);
      bool okay = reqok.value(reply, false);
      if (streamid >= 0 && !okay)
        okay = doHeaders(reply, streamid);
      if (streamid >= 0 && okay)
        {
          ddjvu_stream_close(*q, streamid, false);
          reqid[reply] = -1;
        }
    }
  reqid.remove(reply);
  reqok.remove(reply);
  reply->deleteLater();
}

void 
QDjVuNetDocument::Private::sslErrors(const QList<QSslError>&)
{
  QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
  if (reply)
    {
      static QSet<QString> sslWhiteList;
      QString host = reply->url().host();
      bool okay = sslWhiteList.contains(host);
      if (! okay)
        {
          QString why = tr("Cannot validate the certificate for server %1.").arg(host);
          emit q->sslWhiteList(why, okay);
          if (okay)
            sslWhiteList += host;
        }
      if (okay)
        reply->ignoreSslErrors();
    }
}

bool
QDjVuNetDocument::Private::doAuth(QString why, QAuthenticator *auth)
{
  QString user = auth->user();
  QString pass = QString();
  q->emit authRequired(why, user, pass);
  if (pass.isNull())
    return false;
  auth->setUser(user);
  auth->setPassword(pass);
  return true;
}

void
QDjVuNetDocument::Private::authenticationRequired(QNetworkReply *reply, QAuthenticator *auth)
{
  QString host = reply->url().host();
  QString why = tr("Authentication required for %1 (%2).").arg(auth->realm()).arg(host);
  if (! doAuth(why, auth))
    reply->abort();
}

void
QDjVuNetDocument::Private::proxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *auth)
{
  QString why = tr("Authentication required for proxy %1.").arg(proxy.hostName());
  doAuth(why, auth);
}


/*! \class QDjVuNetDocument
  \brief Represents DjVu documents accessible via the network This class is
  derived from \a QDjVuDocument and reimplements method \a newstream to handle
  documents available throught network requests. */


QDjVuNetDocument::~QDjVuNetDocument()
{
  delete p;
}


/*! Construct a \a QDjVuNetDocument object.
    See \a QDjVuDocument::QDjVuDocument for the other two arguments. */

QDjVuNetDocument::QDjVuNetDocument(bool autoDelete, QObject *parent)
  : QDjVuDocument(autoDelete, parent), 
    p(new QDjVuNetDocument::Private(this))
{
}

/*! \overload */

QDjVuNetDocument::QDjVuNetDocument(QObject *parent)
  : QDjVuDocument(parent), 
    p(new QDjVuNetDocument::Private(this))
{
}

/*! Returns the \a QNetworkAccessManager used to reach the network. */

QNetworkAccessManager* 
QDjVuNetDocument::manager()
{
  static QPointer<QNetworkAccessManager> mgr;
  QObject *app = QCoreApplication::instance();
  if (! mgr)
    mgr = new QNetworkAccessManager(app);
  return mgr;
}

/*! Sets the application proxy using the host, port, user, and password
    specified by url \a proxyUrl.  The proxy type depends on the url scheme.
    Recognized schemes are \a "http", \a "ftp", and \a "socks5".  */

void 
QDjVuNetDocument::setProxy(QUrl proxyUrl)
{
  QNetworkProxy proxy;
  QString scheme = proxyUrl.scheme();
  if (scheme == "http")
    proxy.setType(QNetworkProxy::HttpCachingProxy);
  if (scheme == "ftp")
    proxy.setType(QNetworkProxy::FtpCachingProxy);
  else if (scheme == "socks5")
    proxy.setType(QNetworkProxy::Socks5Proxy);
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
  QString message = tr("Requesting '%1'").arg(url.toString());
  QNetworkRequest request(url);
  QString agent = "Djview/" + QDjViewPrefs::versionString();
  request.setRawHeader("User-Agent", agent.toLatin1());
  QNetworkReply *reply = manager()->get(request);
  connect(reply, SIGNAL(readyRead()), 
          p, SLOT(readyRead()) );
  connect(reply, SIGNAL(finished()),
          p, SLOT(finished()) );
  connect(reply, SIGNAL(sslErrors(const QList<QSslError>&)),
          p, SLOT(sslErrors(const QList<QSslError>&)) );
  connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), 
          p, SLOT(error(QNetworkReply::NetworkError)) );
  p->reqid[reply] = streamid;
  p->reqok[reply] = false;
  emit info(message);
}


/*! \fn 
  void QDjVuNetDocument::authRequired(QString why, QString &user, QString &pass)
  This signal is emitted when a username and password is required.
  String \a why contains a suitable description of the purpose of the username and password.
  Simply set \a user and \a pass and the credentials will be remembered.
 */


/*! \fn QDjVuNetDocument::sslWhiteList(QString host, bool &okay)
  This signal is emitted when there are recoverable errors on a ssl connection
  such as an inability to validate the certificate. String \a why contains
  a description of the problem. Setting \a okay to true allows
  the connection to proceed for the current session. */


// ----------------------------------------
// MOC

#include "qdjvunet.moc"


#else // QT_VERSION < 0x40400

// ----------------------------------------
// STUBS QT < 4.4.0


class QDjVuNetDocument::Private : public QObject
{
  Q_OBJECT
public:
  Private() : QObject() { }
};

QDjVuNetDocument::~QDjVuNetDocument() 
{ }

QDjVuNetDocument::QDjVuNetDocument(bool autoDelete, QObject *parent)
  : QDjVuDocument(autoDelete, parent) 
{ }

QDjVuNetDocument::QDjVuNetDocument(QObject *parent)
  : QDjVuDocument(parent) 
{ }

bool 
QDjVuNetDocument::setUrl(QDjVuContext *ctx, QUrl url, bool cache)
{
  if (url.isValid() && url.scheme() == "file")
    if (url.host().isEmpty() || url.host() == "localhost")
      return setFileName(ctx, url.toLocalFile(), cache);
  return false;
}

QNetworkAccessManager* 
QDjVuNetDocument::manager()
{ return 0; }

void 
QDjVuNetDocument::setProxy(QUrl)
{ }

void 
QDjVuNetDocument::newstream(int, QString, QUrl)
{ }

#endif




/* -------------------------------------------------------------
   Local Variables:
   c++-font-lock-extra-types: ( "\\sw+_t" "Q[A-Z]\\sw*[a-z]\\sw*" )
   End:
   ------------------------------------------------------------- */
