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

#ifndef QDJVIEWEXPORTERS_H
#define QDJVIEWEXPORTERS_H

#if AUTOCONF
# include "config.h"
#endif

#include <QObject>
#include <QDialog>
#include <QList>
#include <QString>
#include <QStringList>
#include <QWidget>

class QPrinter;
class QPrintDialog;

#include <libdjvu/miniexp.h>
#include <libdjvu/ddjvuapi.h>

#include "qdjview.h"
#include "qdjviewdialogs.h"



// ----------- QDJVIEWEXPORTER



class QDjViewExporter : public QObject
{
  Q_OBJECT
public:
  virtual ~QDjViewExporter();
  virtual bool exportOnePageOnly();
  virtual void setFromTo(int fromPage, int toPage);
  virtual void setErrorCaption(QString);
  virtual void resetProperties();
  virtual void loadProperties(QString group=QString());
  virtual void saveProperties(QString group=QString());
  virtual bool loadPrintSetup(QPrinter *printer, QPrintDialog *dialog);
  virtual bool savePrintSetup(QPrinter *printer);
  virtual int propertyPages();
  virtual QWidget* propertyPage(int num);
  virtual ddjvu_status_t status();
  virtual QString name();
public slots:
  virtual bool save(QString fileName) = 0;
  virtual bool print(QPrinter *printer);
  virtual void stop();
  virtual void error(QString, QString, int);
signals:
  void progress(int i);
protected:
  friend class QDjViewExporterFactory;
  QDjViewExporter(QDialog *parent, QDjView *djview, QString name);
  QDialog *parent;
  QDjView *djview;
  QDjViewErrorDialog *errorDialog;
  QString errorCaption;
  QString exporterName;
  int fromPage;
  int toPage;
public:
  static QDjViewExporter *create(QDialog*, QDjView*, QString name);
  static QStringList names();
  static QStringList info(QString name);
};


#endif

/* -------------------------------------------------------------
   Local Variables:
   c++-font-lock-extra-types: ( "\\sw+_t" "[A-Z]\\sw*[a-z]\\sw*" )
   End:
   ------------------------------------------------------------- */
