
# libnpdjvu
# - compile but do not install.

all: libnpdjvu

libnpdjvu: Makefile.qmake FORCE
	${MAKE} -f Makefile.qmake

FORCE:

.PHONY: FORCE

EXTRA_DIST = \
   npdjvu.pro \
   npdjvu.plist \
   npdjvu.rc \
   npdjvu.r \
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

# invoke qmake

qmake_verbose = $(qmake_verbose_@AM_V@)
qmake_verbose_ = $(qmake_verbose_@AM_DEFAULT_V@)
qmake_verbose_0 = @echo "  QMAKE   " $@;

qmake_allflags = $(QMAKEFLAGS) \
  -I.. -I$(srcdir) -I$(top_srcdir)/src \
  -DDIR_DATADIR=\\\"$(datadir)\\\"

Makefile.qmake: ${srcdir}/npdjvu.pro Makefile
	@cmp -s $< npdjvu.pro || cp $< npdjvu.pro
	$(qmake_verbose)$(QMAKE) $(QMAKEARG)\
		QMAKE_UIC=$(UIC) \
		QMAKE_MOC=$(MOC) \
		QMAKE_RCC=$(RCC) \
		QMAKE_LFLAGS+='$(LDFLAGS)' \
		QMAKE_CFLAGS+='$(CFLAGS) $(qmake_allflags)' \
		QMAKE_CXXFLAGS+='$(CXXFLAGS) $(qmake_allflags)' \
		LIBS+='$(QMAKELIBS)' \
		VPATH='$(srcdir) $(top_srcdir)/src' -o - npdjvu.pro |\
	  $(AWK) -f $(top_srcdir)/config/qmsilent.awk > $@

# cleaning

clean-local: clean-qmake

distclean-local: distclean-qmake

clean-qmake:
	-test "${srcdir}" == "." || ${RM} npdjvu.pro
	-test -f Makefile.qmake && ${MAKE} -f Makefile.qmake clean

distclean-qmake: Makefile.qmake
	-test "${srcdir}" == "." || ${RM} npdjvu.pro
	-test -f Makefile.qmake && ${MAKE} -f Makefile.qmake distclean
	-$(RM) Makefile.qmake

.PHONY: clean-qmake distclean-qmake 

MOSTLYCLEANFILES = libnpdjvu.so

# misc

EXTRA_DIST += LICENSE.GPL README
