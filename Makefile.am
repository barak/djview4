ACLOCAL_AMFLAGS = -I config

SUBDIRS = src

if WANT_NSDEJAVU
SUBDIRS += nsdejavu
endif

if WANT_NPDJVU
SUBDIRS += npdjvu
endif

if WANT_DESKTOPFILES
SUBDIRS += desktopfiles
endif

if WANT_MAC
SUBDIRS += mac
endif

libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status libtool

EXTRA_DIST = COPYRIGHT 
EXTRA_DIST += README README_translations
EXTRA_DIST += autogen.sh djview4.spec
EXTRA_DIST += config/qmsilent.awk
EXTRA_DIST += config/qmsources.awk
EXTRA_DIST += debian

