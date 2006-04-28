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
#include <QMessageBox>


// ----------- QDJVIEWDIALOGERROR

class QDjViewDialogError : public QDialog
{
  Q_OBJECT
public:
  ~QDjViewDialogError();
  QDjViewDialogError(QWidget *parent);
  void prepare(QMessageBox::Icon icon, QString caption);
public slots:
  void error(QString message, QString filename, int lineno);
  void okay(void);
private:
  struct Private;
  Private *d;
};





#endif

/* -------------------------------------------------------------
   Local Variables:
   c++-font-lock-extra-types: ( "\\sw+_t" "[A-Z]\\sw*[a-z]\\sw*" )
   End:
   ------------------------------------------------------------- */
