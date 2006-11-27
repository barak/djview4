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

#ifndef QDJVIEWDIALOGS_H
#define QDJVIEWDIALOGS_H

#include <QObject>
#include <QDialog>
#include <QLabel>
#include <QMessageBox>
#include <QString>
#include <QWidget>

#include <libdjvu/miniexp.h>
#include <libdjvu/ddjvuapi.h>

class QCloseEvent;
class QDjView;
class QDjVuDocument;
class QDjVuJob;



// ----------- QDJVIEWERRORDIALOG


class QDjViewErrorDialog : public QDialog
{
  Q_OBJECT
public:
  ~QDjViewErrorDialog();
  QDjViewErrorDialog(QWidget *parent);
  void prepare(QMessageBox::Icon icon, QString caption);
public slots:
  void error(QString message, QString filename, int lineno);
  virtual void done(int);
signals:
  void closing();
private:
  void compose(void);
  struct Private;
  Private *d;
};




// ----------- QDJVIEWINFODIALOG


class QDjViewInfoDialog : public QDialog
{
  Q_OBJECT
public:
  ~QDjViewInfoDialog();
  QDjViewInfoDialog(QDjView *parent);
public slots:
  void clear();
  void refresh();
  void setPage(int pageno);
  void setFile(int fileno);
protected slots:
  void prevFile();
  void nextFile();
  void jumpToSelectedPage();
private:
  void fillFileCombo();
  void fillDocLabel();
  void fillDocTable();
  void fillDocRow(int);
  struct Private;
  Private *d;
};





// ----------- QDJVIEWMETADIALOG


class QDjViewMetaDialog : public QDialog
{
  Q_OBJECT
public:
  ~QDjViewMetaDialog();
  QDjViewMetaDialog(QDjView *parent);
public slots:
  void clear();
  void refresh();
  void prevPage();
  void nextPage();
  void setPage(int pageno);
protected slots:
  void jumpToSelectedPage();
  void copy();
private:
  struct Private;
  Private *d;
};



// ----------- QDJVIEWJOBDIALOG


class QDjViewJobDialog : public QDialog
{
  Q_OBJECT
public:
  QDjViewJobDialog(QDjView *djview);
protected slots:
  void start();
  void stop();
  void refresh();
  void progress(int);
  void done(int);
  void error(QString message, QString filename, int lineno);
protected:
  virtual void closeEvent(QCloseEvent *event);
  virtual void hookStart() = 0;
  virtual void hookStop();
  virtual void hookDocument();
  virtual void hookRefresh();
  virtual void hookProgress(int);
  virtual void hookCleanup(int);
  QDjView            *djview;
  QDjVuDocument      *document;
  QDjVuJob           *job;
  QDjViewErrorDialog *errorDialog;
  QString             errorCaption;
  ddjvu_status_t      status;

};




// ----------- QDJVIEWSAVEDIALOG


class QDjViewSaveDialog : public QDjViewJobDialog
{
  Q_OBJECT
public:
  ~QDjViewSaveDialog();
  QDjViewSaveDialog(QDjView *djview);
protected slots:
  void browse();
protected:
  virtual void hookStart();
  virtual void hookStop();
  virtual void hookDocument();
  virtual void hookRefresh();
  virtual void hookProgress(int);
  virtual void hookCleanup(int);
private:
  struct Private;
  Private *d;
};



// ----------- QDJVIEWPRINTDIALOG


class QDjViewPrintDialog : public QDialog
{
  Q_OBJECT
public:
  ~QDjViewPrintDialog();
  QDjViewPrintDialog(QDjView *djview);
protected slots:
  void refresh();
  void choosePrinter();
  void progress(int percent);
  void print();
  void printDjVu();
  void printNative();
  void stop();
  void error(QString message, QString filename, int lineno);
  virtual void done(int);
  virtual void closeEvent(QCloseEvent *event);
private:
  bool isPrinterPostScript();
  void updatePrinter();
  struct Private;
  Private *d;
};



#endif

/* -------------------------------------------------------------
   Local Variables:
   c++-font-lock-extra-types: ( "\\sw+_t" "[A-Z]\\sw*[a-z]\\sw*" )
   End:
   ------------------------------------------------------------- */
