#C- This program is free software; you can redistribute it and/or 
#C- modify it under the terms of the GNU General Public License, 
#C- version 2.  The license should have accompanied the program 
#C- or you may obtain a copy of the license from the Free Software
#C- Foundation at http://www.fsf.org.
#C- 
#C- This program is distributed in the hope that it will be useful,
#C- but WITHOUT ANY WARRANTY; without even the implied warranty of
#C- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#C- GNU General Public License for more details.



### THIS IS A TEMPORARY MAKEFILE FOR DEVELOPMENT PURPOSES ONLY
MAKE = make
SHELL = /bin/sh
RM = /bin/rm

CC = gcc
CXX = g++
MOC = moc-qt4
UIC = uic-qt4
RCC = rcc

srcdir=.
top_srcdir=..

QTDEBUG=
OPTS = -O2
PKGS= QtCore${QTDEBUG} QtGui${QTDEBUG} QtNetwork${QTDEBUG} ddjvuapi
LIBS=$(shell pkg-config --libs ${PKGS})
FLGS=$(shell pkg-config --cflags ${PKGS})

CXXFLAGS= -Wall ${FLGS} ${OPTS}

SOURCES=qdjvu.h qdjvu.cpp\
	qdjvuhttp.h qdjvuhttp.cpp\
	qdjvuwidget.h qdjvuwidget.cpp qdjvuwidget.qrc\
	qdjviewprefs.h qdjviewprefs.cpp \
	qdjviewdialogs.h qdjviewdialogs.cpp \
	qdjviewdialogerror.ui \
	qdjview.h qdjview.cpp qdjview.qrc \
	djview.cpp

PROGRAM=djview

debug: FORCE
	${MAKE} all QTDEBUG=_debug OPTS=-g

all: ${PROGRAM}

include Makefile.aux

Makefile.aux: Makefile
	${top_srcdir}/etc/miniqmake ${SOURCES} > Makefile.aux

${PROGRAM}: ${OBJS}
	${CXX} ${CXXFLAGS} -o ${PROGRAM} ${OBJS} ${LIBS} 

clean: FORCE
	-${RM} 2>/dev/null ${GENS} ${OBJS} ${PROGRAM} Makefile.aux
	-${RM} -rf 2>/dev/null doc

doc: FORCE
	-${RM} -rf 2>/dev/null doc
	doxygen

depend: FORCE
	${CXX} -MM -MG > Makefile.dep \
	  $(patsubst -I/%,-isystem /%,${CXXFLAGS}) \
	  $(filter %.cpp,${SOURCES})

FORCE:

include Makefile.dep

