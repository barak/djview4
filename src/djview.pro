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


TEMPLATE = app 
TARGET = djview
CONFIG += qt thread warn_on 
QT += network opengl

# -- QT5 modules
greaterThan(QT_MAJOR_VERSION, 4) {
    QT += widgets printsupport
} 


# -- find libraries
CONFIG(autoconf) {
    # for use with autoconf
    DEFINES += AUTOCONF
    #   Autoconf calls qmake with the following variables
    #   LIBS += ...
    #   QMAKE_CXXFLAGS += ...
    #   QMAKE_CFLAGS += ...
    #   QMAKE_LFLAGS += ...
} else {
    # customize below
    #INCLUDEPATH += /usr/include
    #LIBS += -L/usr/local/lib -ldjvulibre -ljpeg
    #QMAKE_CXXFLAGS += 
    #QMAKE_CFLAGS += 
    #QMAKE_LFLAGS += 
    #DEFINES += 
}


# -- no debug in release mode
CONFIG(release,debug|release) {
    DEFINES += NDEBUG QT_NO_DEBUG QT_NO_DEBUG_STREAM
}

# -- mac stuff
macx {
  ICON = images/DjVuApp.icns
  VERSION = $$PACKAGE_VERSION
  RESOURCES.path = Contents/Resources
  RESOURCES.files = images/DjVu.icns
  QMAKE_BUNDLE_DATA += RESOURCES
  QMAKE_INFO_PLIST = djview.plist
  QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.13
} 

# --- windows stuff
win32 {
  RC_FILE = djview.rc
}

# --- djviewplugin logic
greaterThan(QT_MAJOR_VERSION, 4) {
    DEFINES *= WITH_DJVIEWPLUGIN
} else:macx:contains(DEFINES,__USE_WS_X11__) { 
    DEFINES *= WITH_DJVIEWPLUGIN
} else:win32:contains(DEFINES,_WIN32_X11_) {
    DEFINES *= WITH_DJVIEWPLUGIN
} else:!macx:!win32 {
    DEFINES *= WITH_DJVIEWPLUGIN
}
contains(DEFINES,WITH_DJVIEWPLUGIN) {
    greaterThan(QT_MAJOR_VERSION, 4) {
    } else {
        DEFINES *= WITH_X11
        CONFIG += x11
    }
}


# -- djvu files
HEADERS += qdjvu.h 
HEADERS += qdjvunet.h 
HEADERS += qdjvuwidget.h
SOURCES += qdjvu.cpp
SOURCES += qdjvunet.cpp
SOURCES += qdjvuwidget.cpp
RESOURCES += qdjvuwidget.qrc 

# -- djview files
HEADERS += qdjviewprefs.h
HEADERS += qdjviewsidebar.h
HEADERS += qdjviewdialogs.h 
HEADERS += qdjviewexporters.h
HEADERS += qdjview.h
HEADERS += djview.h
HEADERS += version.h
SOURCES += qdjviewprefs.cpp 
SOURCES += qdjviewsidebar.cpp
SOURCES += qdjviewdialogs.cpp
SOURCES += qdjviewexporters.cpp
SOURCES += qdjview.cpp
SOURCES += djview.cpp
RESOURCES += qdjview.qrc 
FORMS += qdjviewerrordialog.ui
FORMS += qdjviewauthdialog.ui 
FORMS += qdjviewinfodialog.ui 
FORMS += qdjviewmetadialog.ui 
FORMS += qdjviewsavedialog.ui 
FORMS += qdjviewprintdialog.ui 
FORMS += qdjviewexportdialog.ui 
FORMS += qdjviewexportps1.ui
FORMS += qdjviewexportps2.ui
FORMS += qdjviewexportps3.ui
FORMS += qdjviewexporttiff.ui
FORMS += qdjviewexportprn.ui
FORMS += qdjviewprefsdialog.ui 
contains(DEFINES,WITH_DJVIEWPLUGIN) {
  HEADERS += qdjviewplugin.h
  SOURCES += qdjviewplugin.cpp
}

# -- helper files
HEADERS += tiff2pdf.h
SOURCES += tiff2pdf.c

# -- transations
TRANSLATIONS += djview_fr.ts
TRANSLATIONS += djview_uk.ts
TRANSLATIONS += djview_de.ts
TRANSLATIONS += djview_cs.ts
TRANSLATIONS += djview_ru.ts
TRANSLATIONS += djview_es.ts
TRANSLATIONS += djview_zh_cn.ts
TRANSLATIONS += djview_zh_tw.ts
