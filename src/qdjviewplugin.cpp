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

#include <QApplication>
#include <QByteArray>
#include <QDebug>
#include <QDesktopWidget>
#include <QEvent>
#include <QEventLoop>
#include <QHBoxLayout>
#include <QList>
#include <QObject>
#include <QPair>
#include <QPointer>
#include <QRegExp>
#include <QSet>
#include <QSocketNotifier>
#include <QString>
#include <QTimer>
#include <QTimerEvent>
#include <QUrl>
#include <QWidget>

#include "qdjvu.h"
#include "qdjview.h"
#include "qdjviewplugin.h"
#include "djview.h"

#include <libdjvu/miniexp.h>
#include <libdjvu/ddjvuapi.h>

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#ifdef Q_OS_UNIX
# include <unistd.h>
# include <fcntl.h>
#endif
#ifdef Q_OS_WIN32
# include <io.h>
# include <fcntl.h>
#endif

#if QT_VERSION < 0x50000
# if defined(Q_WS_X11)
#  include <QX11Info>
#  include <QX11EmbedWidget>
#  define HAVE_QX11EMBED 1
# endif
#endif

#if QT_VERSION >= 0x50000
# include <QWindow>
# include <QAbstractNativeEventFilter>
# define HAVE_QT5EMBED 1
#endif

#if WITH_X11
# ifndef X_DISPLAY_MISSING
#  if QT_VERSION < 0x50000
#   ifdef Q_WS_X11
#    define HAVE_X11 1
#   endif
#  endif
# endif
# if HAVE_X11
#  include <X11/Xlib.h>
#  include <X11/Xutil.h>
#  include <X11/Xatom.h>
#  undef FocusOut
#  undef FocusIn
#  undef KeyPress
#  undef KeyRelease
#  undef None
#  undef RevertToParent
#  undef GrayScale
#  undef CursorShape
# endif
#endif



// ========================================
// STRUCTURES
// ========================================



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
#if HAVE_X11
  virtual bool eventFilter(QObject*, QEvent*);
#endif

public slots:
  void containerClosed();
  void showStatus(QString message);
  void getUrl(QUrl url, QString target);
  void onChange();
  void quit();
  void dispatch();
  void lastViewerClosed();
  void continueExec();
};


// QDjViewPlugin, Stream and Instance cannot be QObjects
// because subtle errors occur when one creates QObjects 
// before the QApplication. 


struct QDjViewPlugin::Stream
{
  typedef QDjViewPlugin::Instance Instance;
  QUrl      url;
  Instance *instance;
  int       streamid;
  bool      started;
  bool      checked;
  bool      closed;
  ~Stream();
  Stream(int streamid, QUrl url, Instance *instance);
};


struct QDjViewPlugin::Instance
{
  QUrl                              url;
  QDjViewPlugin                    *dispatcher;
  QPointer<QDjViewPlugin::Document> document;
  QPointer<QWidget>                 shell;
  QPointer<QDjView>                 djview;
  QPointer<QObject>                 containerwin;
  WId                               containerid;
  QStringList                    args;
  QByteArray                     saved;
  int                            savedformat;
  int                            onchange;
  QDjView::ViewerMode            viewerMode;
  ~Instance();
  Instance(QDjViewPlugin *dispatcher);
  void open();
  void destroy();
  void save(QDjVuWidget*);
  void restore(QDjVuWidget*);
};



// ========================================
// WORKAROUND QT55 XEMBED BUG
// ========================================


#if QT_VERSION >= 0x50500 /* && QT_VERSION < 0x50XXX */
# define WORKAROUND_QT55_XEMBED_BUG 1
#endif

#if WORKAROUND_QT55_XEMBED_BUG

struct my_xcb_configure_notify_event_t 
{
  quint8      response_type, pad0, pad1, pad2;
  quint32     pad3, window, pad4;
  quint16     x, y, width, height;
};

class my_timer_object_t : public QObject 
{
  Q_OBJECT
  quint32 wid; quint16 w,h; int tid; bool tidp;
public:
  my_timer_object_t(quint32 wid)
    : wid(wid), tid(0), tidp(false) {}
  void start(quint16 ww, quint16 hh) {
    if (tidp) killTimer(tid);
    tidp = true; w = ww; h = hh; startTimer(100); } 
  void timerEvent(QTimerEvent*);
};

struct my_event_filter_t : public QAbstractNativeEventFilter
{
  virtual bool nativeEventFilter(const QByteArray &type, void *msg, long*);
  QMap<quint32,QDjViewPlugin::Instance*> instances;
  QMap<quint32,my_timer_object_t*> timers;
  static my_event_filter_t *instance() {
    static my_event_filter_t *filter = 0;
    return filter ? filter : filter = new my_event_filter_t; }
};

bool my_event_filter_t::nativeEventFilter(const QByteArray &type, void *msg, long*)
{
  if (type != "xcb_generic_event_t") return false;
  my_xcb_configure_notify_event_t *ev = (my_xcb_configure_notify_event_t*)msg;
  if (! (ev->response_type == 22 && instances.contains(ev->window))) return false;
  if (! timers.contains(ev->window)) timers.insert(ev->window, new my_timer_object_t(ev->window));
  timers.value(ev->window)->start(ev->width, ev->height);
  return false;
}

void my_timer_object_t::timerEvent(QTimerEvent*)
{
  my_event_filter_t *f = my_event_filter_t::instance();
  QDjViewPlugin::Instance *instance = f->instances.value(wid,0);
  if (instance && instance->shell) // test for resizing bug
    if (instance->shell->width() != w || instance->shell->height() != h )
      instance->shell->resize((int)w, (int)h);
  f->timers.remove(wid);
  killTimer(tid);
  deleteLater();
}

#endif




// ========================================
// FIXING TRANSIENT WINDOW PROPERTIES (QT4)
// ========================================

#if HAVE_X11

static Atom wm_state;
static Atom wm_client_leader;

static Window 
x11ToplevelWindow(Display *dpy, Window start)
{
  Window shell = 0;
  if (!dpy || !start)
    return 0;
  if (! wm_state)
    wm_state = XInternAtom(dpy, "WM_STATE", True); 
  if (! wm_state) 
    return 0;
  while (1)
    {
      int nprops;
      Atom *props = XListProperties(dpy, start, &nprops);
      if (props && nprops)
        {
          for(int i=0; i<nprops; i++)
	    if (props[i] == wm_state) 
              shell = start;
          XFree(props);
        }
      Window root, parent;
      Window *children = 0;
      unsigned int nchildren;
      if (! XQueryTree(dpy, start, &root, &parent, &children, &nchildren))
        break;
      if (children) 
        XFree(children);
      if (parent == root)
        break;
      start = parent;
    }
  return shell;
}

static void
x11SetTransientForHint(QWidget *widget)
{
  QWidget *parent = widget->parentWidget();
  Display *dpy = QX11Info::display();
  if (dpy && parent && (widget->windowFlags() & Qt::Window))
    {
      Window transient = 0;
      XGetTransientForHint(dpy, widget->winId(), &transient);
      if (transient)
        {
          Window group = 0;
          XWMHints *wmhints = XGetWMHints(dpy, widget->winId());
          if (wmhints && (wmhints->flags & WindowGroupHint))
            group = wmhints->window_group;
          Window ntransient = x11ToplevelWindow(dpy, parent->winId());
          Window ngroup = 0;
          if (ntransient)
            {
              XWMHints *morehints = XGetWMHints(dpy, ntransient);
              if (morehints && (morehints->flags & WindowGroupHint))
                ngroup = morehints->window_group;
              if (morehints)
                XFree(morehints);
              if (transient == group && ngroup)
                ntransient = ngroup;
              XSetTransientForHint(dpy, widget->winId(), ntransient);
            }
          if (ngroup)
            {
              wmhints->flags |= WindowGroupHint;
              wmhints->window_group = ngroup;
              XSetWMHints(dpy, widget->winId(), wmhints);
              if (! wm_client_leader)
                wm_client_leader = XInternAtom(dpy, "WM_CLIENT_LEADER", True); 
              if (wm_client_leader)
                XChangeProperty(dpy, widget->winId(), wm_client_leader, 
                                XA_WINDOW, 32, PropModeReplace, 
                                (unsigned char*)&ngroup, 1);
            }
          if (wmhints)
            XFree(wmhints);
        }
    }
}

#endif




// ========================================
// STREAM
// ========================================


QDjViewPlugin::Stream::Stream(int streamid, QUrl url, Instance *instance)
  : url(url), instance(instance), streamid(streamid), 
    started(false), checked(false), closed(false)
{
  if (instance->dispatcher)
    instance->dispatcher->streamCreated(this);
}


QDjViewPlugin::Stream::~Stream()
{
  if (instance->dispatcher)
    instance->dispatcher->streamDestroyed(this);
}



// ========================================
// DOCUMENT
// ========================================



QDjViewPlugin::Document::Document(QDjViewPlugin::Instance *instance)
  : QDjVuDocument(true), instance(instance)
{
  QUrl docurl = QDjView::removeDjVuCgiArguments(instance->url);
  setUrl(instance->dispatcher->context, docurl);
}


void
QDjViewPlugin::Document::newstream(int streamid, QString, QUrl url)
{
  if (streamid > 0)
    {
      new QDjViewPlugin::Stream(streamid, url, instance);
      QString message = tr("Requesting %1.").arg(url.toString());
      instance->dispatcher->showStatus(instance, message);
      instance->dispatcher->getUrl(instance, url, QString());
    }
}




// ========================================
// FORWARDER
// ========================================



QDjViewPlugin::Forwarder::Forwarder(QDjViewPlugin *dispatcher)
  : QObject(), dispatcher(dispatcher)
{
  // The purpose of the Forwarder is to redirect 
  // certain signals towards the QDjViewPlugin
  // (which cannot be a QObject.)
}


void 
QDjViewPlugin::Forwarder::containerClosed()
{
#if HAVE_QT5EMBED
  QDjView *shell = qobject_cast<QDjView*>(sender());
  Instance *instance = dispatcher->findInstance(shell);
  if (instance && dispatcher->xembedFlag)
    instance->destroy();
#elif HAVE_QX11EMBED
  QX11EmbedWidget *shell = qobject_cast<QX11EmbedWidget*>(sender());
  Instance *instance = dispatcher->findInstance(shell);
  if (instance && dispatcher->xembedFlag)
    instance->destroy();
#endif
}


void 
QDjViewPlugin::Forwarder::showStatus(QString message)
{
  QDjView *viewer = qobject_cast<QDjView*>(sender());
  Instance *instance = dispatcher->findInstance(viewer);
  if (instance)
    dispatcher->showStatus(instance, message);
}


void 
QDjViewPlugin::Forwarder::getUrl(QUrl url, QString target)
{
  QDjView *viewer = qobject_cast<QDjView*>(sender());
  Instance *instance = dispatcher->findInstance(viewer);
  if (! target.size())
    target = "_self";
  if (instance)
    dispatcher->getUrl(instance, url, target);
}


void 
QDjViewPlugin::Forwarder::onChange()
{
  QDjView *viewer = qobject_cast<QDjView*>(sender());
  Instance *instance = dispatcher->findInstance(viewer);
  if (instance)
    dispatcher->onChange(instance);
}


void 
QDjViewPlugin::Forwarder::quit()
{
  dispatcher->quit();
}


void 
QDjViewPlugin::Forwarder::dispatch()
{
  dispatcher->dispatch();
}


void 
QDjViewPlugin::Forwarder::lastViewerClosed()
{
  dispatcher->lastViewerClosed();
}

void 
QDjViewPlugin::Forwarder::continueExec()
{
  dispatcher->continueExec();
}

#if HAVE_X11
static bool x11EventFilter(void *message, long *)
{
  // We define a low level handler because we need to make
  // the window active before calling the QX11EmbedWidget event filter.
  XEvent *event = reinterpret_cast<XEvent*>(message);
  if (event->type == ButtonPress)
    {
      QWidget *w = QWidget::find(event->xbutton.window);
      if (w && w->window()->objectName() == "djvu_shell")
        {
          QApplication *app = (QApplication*)QCoreApplication::instance();
          app->setActiveWindow(w->window());
        }
    }
  return false;
}
#endif

#if HAVE_X11
bool 
QDjViewPlugin::Forwarder::eventFilter(QObject *o, QEvent *e)
{
  if (o->isWidgetType())
    {
      QWidget *w = static_cast<QWidget*>(o);
      switch( e->type() )
        {
          // Try to fix transient windows properties.
          // - this does not work too well...
        case QEvent::Show:
          if (w->windowFlags() & Qt::Window)
            QApplication::postEvent(w, new QEvent(QEvent::User));
          break;
        case QEvent::User:
          if (w->windowFlags() & Qt::Window)
            x11SetTransientForHint(w);
          break;
        default:
          break;
        }
    }
  // Keep processing
  return false;
}
#endif




// ========================================
// INSTANCE
// ========================================


QDjViewPlugin::Instance::~Instance()
{
  destroy();
  delete shell;
  delete djview;
  delete containerwin;
  if (document)
    document->deref();
  document = 0;
}


QDjViewPlugin::Instance::Instance(QDjViewPlugin *parent)
  : url(), dispatcher(parent), 
    document(0), shell(0), djview(0), containerid(0),
    onchange(0)
{
}


void
QDjViewPlugin::Instance::open()
{
  if (!document && url.isValid())
    {
      document = new QDjViewPlugin::Document(this);
      document->ref();
    }
  if (document && djview && !djview->getDocument())
    {
      djview->open(document, url);
      restore(djview->getDjVuWidget());
      shell->show();
    }
}

void
QDjViewPlugin::Instance::destroy()
{
  if (djview)
    {
      save(djview->getDjVuWidget());
      djview->close();
      djview = 0;
    }
  if (shell)
    {
#if HAVE_QT5EMBED
      QWindow *window = shell->windowHandle();
      if (window) {
        window->setVisible(false);
        window->setParent(0);
# if WORKAROUND_QT55_XEMBED_BUG
        my_event_filter_t::instance()->instances.remove((quint32)shell->winId());
# endif
      }
#elif HAVE_X11
      Display *dpy = QX11Info::display();
      XUnmapWindow(dpy, shell->winId());  
      XReparentWindow(dpy, shell->winId(), QX11Info::appRootWindow(), 0,0);
#endif
      shell->close();
      dispatcher->registerForDeletion(shell);
      containerid = 0;
      shell = 0;
    }
}


void 
QDjViewPlugin::Instance::save(QDjVuWidget *widget)
{
  if (widget)
    {
      int q[4];
      // get info
      int zoom = widget->zoom();
      int rotation = widget->rotation();
      bool sideBySide = widget->sideBySide();
      bool continuous = widget->continuous();
      QDjVuWidget::DisplayMode mode = widget->displayMode();
      QDjVuWidget::Position pos = widget->position();
      // pack info into four bytes for compatibility.
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
      // make sure first two integers are positive.
      q[0] |= 0x40000000;
      q[1] |= 0x40000000;
      // store
      saved = QByteArray((const char*)q, sizeof(q));
    }
}

void 
QDjViewPlugin::Instance::restore(QDjVuWidget *widget)
{
  int q[4];
  if (saved.size()==sizeof(q) && widget)
    {
      // unpack
      memcpy((void*)q, (const char*)saved, sizeof(q));
      int zoom = q[0] & 0xfff;
      if (zoom & 0x800)
        zoom -= 0x1000;
      int rotation = (q[0] & 0x3000) >> 12;
      bool sideBySide = !! (q[0] & 0x4000);
      bool continuous = !! (q[0] & 0x8000);
      QDjVuWidget::DisplayMode mode 
        = (QDjVuWidget::DisplayMode)((q[0] & 0xf0000) >> 16);
      QDjVuWidget::Position pos;
      pos.inPage = false;
      pos.pageNo = (q[1] & 0xfffffff);
      pos.posView.rx() = q[2];
      pos.posView.ry() = q[3];
      // apply
      widget->setZoom(zoom);
      widget->setRotation(rotation);
      widget->setSideBySide(sideBySide);
      widget->setContinuous(continuous);
      widget->setDisplayMode(mode);
      widget->setPosition(pos);
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
  CMD_HANDSHAKE = 14,
  CMD_SET_DJVUOPT = 15,
  CMD_GET_DJVUOPT = 16,
  CMD_ON_CHANGE = 17,
};

#define OK_STRING   "OK"
#define ERR_STRING  "ERR"


void
QDjViewPlugin::write(int fd, const char *ptr, int size)
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
QDjViewPlugin::read(int fd, char *buffer, int size)
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
QDjViewPlugin::writeString(int fd, QByteArray s)
{
  int type = TYPE_STRING;
  int length = s.size();
  write(fd, (const char*)&type, sizeof(type));
  write(fd, (const char*)&length, sizeof(length));
  write(fd, s.data(), length+1);
}


void
QDjViewPlugin::writeString(int fd, QString x)
{
  writeString(fd, x.toUtf8());
}


void
QDjViewPlugin::writeInteger(int fd, int x)
{
  int type = TYPE_INTEGER;
  write(fd, (const char*)&type, sizeof(type));
  write(fd, (const char*)&x, sizeof(x));
}


void
QDjViewPlugin::writeDouble(int fd, double x)
{
  int type = TYPE_DOUBLE;
  write(fd, (const char*)&type, sizeof(type));
  write(fd, (const char*)&x, sizeof(x));
}


void
QDjViewPlugin::writePointer(int fd, const void *x)
{
  int type = TYPE_POINTER;
  write(fd, (const char*)&type, sizeof(type));
  write(fd, (const char*)&x, sizeof(x));
}


void
QDjViewPlugin::writeArray(int fd, QByteArray s)
{
  int type = TYPE_ARRAY;
  int length = s.size();
  write(fd, (const char*)&type, sizeof(type));
  write(fd, (const char*)&length, sizeof(length));
  write(fd, s.data(), length);
}


QString
QDjViewPlugin::readString(int fd)
{
  int type, length;
  read(fd, (char*)&type, sizeof(type));
  if (type != TYPE_STRING)
    throw 1;
  read(fd, (char*)&length, sizeof(length));
  if (length < 0) 
    throw 2;
  QByteArray utf(length+1, 0);
  read(fd, utf.data(), length+1);
  return QString::fromUtf8(utf);
}


QByteArray
QDjViewPlugin::readRawString(int fd)
{
  int type, length;
  read(fd, (char*)&type, sizeof(type));
  if (type != TYPE_STRING)
    throw 1;
  read(fd, (char*)&length, sizeof(length));
  if (length < 0) 
    throw 2;
  QByteArray x(length+1, 0);
  read(fd, x.data(), length+1);
  x.truncate(length);
  return x;
}


int
QDjViewPlugin::readInteger(int fd)
{
  int type;
  read(fd, (char*)&type, sizeof(type));
  if (type != TYPE_INTEGER)
    throw 1;
  int x;
  read(fd, (char*)&x, sizeof(x));
  return x;
}


double
QDjViewPlugin::readDouble(int fd)
{
  int type;
  read(fd, (char*)&type, sizeof(type));
  if (type != TYPE_DOUBLE)
    throw 1;
  double x;
  read(fd, (char*)&x, sizeof(x));
  return x;
}


void*
QDjViewPlugin::readPointer(int fd)
{
  int type;
  read(fd, (char*)&type, sizeof(type));
  if (type != TYPE_POINTER)
    throw 1;
  void *x;
  read(fd, (char*)&x, sizeof(x));
  return x;
}


QByteArray
QDjViewPlugin::readArray(int fd)
{
  int type, length;
  read(fd, (char*)&type, sizeof(type));
  if (type != TYPE_ARRAY)
    throw 1;
  read(fd, (char*)&length, sizeof(length));
  if (length < 0) 
    throw 2;
  QByteArray x(length, 0);
  read(fd, x.data(), length);
  return x;
}



// ========================================
// COMMANDS
// ========================================



void
QDjViewPlugin::cmdNew()
{
  // read arguments
  bool fullPage = !!readInteger(pipeRead);
  readString(pipeRead);  // djvuDir (unused)
  int argc = readInteger(pipeRead);
  QStringList args;
  for (int i=0; i<argc; i++)
    {
      QString key = readString(pipeRead);
      QString val = readString(pipeRead);
      QString k = key.toLower();
      if (k == "flags")
#if QT_VERSION >= 0x50E00
        args += val.split(QRegExp("\\s+"), Qt::SkipEmptyParts);
#else
        args += val.split(QRegExp("\\s+"), QString::SkipEmptyParts);
#endif
      else
        args += key + QString("=") + val;
    }
  
  // read saved data
  QByteArray saved;
  int savedformat = readInteger(pipeRead);
  if (savedformat == 1)
    {
      int q[4];
      q[0] = readInteger(pipeRead);
      q[1] = readInteger(pipeRead);
      q[2] = readInteger(pipeRead);
      q[3] = readInteger(pipeRead);
      saved = QByteArray((const char*)&q[0], sizeof(q));
    }
  else if (savedformat)
    saved = readArray(pipeRead);

  // create instance
  Instance *instance = new Instance(this);
  if (fullPage)
    instance->viewerMode = QDjView::FULLPAGE_PLUGIN;
  else
    instance->viewerMode = QDjView::EMBEDDED_PLUGIN;
  instance->saved = saved;
  instance->savedformat = savedformat;
  instance->args = args;
  // success
  instances.insert(instance);
  writeString(pipeWrite, QByteArray(OK_STRING));
  writePointer(pipeWrite, (void*)instance);
}


void
QDjViewPlugin::cmdAttachWindow()
{
  Instance *instance = (Instance*) readPointer(pipeRead);
  QByteArray display = readRawString(pipeRead);
  QString protocol = readString(pipeRead);
  WId window = (WId)readInteger(pipeRead);
  readInteger(pipeRead);   // colormap
  readInteger(pipeRead);   // visualid
  int width = readInteger(pipeRead);
  int height = readInteger(pipeRead);
  // check
  if (!instances.contains(instance))
    {
      fprintf(stderr, "djview dispatcher: bad instance\n");
      writeString(pipeWrite, QByteArray(ERR_STRING));
      return;
    }
  // create application object
  if (! application)
    {
      argc = 0;
      argv[argc++] = progname;
      if (! display.isEmpty())
        {
          argv[argc++] = "-display";
          argv[argc++] = (const char*) display;
        }
      application = new QDjViewApplication(argc, const_cast<char**>(argv));
      application->setQuitOnLastWindowClosed(false);
      argc = 1;
      context = application->djvuContext();
      forwarder = new Forwarder(this);
#if HAVE_X11
      application->setEventFilter(x11EventFilter);
      application->installEventFilter(forwarder);
#endif
#if WORKAROUND_QT55_XEMBED_BUG
      application->installNativeEventFilter(my_event_filter_t::instance());
#endif
      QObject::connect(application, SIGNAL(lastWindowClosed()), 
                       forwarder, SLOT(lastViewerClosed()));
      notifier = new QSocketNotifier(pipeRead, QSocketNotifier::Read); 
      QObject::connect(notifier, SIGNAL(activated(int)), 
                       forwarder, SLOT(dispatch()));
      timer = new QTimer();
      timer->setSingleShot(true);
      timer->start(5*60*1000);
      QObject::connect(timer, SIGNAL(timeout()), 
                       forwarder, SLOT(quit()));
#if HAVE_QX11EMBED || HAVE_QT5EMBED
      if (protocol.startsWith("XEMBED"))
        xembedFlag = true;
#endif
    }
  
  // create djview object
  QWidget *shell = instance->shell;
  QDjView *djview = instance->djview;
  if (! shell)
    {
#if HAVE_QT5EMBED
      if (xembedFlag)
        {
          djview = new QDjView(*context, instance->viewerMode, 0);
          djview->winId();
          shell = djview;
          QObject::connect(shell, SIGNAL(destroyed()),
                           forwarder, SLOT(containerClosed()) );
          shell->setObjectName("djvu_shell");
          QWindow *dwindow = djview->windowHandle();
          QWindow *cwindow = QWindow::fromWinId(window);
          instance->containerwin = cwindow;
          dwindow->setParent(cwindow);
# if WORKAROUND_QT55_XEMBED_BUG
          my_event_filter_t::instance()->instances.insert((quint32)shell->winId(), instance);
# endif
        }
      else
#elif HAVE_QX11EMBED
      if (xembedFlag)
        {
          QX11EmbedWidget *embed = new QX11EmbedWidget();
          shell = embed;
          QObject::connect(shell, SIGNAL(containerClosed()),
                           forwarder, SLOT(containerClosed()) );
          shell->setObjectName("djvu_shell");
          shell->setGeometry(0, 0, width, height);
          djview = new QDjView(*context, instance->viewerMode, shell);
          djview->setWindowFlags(djview->windowFlags() & ~Qt::Window);
          djview->setAttribute(Qt::WA_DeleteOnClose, false);
          instance->containerid = window;
          embed->embedInto(window);
        }
      else
#endif
        {
          shell = new QWidget();
          shell->setObjectName("djvu_shell");
          shell->setGeometry(0, 0, width, height);
          djview = new QDjView(*context, instance->viewerMode, shell);
          djview->setWindowFlags(djview->windowFlags() & ~Qt::Window);
          djview->setAttribute(Qt::WA_DeleteOnClose, false);
#if HAVE_X11
          instance->containerid = window;
          Display *dpy = QX11Info::display();
          XReparentWindow(dpy, shell->winId(), (Window)window, 0,0);
#elif HAVE_QT5EMBED
          shell->winId();
          QWindow *dwindow = shell->windowHandle();
          QWindow *cwindow = QWindow::fromWinId(window);
          instance->containerwin = cwindow;
          dwindow->setParent(cwindow);
#else
          qWarning("djview: unable to embed the djview window.");
#endif
        }
      if (shell && djview && shell != djview)
        {
          QLayout *layout = new QHBoxLayout(shell);
          layout->setMargin(0);
          layout->setSpacing(0);
          layout->addWidget(djview);
        }
      QObject::connect(djview, SIGNAL(pluginStatusMessage(QString)),
                       forwarder, SLOT(showStatus(QString)) );
      QObject::connect(djview, SIGNAL(pluginGetUrl(QUrl,QString)),
                       forwarder, SLOT(getUrl(QUrl,QString)) );
      QObject::connect(djview, SIGNAL(pluginOnChange()),
                       forwarder, SLOT(onChange()) );
      instance->shell = shell;
      instance->djview = djview;
      // apply arguments
      while (instance->args.size() > 0)
        djview->parseArgument(instance->args.takeFirst());
      instance->args.clear();
    }
  // map and reparent djview object
  instance->open();
  writeString(pipeWrite, QByteArray(OK_STRING));
  timer->stop();
}


void
QDjViewPlugin::cmdDetachWindow()
{
  Instance *instance = (Instance*) readPointer(pipeRead);
  if (instances.contains(instance))
    instance->destroy();
  writeString(pipeWrite, QByteArray(OK_STRING));
}


void
QDjViewPlugin::cmdResize()
{
  Instance *instance = (Instance*) readPointer(pipeRead);
  int width = readInteger(pipeRead);
  int height = readInteger(pipeRead);
  if (instances.contains(instance) && width>0 && height>0)
    if (instance->shell && application)
      instance->shell->resize(width, height);
  writeString(pipeWrite, QByteArray(OK_STRING));
}


void
QDjViewPlugin::cmdDestroy()
{
  int savedformat = 0;
  QByteArray saved;
  Instance *instance = (Instance*) readPointer(pipeRead);
  if (instances.contains(instance))
    {
      instance->destroy();
      QList<Stream*> streamList = streams.values();
      foreach(Stream *stream, streamList)
        if (stream->instance == instance)
          delete(stream);
      savedformat = instance->savedformat;
      saved = instance->saved;
      delete instance;
      instances.remove(instance);
    }  
  writeString(pipeWrite, QByteArray(OK_STRING));
  if (savedformat == 1 || savedformat == 0)
    {
      int q[4];
      q[0] = q[1] = q[2] = q[3] = 0;
      if (saved.size() == sizeof(q))
        memcpy((void*)q, (const char*)saved, sizeof(q));
      writeInteger(pipeWrite, q[0]);
      writeInteger(pipeWrite, q[1]);
      writeInteger(pipeWrite, q[2]);
      writeInteger(pipeWrite, q[3]);
    }
  else
    writeArray(pipeWrite, saved);
}


void
QDjViewPlugin::cmdNewStream()
{
  // read
  Instance *instance = (Instance*) readPointer(pipeRead);
  QUrl url = QUrl::fromEncoded(readRawString(pipeRead));
  // check
  if (! url.isValid() )
    {
      fprintf(stderr,"djview dispatcher: invalid url '%s'\n",
              (const char*) url.toEncoded());
      writeString(pipeWrite, QByteArray(ERR_STRING));
      return;
    }
  // proceed
  Stream *stream = 0;
  if (instances.contains(instance))
    {
      // first stream
      if (! instance->url.isValid())
        {
          instance->url = url;
          url = QDjView::removeDjVuCgiArguments(url);
          Stream *s = new QDjViewPlugin::Stream(0, url, instance);
          s->checked = true;
          instance->open();
        }
      // search stream
      foreach(Stream *s, streams)
        if (!stream && !s->started)
          if (s->instance == instance && s->url == url)
            stream = s;
    }
  // mark
  if (stream)
    stream->started = true;
  // reply (null means discard stream)
  writeString(pipeWrite, QByteArray(OK_STRING));
  writePointer(pipeWrite, (void*)stream);
}


void
QDjViewPlugin::cmdWrite()
{
  Stream *stream = (Stream*) readPointer(pipeRead);
  QByteArray data = readArray(pipeRead);
  // locate stream
  if (!streams.contains(stream) || stream->closed)
    {
      // -- null return value means that stream must be discarded.
      writeString(pipeWrite, QByteArray(OK_STRING));
      writeInteger(pipeWrite, 0);
      return;
    }
  // check stream
  if (!stream->checked && data.size()>0)
    {
      stream->checked = true;
      // -- if the stream is not a djvu file, 
      //    this is probably a html error message.
      //    But we need at least 8 bytes to check
      if (data.size() >= 8)
        {
          const char *cdata = (const char*)data;
          if (strncmp(cdata, "FORM", 4) && strncmp(cdata+4, "FORM", 4))
            {
              getUrl(stream->instance, stream->url, QString("_self")); 
              delete stream;
              writeString(pipeWrite, QByteArray(OK_STRING));
              writeInteger(pipeWrite, 0);
              return;
            }
        }
    }
  // pass data
  Document *document = stream->instance->document;
  ddjvu_stream_write(*document, stream->streamid, data, data.size());
  writeString(pipeWrite, QByteArray(OK_STRING));
  writeInteger(pipeWrite, data.size());
}


void
QDjViewPlugin::cmdDestroyStream()
{
  Stream *stream = (Stream*) readPointer(pipeRead);
  int okay = readInteger(pipeRead);
  if (streams.contains(stream))
    {
      Document *document = stream->instance->document;
      if (document && !stream->closed)
        ddjvu_stream_close(*document, stream->streamid, !okay);
      stream->closed = true;
      showStatus(stream->instance, QString());
      delete stream;
    }
  writeString(pipeWrite, QByteArray(OK_STRING));
}


void
QDjViewPlugin::cmdUrlNotify()
{
  QUrl url = QUrl::fromEncoded(readRawString(pipeRead));
  readInteger(pipeRead); // notification code (unused)
  Stream *stream = 0;
  QList<Stream*> streamList = streams.values();
  foreach(stream, streamList)
    if (!stream->started && stream->url == url)
      delete stream;
  writeString(pipeWrite, QByteArray(OK_STRING));
}


void
QDjViewPlugin::cmdPrint()
{
  Instance *instance = (Instance*) readPointer(pipeRead);
  readInteger(pipeRead); // full mode (unused)
  if (instances.contains(instance))
    if (instance->djview)
      QTimer::singleShot(0, instance->djview, SLOT(print()));
  writeString(pipeWrite, QByteArray(OK_STRING));
}


void
QDjViewPlugin::cmdHandshake()
{
  writeString(pipeWrite, QByteArray(OK_STRING));
}


void
QDjViewPlugin::cmdSetDjVuOpt()
{
  // protocol extension for plugin scripting
  Instance *instance = (Instance*) readPointer(pipeRead);
  QString key = readString(pipeRead);
  QString value = readString(pipeRead);
  // check instance
  if (!instances.contains(instance))
    {
      fprintf(stderr, "djview dispatcher: bad instance\n");
      writeString(pipeWrite, QByteArray(ERR_STRING));
      return;
    }
  // return code
  writeString(pipeWrite, QByteArray(OK_STRING));
  // apply options
  QStringList errors;
  if (instance->djview)
    errors = instance->djview->parseArgument(key, value);
  else
    instance->args += key + QString("=") + value;
  // print error messages
  if (errors.size() > 0)
    foreach(QString error, errors)
      qWarning("djvuopt: %s",(const char*)error.toLocal8Bit());
}


void
QDjViewPlugin::cmdGetDjVuOpt()
{
  // protocol extension for plugin scripting.
  Instance *instance = (Instance*) readPointer(pipeRead);
  QString key = readString(pipeRead);
  // check instance.
  if (!instances.contains(instance))
    {
      fprintf(stderr, "djview dispatcher: bad instance\n");
      writeString(pipeWrite, QByteArray(ERR_STRING));
      return;
    }
  // get argument.
  QString value;
  if (instance->djview)
    value = instance->djview->getArgument(key);
  writeString(pipeWrite, QByteArray(OK_STRING));
  writeString(pipeWrite, value);
}


void
QDjViewPlugin::cmdOnChange()
{
  Instance *instance = (Instance*) readPointer(pipeRead);
  instance->onchange = readInteger(pipeRead);
  writeString(pipeWrite, QByteArray(OK_STRING));
}


void
QDjViewPlugin::cmdShutdown()
{
  QList<Stream*> streamList = streams.values();
  QList<Instance*> instanceList = instances.values();
  foreach(Instance *s, instanceList)
    s->destroy();
  foreach(Stream *s, streamList) 
    delete(s);
  foreach(Instance *s, instanceList)
    delete(s);
  streams.clear();
  instances.clear();
  lastViewerClosed();  
}





// ========================================
// QDJVIEWPLUGIN
// ========================================



static QDjViewPlugin *thePlugin;


QDjViewPlugin::~QDjViewPlugin()
{
  thePlugin = 0;
  QList<Stream*> streamList = streams.values();
  QList<Instance*> instanceList = instances.values();
  foreach(Stream *s, streamList) 
    delete(s);
  foreach(Instance *s, instanceList)
    delete(s);
  delete forwarder;
  delete notifier;
  delete timer;
  delete application;
}


QDjViewPlugin::QDjViewPlugin(const char *progname)
  : progname(progname),
    context(0),
    timer(0),
    notifier(0),
    application(0),
    forwarder(0),
    xembedFlag(false),
    eventLoop(0),
    returnCode(0),
    quitFlag(false),
    pipeRead(3),
    pipeWrite(4),
    pipeRequest(5)
{
  // Disable SIGPIPE
#ifdef SIGPIPE
# ifndef HAVE_SIGACTION
#  ifdef SA_RESTART
#   define HAVE_SIGACTION 1
#  endif
# endif
# if HAVE_SIGACTION
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
  // Set global dispatcher
  if (thePlugin)
    qWarning("Constructing multiple dispatchers");
  thePlugin = this;
}


QDjViewPlugin *
QDjViewPlugin::instance()
{
  return thePlugin;
}


int 
QDjViewPlugin::exec()
{
#ifdef Q_OS_UNIX
 #ifndef QT_NO_DEBUG
  const char *s = ::getenv("DJVIEW_DEBUG");
  if (s && strcmp(s,"0"))
    {
      static int loop = 1;
      qWarning("QDjViewPlugin::exec() looping for gdb");
      while (loop)
        sleep(1);
    }
# endif
#endif
  returnCode = 0;
  quitFlag = false;
  qWarning("QDjViewPlugin::exec() begin");
  try 
    {
      // startup message announcing capabilities
#if HAVE_QX11EMBED || HAVE_QT5EMBED
      const char *djviewName = "DJVIEW/4 SCRIPT XEMBED";
#else
      const char *djviewName = "DJVIEW/4 SCRIPT";
#endif
      writeString(pipeWrite, QByteArray(djviewName));
      // dispatch until we get display
      while (!application && !quitFlag)
        dispatch();
      // start application
      if (application && forwarder)
        {
          QTimer::singleShot(0, forwarder, SLOT(continueExec()));
          application->exec();
        }
    }
  catch(int err)
    {
      reportError(err);
      returnCode = 10;
    }
  qWarning("QDjViewPlugin::exec() end code=%d", returnCode);
  return returnCode;
}


void 
QDjViewPlugin::continueExec()
{
  try 
    {
      // handle events in private loop
      while (!quitFlag)
        {
          QEventLoop loop;
          eventLoop = &loop;
          eventLoop->exec();
          eventLoop = 0;
          // see registerForDeletion()
          QObjectList toDelete = pendingDelete;
          pendingDelete.clear();
          foreach(QObject *o, toDelete)
            delete o;
        }
    }
  catch(int err)
    {
      reportError(err);
      returnCode = 10;
    }
  if (application)
    application->exit(returnCode);
}


void 
QDjViewPlugin::exit(int retcode)
{
  returnCode = retcode;
  quitFlag = true;
  if (timer)
    timer->stop();
  if (eventLoop)
    eventLoop->exit(retcode);
  else if (application)
    application->exit(retcode);
}


void 
QDjViewPlugin::quit()
{
  this->exit(0);
}


void 
QDjViewPlugin::reportError(int err)
{
  if (err < 0)
    perror("djview dispatcher");
  else if (err > 0)
    fprintf(stderr, "djview dispatcher: protocol error (%d)\n", err);
}


void 
QDjViewPlugin::streamCreated(Stream *stream)
{
  streams.insert(stream);
}


void 
QDjViewPlugin::streamDestroyed(Stream *stream)
{
  if (streams.contains(stream))
    {
      if (instances.contains(stream->instance))
        {
          Document *document = stream->instance->document;
          if (document && !stream->closed)
            ddjvu_stream_close(*document, stream->streamid, true);
        }
      stream->closed = true;
      stream->started = stream->checked = false;
      streams.remove(stream);
    }
}


QDjViewPlugin::Instance*
QDjViewPlugin::findInstance(QWidget *widget)
{
  if (widget)
    foreach(Instance *instance, instances)
      if (widget == instance->djview || widget == instance->shell)
        return instance;
  return 0;
}


void
QDjViewPlugin::getUrl(Instance *instance, QUrl url, QString target)
{
  try
    {
      writeInteger(pipeRequest, CMD_GET_URL);
      writePointer(pipeRequest, (void*) instance);
      writeString(pipeRequest, url.toEncoded());
      writeString(pipeRequest, target);
    }
  catch(int err)
    {
      reportError(err);
      exit(10);
    }
}


void
QDjViewPlugin::onChange(Instance *instance)
{
  try
    {
      if (!instance->onchange)
        return;
      writeInteger(pipeRequest, CMD_ON_CHANGE);
      writePointer(pipeRequest, (void*) instance);
    }
  catch(int err)
    {
      reportError(err);
      exit(10);
    }
}


void
QDjViewPlugin::showStatus(Instance *instance, QString message)
{
  try
    {
      message.replace(QRegExp("\\s"), " ");
      writeInteger(pipeRequest, CMD_SHOW_STATUS);
      writePointer(pipeRequest, (void*) instance);
      writeString(pipeRequest, message);
    }
  catch(int err)
    {
      reportError(err);
      exit(10);
    }
}


void
QDjViewPlugin::dispatch()
{
  try
    {
      int cmd = readInteger(pipeRead);
      switch (cmd)
        {
        case CMD_SHUTDOWN:       cmdShutdown(); break;
        case CMD_NEW:            cmdNew(); break;
        case CMD_DETACH_WINDOW:  cmdDetachWindow(); break;
        case CMD_ATTACH_WINDOW:  cmdAttachWindow(); break;
        case CMD_RESIZE:         cmdResize(); break;
        case CMD_DESTROY:        cmdDestroy(); break;
        case CMD_PRINT:          cmdPrint(); break;
        case CMD_NEW_STREAM:     cmdNewStream(); break;
        case CMD_WRITE:          cmdWrite(); break;
        case CMD_DESTROY_STREAM: cmdDestroyStream(); break;
        case CMD_URL_NOTIFY:     cmdUrlNotify(); break;
        case CMD_HANDSHAKE:      cmdHandshake(); break;
        case CMD_SET_DJVUOPT:    cmdSetDjVuOpt(); break;
        case CMD_GET_DJVUOPT:    cmdGetDjVuOpt(); break;
        case CMD_ON_CHANGE:      cmdOnChange(); break;
        default:                 throw 3;
        }
    }
  catch(int err)
    {
      reportError(err);
      exit(err ? 10 : 0);
    }
}


void
QDjViewPlugin::lastViewerClosed()
{
  if (timer)
    {
      timer->stop();
      timer->start(5*60*1000);
      return;
    }
  exit(0);
}


void 
QDjViewPlugin::registerForDeletion(QObject *p)
{
  // QObject::deleteLater() deletes when 
  // returning to the current event loop.
  // Here we want deletions to happen when
  // returning to the main loop.
  // Otherwise Instance::destroy() might
  // delete window modal dialogs while
  // they are running.
  if (application)
    {
      pendingDelete.append(p);
      if (eventLoop)
        eventLoop->exit(0);
      return;
    }
  // Fallback
  p->deleteLater();
}





// ----------------------------------------
// MOC

#include "qdjviewplugin.moc"


// ----------------------------------------
// END



/* -------------------------------------------------------------
   Local Variables:
   c++-font-lock-extra-types: ( "\\sw+_t" "[A-Z]\\sw*[a-z]\\sw*" )
   End:
   ------------------------------------------------------------- */
