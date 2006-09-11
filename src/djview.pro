TEMPLATE = app 
TARGET = djview 
CONFIG += qt thread warn_on 
QT += network 
LIBS += -ldjvulibre 
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
