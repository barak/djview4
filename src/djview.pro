#C- -*- shell-script -*-
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
CONFIG += qt thread warn_on 
QT += network 



# -- find libraries
CONFIG(autoconf) {
    # for use with autoconf
    DEFINES += AUTOCONF
    #   Autoconf calls qmake with the following variables
    #   LIBS += ...
    #   QMAKE_CXXFLAGS += ...
    #   QMAKE_CFLAGS += ...
    #   QMAKE_LFLAGS += ...
} else:unix:!macx {
    # for use under unix with pkgconfig
    CONFIG += link_pkgconfig
    PKGCONFIG += ddjvuapi
} else {
    # for use on other platforms
    LIBS += -ldjvulibre
    # QMAKE_CXXFLAGS +=  ... (c++ flags)
    # QMAKE_CFLAGS += ...    (c flags)
    # QMAKE_LFLAGS += ...    (link flags)
    # DEFINES += ...         (definitions)
}


# -- no debug in release mode
CONFIG(release,debug|release) {
    DEFINES += NDEBUG QT_NO_DEBUG QT_NO_DEBUG_STREAM
}

# -- check for x11
macx {
  contains(DEFINES,__USE_WS_X11__): CONFIG += x11
  ICON = images/DjVuApp.icns
} else:win32 {
  contains(DEFINES,_WIN32_X11_): CONFIG += x11
} else:unix {
  CONFIG += x11
}

# -- djvu files
HEADERS += qdjvu.h 
HEADERS += qdjvuhttp.h 
HEADERS += qdjvuwidget.h
SOURCES += qdjvu.cpp
SOURCES += qdjvuhttp.cpp
SOURCES += qdjvuwidget.cpp
RESOURCES += qdjvuwidget.qrc 

# -- djview files
HEADERS += qdjviewprefs.h
HEADERS += qdjviewdialogs.h 
HEADERS += qdjviewsidebar.h
HEADERS += qdjview.h
SOURCES += qdjviewprefs.cpp 
SOURCES += qdjviewdialogs.cpp
SOURCES += qdjviewsidebar.cpp
SOURCES += qdjview.cpp
RESOURCES += qdjview.qrc 
FORMS += qdjviewerrordialog.ui
FORMS += qdjviewinfodialog.ui 
FORMS += qdjviewmetadialog.ui 
FORMS += qdjviewexportps1.ui
FORMS += qdjviewexportps2.ui
FORMS += qdjviewexportps3.ui
FORMS += qdjviewsavedialog.ui 
FORMS += qdjviewprintdialog.ui 
FORMS += qdjviewprefsdialog.ui 
SOURCES += djview.cpp
x11 {
  HEADERS += qdjviewplugin.h
  SOURCES += qdjviewplugin.cpp
}

# -- transations
TRANSLATIONS += djview_fr.ts
