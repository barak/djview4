
# compile npdjvu but do not install it.

all: npdjvu

npdjvu: Makefile.qmake
	${MAKE} -f Makefile.qmake

.PHONY: npdjvu

# invoke qmake

qmake_verbose = $(qmake_verbose_@AM_V@)
qmake_verbose_ = $(qmake_verbose_@AM_DEFAULT_V@)
qmake_verbose_0 = @echo "  QMAKE   " $@;

qmake_allflags = $(QMAKEFLAGS) \
  -I.. -I$(srcdir) -I$(top_srcdir)/src \
  -DDIR_DATADIR=\\\"$(datadir)\\\"

Makefile.qmake: npdjvu.pro Makefile
	@cmp -s $< npdjvu.pro || cp $< npdjvu.pro
	$(qmake_verbose)$(QMAKE) $(QMAKEARG)\
		QMAKE_UIC=$(UIC) \
		QMAKE_MOC=$(MOC) \
		QMAKE_RCC=$(RCC) \
		QMAKE_LFLAGS+='$(LDFLAGS)' \
		QMAKE_CFLAGS+='$(CFLAGS) $(qmake_allflags)' \
		QMAKE_CXXFLAGS+='$(CXXFLAGS) $(qmake_allflags)' \
		QMAKE_CPPFLAGS+='$(CPPFLAGS) $(qmake_allflags)' \
		LIBS+='$(QMAKELIBS)' \
		VPATH='$(srcdir):$(top_srcdir)/src' -o - npdjvu.pro |\
	  $(AWK) -f $(top_srcdir)/config/qmsilent.awk > $@

# cleaning

clean-local: clean-qmake

distclean-local: distclean-qmake clean-qm

clean-qmake:
	-test -f Makefile.qmake && ${MAKE} -f Makefile.qmake clean

distclean-qmake: Makefile.qmake
	-test -f Makefile.qmake && ${MAKE} -f Makefile.qmake distclean
	-$(RM) Makefile.qmake

.PHONY: clean-qm clean-qmake distclean-qmake 

# misc
EXTRA_DIST = LICENSE.GPL README
EXTRA_DIST += npdjvu.pro npdjvu.plist npdjvu.rc npdjvu.r 

# actual sources
EXTRA_DIST += \
   npdjvu.cpp \
   npdjvu.h \
   qtbrowserplugin.def \
   qtbrowserplugin.h \
   qtbrowserplugin_mac.cpp \
   qtbrowserplugin_p.h \
   qtbrowserplugin_win.cpp \
   qtbrowserplugin_x11.cpp \
   qtnpapi.cpp \
   qtnpapi.h
