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

libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status libtool

EXTRA_DIST = COPYRIGHT autogen.sh djview4.spec
EXTRA_DIST += config/qmsilent.awk
EXTRA_DIST += debian

