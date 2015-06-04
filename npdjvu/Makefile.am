# include data gathered from npdjvu.pro file

include $(srcdir)/npdjvu.am

$(srcdir)/npdjvu.am: npdjvu.pro
	$(AWK) -f $(top_srcdir)/config/qmsources.awk < $< > $@


# libnpdjvu
# - compile but do not install.

all: libnpdjvu

libnpdjvu: Makefile.qmake FORCE
	${MAKE} -f Makefile.qmake

FORCE:

.PHONY: FORCE

EXTRA_DIST = npdjvu.pro LICENSE.GPL README $(qmake_sources)

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
		VPATH='$(srcdir) $(top_srcdir)/src' -o $@0 npdjvu.pro
	$(AWK) -f $(top_srcdir)/config/qmsilent.awk < $@0 > $@ && ${RM} $@0

# cleaning

mostlyclean-local: mostlyclean-qmake

mostlyclean-qmake:
	-test "${srcdir}" == "." || ${RM} djview.pro
	-test -f Makefile.qmake && ${MAKE} -f Makefile.qmake clean
	-$(RM) Makefile.qmake

.PHONY: mostlyclean-qmake

MOSTLYCLEANFILES = libnpdjvu.so

