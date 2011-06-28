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

#ifndef QDJVIEWSIDEBAR_H
#define QDJVIEWSIDEBAR_H

#if AUTOCONF
# include "config.h"
#endif

#include <Qt>
#include <QObject>
#include <QModelIndex>
#include <QString>
#include <QUrl>
#include <QWidget>

#include <libdjvu/miniexp.h>
#include <libdjvu/ddjvuapi.h>

#include "qdjvu.h"
#include "qdjvuwidget.h"
#include "qdjview.h"

class QAction;
class QComboBox;
class QContextMenuEvent;
class QItemSelectionModel;
class QLabel;
class QLineEdit;
class QListView;
class QMenu;
class QPushButton;
class QStackedLayout;
class QToolButton;
class QTreeWidget;
class QTreeWidgetItem;

// ----------------------------------------
// OUTLINE


class QDjViewOutline : public QWidget
{
  Q_OBJECT
public:
  QDjViewOutline(QDjView *djview);
public slots:
  void clear(); 
  void refresh(); 
protected slots:
  void pageChanged(int pageno);
  void itemActivated(QTreeWidgetItem *item);
private:
  QDjView *djview;
  QTreeWidget *tree;
  bool loaded;
  int pageNumber(const char *link);
  void fillItems(QTreeWidgetItem *item, miniexp_t expr);
  void searchItem(QTreeWidgetItem *item, int pageno, 
                  QTreeWidgetItem *&fi, int &fp);
};



// ----------------------------------------
// THUMBNAILS


class QDjViewThumbnails : public QWidget
{
  Q_OBJECT
  Q_PROPERTY(int size READ size WRITE setSize)
  Q_PROPERTY(bool smart READ smart WRITE setSmart)
public:
  QDjViewThumbnails(QDjView *djview);
  int size();
  bool smart();
public slots:
  void setSize(int);
  void setSmart(bool);
protected slots:
  void setSize();
  void pageChanged(int pageno);
  void activated(const QModelIndex &index);
protected:
  void contextMenuEvent(QContextMenuEvent *event);
  void updateActions();
private:
  class Model;
  class View;
  QDjView             *djview;
  Model               *model;
  View                *view;
  QItemSelectionModel *selection;
  QMenu               *menu;
};




// ----------------------------------------
// THUMBNAILS


class QDjViewFind : public QWidget
{
  Q_OBJECT
  Q_PROPERTY(QString text READ text WRITE setText)
  Q_PROPERTY(bool caseSensitive READ caseSensitive WRITE setCaseSensitive)
  Q_PROPERTY(bool wordOnly READ wordOnly WRITE setWordOnly)
public:
  QDjViewFind(QDjView *djview);
  void takeFocus(Qt::FocusReason);
  QString text();
  bool caseSensitive();
  bool wordOnly();
  bool regExpMode();
public slots:
  void selectAll();
  void eraseText();
  void setText(QString s);
  void setCaseSensitive(bool);
  void setWordOnly(bool);
  void setRegExpMode(bool);
  void findNext();
  void findPrev();
  void findAgain();
protected slots:
  void pageChanged(int);
protected:
  void contextMenuEvent(QContextMenuEvent *event);
private:
  class Model;
  QDjView        *djview;
  Model          *model;
  QListView      *view;
  QMenu          *menu;
  QComboBox      *combo;
  QToolButton    *upButton;
  QToolButton    *downButton;
  QToolButton    *resetButton;
  QStackedLayout *stack;
  QLabel         *label;
  QAction        *caseSensitiveAction;
  QAction        *wordOnlyAction;
  QAction        *regExpModeAction;
};



#endif
/* -------------------------------------------------------------
   Local Variables:
   c++-font-lock-extra-types: ( "\\sw+_t" "[A-Z]\\sw*[a-z]\\sw*" )
   End:
   ------------------------------------------------------------- */
