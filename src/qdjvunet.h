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

#ifndef QDJVUNET_H
#define QDJVUNET_H

#if AUTOCONF
# include "config.h"
#endif

#include <qdjvu.h>

class QAuthenticator;
class QNetworkAccessManager;

class QDjVuNetDocument : public QDjVuDocument
{
  Q_OBJECT
public:
  ~QDjVuNetDocument();
  QDjVuNetDocument(bool autoDelete=false, QObject *parent=0);
  QDjVuNetDocument(QObject *parent);
  bool setUrl(QDjVuContext *ctx, QUrl url, bool cache=true);
  static QNetworkAccessManager* manager();
  static void setProxy(QUrl proxyurl);
protected:
  virtual void newstream(int streamid, QString name, QUrl url);
signals:
  void authRequired(QString why, QString &user, QString &pass);
  void sslWhiteList(QString why, bool &okay);
  void gotContentType(QString type, bool &okay);
private:
  class Private;
  Private *p;
};

#endif

/* -------------------------------------------------------------
   Local Variables:
   c++-font-lock-extra-types: ( "\\sw+_t" "Q[A-Z]\\sw*[a-z]\\sw*" )
   End:
   ------------------------------------------------------------- */
