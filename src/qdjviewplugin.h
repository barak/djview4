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
class QEventLoop;


class QDjViewPlugin : public QObject
{
  Q_OBJECT
    
public:
  ~QDjViewPlugin();
  QDjViewPlugin(const char *progname);
  static QDjViewPlugin *instance();
  int  exec();
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
  QByteArray readRawString(int fd);
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
  
  class  Document;
  class  Forwarder;
  struct Instance;
  struct Stream;

  void reportError(int err);
  void streamCreated(Stream *s);
  void streamDestroyed(Stream *s);
  Instance *findInstance(QDjView *djview);
  void getUrl(Instance *instance, QUrl url, QString target);
  void showStatus(Instance *instance, QString message);
  void dispatch();
  void lastViewerClosed();
  void registerForDeletion(QObject*);

  QByteArray       progname;
  QDjVuContext    *context;
  QTimer          *timer;
  QSocketNotifier *notifier;
  QApplication    *application;
  Forwarder       *forwarder;
  QSet<Instance*>  instances;
  QSet<Stream*>    streams;
  bool             xembedFlag;
  int              argc;
  const char*      argv[4];
  QEventLoop   *eventLoop;
  QObjectList   pendingDelete;
  int           returnCode;
  bool          quitFlag;
  int        pipeRead;
  int        pipeWrite;
  int        pipeRequest;
};

class QDjViewPlugin::Document : public QDjVuDocument
{
  Q_OBJECT
public: 
  QDjViewPlugin::Instance * const instance;
  Document(QDjViewPlugin::Instance *instance);
  virtual void newstream(int streamid, QString name, QUrl url);
};


class QDjViewPlugin::Forwarder : public QObject
{
  Q_OBJECT
public:
  QDjViewPlugin * const dispatcher;
  Forwarder(QDjViewPlugin *dispatcher);
  virtual bool eventFilter(QObject*, QEvent*);
public slots:
  void showStatus(QString message);
  void getUrl(QUrl url, QString target);
  void quit();
  void dispatch();
  void lastViewerClosed();
};





#endif // Q_WS_X11

#endif // QDJVIEWPLUGIN_H

/* -------------------------------------------------------------
   Local Variables:
   c++-font-lock-extra-types: ( "\\sw+_t" "Q[A-Z]\\sw*[a-z]\\sw*" )
   End:
   ------------------------------------------------------------- */
