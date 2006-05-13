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

#ifndef QDJVUPLUGIN_H
#define QDJVUPLUGIN_H

#ifdef Q_WS_X11


#include <QApplication>
#include <QByteArray>
#include <QList>
#include <QObject>
#include <QSet>
#include <QString>
#include <QUrl>

#include "qdjvu.h"
#include "qdjview.h"


class QDjViewDispatcher;
class QDjVuPluginDocument;
class QSocketNotifier;
class QTimer;



class QDjViewDispatcher : public QObject
{
  Q_OBJECT
    
public:
  QDjViewDispatcher();
  ~QDjViewDispatcher();
  static QDjViewDispatcher *instance();
  int exec();

public slots:
  void viewerClosed();
  void lastViewerClosed();
  void dispatch(); 
  void quit();
  void showStatus(QDjView *viewer, QString message);
  void showStatus(QString message);
  void getUrl(QDjView *viewer, QUrl url, QString target);
  void getUrl(QUrl url, QString target);

protected:
  void cmdNew();
  void cmdAttachWindow();
  void cmdDetachWindow();
  void cmdResize();
  void cmdPrint();
  void cmdNewStream();
  void cmdWrite();
  void cmdDestroyStream();
  void cmdUrlNotify();
  void cmdPrint();
  void cmdHandshake();
  void cmdShutdown();

  
  class PipeError;
  int  write(int fd, const void *buffer, int size);
  int  read(int fd, void *buffer, int size);
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
  
private:
  friend class QDjVuPluginDocument;
  class Stream;

  QTimer          *timer;
  QSocketNotifier *notifier;
  QApplication    *application;
  QSet<QDjView*>   viewers;
  QSet<Stream*>    streams;
  int  pipe_read;
  int  pipe_reply;
  int  pipe_request;
  bool quit_flag;

};



class QDjVuPluginDocument : public QDjVuDocument
{
  Q_OBJECT
protected:
  virtual void newstream(int streamid, QString name, QUrl url);
private:
  friend class QDjViewDispatcher;
  QDjViewPluginDocument(QDjViewDispatcher*, QDjView*);
  const QDjViewDispatcher *dispatcher;
  const QDjView           *viewer;
};


#endif // Q_WS_X11

#endif // QDJVUPLUGIN_H

/* -------------------------------------------------------------
   Local Variables:
   c++-font-lock-extra-types: ( "\\sw+_t" "Q[A-Z]\\sw*[a-z]\\sw*" )
   End:
   ------------------------------------------------------------- */
