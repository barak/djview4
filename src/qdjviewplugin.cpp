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

#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>

#include <QApplication>
#include <QByteArray>
#include <QList>
#include <QMutex>
#include <QMutexLocker>
#include <QObject>
#include <QPair>
#include <QRegExp>
#include <QSet>
#include <QSocketNotifier>
#include <QString>
#include <QTimer>
#include <QUrl>

#include "qdjvu.h"
#include "qdjview.h"
#include "qdjviewplugin.h"

#include <libdjvu/miniexp.h>
#include <libdjvu/ddjvuapi.h>

#ifdef Q_WS_X11


// ========================================
// UTILITIES
// ========================================


struct QDjViewDispatcher::Stream
{
  QUrl url;
  QDjView *viewer;
  QDjVuDocument *document;
  int streamid;
};

typedef QPair<QString,QString> StringPair;

static QList<StringPair>
splitFlags(QString flags)
{
  QList<StringPair> result;
  // TODO
  return result;
}

struct QDjViewDispatcher::Saved
{
  bool valid;
  int zoom;
  int rotation;
  bool sideBySide;
  bool continuous;
  QDjVuWidget::DisplayMode mode;
  QDjVuWidget::Position pos;

  void unpack(int q[4]);
  void pack(int q[4]);
  void save(QDjVuWidget *widget);
  void restore(QDjVuWidget *widget);
};

void 
QDjViewDispatcher::Saved::unpack(int q[4])
{
  // embedding protocol only provides 4 integers
  zoom = q[0] & 0xfff;
  if (zoom & 0x800)
    zoom -= 0x1000;
  rotation = (q[0] & 0x3000) >> 12;
  sideBySide = !! (q[0] & 0x4000);
  continuous = !! (q[0] & 0x8000);
  mode = (QDjVuWidget::DisplayMode)((q[0] & 0xf0000) >> 16);
  pos.anchorRight = false;
  pos.anchorBottom = false;
  pos.inPage = false;
  pos.pageNo = q[1];
  pos.posView.rx() = q[2];
  pos.posView.ry() = q[3];
}

void 
QDjViewDispatcher::Saved::pack(int q[4])
{
  q[0] = 0;
  q[1] = pos.pageNo;
  q[2] = pos.posView.x();
  q[3] = pos.posView.y();
  q[0] |= (rotation & 0x3) << 12;
  if (zoom >= 0)
    q[0] = zoom & 0x7ff;
  else
    q[0] = 0x1000 - ((- zoom) & 0x7ff);
  if (sideBySide)
    q[0] |= 0x4000;
  if (continuous)
    q[0] |= 0x8000;
  q[0] |= (((int)mode) & 0xf) << 16;
}

void 
QDjViewDispatcher::Saved::save(QDjVuWidget *widget)
{
  zoom = widget->zoom();
  rotation = widget->rotation();
  sideBySide = widget->sideBySide();
  continuous = widget->continuous();
  mode = widget->displayMode();
  pos = widget->position();
}

void 
QDjViewDispatcher::Saved::restore(QDjVuWidget *widget)
{
  widget->setZoom(zoom);
  widget->setRotation(rotation);
  widget->setSideBySide(sideBySide);
  widget->setContinuous(continuous);
  widget->setDisplayMode(mode);
  widget->setPosition(pos);
}





// ========================================
// QDJVUPLUGINDOCUMENT
// ========================================



void 
QDjViewDispatcher::makeStream(QUrl url, QDjView *viewer, 
                              QDjVuDocument *document, int streamid)
{
  QMutexLocker lock(mutex);
  Stream *stream = new Stream;
  stream->url = url;
  stream->viewer = viewer;
  stream->document = document;
  stream->streamid = streamid;
  streams.insert(stream);
}


QDjVuPluginDocument::QDjVuPluginDocument(QDjViewDispatcher *dispatcher, 
                                         QDjView *viewer, QUrl url)
  : QDjVuDocument(true),
    dispatcher(dispatcher), 
    viewer(viewer)
{
  setUrl(&dispatcher->djvuContext, url);
  dispatcher->makeStream(url, viewer, this, 0);
}


void 
QDjVuPluginDocument::newstream(int streamid, QString name, QUrl url)
{
  if (streamid > 0)
    {
      dispatcher->makeStream(url, viewer, this, streamid);
      dispatcher->getUrl(viewer, url, QString());
    }
}





// ========================================
// PROTOCOL
// ========================================


enum {
  TYPE_INTEGER = 1,
  TYPE_DOUBLE = 2,
  TYPE_STRING = 3,
  TYPE_POINTER = 4,
  TYPE_ARRAY = 5
};

enum {
  CMD_SHUTDOWN = 0,
  CMD_NEW = 1,
  CMD_DETACH_WINDOW = 2,
  CMD_ATTACH_WINDOW = 3,
  CMD_RESIZE = 4,
  CMD_DESTROY = 5,
  CMD_PRINT = 6,
  CMD_NEW_STREAM = 7,
  CMD_WRITE = 8,
  CMD_DESTROY_STREAM = 9,
  CMD_SHOW_STATUS = 10,
  CMD_GET_URL = 11,
  CMD_GET_URL_NOTIFY = 12,
  CMD_URL_NOTIFY = 13,
  CMD_HANDSHAKE = 14
};

#define OK_STRING   "OK"
#define ERR_STRING  "ERR"


void
QDjViewDispatcher::write(int fd, const char *ptr, int size)
{
  while(size>0)
    {
      errno = 0;
      int res = ::write(fd, ptr, size);
      if (res<0 && errno==EINTR) 
        continue;
      if (res<=0) 
        throw res;
      size -= res; 
      ptr += res;
    }
}

void
QDjViewDispatcher::read(int fd, char *buffer, int size)
{
  char *ptr = buffer;
  while(size>0)
    {
      errno = 0;
      int res = ::read(fd, ptr, size);
      if (res<0 && errno==EINTR)
        continue;
      if (res <= 0) 
        throw res;
      size -= res; 
      ptr += res;
    }
}


void
QDjViewDispatcher::writeString(int fd, QByteArray s)
{
  int type = TYPE_STRING;
  int length = s.size();
  QMutexLocker lock(mutex);
  write(fd, (const char*)&type, sizeof(type));
  write(fd, (const char*)&length, sizeof(length));
  write(fd, s.data(), length);
}


void
QDjViewDispatcher::writeString(int fd, QString x)
{
  writeString(fd, x.toUtf8());
}


void
QDjViewDispatcher::writeInteger(int fd, int x)
{
  int type = TYPE_INTEGER;
  QMutexLocker lock(mutex);
  write(fd, (const char*)&type, sizeof(type));
  write(fd, (const char*)&x, sizeof(x));
}


void
QDjViewDispatcher::writeDouble(int fd, double x)
{
  int type = TYPE_DOUBLE;
  QMutexLocker lock(mutex);
  write(fd, (const char*)&type, sizeof(type));
  write(fd, (const char*)&x, sizeof(x));
}


void
QDjViewDispatcher::writePointer(int fd, const void *x)
{
  int type = TYPE_POINTER;
  QMutexLocker lock(mutex);
  write(fd, (const char*)&type, sizeof(type));
  write(fd, (const char*)&x, sizeof(x));
}


void
QDjViewDispatcher::writeArray(int fd, QByteArray s)
{
  int type = TYPE_ARRAY;
  int length = s.size();
  QMutexLocker lock(mutex);
  write(fd, (const char*)&type, sizeof(type));
  write(fd, (const char*)&length, sizeof(length));
  write(fd, s.data(), length);
}


QString
QDjViewDispatcher::readString(int fd)
{
  int type, length;
  QMutexLocker lock(mutex);
  read(fd, (char*)&type, sizeof(type));
  if (type != TYPE_STRING)
    throw 1;
  read(fd, (char*)&length, sizeof(length));
  if (length < 0) 
    throw 2;
  QByteArray utf(length, 0);
  read(fd, utf.data(), length);
  return QString::fromUtf8(utf);
}


int
QDjViewDispatcher::readInteger(int fd)
{
  int type;
  QMutexLocker lock(mutex);
  read(fd, (char*)&type, sizeof(type));
  if (type != TYPE_INTEGER)
    throw 1;
  int x;
  read(fd, (char*)&x, sizeof(x));
  return x;
}


double
QDjViewDispatcher::readDouble(int fd)
{
  int type;
  QMutexLocker lock(mutex);
  read(fd, (char*)&type, sizeof(type));
  if (type != TYPE_DOUBLE)
    throw 1;
  double x;
  read(fd, (char*)&x, sizeof(x));
  return x;
}


void*
QDjViewDispatcher::readPointer(int fd)
{
  int type;
  QMutexLocker lock(mutex);
  read(fd, (char*)&type, sizeof(type));
  if (type != TYPE_POINTER)
    throw 1;
  void *x;
  read(fd, (char*)&x, sizeof(x));
  return x;
}


QByteArray
QDjViewDispatcher::readArray(int fd)
{
  int type, length;
  QMutexLocker lock(mutex);
  read(fd, (char*)&type, sizeof(type));
  if (type != TYPE_STRING)
    throw 1;
  read(fd, (char*)&length, sizeof(length));
  if (length < 0) 
    throw 2;
  QByteArray x(length, 0);
  read(fd, x.data(), length);
  return x;
}



// ========================================
// REQUESTS AND SLOTS
// ========================================



void
QDjViewDispatcher::viewerClosed()
{
  QDjView *viewer = qobject_cast<QDjView*>(sender());
  if (viewer)
    {
      viewers.remove(viewer);
      QList<Stream*> streamList = streams.toList();
      foreach(Stream *s, streamList)
        if (s->viewer == viewer)
          {
            streams.remove(s);
            ddjvu_stream_close(*s->document, s->streamid, true);
            delete s;
          }
    }
}


void
QDjViewDispatcher::lastViewerClosed()
{
  timer->stop();
  timer->start(5*60*1000);
}


void
QDjViewDispatcher::showStatus(QDjView *viewer, QString message)
{
  try
    {
      message.replace(QRegExp("\\s"), " ");
      QMutexLocker lock(mutex);
      writeInteger(pipe_request, CMD_SHOW_STATUS);
      writePointer(pipe_request, (void*) viewer);
      writeString(pipe_request, message);
    }
  catch(...)
    {
      exit(10);
    }
}


void
QDjViewDispatcher::showStatus(QString message)
{
  QDjView *viewer = qobject_cast<QDjView*>(sender());
  if (viewer)
    showStatus(viewer, message);
}


void
QDjViewDispatcher::getUrl(QDjView *viewer, QUrl url, QString target)
{
  try
    {
      QMutexLocker lock(mutex);
      writeInteger(pipe_request, CMD_GET_URL);
      writePointer(pipe_request, (void*) viewer);
      writeString(pipe_request, url.toEncoded());
      writeString(pipe_request, target);
    }
  catch(...)
    {
      exit(10);
    }
}


void
QDjViewDispatcher::getUrl(QUrl url, QString target)
{
  QDjView *viewer = qobject_cast<QDjView*>(sender());
  if (viewer)
    getUrl(viewer, url, target);
}





// ========================================
// COMMANDS
// ========================================



void
QDjViewDispatcher::cmdNew()
{
  // read arguments
  bool fullPage = !!readInteger(pipe_cmd);
  QString djvuDir = readString(pipe_cmd);
  int argc = readInteger(pipe_cmd);
  QList<StringPair> args;
  QUrl url;
  for (int i=0; i<argc; i++)
    {
      QString key = readString(pipe_cmd);
      QString val = readString(pipe_cmd);
      if (key.toLower() == "url")
        url = QUrl(val);
      else if (key.toLower() == "flags")
        args << splitFlags(val);
      else
        args << StringPair(key, val);
    }
  Saved saved;
  saved.valid = !!readInteger(pipe_cmd);
  if (saved.valid)
    {
      int q[4];
      q[0] = readInteger(pipe_cmd);
      q[1] = readInteger(pipe_cmd);
      q[2] = readInteger(pipe_cmd);
      q[3] = readInteger(pipe_cmd);
      saved.unpack(q);
    }
  // checks
  if (! url.isValid())
    {
      writeString(pipe_reply, QByteArray(ERR_STRING));
      writeString(pipe_reply, QString("Url is invalid"));
      return;
    }
  // create viewer and document
  timer->stop();
  QDjView::ViewerMode viewerMode = QDjView::EMBEDDED_PLUGIN;
  if (fullPage)
    viewerMode = QDjView::FULLPAGE_PLUGIN;
  QDjView *viewer = new QDjView(djvuContext, viewerMode);
  QDjVuDocument *doc = new QDjVuPluginDocument(this, viewer, url);
  viewer->open(doc, url);
  // apply flags
  foreach(StringPair pair, args)
    if (pair.first.toLower() != "url")
      viewer->parseArgument(pair.first, pair.second);
  // apply saved data
  if (saved.valid)
    saved.restore(viewer->getDjVuWidget());
  // success
  viewers.insert(viewer);
  writeString(pipe_reply, QByteArray(OK_STRING));
  writePointer(pipe_reply, (void*)viewer);
}


void
QDjViewDispatcher::cmdAttachWindow()
{
}


void
QDjViewDispatcher::cmdDetachWindow()
{
}


void
QDjViewDispatcher::cmdResize()
{
}


void
QDjViewDispatcher::cmdNewStream()
{
}


void
QDjViewDispatcher::cmdWrite()
{
}


void
QDjViewDispatcher::cmdDestroyStream()
{
}


void
QDjViewDispatcher::cmdUrlNotify()
{
}


void
QDjViewDispatcher::cmdPrint()
{
}


void
QDjViewDispatcher::cmdHandshake()
{
}


void
QDjViewDispatcher::cmdShutdown()
{
}


void
QDjViewDispatcher::dispatch()
{
}

 


// ========================================
// DISPATCHER PROPER
// ========================================



static QDjViewDispatcher *theDispatcher;


QDjViewDispatcher::QDjViewDispatcher(QDjVuContext &context)
  : djvuContext(context),
    mutex(0),
    timer(0),
    notifier(0),
    application(0),
    pipe_cmd(3),
    pipe_reply(4),
    pipe_request(5),
    return_code(0),
    quit_flag(false)
{
  // Disable SIGPIPE
#ifdef SIGPIPE
# ifdef SA_RESTART
  sigset_t mask;
  struct sigaction act;
  sigemptyset(&mask);
  sigaddset(&mask, SIGPIPE);
  sigprocmask(SIG_BLOCK, &mask, 0);
  sigaction(SIGPIPE, 0, &act);
  act.sa_handler = SIG_IGN;
  sigaction(SIGPIPE, &act, 0);
# else
  signal(SIGPIPE, SIG_IGN);
# endif
#endif
  // Prepare mutex
  mutex = new QMutex();
  // Prepare timer
  timer = new QTimer(this);
  timer->setSingleShot(true);
  connect(timer, SIGNAL(timeout()), this, SLOT(quit()));
  // Set global dispatcher
  if (theDispatcher)
    qWarning("Constructing multiple dispatchers");
  theDispatcher = this;
}


QDjViewDispatcher::~QDjViewDispatcher()
{
  QList<Stream*> streamList = streams.toList();
  QList<QDjView*> viewerList = viewers.toList();
  streams.clear();
  viewers.clear();
  foreach(Stream *s, streamList) 
    ddjvu_stream_close(*s->document, s->streamid, true);
  foreach(Stream *s, streamList) 
    delete(s);
  foreach(QDjView *v, viewerList)
    v->close();
  delete application;
  delete notifier;
  delete timer;
  delete mutex;
  theDispatcher = 0;
}


QDjViewDispatcher *
QDjViewDispatcher::instance()
{
  return theDispatcher;
}


void 
QDjViewDispatcher::exit(int retcode)
{
  return_code = retcode;
  quit_flag = true;
  if (timer)
    timer->stop();
  if (application)
    application->exit(retcode);
}


void 
QDjViewDispatcher::quit()
{
  this->exit(0);
}


int 
QDjViewDispatcher::exec()
{
  return 0;
}


// ----------------------------------------
// END


#endif // Q_WS_X11

/* -------------------------------------------------------------
   Local Variables:
   c++-font-lock-extra-types: ( "\\sw+_t" "[A-Z]\\sw*[a-z]\\sw*" )
   End:
   ------------------------------------------------------------- */
