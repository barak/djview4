#C- -------------------------------------------------------------------
#C- DjView4
#C- Copyright (c) 2006  Leon Bottou
#C-
#C- This software is subject to, and may be distributed under, the
#C- GNU General Public License. The license should have
#C- accompanied the software or you may obtain a copy of the license
#C- from the Free Software Foundation at http:#www.fsf.org .
#C-
#C- This program is distributed in the hope that it will be useful,
#C- but WITHOUT ANY WARRANTY; without even the implied warranty of
#C- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#C- GNU General Public License for more details.
#C-  ------------------------------------------------------------------
#C-
#C- $Id$

TEMPLATE = app 
TARGET = djview

CONFIG += qt thread warn_on link_pkgconfig
PKGCONFIG += ddjvuapi
QT += network 

HEADERS = qdjvu.h qdjvuhttp.h 
HEADERS += qdjvuwidget.h
HEADERS += qdjviewprefs.h qdjviewdialogs.h qdjview.h

RESOURCES = qdjview.qrc qdjvuwidget.qrc 

SOURCES = qdjvu.cpp qdjvuhttp.cpp
SOURCES += qdjvuwidget.cpp
SOURCES += qdjviewprefs.cpp qdjviewdialogs.cpp qdjview.cpp
SOURCES += djview.cpp

FORMS = qdjviewerrordialog.ui
FORMS += qdjviewinfodialog.ui qdjviewmetadialog.ui 

unix {
 SOURCES += qdjviewplugin.cpp
 HEADERS += qdjviewplugin.h
}
