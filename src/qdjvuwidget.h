//C-  -*- C++ -*-
//C- -------------------------------------------------------------------
//C- DjView4
//C- Copyright (c) 2006  Leon Bottou
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

// $Id$

#ifndef QDJVUWIDGET_H
#define QDJVUWIDGET_H

#if AUTOCONF
# include "config.h"
#endif

#include "qdjvu.h"

#include <QPoint>
#include <QWidget>
#include <QMenu>
#include <QAbstractScrollArea>


class QDjVuPrivate;
class QDjVuLens;

class QDjVuWidget : public QAbstractScrollArea
{
  Q_OBJECT
  Q_ENUMS(DisplayMode PointerMode Align Priority)
  Q_PROPERTY(int page 
             READ page WRITE setPage)
  Q_PROPERTY(int rotation 
             READ rotation WRITE setRotation)
  Q_PROPERTY(int zoom 
             READ zoom WRITE setZoom)
  Q_PROPERTY(double gamma 
             READ gamma WRITE setGamma)
  Q_PROPERTY(DisplayMode displayMode 
             READ displayMode WRITE setDisplayMode)
  Q_PROPERTY(bool displayFrame 
             READ displayFrame WRITE setDisplayFrame)
  Q_PROPERTY(Align horizAlign 
             READ horizAlign WRITE setHorizAlign)
  Q_PROPERTY(Align vertAlign 
             READ vertAlign WRITE setVertAlign)
  Q_PROPERTY(bool sideBySide 
             READ sideBySide WRITE setSideBySide)
  Q_PROPERTY(bool continuous 
             READ continuous WRITE setContinuous)
  Q_PROPERTY(QBrush borderBrush 
             READ borderBrush WRITE setBorderBrush)
  Q_PROPERTY(int borderSize 
             READ borderSize WRITE setBorderSize)
  Q_PROPERTY(int pixelCacheSize 
             READ pixelCacheSize WRITE setPixelCacheSize)
  Q_PROPERTY(bool displayMapAreas 
             READ displayMapAreas WRITE setDisplayMapAreas)
  Q_PROPERTY(bool keyboardEnabled 
             READ keyboardEnabled WRITE enableKeyboard)
  Q_PROPERTY(bool hyperlinkEnabled 
             READ hyperlinkEnabled WRITE enableHyperlink)
  Q_PROPERTY(bool mouseEnabled 
             READ mouseEnabled WRITE enableMouse)
  Q_PROPERTY(int lensPower 
             READ lensPower WRITE setLensPower)
  Q_PROPERTY(int lensSize 
             READ lensSize WRITE setLensSize)
  Q_PROPERTY(QDjVuDocument* document 
             READ document WRITE setDocument)
  Q_PROPERTY(QMenu* contextMenu 
             READ contextMenu WRITE setContextMenu)
  Q_PROPERTY(Position position 
             READ position WRITE setPosition)
  Q_PROPERTY(Qt::KeyboardModifiers modifiersForLens 
             READ modifiersForLens WRITE setModifiersForLens)
  Q_PROPERTY(Qt::KeyboardModifiers modifiersForSelect 
             READ modifiersForSelect WRITE setModifiersForSelect)
  Q_PROPERTY(Qt::KeyboardModifiers modifiersForLinks
             READ modifiersForLinks WRITE setModifiersForLinks)

public:

  enum {
    ZOOM_STRETCH = -4,          //!< Stretch full page into viewport
    ZOOM_ONE2ONE = -3,          //!< Maximal resolution for each page
    ZOOM_FITPAGE = -2,          //!< Fit entire pages into viewport
    ZOOM_FITWIDTH = -1,         //!< Fit page width into viewport
    ZOOM_MIN = 5,               //!< Minimal magnification
    ZOOM_100 = 100,             //!< Standard magnification
    ZOOM_MAX = 1200,            //!< Maximal magnification
  };

  enum DisplayMode {
    DISPLAY_COLOR,              //!< Default dislplay mode
    DISPLAY_STENCIL,            //!< Only display the b&w mask layer
    DISPLAY_BG,                 //!< Onlu display the background layer
    DISPLAY_FG,                 //!< Onlu display the foregroud layer
  };

  enum Align {
    ALIGN_TOP,                  //!< Align page top sides.
    ALIGN_LEFT = ALIGN_TOP,     //!< Align page left sides.
    ALIGN_CENTER,               //!< Center pages
    ALIGN_BOTTOM,               //!< Align page bottom sides.
    ALIGN_RIGHT = ALIGN_BOTTOM, //!< Align page right sides.
  };

  enum Priority {
    PRIORITY_DEFAULT,           //!< Priority for default option values.
    PRIORITY_ANNO,              //!< Priority for annotation defined options.
    PRIORITY_CGI,               //!< Priority for cgi defined options.
    PRIORITY_USER               //!< Priority for gui defined options.
  };  

  struct Position {
    int    pageNo;
    QPoint posPage;
    QPoint posView;
    bool   inPage;
    bool   anchorRight;
    bool   anchorBottom;
    bool   valid;
    Position();
  };

  struct PageInfo {
    int   pageno;
    int   dpi;
    int   width;
    int   height;
    QRect segment;
  };

  ~QDjVuWidget();

  QDjVuWidget(QWidget *parent=0);
  QDjVuWidget(QDjVuDocument *doc, QWidget *parent=0);
  
  QDjVuDocument *document(void) const;
  int page(void) const;
  QPoint hotSpot(void) const;
  Position position(void) const;
  Position position(const QPoint &point) const;
  int rotation(void) const;
  int zoom(void) const;
  int zoomFactor(void) const;
  double gamma(void) const;
  DisplayMode displayMode(void) const;
  bool displayFrame(void) const;
  Align horizAlign(void) const;
  Align vertAlign(void) const;
  bool sideBySide(void) const;
  bool continuous(void) const;
  QBrush borderBrush(void) const;
  int borderSize(void) const;
  QMenu* contextMenu(void) const;
  int  pixelCacheSize(void) const;
  bool displayMapAreas(void) const;
  bool keyboardEnabled(void) const;
  bool mouseEnabled(void) const;
  bool hyperlinkEnabled(void) const;
  int lensPower(void) const;
  int lensSize(void) const;
  Qt::KeyboardModifiers modifiersForLens() const;
  Qt::KeyboardModifiers modifiersForSelect() const;
  Qt::KeyboardModifiers modifiersForLinks() const;

public slots:
  void setDocument(QDjVuDocument *d);
  void setPage(int p);
  void setPosition(const Position &pos);
  void setPosition(const Position &pos, const QPoint &point);
  void setRotation(int);
  void setZoom(int);
  void setGamma(double);
  void setDisplayMode(DisplayMode m);
  void setDisplayFrame(bool);
  void setHorizAlign(Align);
  void setVertAlign(Align);
  void setSideBySide(bool);
  void setContinuous(bool);
  void setBorderBrush(QBrush);
  void setBorderSize(int);
  void setContextMenu(QMenu*);
  void setDisplayMapAreas(bool);
  void setPixelCacheSize(int);
  void enableKeyboard(bool);
  void enableMouse(bool);
  void enableHyperlink(bool);
  void setLensPower(int);
  void setLensSize(int);
  void setModifiersForLens(Qt::KeyboardModifiers);
  void setModifiersForSelect(Qt::KeyboardModifiers);
  void setModifiersForLinks(Qt::KeyboardModifiers);
  void reduceOptionsToPriority(Priority);

public:
  QString pastErrorMessage(int n=0);
  QString pastInfoMessage(int n=0);
  bool startSelecting(const QPoint &point);
  bool startPanning(const QPoint &point);
  bool startLensing(const QPoint &point);
  bool startLinking(const QPoint &point);
  bool stopInteraction(void);
  QString linkUrl(void);
  QString linkTarget(void);
  QString linkComment(void);
  QString getTextForRect(const QRect &rect);
  QImage  getImageForRect(const QRect &rect);
  QRect   getSegmentForRect(const QRect &rect, int pageNo);
  bool pageSizeKnown(int pageno) const;
  void clearHighlights(int pageno);
  void addHighlight(int pageno, int x, int y, int w, int h, QColor color);
  
protected:
  virtual bool viewportEvent (QEvent *event);
  virtual void scrollContentsBy(int dx, int dy);
  virtual void resizeEvent(QResizeEvent *event);
  virtual void keyPressEvent(QKeyEvent *event);
  virtual void modifierEvent(Qt::KeyboardModifiers, Qt::MouseButtons, QPoint);
  virtual void mousePressEvent(QMouseEvent *event);
  virtual void mouseDoubleClickEvent(QMouseEvent *event);
  virtual void mouseReleaseEvent(QMouseEvent *event);
  virtual void mouseMoveEvent(QMouseEvent *event);
  virtual void contextMenuEvent (QContextMenuEvent *event);
  virtual void paintEvent(QPaintEvent *event);
  virtual void chooseTooltip(void);
  virtual void paintDesk(QPainter &p, const QRegion &region);
  virtual void paintFrame(QPainter &p, const QRect &rect, int width);
  virtual void paintEmpty(QPainter &p, const QRect &rect, bool, bool, bool);

public slots:
  void nextPage(void);
  void prevPage(void);
  void firstPage(void);
  void lastPage(void);
  void moveToPageTop(void);
  void moveToPageBottom(void);
  void readNext(void);
  void readPrev(void);
  void zoomIn(void);
  void zoomOut(void);
  void zoomRect(QRect rect);
  void displayModeColor(void);
  void displayModeStencil(void);
  void displayModeBackground(void);
  void displayModeForeground(void);
  void rotateRight(void);
  void rotateLeft(void);

signals:
  void layoutChanged();
  void pageChanged(int pageno);
  void pointerPosition(const Position &pos, const PageInfo &page);
  void pointerEnter(const Position &pos, miniexp_t maparea);
  void pointerLeave(const Position &pos, miniexp_t maparea);
  void pointerClick(const Position &pos, miniexp_t maparea);
  void pointerSelect(const QPoint &pointerPos, const QRect &rect);
  void errorCondition(int pageno);
  void stopCondition(int pageno);
  void keyPressSignal(QKeyEvent *event, bool &done);
  void error(QString message, QString filename, int lineno);
  void info(QString message);
  
private:
  QDjVuPrivate *priv;
  friend class QDjVuPrivate;
  friend class QDjVuLens;
};





#endif

/* -------------------------------------------------------------
   Local Variables:
   c++-font-lock-extra-types: ( "\\sw+_t" "[A-Z]\\sw*[a-z]\\sw*" )
   End:
   ------------------------------------------------------------- */
