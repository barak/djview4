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

#ifndef QDJVUHTTP_H
#define QDJVUHTTP_H

#include <qdjvu.h>

#include <QList>
#include <QVector>
#include <QHttp>


class QDjVuHttpDocument : public QDjVuDocument
{
  Q_OBJECT

public:
  QDjVuHttpDocument(int nConnections=4, bool autoDelete=false, QObject *parent=0);
  QDjVuHttpDocument(bool autoDelete, QObject *parent=0);
  QDjVuHttpDocument(QObject *parent);
  void setProxy(QString host, int port=8080, QString user="", QString pass="");
  bool setUrl(QDjVuContext *ctx, QUrl url, bool cache=true);
  
protected:
  virtual void newstream(int streamid, QString name, QUrl url);
  
private:
  struct Req  { int streamid; QUrl url; };
  struct Conn { QHttp *http; int reqid; int streamid; };
  QList<Req>    requests;
  QVector<Conn> connections;
  QDjVuContext *ctx;
  QUrl          url;
  bool          cache;
  void schedule(void);
  void init(void);
  
private slots:
  void response(const QHttpResponseHeader &resp);
  void read(const QHttpResponseHeader &resp);
  void finished(int id, bool error);
};







#endif

/* -------------------------------------------------------------
   Local Variables:
   c++-font-lock-extra-types: ( "\\sw+_t" "Q[A-Z]\\sw*[a-z]\\sw*" )
   End:
   ------------------------------------------------------------- */
