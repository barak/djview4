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

#ifndef QDJVIEWPLUGIN_H
#define QDJVIEWPLUGIN_H

#include <QApplication>
#include <QByteArray>
#include <QList>
#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QUrl>

#ifdef Q_WS_X11

#include "qdjvu.h"
#include "qdjview.h"


class QMutex;
class QSocketNotifier;
class QTimer;



class QDjViewPlugin : public QObject
{
  Q_OBJECT
    
public:
  QDjViewPlugin(QDjVuContext &context);
  ~QDjViewPlugin();
  static QDjViewPlugin *instance();
  int exec();

public slots:
  void viewerClosed();
  void lastViewerClosed();
  void showStatus(QString message);
  void getUrl(QUrl url, QString target);
  void exit(int retcode);
  void quit();

private:
  void  write(int fd, const char *buffer, int size);
  void  read(int fd, char *buffer, int size);
  void    writeString(int fd, QByteArray x);
  void    writeString(int fd, QString x);
  void    writeInteger(int fd, int x);
  void    writeDouble(int fd, double x);
  void    writePointer(int fd, const void *x);
  void    writeArray(int fd, QByteArray x);
  QString    readString(int fd);
  int        readInteger(int fd);
  double     readDouble(int fd);
  void *     readPointer(int fd);
  QByteArray readArray(int fd);

  void cmdNew();
  void cmdAttachWindow();
  void cmdDetachWindow();
  void cmdResize();
  void cmdDestroy();
  void cmdNewStream();
  void cmdWrite();
  void cmdDestroyStream();
  void cmdUrlNotify();
  void cmdPrint();
  void cmdHandshake();
  void cmdShutdown();
  int dispatch();
  
  class Saved;
  class Instance;
  class Stream;

  void reportError(int err);
  void makeStream(int streamid, QUrl url, Instance *instance);
  void getUrl(Instance *instance, QUrl url, QString target);
  void showStatus(Instance *instance, QString message);
  Instance *findInstance(QDjView *djview);

  QDjVuContext    &djvuContext;
  QMutex          *mutex;
  QTimer          *timer;
  QSocketNotifier *notifier;
  QApplication    *application;
  QSet<Instance*>  instances;
  QSet<Stream*>    streams;
  int  pipe_cmd;
  int  pipe_reply;
  int  pipe_request;
  int  return_code;
  bool quit_flag;

};


class QDjViewPlugin::Instance : public QDjVuDocument
{
  Q_OBJECT
public:
  QUrl            url;
  QDjViewPlugin  *dispatcher;
  QDjView        *djview;
  QStringList           args;
  QDjViewPlugin::Saved *saved;
  QDjView::ViewerMode   viewerMode;

  Instance(QUrl url, QDjViewPlugin *parent);
  ~Instance();
  virtual void newstream(int streamid, QString name, QUrl url);
};
  

class QDjViewPlugin::Stream : public QObject
{
  Q_OBJECT
public:
  QUrl                     url;
  QDjViewPlugin::Instance *instance;
  int                      streamid;

  Stream(int streamind, QUrl url, QDjViewPlugin::Instance *parent);
  ~Stream();
};


#endif // Q_WS_X11

#endif // QDJVIEWPLUGIN_H

/* -------------------------------------------------------------
   Local Variables:
   c++-font-lock-extra-types: ( "\\sw+_t" "Q[A-Z]\\sw*[a-z]\\sw*" )
   End:
   ------------------------------------------------------------- */
