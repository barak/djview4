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

TARGET = npdjvu
TEMPLATE = lib
CONFIG  += dll
DEFINES += NPDJVU

# ============ qtbrowserplugin stuff

HEADERS += qtnpapi.h
HEADERS += npdjvu.h
HEADERS += qtbrowserplugin.h
HEADERS += qtbrowserplugin_p.h
SOURCES += qtnpapi.cpp
SOURCES += npdjvu.cpp

win32 {
  contains(DEFINES,_WIN32_X11_) { CONFIG += x11 }
  RC_FILE = npdjvu.rc
  SOURCES += qtbrowserplugin_win.cpp
  LIBS += -luser32
  qaxserver { DEF_FILE += qtbrowserpluginax.def }
  else { DEF_FILE += qtbrowserplugin.def }
} else:mac {
  CONFIG += plugin
  CONFIG += plugin_bundle
  contains(DEFINES,__USE_WS_X11__) { CONFIG += x11 }
  SOURCES += qtbrowserplugin_max.cpp
  ICON = ../src/images/DjVuApp.icns
  REZ_FILES += npdjvu.r
  RESOURCES.path = Contents/Resources
  RESOURCES.files = npdjvu.rsrc
  QMAKE_BUNDLE_DATA += RESOURCES
  QMAKE_INFO_PLIST = djview.plist
} else {
  CONFIG += plugin
  CONFIG += x11
  SOURCES += qtbrowserplugin_x11.cpp
}


# ============ djview stuff

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
    # LIBS += -ldjvulibre
    # QMAKE_CXXFLAGS +=  ... (c++ flags)
    # QMAKE_CFLAGS += ...    (c flags)
    # QMAKE_LFLAGS += ...    (link flags)
    # DEFINES += ...         (definitions)
}

# -- config
CONFIG += qt thread warn_on 
QT += network 
CONFIG(release,debug|release) {
    DEFINES += NDEBUG QT_NO_DEBUG QT_NO_DEBUG_STREAM
}

# -- djvu
HEADERS += ../src/qdjvu.h 
HEADERS += ../src/qdjvuhttp.h 
HEADERS += ../src/qdjvuwidget.h
SOURCES += ../src/qdjvu.cpp
SOURCES += ../src/qdjvuhttp.cpp
SOURCES += ../src/qdjvuwidget.cpp
RESOURCES += ../src/qdjvuwidget.qrc 

# -- djview
HEADERS += ../src/qdjviewprefs.h
HEADERS += ../src/qdjviewsidebar.h
HEADERS += ../src/qdjviewdialogs.h 
HEADERS += ../src/qdjviewexporters.h
HEADERS += ../src/qdjview.h
HEADERS += ../src/djview.h
SOURCES += ../src/qdjviewprefs.cpp 
SOURCES += ../src/qdjviewsidebar.cpp 
SOURCES += ../src/qdjviewdialogs.cpp
SOURCES += ../src/qdjviewexporters.cpp
SOURCES += ../src/qdjview.cpp
SOURCES += ../src/djview.cpp
RESOURCES += ../src/qdjview.qrc 
FORMS += ../src/qdjviewerrordialog.ui
FORMS += ../src/qdjviewinfodialog.ui 
FORMS += ../src/qdjviewmetadialog.ui 
FORMS += ../src/qdjviewsavedialog.ui 
FORMS += ../src/qdjviewprintdialog.ui 
FORMS += ../src/qdjviewexportdialog.ui 
FORMS += ../src/qdjviewexportps1.ui
FORMS += ../src/qdjviewexportps2.ui
FORMS += ../src/qdjviewexportps3.ui
FORMS += ../src/qdjviewexporttiff.ui
FORMS += ../src/qdjviewexportprn.ui
FORMS += ../src/qdjviewprefsdialog.ui 

# -- helper files
HEADERS += ../src/tiff2pdf.h
SOURCES += ../src/tiff2pdf.c

