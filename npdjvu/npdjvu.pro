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

TEMPLATE = lib
TARGET = npdjvu
CONFIG += qt thread warn_on 
QT += network
greaterThan(QT_MAJOR_VERSION, 4) {
    QT += widgets printsupport
} 

# ============ qtbrowserplugin code

HEADERS += qtnpapi.h
HEADERS += npdjvu.h
HEADERS += qtbrowserplugin.h
HEADERS += qtbrowserplugin_p.h
SOURCES += qtnpapi.cpp
SOURCES += npdjvu.cpp

# ============ test for X11

win32:contains(DEFINES,__WIN32_X11__) { 
    DEFINES *= WITH_X11 
} else:macx:contains(DEFINES,__USE_WS_X11__) { 
    DEFINES *= WITH_X11 
} else:!macx:!win32 {
    DEFINES *= WITH_X11
}
contains(DEFINES,WITH_X11) {
    CONFIG += x11
}    
greaterThan(QT_MAJOR_VERSION, 4) {
    contains(DEFINES,WITH_X11) {
        DEFINES *= Q_WS_X11
        DEFINES *= QX11Info=qintptr
    } else:win32 { 
        DEFINES *= Q_WS_WIN
    } else:macx {
        DEFINES *= Q_WS_MAC
    }
}

# ============ plugin stuff

x11 {
    CONFIG += dll
    CONFIG += plugin
    SOURCES += qtbrowserplugin_x11.cpp
} else:win32 {
    CONFIG  += dll
    SOURCES += qtbrowserplugin_win.cpp
    RC_FILE = npdjvu.rc
    DEF_FILE += qtbrowserplugin.def
    LIBS += -luser32
} else:mac {
    CONFIG += dll
    CONFIG += plugin
    CONFIG += plugin_bundle
    SOURCES += qtbrowserplugin_mac.cpp
    REZ_FILES += npdjvu.r
    RESOURCES.path = Contents/Resources
    RESOURCES.files = npdjvu.rsrc
    QMAKE_BUNDLE_DATA += RESOURCES
    QMAKE_INFO_PLIST = npdjvu.plist
}


# ============ djview stuff

CONFIG += qt thread warn_on 
QT += network opengl

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
    #LIBS += -ldjvulibre
    #QMAKE_CXXFLAGS += 
    #QMAKE_CFLAGS += 
    #QMAKE_LFLAGS += 
    #DEFINES += 
}

# -- config
CONFIG(release,debug|release) {
    DEFINES += NDEBUG QT_NO_DEBUG QT_NO_DEBUG_STREAM
}

# -- djvu
HEADERS += ../src/qdjvu.h 
HEADERS += ../src/qdjvunet.h 
HEADERS += ../src/qdjvuwidget.h
SOURCES += ../src/qdjvu.cpp
SOURCES += ../src/qdjvunet.cpp
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
FORMS += ../src/qdjviewauthdialog.ui
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

